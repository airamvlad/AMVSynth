// Offline checks for the tuning tables and the synth engine. Exits non-zero on failure.
#include "Arpeggiator.h"
#include "Effects.h"
#include "Presets.h"
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

/** Just enough of a plugin to own the real parameter set. */
struct ParameterHost final : public juce::AudioProcessor
{
    const juce::String getName() const override { return "test"; }
    void prepareToPlay (double, int) override {}
    void releaseResources() override {}
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override {}
    using AudioProcessor::processBlock;
    double getTailLengthSeconds() const override { return 0.0; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    bool hasEditor() const override { return false; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override {}
    void setStateInformation (const void*, int) override {}

    juce::AudioProcessorValueTreeState apvts { *this, nullptr, "AMVSynth", amv::createParameterLayout() };
};

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
        const char* names[] { "lead", "accordion", "gusle", "frula", "kaval", "gajde", "zurla", "tambura", "tapan" };
        for (int m = 0; m < 9; ++m)
        {
            EngineParams p;
            p.model = (Model) m;
            if (p.model == Model::gajde)
                p.macro = { 0.6f, 0.0f, 0.8f, 0.5f };
            if (p.model == Model::zurla)
                p.macro = { 0.75f, 0.5f, 0.2f, 0.6f };
            if (p.model == Model::tambura)
                p.macro = { 0.65f, 0.55f, 0.4f, 0.6f };
            if (p.model == Model::tapan)
                p.macro = { 0.5f, 0.5f, 0.45f, 0.3f };
            const auto r = render (p);
            std::printf ("      %-10s peak %.3f\n", names[m], (double) r.peak);
            check (r.finite && r.peak > 0.05f && r.peak < 3.0f && r.tailPeak < 1.0e-4f, names[m]);
        }
    }

    // The plucked string is in tune: find the period of a steady A3 (220 Hz) by autocorrelation.
    {
        const auto measure = [] (float tone)
        {
            SynthEngine engine;
            engine.prepare (48000.0, 512);
            EngineParams p;
            p.model = Model::tambura;
            p.macro = { 0.2f, 1.0f, 0.0f, tone };
            p.vibDepthCents = 0.0f;
            engine.setParams (p);

            std::vector<float> out;
            for (int block = 0; block < 60; ++block)
            {
                juce::MidiBuffer midi;
                if (block == 0)
                    midi.addEvent (juce::MidiMessage::noteOn (1, 57, 0.9f), 0);
                juce::AudioBuffer<float> buffer (1, 512);
                buffer.clear();
                engine.process (buffer, midi);
                out.insert (out.end(), buffer.getReadPointer (0), buffer.getReadPointer (0) + 512);
            }

            const int start = 9600, length = 12000;
            std::vector<double> ac (400, 0.0);
            int best = 150;
            for (int lag = 150; lag < 300; ++lag)
            {
                for (int i = 0; i < length; ++i)
                    ac[(size_t) lag] += (double) out[(size_t) (start + i)] * out[(size_t) (start + i + lag)];
                if (ac[(size_t) lag] > ac[(size_t) best])
                    best = lag;
            }

            // Refine the peak between samples with a parabola.
            const double a = ac[(size_t) best - 1], b = ac[(size_t) best], c = ac[(size_t) best + 1];
            return (float) (48000.0 / ((double) best + 0.5 * (a - c) / (a - 2.0 * b + c)));
        };
        const float bright = measure (0.9f), dark = measure (0.1f);
        std::printf ("      tambura A3: %.2f Hz bright, %.2f Hz dark\n", (double) bright, (double) dark);
        check (near (bright, 220.0f, 0.4f) && near (dark, 220.0f, 0.4f), "tambura string is in tune at both tone settings");
    }

    // The tapan is struck again on every note, even in mono legato.
    {
        SynthEngine engine;
        engine.prepare (48000.0, 512);
        EngineParams p;
        p.model = Model::tapan;
        p.macro = { 0.5f, 0.0f, 0.2f, 0.0f };
        p.vibDepthCents = 0.0f;
        engine.setParams (p);
        float before = 0.0f, after = 0.0f;
        for (int block = 0; block < 60; ++block)
        {
            juce::MidiBuffer midi;
            if (block == 0)
                midi.addEvent (juce::MidiMessage::noteOn (1, 40, 0.9f), 0);
            if (block == 50)
                midi.addEvent (juce::MidiMessage::noteOn (1, 43, 0.9f), 0); // overlaps the first note
            juce::AudioBuffer<float> buffer (1, 512);
            buffer.clear();
            engine.process (buffer, midi);
            const float peak = buffer.getMagnitude (0, 0, 512);
            if (block >= 46 && block < 50)
                before = juce::jmax (before, peak);
            if (block >= 50 && block < 54)
                after = juce::jmax (after, peak);
        }
        check (before < 0.05f && after > 0.2f, "tapan decays on its own and is struck again by an overlapping note");
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

        // Distortion does not alias: drive a 5 kHz sine hard and look at 3 kHz, where the ninth
        // harmonic (45 kHz) would land if it folded back at 48 kHz.
        {
            FxChain fx;
            fx.prepare (sr, block);
            FxParams p;
            p.distOn = true;
            p.distDriveDb = 24.0f;
            const auto twoPi = juce::MathConstants<double>::twoPi;
            double re5 = 0.0, im5 = 0.0, re3 = 0.0, im3 = 0.0;

            for (int b = 0; b < 60; ++b)
            {
                juce::AudioBuffer<float> buffer (2, block);
                for (int i = 0; i < block; ++i)
                {
                    const auto s = (float) (0.25 * std::sin (twoPi * 5000.0 * (double) (b * block + i) / sr));
                    buffer.setSample (0, i, s);
                    buffer.setSample (1, i, s);
                }
                fx.process (buffer, p, {});

                if (b >= 10)
                    for (int i = 0; i < block; ++i)
                    {
                        const double t = (double) (b * block + i) / sr, v = buffer.getSample (0, i);
                        re5 += v * std::cos (twoPi * 5000.0 * t);
                        im5 += v * std::sin (twoPi * 5000.0 * t);
                        re3 += v * std::cos (twoPi * 3000.0 * t);
                        im3 += v * std::sin (twoPi * 3000.0 * t);
                    }
            }

            const double ratio = std::hypot (re3, im3) / std::hypot (re5, im5);
            std::printf ("      alias at 3 kHz: %.1f dB below the tone\n", -20.0 * std::log10 (ratio + 1.0e-12));
            check (ratio < 0.003, "distortion is oversampled and does not alias");
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

        // Unison width: spread copies differ between the sides, width zero stays mono.
        {
            const auto difference = [] (float width, int unison)
            {
                SynthEngine engine;
                engine.prepare (48000.0, 512);
                EngineParams w;
                w.engineLevel = 0.0f;
                w.oscA = { 0, 0.66f, unison, 25.0f, 0.0f, 0.0f, 1.0f, width };
                engine.setParams (w);
                float diff = 0.0f, level = 0.0f;
                for (int block = 0; block < 40; ++block)
                {
                    juce::MidiBuffer midi;
                    if (block == 0)
                        midi.addEvent (juce::MidiMessage::noteOn (1, 57, 0.9f), 0);
                    juce::AudioBuffer<float> buffer (2, 512);
                    buffer.clear();
                    engine.process (buffer, midi);
                    for (int i = 0; i < 512; ++i)
                    {
                        diff = juce::jmax (diff, std::abs (buffer.getSample (0, i) - buffer.getSample (1, i)));
                        level = juce::jmax (level, std::abs (buffer.getSample (0, i)));
                    }
                }
                return level > 0.05f ? diff / level : -1.0f;
            };
            const float wide = difference (1.0f, 7), narrow = difference (0.0f, 7), single = difference (1.0f, 1);
            check (wide > 0.3f && near (narrow, 0.0f, 1.0e-6f) && near (single, 0.0f, 1.0e-6f), "unison width spreads the copies, a single copy stays centred");
        }

        base.oscA.level = 0.5f;
        const auto layered = render (base);
        check (layered.finite && ! near (layered.peak, reference.peak, 1.0e-4f), "oscillators layer with the Balkan engine");
    }

    // Modulation
    {
        const double sr = 48000.0;
        TransportInfo stopped, playing;
        playing.playing = true;
        playing.bpm = 120.0;

        // A free 10 Hz sine LFO crosses zero upwards 10 times a second.
        {
            Lfo lfo;
            LfoParams lp;
            lp.rateHz = 10.0f;
            int crossings = 0;
            float last = -1.0f, lowest = 0.0f, highest = 0.0f;
            for (int i = 0; i < 1500; ++i)
            {
                const float v = lfo.next (32, sr, lp, stopped, 0.0);
                if (last < 0.0f && v >= 0.0f)
                    ++crossings;
                last = v;
                lowest = juce::jmin (lowest, v);
                highest = juce::jmax (highest, v);
            }
            check (crossings == 10 && near (lowest, -1.0f, 0.01f) && near (highest, 1.0f, 0.01f), "free LFO runs at its rate over the full range");
        }

        // A synced LFO follows the host position.
        {
            Lfo lfo;
            LfoParams lp;
            lp.sync = true;
            lp.division = 5; // 1/4
            lp.shape = 2;    // saw, falling from 1 to -1
            check (near (lfo.next (32, sr, lp, playing, 8.0), 1.0f) && near (lfo.next (32, sr, lp, playing, 8.5), 0.0f)
                       && near (lfo.next (32, sr, lp, playing, 8.75), -0.5f),
                   "synced LFO follows the host position");

            lp.shape = 4;
            const float a = lfo.next (32, sr, lp, playing, 3.2), b = lfo.next (32, sr, lp, playing, 3.9), c = lfo.next (32, sr, lp, playing, 4.1);
            const float again = lfo.next (32, sr, lp, playing, 3.5);
            check (near (a, b) && ! near (a, c) && near (a, again) && std::abs (a) <= 1.0f, "random LFO holds one repeatable value per cycle");
        }

        EngineParams base;
        base.vibDepthCents = 0.0f;
        const auto reference = render (base);

        // Slots without a source or amount change nothing.
        {
            auto p = base;
            p.slots[0] = { mod::srcNone, mod::dstVolume, 1.0f };
            p.slots[1] = { mod::srcLfo1, mod::dstVolume, 0.0f };
            check (near (render (p).peak, reference.peak, 1.0e-6f), "unused modulation slots leave the sound alone");
        }

        // A macro on volume scales the level: fully down is silent.
        {
            auto p = base;
            p.modMacros[0] = 1.0f;
            p.slots[0] = { mod::srcMacro1, mod::dstVolume, -1.0f };
            const auto muted = render (p);
            p.slots[0].amount = -0.5f;
            const auto half = render (p);
            check (muted.peak < 1.0e-6f && near (half.peak, reference.peak * 0.5f, reference.peak * 0.02f), "macro to volume scales the level");
        }

        // Everything at once, at full amounts, stays finite and bounded.
        {
            auto p = base;
            p.oscA.level = 0.6f;
            p.oscB.level = 0.6f;
            p.noiseLevel = 0.2f;
            p.lfos[0] = { 3, 31.0f, false, 0 };
            p.lfos[1] = { 4, 9.0f, false, 0 };
            p.modMacros = { 1.0f, 0.5f, 0.2f, 0.9f };
            p.slots[0] = { mod::srcLfo1, mod::dstPitch, 1.0f };
            p.slots[1] = { mod::srcLfo2, mod::dstCutoff, 1.0f };
            p.slots[2] = { mod::srcFilterEnv, mod::dstOscAPosition, 1.0f };
            p.slots[3] = { mod::srcVelocity, mod::dstResonance, 1.0f };
            p.slots[4] = { mod::srcMacro1, mod::dstOscBLevel, -1.0f };
            p.slots[5] = { mod::srcLfo1, mod::dstVolume, 1.0f };
            const auto r = render (p);
            check (r.finite && r.peak > 0.02f && r.peak < 20.0f && r.tailPeak < 0.001f, "full modulation matrix stays finite" /* full resonance is loud before the limiter */);
        }

        // The mod wheel opens the filter.
        {
            const auto wheel = [] (int value)
            {
                SynthEngine engine;
                engine.prepare (48000.0, 512);
                EngineParams p;
                p.cutoffHz = 200.0f;
                p.filterEnv = 0.0f;
                p.vibDepthCents = 0.0f;
                p.slots[0] = { mod::srcModWheel, mod::dstCutoff, 1.0f };
                engine.setParams (p);
                double energy = 0.0;
                for (int block = 0; block < 60; ++block)
                {
                    juce::MidiBuffer midi;
                    if (block == 0)
                    {
                        midi.addEvent (juce::MidiMessage::controllerEvent (1, 1, value), 0);
                        midi.addEvent (juce::MidiMessage::noteOn (1, 50, 0.9f), 1);
                    }
                    juce::AudioBuffer<float> buffer (1, 512);
                    buffer.clear();
                    engine.process (buffer, midi);
                    if (block > 20)
                        for (int i = 1; i < 512; ++i)
                        {
                            const double d = buffer.getSample (0, i) - buffer.getSample (0, i - 1);
                            energy += d * d; // the sample to sample difference grows with brightness
                        }
                }
                return energy;
            };
            check (wheel (127) > wheel (0) * 4.0, "mod wheel to cutoff opens the filter");
        }
    }

    // Factory presets
    {
        juce::ScopedJuceInitialiser_GUI gui;
        ParameterHost host;
        const auto& presets = getFactoryPresets();
        bool valid = true, unique = true;

        for (size_t i = 0; i < presets.size(); ++i)
        {
            for (size_t j = 0; j < i; ++j)
                unique = unique && juce::String (presets[i].name) != presets[j].name;

            for (const auto& [paramId, value] : presets[i].values)
            {
                auto* p = host.apvts.getParameter (paramId);
                const bool ok = p != nullptr && host.apvts.getParameterRange (paramId).getRange().expanded (1.0e-4f).contains (value);
                if (! ok)
                    std::printf ("      %s: %s = %f\n", presets[i].name, paramId.toRawUTF8(), (double) value);
                valid = valid && ok;
            }
        }
        check (presets.size() >= 16 && unique, "factory presets have unique names");
        check (valid, "every preset value names a real parameter and is in range");

        // Applying a preset sets its values and resets everything else.
        applyPreset (host.apvts, presets[1]);
        const bool applied = near (host.apvts.getRawParameterValue (id::scale)->load(), 1.0f) && host.apvts.getRawParameterValue (id::delayOn)->load() > 0.5f;
        applyPreset (host.apvts, presets[0]);
        const bool reset = near (host.apvts.getRawParameterValue (id::scale)->load(), 0.0f) && host.apvts.getRawParameterValue (id::delayOn)->load() < 0.5f
                               && near (host.apvts.getRawParameterValue (id::cutoff)->load(), 3200.0f, 1.0f);
        check (applied && reset, "applying a preset sets its values and Init restores the defaults");
    }

    std::printf ("%d failure(s)\n", failures);
    return failures == 0 ? 0 : 1;
}
