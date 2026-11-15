#pragma once

#include "Arpeggiator.h"

namespace amv
{
/** One step of the sequencer. */
struct SeqStep
{
    int note = 0;           // semitones from the held key, -12..12
    float velocity = 0.8f;  // 0..1
    float gate = 0.5f;      // part of the step the note sounds; 0 is a rest
    bool slide = false;     // hold the note into the next step: a legato slide, or a tie when the note repeats
};

/** The pattern of the step sequencer. All 32 steps are always there; the length decides how many play. */
struct SeqPattern
{
    static constexpr int maxSteps = 32;
    static constexpr int noteRange = 12;

    std::array<SeqStep, maxSteps> steps {};

    /** "note,velocity,gate,slide;" for every step. */
    juce::String toString() const
    {
        juce::String text;
        for (const auto& s : steps)
            text << s.note << ',' << juce::String (s.velocity, 3) << ',' << juce::String (s.gate, 3) << ',' << (s.slide ? 1 : 0) << ';';
        return text;
    }

    /** Steps that are missing or broken in the text keep their defaults. */
    static SeqPattern fromString (const juce::String& text)
    {
        SeqPattern pattern;
        const auto parts = juce::StringArray::fromTokens (text, ";", "");

        for (int i = 0; i < juce::jmin (parts.size(), (int) maxSteps); ++i)
        {
            const auto fields = juce::StringArray::fromTokens (parts[i], ",", "");
            if (fields.size() < 4)
                continue;

            auto& s = pattern.steps[(size_t) i];
            s.note = juce::jlimit (-noteRange, (int) noteRange, fields[0].getIntValue());
            s.velocity = juce::jlimit (0.0f, 1.0f, fields[1].getFloatValue());
            s.gate = juce::jlimit (0.0f, 1.0f, fields[2].getFloatValue());
            s.slide = fields[3].getIntValue() != 0;
        }

        return pattern;
    }
};

struct SeqParams
{
    bool on = false;
    int rate = 1;            // index into getArpRates()
    int length = 7;          // steps that play, 1..32
    float gate = 1.0f;       // scales the gate of every step
    float swing = 0.0f;      // 0..0.75: how late every second step is, as a part of half a step
    bool freeCycle = true;   // true: the pattern cycles freely over the host bar; false: it restarts every bar
    bool latch = false;      // keep playing after the key is released
    const SeqPattern* pattern = nullptr;
};

/** Plays a pattern of steps from the held key, which transposes it. Every step has its own note,
    velocity, gate and slide, and the pattern can have any length up to 32 steps, so it fits the
    odd meters. Follows the host transport when it is playing, otherwise it starts on the key
    press and runs at the host tempo. */
class StepSequencer
{
public:
    void prepare (double newSampleRate)
    {
        sampleRate = newSampleRate;
        held.clear();
        held.reserve (32);
        sounding.clear();
        sounding.reserve (8);
        wasOn = false;
        shownStep = -1;
    }

    /** The step that played last, or -1 when the sequencer is silent. */
    int getCurrentStep() const noexcept { return shownStep; }

    void process (const juce::MidiBuffer& in, juce::MidiBuffer& out, int numSamples, const SeqParams& p, const TransportInfo& t)
    {
        out.clear();

        if (! p.on || p.pattern == nullptr)
        {
            if (wasOn)
            {
                allOff (out, 0);
                held.clear();
                hasRoot = false;
                wasOn = false;
                shownStep = -1;
            }

            out.addEvents (in, 0, numSamples, 0);
            return;
        }

        if (! wasOn || t.playing != wasPlaying)
        {
            if (! wasOn)
            {
                held.clear();
                hasRoot = false;
            }

            freeNext = 0.0;
            freeStep = 0;
            freeCount = 0;
            wasOn = true;
            wasPlaying = t.playing;
        }

        if (! p.latch && held.empty())
            hasRoot = false;

        params = p;
        params.length = juce::jlimit (1, (int) SeqPattern::maxSteps, p.length);
        params.swing = juce::jlimit (0.0f, 0.75f, p.swing);
        const double stepBeats = 0.5 / (double) (1 << juce::jlimit (0, 2, p.rate));
        const double samplesPerBeat = sampleRate * 60.0 / juce::jmax (20.0, t.bpm);
        stepSamples = stepBeats * samplesPerBeat;

        Grid grid;
        grid.playing = t.playing;
        grid.stepBeats = stepBeats;
        grid.samplesPerBeat = samplesPerBeat;
        grid.ppq = t.ppq;
        grid.barSteps = juce::jmax (1.0, t.barBeats / stepBeats);

        // The first step that has not started yet. A swung step starts later than its place on
        // the grid, so the search begins one step back.
        grid.nextStep = (juce::int64) std::floor (t.ppq / stepBeats) - 1;
        while (stepTime (grid.nextStep) < t.ppq / stepBeats - 1.0e-9)
            ++grid.nextStep;

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

        if (! hasRoot && sounding.empty())
            shownStep = -1;
    }

private:
    struct HeldKey
    {
        int note;
        float velocity;
        int channel;
    };

    struct SoundingNote
    {
        int note;
        int channel;
        int remaining; // samples from the start of the current block until its note off
        bool slides;   // held into the next step
    };

    struct Grid
    {
        bool playing = false;
        double stepBeats = 0.25, samplesPerBeat = 0.0, ppq = 0.0, barSteps = 16.0;
        juce::int64 nextStep = 0;
    };

    /** When a step of the grid starts, in steps. Every second step is late by the swing. */
    double stepTime (juce::int64 step) const noexcept
    {
        return (double) step + ((step & 1) != 0 ? 0.5 * (double) params.swing : 0.0);
    }

    /** How long a step lasts, in steps. */
    double stepDuration (juce::int64 step) const noexcept
    {
        return 1.0 + ((step & 1) != 0 ? -0.5 : 0.5) * (double) params.swing;
    }

    void allOff (juce::MidiBuffer& out, int pos)
    {
        for (const auto& s : sounding)
            out.addEvent (juce::MidiMessage::noteOff (s.channel, s.note), pos);

        sounding.clear();
    }

    void handleInput (const juce::MidiMessage& m, int pos, juce::MidiBuffer& out)
    {
        if (m.isNoteOn())
        {
            if (held.empty() && ! (params.latch && hasRoot))
            {
                // First key of a phrase: when the host is stopped the pattern starts on this key press.
                freeNext = (double) pos;
                freeStep = 0;
                freeCount = 0;
            }

            removeHeld (m.getNoteNumber());
            if (held.size() < held.capacity())
                held.push_back ({ m.getNoteNumber(), m.getFloatVelocity(), m.getChannel() });

            root = held.back();
            hasRoot = true;
        }
        else if (m.isNoteOff())
        {
            removeHeld (m.getNoteNumber());

            // The pattern follows the newest key that is still held.
            if (! held.empty())
                root = held.back();
            else if (! params.latch)
                hasRoot = false;
        }
        else
        {
            if (m.isAllNotesOff() || m.isAllSoundOff())
            {
                held.clear();
                hasRoot = false;
            }

            out.addEvent (m, pos);
        }
    }

    void removeHeld (int note)
    {
        held.erase (std::remove_if (held.begin(), held.end(), [note] (const HeldKey& k) { return k.note == note; }), held.end());
    }

    void emitSteps (int from, int to, Grid& grid, juce::MidiBuffer& out)
    {
        const int length = params.length;

        if (grid.playing)
        {
            for (;;)
            {
                const double offset = (stepTime (grid.nextStep) * grid.stepBeats - grid.ppq) * grid.samplesPerBeat;
                const int pos = juce::jmax (from, (int) std::floor (offset + 1.0e-6));
                if (pos >= to)
                    break;

                juce::int64 index = grid.nextStep;
                if (! params.freeCycle)
                    index = (juce::int64) std::floor (std::fmod ((double) index, grid.barSteps) + (index < 0 ? grid.barSteps : 0.0));

                fire (pos, (int) (((index % length) + length) % length), stepDuration (grid.nextStep), out);
                ++grid.nextStep;
            }

            return;
        }

        while (freeNext < (double) to)
        {
            if (freeStep >= length)
                freeStep = 0;

            const double duration = stepDuration (freeCount);
            fire (juce::jmax (from, (int) std::floor (freeNext + 1.0e-6)), freeStep, duration, out);
            freeStep = (freeStep + 1) % length;
            ++freeCount;
            freeNext += stepSamples * duration;
        }
    }

    void fire (int pos, int index, double duration, juce::MidiBuffer& out)
    {
        if (! hasRoot)
            return;

        shownStep = index;
        const auto& step = params.pattern->steps[(size_t) index];
        if (step.gate <= 0.0f)
            return; // a rest

        const int note = juce::jlimit (0, 127, root.note + step.note);
        const double samples = stepSamples * duration;

        // A slide is held a little past the start of the next step, so that step joins it.
        const int overlap = juce::jmax (2, (int) (0.01 * sampleRate));
        const int lengthSamples = step.slide ? (int) std::lround (samples) + overlap
                                             : juce::jmax (1, (int) std::lround (samples * (double) (step.gate * params.gate)));

        for (size_t i = 0; i < sounding.size(); ++i)
            if (sounding[i].note == note)
            {
                if (sounding[i].slides)
                {
                    // The same note again after a slide is a tie: it just lasts longer.
                    sounding[i].remaining = pos + lengthSamples;
                    sounding[i].slides = step.slide;
                    return;
                }

                // Retriggering a note that is still sounding needs its note off first.
                out.addEvent (juce::MidiMessage::noteOff (sounding[i].channel, note), pos);
                sounding.erase (sounding.begin() + (std::ptrdiff_t) i);
                break;
            }

        if (sounding.size() >= sounding.capacity())
        {
            out.addEvent (juce::MidiMessage::noteOff (sounding.front().channel, sounding.front().note), pos);
            sounding.erase (sounding.begin());
        }

        const float velocity = juce::jlimit (0.02f, 1.0f, step.velocity * (0.4f + 0.6f * root.velocity));
        out.addEvent (juce::MidiMessage::noteOn (root.channel, note, velocity), pos);
        sounding.push_back ({ note, root.channel, pos + lengthSamples, step.slide });
    }

    double sampleRate = 44100.0;
    std::vector<HeldKey> held;
    std::vector<SoundingNote> sounding;
    SeqParams params;
    HeldKey root { 60, 1.0f, 1 };

    bool wasOn = false, wasPlaying = false, hasRoot = false;
    int freeStep = 0, shownStep = -1;
    juce::int64 freeCount = 0;
    double stepSamples = 6000.0, freeNext = 0.0;
};
} // namespace amv
