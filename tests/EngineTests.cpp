// Offline checks for the tuning tables and the synth engine. Exits non-zero on failure.
#include "Arpeggiator.h"
#include "Effects.h"
#include "SynthEngine.h"

#include <cstdio>

namespace
{
int failures = 0;

void check (bool ok, const char* what)
{
    std::printf ("%s  %s\n", ok ? "ok  " : "FAIL", what);
    if (! ok)
        ++failures;
}

bool near (float a, float b, float tolerance = 0.001f) { return std::abs (a - b) < tolerance; }

struct Render
{
    float peak = 0.0f, tailPeak = 0.0f;
    bool finite = true;
};

/** Plays a short phrase and reports the peak while playing and at the very end, after release. */
Render render (amv::EngineParams p)
{
    amv::SynthEngine engine;
    engine.prepare (48000.0, 512);
    engine.setRandomSeed (1);
    engine.setParams (p);

    Render result;
    const int numBlocks = 400;

    for (int block = 0; block < numBlocks; ++block)
    {
        juce::MidiBuffer midi;
        if (block == 0)   midi.addEvent (juce::MidiMessage::noteOn (1, 62, 0.9f), 3);
        if (block == 20)  midi.addEvent (juce::MidiMessage::noteOn (1, 65, 0.9f), 100);
        if (block == 40)  midi.addEvent (juce::MidiMessage::pitchWheel (1, 12000), 0);
        if (block == 60)  midi.addEvent (juce::MidiMessage::noteOff (1, 65), 7);
        if (block == 80)  midi.addEvent (juce::MidiMessage::noteOff (1, 62), 0);

        juce::AudioBuffer<float> buffer (2, 512);
        buffer.clear();
        engine.process (buffer, midi);

        for (int i = 0; i < 512; ++i)
        {
            const float s = buffer.getSample (0, i);
            result.finite = result.finite && std::isfinite (s);
            result.peak = juce::jmax (result.peak, std::abs (s));
            if (block == numBlocks - 1)
                result.tailPeak = juce::jmax (result.tailPeak, std::abs (s));
        }
    }

    return result;
}
} // namespace

int main()
{
    using namespace amv;

    // Tuning
    {
        Tuning t;
        t.set (2, 0, false);
        check (near (t.pitchFor (69), 69.0f) && near (t.stepAbove (69), 1.0f), "chromatic keeps equal temperament");

        t.set (2, 1, false); // D Hijaz: D Eb F# G A Bb C
        check (near (t.pitchFor (62), 62.0f) && near (t.pitchFor (63), 63.0f) && near (t.pitchFor (66), 66.0f), "Hijaz degrees on their keys");
        check (near (t.pitchFor (64), 64.0f), "non-scale key stays put without snap");
        check (near (t.stepAbove (63), 3.0f), "Hijaz augmented second above the flat second");
        check (near (t.stepAbove (72), 2.0f), "step above the seventh wraps to the root");

        t.set (2, 1, true);
        check (near (t.pitchFor (64), 63.0f), "snap: E goes down to Eb in D Hijaz");
        check (near (t.pitchFor (65), 66.0f), "snap: F goes up to F#");
        check (near (t.pitchFor (73), 72.0f), "snap: C# sits between two degrees and goes down to C");

        t.set (2, 2, true); // D Hijazkar has C#, so C snaps up to it
        check (near (t.pitchFor (72), 73.0f) && near (t.pitchFor (75), 75.0f), "snap: C goes up to C# in D Hijazkar");

        t.set (0, 6, false); // C Rast: quarter-flat third and seventh
        check (near (t.pitchFor (64), 63.5f) && near (t.pitchFor (71), 70.5f), "Rast quarter tones on E and B");
        check (near (t.pitchFor (76), 75.5f) && near (t.pitchFor (52), 51.5f), "quarter tones repeat in every octave");
        check (near (t.stepAbove (62), 1.5f), "Rast three-quarter step above the second");
    }

    // Engine
    {
        EngineParams lead;
        const auto a = render (lead);
        check (a.finite && a.peak > 0.05f && a.tailPeak < 1.0e-4f, "lead renders and falls silent");

        EngineParams acc;
        acc.model = Model::accordion;
        acc.registerPos = 1.0f;
        const auto b = render (acc);
        check (b.finite && b.peak > 0.05f && b.peak < 4.0f && b.tailPeak < 1.0e-4f, "accordion renders and falls silent");

        EngineParams orn = acc;
        orn.mono = false;
        orn.scale = 7;
        orn.snap = true;
        orn.grace = orn.mordent = orn.trill = orn.fall = true;
        orn.resonance = 1.0f;
        orn.filterEnv = 1.0f;
        const auto c = render (orn);
        check (c.finite && c.peak > 0.05f && c.tailPeak < 1.0e-4f, "poly with every ornament and full resonance stays finite");
    }

    // Every model renders, stays in a sane range and falls silent.
    {
        const char* names[] { "lead", "accordion", "gusle", "frula", "kaval", "gajde" };
        for (int m = 0; m < 6; ++m)
        {
            EngineParams p;
            p.model = (Model) m;
            if (p.model == Model::gajde)
                p.macro = { 0.6f, 0.0f, 0.8f, 0.5f };
            const auto r = render (p);
            std::printf ("      %-10s peak %.3f\n", names[m], (double) r.peak);
            check (r.finite && r.peak > 0.05f && r.peak < 3.0f && r.tailPeak < 1.0e-4f, names[m]);
        }
    }

    // Arpeggiator: 7/8 (3+2+2) in sixteenths at 120 bpm, host playing from bar start.
    {
        Arpeggiator arp;
        arp.prepare (48000.0);
        ArpParams ap;
        ap.on = true;
        ap.meter = 2;
        ap.rate = 1;
        ap.gate = 0.5f;
        ap.accent = 1.0f;

        std::vector<int> onTimes, onNotes;
        std::vector<float> onVelocities;
        int ons = 0, offs = 0;
        const int block = 512, stepSamples = 6000;

        for (int b = 0; b < 400; ++b)
        {
            juce::MidiBuffer in, out;
            if (b == 0)
            {
                in.addEvent (juce::MidiMessage::noteOn (1, 62, 1.0f), 0);
                in.addEvent (juce::MidiMessage::noteOn (1, 65, 1.0f), 0);
                in.addEvent (juce::MidiMessage::noteOn (1, 69, 1.0f), 0);
            }
            if (b == 300)
                for (int n : { 62, 65, 69 })
                    in.addEvent (juce::MidiMessage::noteOff (1, n), 0);

            TransportInfo t;
            t.playing = true;
            t.bpm = 120.0;
            t.ppq = (double) (b * block) / 24000.0;
            arp.process (in, out, block, ap, t);

            for (const auto meta : out)
            {
                const auto m = meta.getMessage();
                if (m.isNoteOn())
                {
                    ++ons;
                    onTimes.push_back (b * block + meta.samplePosition);
                    onNotes.push_back (m.getNoteNumber());
                    onVelocities.push_back (m.getFloatVelocity());
                }
                else if (m.isNoteOff())
                    ++offs;
            }
        }

        bool timing = onTimes.size() >= 14;
        bool accents = timing, order = timing;
        for (size_t i = 0; timing && i < 14; ++i)
        {
            timing = timing && std::abs (onTimes[i] - (int) i * stepSamples) <= 1;
            const bool accent = (i % 7 == 0) || (i % 7 == 3) || (i % 7 == 5);
            accents = accents && (accent ? onVelocities[i] > 0.9f : onVelocities[i] < 0.5f);
            const int expected[] { 62, 65, 69 };
            order = order && onNotes[i] == expected[i % 3];
        }

        check (timing, "arp steps land on the sixteenth grid");
        check (accents, "arp accents follow 3+2+2");
        check (order, "arp plays held notes upwards");
        check (ons == 26 && offs == ons, "arp stops after release and every note gets a note off");

        // Bar-locked mode restarts the pattern on each 4/4 bar: step 16 is an accent again.
        Arpeggiator locked;
        locked.prepare (48000.0);
        ap.polymeter = false;
        std::vector<float> velocities;
        for (int b = 0; b < 240; ++b)
        {
            juce::MidiBuffer in, out;
            if (b == 0)
                in.addEvent (juce::MidiMessage::noteOn (1, 60, 1.0f), 0);
            TransportInfo t;
            t.playing = true;
            t.bpm = 120.0;
            t.ppq = (double) (b * block) / 24000.0;
            locked.process (in, out, block, ap, t);
            for (const auto meta : out)
                if (meta.getMessage().isNoteOn())
                    velocities.push_back (meta.getMessage().getFloatVelocity());
        }

        check (velocities.size() > 17 && velocities[14] > 0.9f && velocities[15] < 0.5f && velocities[16] > 0.9f && velocities[17] < 0.5f,
               "arp without polymeter restarts on the bar");
    }

    // Effects
    {
        const double sr = 48000.0;
        const int block = 480;
        const auto run = [&] (FxParams p, int blocks, bool impulse, auto&& perBlock)
        {
            FxChain fx;
            fx.prepare (sr, block);
            for (int b = 0; b < blocks; ++b)
            {
                juce::AudioBuffer<float> buffer (2, block);
                for (int i = 0; i < block; ++i)
                {
                    const float s = impulse ? ((b == 0 && i < 48) ? 0.8f : 0.0f)
                                            : 0.5f * std::sin (juce::MathConstants<float>::twoPi * 220.0f * (float) (b * block + i) / (float) sr);
                    buffer.setSample (0, i, s);
                    buffer.setSample (1, i, s);
                }

                TransportInfo t;
                t.playing = true;
                t.bpm = 120.0;
                t.ppq = (double) (b * block) / 24000.0;
                fx.process (buffer, p, t);
                perBlock (b, buffer);
            }
        };

        // Everything on: finite and never above full scale.
        {
            FxParams p;
            p.distOn = p.gateOn = p.delayOn = p.reverbOn = p.duckOn = true;
            p.distDriveDb = 36.0f;
            p.delayFeedback = 0.95f;
            bool ok = true;
            float peak = 0.0f;
            for (int type = 0; type < 3; ++type)
            {
                p.distType = type;
                run (p, 300, false, [&] (int, const juce::AudioBuffer<float>& b)
                {
                    for (int ch = 0; ch < 2; ++ch)
                        for (int i = 0; i < block; ++i)
                        {
                            ok = ok && std::isfinite (b.getSample (ch, i));
                            peak = juce::jmax (peak, std::abs (b.getSample (ch, i)));
                        }
                });
            }
            check (ok && peak > 0.05f && peak <= 1.0f, "all effects at extreme settings stay finite and under full scale");
        }

        // Gate: sixteenths at 120 bpm are 6000 samples; open for the first half of each step.
        {
            FxParams p;
            p.gateOn = true;
            p.gateSmoothMs = 0.5f;
            float openPeak = 0.0f, closedPeak = 0.0f;
            run (p, 50, false, [&] (int b, const juce::AudioBuffer<float>& buf)
            {
                for (int i = 0; i < block; ++i)
                {
                    const int inStep = (b * block + i) % 6000;
                    const float a = std::abs (buf.getSample (0, i));
                    if (inStep > 500 && inStep < 2500)
                        openPeak = juce::jmax (openPeak, a);
                    if (inStep > 3500 && inStep < 5500)
                        closedPeak = juce::jmax (closedPeak, a);
                }
            });
            check (openPeak > 0.4f && closedPeak < 0.01f, "trance gate opens and closes on the grid");
        }

        // Delay: 3/16 at 120 bpm is 18000 samples; ping-pong sends the first echo left, the second right.
        {
            FxParams p;
            p.delayOn = true;
            p.delayMix = 1.0f;
            float firstL = 0.0f, firstR = 0.0f, secondR = 0.0f, between = 0.0f;
            run (p, 100, true, [&] (int b, const juce::AudioBuffer<float>& buf)
            {
                for (int i = 0; i < block; ++i)
                {
                    const int n = b * block + i;
                    const float l = std::abs (buf.getSample (0, i)), r = std::abs (buf.getSample (1, i));
                    if (n >= 17900 && n < 18400) { firstL = juce::jmax (firstL, l); firstR = juce::jmax (firstR, r); }
                    if (n >= 35900 && n < 36400) secondR = juce::jmax (secondR, r);
                    if (n >= 6000 && n < 15000) between = juce::jmax (between, juce::jmax (l, r));
                }
            });
            check (firstL > 0.1f && firstR < 0.01f && secondR > 0.02f && between < 0.01f, "ping-pong delay echoes at 3/16 on alternating sides");
        }

        // Kick duck: quiet right after each beat, back to full level before the next.
        {
            FxParams p;
            p.duckOn = true;
            p.duckDepth = 1.0f;
            float early = 0.0f, late = 0.0f;
            run (p, 150, false, [&] (int b, const juce::AudioBuffer<float>& buf)
            {
                for (int i = 0; i < block; ++i)
                {
                    const int inBeat = (b * block + i) % 24000;
                    const float a = std::abs (buf.getSample (0, i));
                    if (b > 50 && inBeat > 300 && inBeat < 900)
                        early = juce::jmax (early, a);
                    if (inBeat > 18000 && inBeat < 23000)
                        late = juce::jmax (late, a);
                }
            });
            check (early < 0.1f && late > 0.4f, "kick duck dips on the beat and recovers");
        }
    }

    // Wavetables and the oscillator section
    {
        const auto& bank = WavetableBank::get();
        const float* sine = bank.wave (0, 0, 0);
        bool isSine = true, allFinite = true, wraps = true;
        for (int i = 0; i < WavetableBank::tableSize; ++i)
            isSine = isSine && near (sine[i], std::sin (juce::MathConstants<float>::twoPi * (float) i / (float) WavetableBank::tableSize), 0.002f);
        check (isSine, "first frame of Basic Shapes is a sine");

        for (int t = 0; t < WavetableBank::numTables; ++t)
            for (int f = 0; f < WavetableBank::numFrames; ++f)
            {
                float peak = 0.0f;
                const float* w = bank.wave (t, f, 0);
                for (int i = 0; i <= WavetableBank::tableSize; ++i)
                {
                    allFinite = allFinite && std::isfinite (w[i]);
                    peak = juce::jmax (peak, std::abs (w[i]));
                }
                allFinite = allFinite && near (peak, 1.0f, 0.01f);
                wraps = wraps && near (w[WavetableBank::tableSize], w[0], 1.0e-9f);
            }
        check (allFinite && wraps, "every wavetable frame is normalised and wraps");

        // The top level of a saw keeps two harmonics, so it must be smooth.
        const float* top = bank.wave (0, 10, WavetableBank::numLevels - 1);
        float maxStep = 0.0f;
        for (int i = 0; i < WavetableBank::tableSize; ++i)
            maxStep = juce::jmax (maxStep, std::abs (top[i + 1] - top[i]));
        check (maxStep < 0.01f, "highest band-limited level has no sharp edges");
        check (WavetableBank::levelFor (0.0001f) == 0 && WavetableBank::levelFor (0.01f) == 5 && WavetableBank::levelFor (0.4f) == 9,
               "band-limited level follows the pitch");

        EngineParams base;
        const auto reference = render (base);

        EngineParams p;
        p.engineLevel = 0.0f;
        const auto silent = render (p);
        check (silent.peak < 1.0e-9f, "nothing sounds with every source turned down");

        p.oscA.level = 1.0f;
        p.oscA.table = 0;
        p.oscA.position = 0.66f;
        p.oscA.unison = 7;
        p.oscA.detuneCents = 30.0f;
        const auto supersaw = render (p);
        check (supersaw.finite && supersaw.peak > 0.05f && supersaw.peak < 2.5f && supersaw.tailPeak < 0.001f, "oscillator A with 7 voice unison");

        for (int t = 0; t < WavetableBank::numTables; ++t)
        {
            EngineParams q;
            q.engineLevel = 0.0f;
            q.oscB.level = 1.0f;
            q.oscB.table = t;
            q.oscB.position = 0.5f;
            q.oscB.pitch = 12.0f;
            const auto r = render (q);
            check (r.finite && r.peak > 0.05f && r.peak < 2.0f, getWavetableNames()[(size_t) t]);
        }

        EngineParams q;
        q.engineLevel = 0.0f;
        q.subLevel = 1.0f;
        q.subShape = 0.5f;
        q.subOctave = 2;
        const auto sub = render (q);
        q.subLevel = 0.0f;
        q.noiseLevel = 1.0f;
        const auto noiseOnly = render (q);
        check (sub.finite && sub.peak > 0.05f && noiseOnly.finite && noiseOnly.peak > 0.02f, "sub oscillator and noise");

        base.oscA.level = 0.5f;
        const auto layered = render (base);
        check (layered.finite && ! near (layered.peak, reference.peak, 1.0e-4f), "oscillators layer with the Balkan engine");
    }

    std::printf ("%d failure(s)\n", failures);
    return failures == 0 ? 0 : 1;
}
