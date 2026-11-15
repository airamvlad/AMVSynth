#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

namespace amv
{
/** A meter as groups of steps. The first step of every group is accented. */
struct MeterDef
{
    const char* name;
    std::vector<int> groups;
};

inline const std::vector<MeterDef>& getMeters()
{
    static const std::vector<MeterDef> meters {
        { "4/4", { 4, 4, 4, 4 } },
        { "5/8  (2+3)", { 2, 3 } },
        { "7/8  (3+2+2)", { 3, 2, 2 } },
        { "7/8  (2+2+3)", { 2, 2, 3 } },
        { "9/8  (2+2+2+3)", { 2, 2, 2, 3 } },
        { "11/8  (2+2+3+2+2)", { 2, 2, 3, 2, 2 } },
    };
    return meters;
}

inline const std::array<const char*, 3>& getArpRates()
{
    static const std::array<const char*, 3> rates { "1/8", "1/16", "1/32" };
    return rates;
}

inline const std::array<const char*, 5>& getArpModes()
{
    static const std::array<const char*, 5> modes { "Up", "Down", "Up-Down", "As Played", "Random" };
    return modes;
}

struct ArpParams
{
    bool on = false;
    int meter = 2;
    int rate = 1;          // index into getArpRates()
    int mode = 0;          // index into getArpModes()
    int octaves = 1;
    float gate = 0.6f;     // fraction of a step; above 1 the notes overlap, which slides in mono legato
    float accent = 0.6f;   // how much quieter the unaccented steps are
    bool polymeter = true; // true: the pattern cycles freely over the host bar; false: it restarts every bar
};

struct TransportInfo
{
    double bpm = 140.0;
    double ppq = 0.0;      // position of the block start in quarter notes
    bool playing = false;
    double barBeats = 4.0; // length of the host bar in quarter notes
};

/** Turns held keys into a stepped pattern with Balkan style accent groups.
    Follows the host transport when it is playing, otherwise runs freely at the host tempo. */
class Arpeggiator
{
public:
    void prepare (double newSampleRate)
    {
        sampleRate = newSampleRate;
        held.clear();
        held.reserve (32);
        sounding.clear();
        sounding.reserve (16);
        wasOn = false;
    }

    /** Number of steps in one pass of a meter. */
    static int patternLength (int meter)
    {
        int total = 0;
        for (int g : getMeters()[(size_t) clampMeter (meter)].groups)
            total += g;
        return total;
    }

    /** True when a step is the first of its group. */
    static bool isAccent (int meter, int step)
    {
        int start = 0;
        for (int g : getMeters()[(size_t) clampMeter (meter)].groups)
        {
            if (step == start)
                return true;
            start += g;
        }

        return false;
    }

    void process (const juce::MidiBuffer& in, juce::MidiBuffer& out, int numSamples, const ArpParams& p, const TransportInfo& t)
    {
        out.clear();

        if (! p.on)
        {
            if (wasOn)
            {
                for (const auto& s : sounding)
                    out.addEvent (juce::MidiMessage::noteOff (s.channel, s.note), 0);

                sounding.clear();
                held.clear();
                wasOn = false;
            }

            out.addEvents (in, 0, numSamples, 0);
            return;
        }

        if (! wasOn || t.playing != wasPlaying)
        {
            if (! wasOn)
            {
                held.clear();
                noteCounter = 0;
            }

            freeNext = 0.0;
            freeStep = 0;
            wasOn = true;
            wasPlaying = t.playing;
        }

        params = p;
        const double stepBeats = 0.5 / (double) (1 << juce::jlimit (0, 2, p.rate));
        const double samplesPerBeat = sampleRate * 60.0 / juce::jmax (20.0, t.bpm);
        stepSamples = stepBeats * samplesPerBeat;
        length = patternLength (p.meter);

        Grid grid;
        grid.playing = t.playing;
        grid.stepBeats = stepBeats;
        grid.samplesPerBeat = samplesPerBeat;
        grid.ppq = t.ppq;
        grid.nextStep = (juce::int64) std::ceil (t.ppq / stepBeats - 1.0e-9);
        grid.barSteps = juce::jmax (1.0, t.barBeats / stepBeats);

        int cursor = 0;
        for (const auto meta : in)
        {
            const int pos = juce::jlimit (0, numSamples, meta.samplePosition);
            emitSteps (cursor, pos, grid, out);
            cursor = pos;
            handleInput (meta.getMessage(), pos, out);
        }

        emitSteps (cursor, numSamples, grid, out);

        if (! t.playing)
            freeNext -= (double) numSamples;

        // Note offs that fall inside this block.
        for (size_t i = 0; i < sounding.size();)
        {
            if (sounding[i].remaining < numSamples)
            {
                out.addEvent (juce::MidiMessage::noteOff (sounding[i].channel, sounding[i].note), juce::jmax (0, sounding[i].remaining));
                sounding.erase (sounding.begin() + (std::ptrdiff_t) i);
            }
            else
            {
                sounding[i].remaining -= numSamples;
                ++i;
            }
        }
    }

private:
    struct HeldKey
    {
        int note;
        float velocity;
        int channel; // kept so MPE bend and pressure still reach the arpeggiated notes
    };

    struct SoundingNote
    {
        int note;
        int channel;
        int remaining; // samples from the start of the current block until its note off
    };

    struct Grid
    {
        bool playing = false;
        double stepBeats = 0.25, samplesPerBeat = 0.0, ppq = 0.0, barSteps = 16.0;
        juce::int64 nextStep = 0;
    };

    static int clampMeter (int meter)
    {
        return juce::jlimit (0, (int) getMeters().size() - 1, meter);
    }

    void handleInput (const juce::MidiMessage& m, int pos, juce::MidiBuffer& out)
    {
        if (m.isNoteOn())
        {
            if (held.empty())
            {
                // First key of a phrase: start the note order again, and when the host is stopped
                // start the pattern on this key press.
                noteCounter = 0;
                freeNext = (double) pos;
                freeStep = 0;
            }

            removeHeld (m.getNoteNumber());
            if (held.size() < held.capacity())
                held.push_back ({ m.getNoteNumber(), m.getFloatVelocity(), m.getChannel() });
        }
        else if (m.isNoteOff())
        {
            removeHeld (m.getNoteNumber());
        }
        else
        {
            if (m.isAllNotesOff() || m.isAllSoundOff())
                held.clear();

            out.addEvent (m, pos);
        }
    }

    void removeHeld (int note)
    {
        held.erase (std::remove_if (held.begin(), held.end(), [note] (const HeldKey& k) { return k.note == note; }), held.end());
    }

    void emitSteps (int from, int to, Grid& grid, juce::MidiBuffer& out)
    {
        if (grid.playing)
        {
            for (;;)
            {
                const double offset = ((double) grid.nextStep * grid.stepBeats - grid.ppq) * grid.samplesPerBeat;
                const int pos = juce::jmax (from, (int) std::floor (offset + 1.0e-6)); // rounding up could push a step past the block and lose it
                if (pos >= to)
                    break;

                juce::int64 index = grid.nextStep;
                if (! params.polymeter)
                    index = (juce::int64) std::floor (std::fmod ((double) index, grid.barSteps) + (index < 0 ? grid.barSteps : 0.0));

                fire (pos, (int) (((index % length) + length) % length), out);
                ++grid.nextStep;
            }

            return;
        }

        while (freeNext < (double) to)
        {
            fire (juce::jmax (from, (int) std::lround (freeNext)), freeStep, out);
            freeStep = (freeStep + 1) % length;
            freeNext += stepSamples;
        }
    }

    void fire (int pos, int step, juce::MidiBuffer& out)
    {
        if (held.empty())
            return;

        const auto key = pickNote();
        const bool accent = isAccent (params.meter, step);
        const float velocity = juce::jlimit (0.05f, 1.0f, key.velocity * (accent ? 1.0f : 1.0f - 0.7f * params.accent));

        // Retriggering a note that is still sounding needs its note off first.
        for (size_t i = 0; i < sounding.size(); ++i)
            if (sounding[i].note == key.note)
            {
                out.addEvent (juce::MidiMessage::noteOff (sounding[i].channel, key.note), pos);
                sounding.erase (sounding.begin() + (std::ptrdiff_t) i);
                break;
            }

        if (sounding.size() >= sounding.capacity())
        {
            out.addEvent (juce::MidiMessage::noteOff (sounding.front().channel, sounding.front().note), pos);
            sounding.erase (sounding.begin());
        }

        out.addEvent (juce::MidiMessage::noteOn (key.channel, key.note, velocity), pos);
        sounding.push_back ({ key.note, key.channel, pos + juce::jmax (1, (int) std::lround (stepSamples * params.gate)) });
    }

    HeldKey pickNote()
    {
        // Build the note order for the current keys: sorted (or as played), repeated over the octaves.
        std::array<HeldKey, 96> order;
        const int keys = (int) held.size();
        const int octaves = juce::jlimit (1, 3, params.octaves);

        std::array<HeldKey, 32> base;
        std::copy (held.begin(), held.end(), base.begin());
        if (params.mode != 3)
            std::sort (base.begin(), base.begin() + keys, [] (const HeldKey& a, const HeldKey& b) { return a.note < b.note; });

        int n = 0;
        for (int o = 0; o < octaves; ++o)
            for (int k = 0; k < keys; ++k)
                order[(size_t) n++] = { juce::jmin (127, base[(size_t) k].note + 12 * o), base[(size_t) k].velocity, base[(size_t) k].channel };

        const int i = noteCounter++;
        int index = 0;

        switch (params.mode)
        {
            case 1:  index = n - 1 - (i % n); break;
            case 2:
            {
                const int period = juce::jmax (1, 2 * n - 2);
                const int x = i % period;
                index = x < n ? x : period - x;
                break;
            }
            case 4:  index = random.nextInt (n); break;
            default: index = i % n; break;
        }

        return order[(size_t) index];
    }

    double sampleRate = 44100.0;
    std::vector<HeldKey> held;
    std::vector<SoundingNote> sounding;
    ArpParams params;
    juce::Random random;

    bool wasOn = false, wasPlaying = false;
    int noteCounter = 0, length = 7, freeStep = 0;
    double stepSamples = 6000.0, freeNext = 0.0;
};
} // namespace amv
