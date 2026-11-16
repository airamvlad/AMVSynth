// Offline checks for the tuning tables and the synth engine. Exits non-zero on failure.
#include "Arpeggiator.h"
#include "Effects.h"
#include "Presets.h"
#include "Sequencer.h"
#include "SynthEngine.h"

#include <cstdio>
#include <functional>

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

        // Pythagorean on D: the fifth is a pure 3:2 and the major third is the wide 81:64.
        t.set (2, (int) getScales().size() - 1, false);
        const float fifth = 12.0f * std::log2 (1.5f), third = 12.0f * std::log2 (81.0f / 64.0f);
        check (juce::String (getScales().back().name) == "Pythagorean" && near (t.pitchFor (62), 62.0f)
                   && near (t.pitchFor (69), 62.0f + fifth, 1.0e-4f) && near (t.pitchFor (66), 62.0f + third, 1.0e-4f)
                   && near (t.pitchFor (57), 50.0f + fifth, 1.0e-4f),
               "Pythagorean tuning has pure fifths from the root");
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
        const char* names[] { "lead", "accordion", "gusle", "frula", "kaval", "gajde", "zurla", "tambura", "tapan", "dvojnice", "clarinet", "truba", "violin",
                                "sargija", "kanun", "darbuka", "voice" };
        for (int m = 0; m < 17; ++m)
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
            if (p.model == Model::dvojnice)
                p.macro = { 0.7f, 0.7f, -3.0f, 0.3f };
            if (p.model == Model::clarinet)
                p.macro = { 0.65f, 0.3f, 0.1f, 0.5f };
            if (p.model == Model::truba)
                p.macro = { 0.65f, 0.5f, 0.15f, 0.0f };
            if (p.model == Model::violin)
                p.macro = { 0.7f, 0.3f, 0.2f, 0.6f };
            if (p.model == Model::sargija)
                p.macro = { 0.75f, 0.5f, 0.5f, 0.7f };
            if (p.model == Model::kanun)
                p.macro = { 0.7f, 0.6f, 0.4f, 0.5f };
            if (p.model == Model::darbuka)
                p.macro = { 0.2f, 0.5f, 0.5f, 0.3f };
            if (p.model == Model::voice)
                p.macro = { 0.0f, 0.2f, 0.5f, 0.5f };
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

    // Step sequencer: 7 steps in sixteenths at 120 bpm, so a step is 6000 samples.
    {
        struct Event { int time; int note; float velocity; bool on; };
        const int block = 512;

        const auto run = [&] (const SeqPattern& pattern, SeqParams sp, int blocks, int releaseBlock, bool playing, int pressAt = 0)
        {
            StepSequencer seq;
            seq.prepare (48000.0);
            sp.on = true;
            sp.pattern = &pattern;
            std::vector<Event> events;

            for (int b = 0; b < blocks; ++b)
            {
                juce::MidiBuffer in, out;
                if (b == 0)
                    in.addEvent (juce::MidiMessage::noteOn (1, 62, 1.0f), pressAt);
                if (b == releaseBlock)
                    in.addEvent (juce::MidiMessage::noteOff (1, 62), 0);

                TransportInfo t;
                t.playing = playing;
                t.bpm = 120.0;
                t.ppq = (double) (b * block) / 24000.0;
                seq.process (in, out, block, sp, t);

                for (const auto meta : out)
                {
                    const auto m = meta.getMessage();
                    if (m.isNoteOn() || m.isNoteOff())
                        events.push_back ({ b * block + meta.samplePosition, m.getNoteNumber(), m.getFloatVelocity(), m.isNoteOn() });
                }
            }

            return events;
        };

        const auto onsOf = [] (const std::vector<Event>& events)
        {
            std::vector<Event> ons;
            for (const auto& e : events)
                if (e.on)
                    ons.push_back (e);
            return ons;
        };

        SeqPattern pattern;
        const int notes[] { 0, 3, 5, 7, 0, -2, 12 };
        for (size_t i = 0; i < 7; ++i)
        {
            pattern.steps[i].note = notes[i];
            pattern.steps[i].velocity = i == 0 ? 1.0f : 0.5f;
            pattern.steps[i].gate = 0.5f;
        }
        pattern.steps[2].gate = 0.0f;  // a rest
        pattern.steps[4].slide = true; // slides into step 5

        SeqParams sp;
        sp.length = 7;
        sp.rate = 1;

        const auto events = run (pattern, sp, 200, 160, true);
        const auto ons = onsOf (events);

        // Twelve note ons in two passes: steps 0 1 3 4 5 6, twice.
        const int expectedStep[] { 0, 1, 3, 4, 5, 6, 7, 8, 10, 11, 12, 13 };
        bool timing = ons.size() >= 12, pitches = timing, velocities = timing;
        for (size_t i = 0; timing && i < 12; ++i)
        {
            timing = timing && std::abs (ons[i].time - expectedStep[i] * 6000) <= 1;
            pitches = pitches && ons[i].note == 62 + notes[expectedStep[i] % 7];
            velocities = velocities && (expectedStep[i] % 7 == 0 ? ons[i].velocity > 0.95f : std::abs (ons[i].velocity - 0.5f) < 0.02f);
        }

        check (timing, "sequencer steps land on the grid and a rest stays silent");
        check (pitches, "sequencer notes are the held key plus the step");
        check (velocities, "sequencer steps have their own velocity");

        int offs = 0, slideOff = -1, plainOff = -1;
        for (const auto& e : events)
        {
            if (! e.on)
                ++offs;
            if (! e.on && e.note == 62 && slideOff < 0 && e.time > 4 * 6000)
                slideOff = e.time;
            if (! e.on && e.note == 65 && plainOff < 0)
                plainOff = e.time;
        }

        check (std::abs (plainOff - 9000) <= 1, "a step lasts as long as its gate");
        check (slideOff > 5 * 6000 && slideOff < 5 * 6000 + 1000, "a slide holds the note into the next step");
        check (offs == (int) ons.size() && ons.back().time < 160 * block, "sequencer stops on release and every note gets a note off");

        // Latch keeps it running after the key is released.
        SeqParams latched = sp;
        latched.latch = true;
        check (onsOf (run (pattern, latched, 200, 20, true)).back().time > 180 * block, "latch keeps the sequence playing after release");

        // Two steps with the same note joined by a slide are one long note.
        SeqPattern tied;
        tied.steps[0].slide = true;
        const auto tiedEvents = run (tied, sp, 40, 39, true);
        check (tiedEvents.size() >= 3 && tiedEvents[0].on && ! tiedEvents[1].on && std::abs (tiedEvents[1].time - 9000) <= 1 && tiedEvents[2].time == 12000,
               "a slide into the same note is a tie");

        // Swing makes every second step late.
        SeqParams swung = sp;
        swung.swing = 0.5f;
        const auto swungOns = onsOf (run (SeqPattern(), swung, 60, 59, true));
        check (swungOns.size() > 3 && swungOns[0].time == 0 && std::abs (swungOns[1].time - 7500) <= 1 && std::abs (swungOns[2].time - 12000) <= 1
                   && std::abs (swungOns[3].time - 19500) <= 1,
               "swing delays every second step");

        // With the host stopped the pattern starts on the key press.
        const auto freeOns = onsOf (run (pattern, sp, 60, 59, false, 100));
        check (freeOns.size() > 2 && freeOns[0].time == 100 && freeOns[0].note == 62 && std::abs (freeOns[1].time - 6100) <= 1,
               "with the host stopped the sequence starts on the key press");

        // The bar-locked mode starts the pattern again on every 4/4 bar.
        SeqParams locked = sp;
        locked.freeCycle = false;
        const auto lockedOns = onsOf (run (pattern, locked, 240, 239, true));
        bool restarts = false;
        for (const auto& e : lockedOns)
            restarts = restarts || (std::abs (e.time - 16 * 6000) <= 1 && e.note == 62 && e.velocity > 0.95f);
        check (restarts, "sequencer without free cycle restarts on the bar");

        const auto copy = SeqPattern::fromString (pattern.toString());
        bool same = true;
        for (size_t i = 0; i < copy.steps.size(); ++i)
            same = same && copy.steps[i].note == pattern.steps[i].note && copy.steps[i].slide == pattern.steps[i].slide
                   && std::abs (copy.steps[i].gate - pattern.steps[i].gate) < 0.002f && std::abs (copy.steps[i].velocity - pattern.steps[i].velocity) < 0.002f;
        check (same, "a sequencer pattern survives being saved as text");

        // Switched off, it lets the notes through.
        StepSequencer off;
        off.prepare (48000.0);
        juce::MidiBuffer in, out;
        in.addEvent (juce::MidiMessage::noteOn (1, 60, 0.7f), 5);
        off.process (in, out, block, SeqParams(), TransportInfo());
        check (out.getNumEvents() == 1, "sequencer switched off passes the notes through");
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

        // Effect order: with the gate after the delay there are no echoes while the gate is shut.
        {
            const auto echoLevel = [&] (int order)
            {
                FxParams p;
                p.order = order;
                p.gateOn = true;
                p.gatePattern = 1;    // offbeat: shut on the first step
                p.gateRate = 0;       // 1/8 notes, 12000 samples at 120 bpm
                p.gateLength = 1.0f;
                p.delayOn = true;
                p.delayTime = 0;      // 1/16, so the echo of a hit at 12000 lands at 18000 and 24000
                p.delayFeedback = 0.6f;
                p.delayMix = 0.5f;
                p.delayPingPong = false;
                float inShutStep = 0.0f;
                bool clicked = false;
                FxChain fx;
                fx.prepare (sr, block);
                for (int b = 0; b < 110; ++b)
                {
                    juce::AudioBuffer<float> buffer (2, block);
                    buffer.clear();
                    if (! clicked && b * block >= 12100) // a click early in the open step
                    {
                        clicked = true;
                        for (int i = 0; i < 48; ++i)
                        {
                            buffer.setSample (0, i, 0.8f);
                            buffer.setSample (1, i, 0.8f);
                        }
                    }
                    TransportInfo t;
                    t.playing = true;
                    t.bpm = 120.0;
                    t.ppq = (double) (b * block) / 24000.0;
                    fx.process (buffer, p, t);
                    for (int i = 0; i < block; ++i)
                    {
                        const int pos = b * block + i;
                        if (pos > 25000 && pos < 35000) // the middle of the next shut step
                            inShutStep = juce::jmax (inShutStep, std::abs (buffer.getSample (0, i)));
                    }
                }
                return inShutStep;
            };
            const float gateFirst = echoLevel (0), gateLast = echoLevel (1);
            std::printf ("      echo in the shut step: %.4f gate first, %.4f gate last\n", (double) gateFirst, (double) gateLast);
            check (gateFirst > 0.02f && gateLast < 0.002f, "effect order decides whether the gate chops the echoes");
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

    // Imported wavetables
    {
        const float twoPi = juce::MathConstants<float>::twoPi;
        const int size = UserWavetable::tableSize;

        // A single cycle of an odd length is stretched to the table size.
        std::vector<float> cycle (600);
        for (size_t i = 0; i < cycle.size(); ++i)
            cycle[i] = 0.3f * std::sin (twoPi * (float) i / (float) cycle.size()) + 0.2f; // quiet and with an offset
        const auto single = UserWavetable::fromAudio (cycle.data(), (int) cycle.size(), "single");
        bool isSine = single != nullptr;
        for (int i = 0; isSine && i < size; ++i)
            isSine = near (single->wave (7, 0)[i], std::sin (twoPi * (float) i / (float) size), 0.01f);
        check (isSine, "a single cycle of any length becomes a normalised table without its offset");

        // A row of three frames: a sine, its octave, a saw. They land on the first, middle and last position.
        std::vector<float> row ((size_t) size * 3);
        for (int i = 0; i < size; ++i)
        {
            const float ph = (float) i / (float) size;
            row[(size_t) i] = std::sin (twoPi * ph);
            row[(size_t) (size + i)] = std::sin (2.0f * twoPi * ph);
            row[(size_t) (2 * size + i)] = 2.0f * ph - 1.0f;
        }
        const auto table = UserWavetable::fromAudio (row.data(), (int) row.size(), "row");
        bool first = table != nullptr, last = table != nullptr, finite = table != nullptr;
        float maxStep = 0.0f;
        for (int i = 0; table != nullptr && i < size; ++i)
        {
            first = first && near (table->wave (0, 0)[i], std::sin (twoPi * (float) i / (float) size), 0.01f);
            if (i > 40 && i < size - 40)
                last = last && near (table->wave (UserWavetable::numFrames - 1, 0)[i], 2.0f * (float) i / (float) size - 1.0f, 0.06f);
            const float* top = table->wave (UserWavetable::numFrames - 1, UserWavetable::numLevels - 1);
            maxStep = juce::jmax (maxStep, std::abs (top[i + 1] - top[i]));
            for (int f = 0; f < UserWavetable::numFrames; ++f)
                finite = finite && std::isfinite (table->wave (f, 0)[i]) && std::abs (table->wave (f, 0)[i]) <= 1.0001f;
        }
        check (first && last && finite, "a row of frames is spread from the first to the last position");
        check (maxStep < 0.01f, "the highest level of an imported saw has no sharp edges");
        check (UserWavetable::fromAudio (row.data(), 4, "short") == nullptr, "audio that is too short is refused");

        // The engine plays it when the oscillator is set to the User table, and a sine when nothing was imported.
        EngineParams p;
        p.engineLevel = 0.0f;
        p.oscA.table = WavetableBank::numTables;
        p.oscA.position = 1.0f;
        p.oscA.level = 1.0f;
        const auto withoutImport = render (p);
        p.userWave[0] = table.get();
        const auto withImport = render (p);
        check (withImport.finite && withImport.peak > 0.05f && withoutImport.finite && withoutImport.peak > 0.05f
                   && ! near (withImport.peak, withoutImport.peak, 1.0e-3f),
               "the User table plays the imported wavetable");
    }

    // Filter types, reference pitch and MPE, measured on a plain sine from oscillator A.
    {
        struct Measured
        {
            float hz = 0.0f, peak = 0.0f;
            bool finite = true;
        };

        // Plays A3 and returns the frequency (from the zero crossings) and level of the held note.
        const auto play = [] (EngineParams p, int channel, const std::vector<juce::MidiMessage>& before)
        {
            p.engineLevel = 0.0f;
            p.oscA.table = 0;
            p.oscA.position = 0.0f;
            p.oscA.unison = 1;
            p.oscA.level = 1.0f;
            p.vibDepthCents = 0.0f;
            p.filterEnv = 0.0f;
            p.amp = { 0.002f, 0.1f, 1.0f, 0.05f };

            SynthEngine engine;
            engine.prepare (48000.0, 512);
            engine.setParams (p);

            std::vector<float> out;
            for (int block = 0; block < 100; ++block)
            {
                juce::MidiBuffer midi;
                if (block == 0)
                {
                    for (const auto& m : before)
                        midi.addEvent (m, 0);
                    midi.addEvent (juce::MidiMessage::noteOn (channel, 57, 0.9f), 1);
                }

                juce::AudioBuffer<float> buffer (2, 512);
                buffer.clear();
                engine.process (buffer, midi);
                out.insert (out.end(), buffer.getReadPointer (0), buffer.getReadPointer (0) + 512);
            }

            Measured result;
            double first = -1.0, last = -1.0;
            int crossings = 0;
            for (size_t i = 9600; i + 1 < out.size(); ++i)
            {
                result.finite = result.finite && std::isfinite (out[i]);
                result.peak = juce::jmax (result.peak, std::abs (out[i]));
                if (out[i] < 0.0f && out[i + 1] >= 0.0f)
                {
                    last = (double) i + (double) out[i] / ((double) out[i] - (double) out[i + 1]);
                    if (first < 0.0)
                        first = last;
                    else
                        ++crossings;
                }
            }

            if (crossings > 0)
                result.hz = (float) (48000.0 * crossings / (last - first));
            return result;
        };

        EngineParams open;
        open.cutoffHz = 18000.0f;
        open.resonance = 0.0f;
        const auto reference = play (open, 1, {});
        check (near (reference.hz, 220.0f, 0.05f), "A3 plays at 220 Hz with the reference at 440");

        // Reference pitch
        {
            auto p = open;
            p.tuneHz = 432.0f;
            const auto low = play (p, 1, {});
            std::printf ("      A3 with A4 = 432: %.3f Hz\n", (double) low.hz);
            check (near (low.hz, 216.0f, 0.05f), "A4 = 432 Hz lowers every note by the same ratio");
        }

        // MPE: a bend on the note's own channel uses the MPE range, other channels are left alone.
        {
            const int bend = 8192 + 171; // about one semitone at a range of 48
            const float semitones = 171.0f / 8192.0f * 48.0f;
            const float expected = 220.0f * std::exp2 (semitones / 12.0f);

            auto p = open;
            p.mpe = true;
            const auto own = play (p, 2, { juce::MidiMessage::pitchWheel (2, bend) });
            const auto other = play (p, 2, { juce::MidiMessage::pitchWheel (3, bend) });
            const auto master = play (p, 2, { juce::MidiMessage::pitchWheel (1, 16383) });
            std::printf ("      MPE bend on the note's channel: %.3f Hz, expected %.3f Hz\n", (double) own.hz, (double) expected);
            check (near (own.hz, expected, 0.05f), "MPE bends a note from its own channel over 48 semitones");
            check (near (other.hz, 220.0f, 0.05f), "MPE bend on another channel leaves the note alone");
            check (near (master.hz, 220.0f * std::exp2 (2.0f / 12.0f), 0.2f), "MPE master channel bends every note by the normal range");

            p.mpe = false;
            const auto plain = play (p, 2, { juce::MidiMessage::pitchWheel (2, 16383) });
            check (near (plain.hz, 220.0f * std::exp2 (2.0f / 12.0f), 0.2f), "without MPE every channel bends by the normal range");
        }

        // Modulation destinations that follow a knob's own range.
        {
            auto p = open;
            p.modMacros[0] = 0.25f;
            p.slots[0] = { mod::srcMacro1, mod::dstOscAPitch, 1.0f }; // a quarter of the 48 semitone range
            const auto octave = play (p, 1, {});
            check (near (octave.hz, 440.0f, 0.1f), "a macro on oscillator pitch moves it across the knob's range");

            EngineParams lead;
            lead.modMacros[1] = 1.0f;
            const auto plain = render (lead);
            lead.slots[0] = { mod::srcMacro2, mod::dstEngine1, 0.45f }; // tone, from saws towards pulse
            const auto toned = render (lead);
            lead.slots[0] = { mod::srcMacro2, mod::dstEngine4, 1.0f };  // sub oscillator
            const auto withSub = render (lead);
            check (toned.finite && withSub.finite && ! near (toned.peak, plain.peak, 1.0e-3f) && ! near (withSub.peak, plain.peak, 1.0e-3f),
                   "modulation reaches the knobs of the Balkan model");

            check (modDestFor (id::cutoff) == mod::dstCutoff && modDestFor (id::gusRasp) == mod::dstEngine3
                       && modDestFor (id::oscBDetune) == mod::dstOscBDetune && modDestFor (id::glide) == -1,
                   "knobs map to their modulation destinations");

            // Polarity: a bipolar macro rests in the middle and swings both ways.
            auto b = open;
            b.slots[0] = { mod::srcMacro1, mod::dstOscAPitch, 1.0f, mod::polarityBipolar };
            b.modMacros[0] = 0.5f;
            const auto centre = play (b, 1, {});
            b.modMacros[0] = 0.75f;
            const auto up = play (b, 1, {});
            b.modMacros[0] = 0.25f;
            const auto down = play (b, 1, {});
            check (near (centre.hz, 220.0f, 0.1f) && near (up.hz, 880.0f, 0.4f) && near (down.hz, 55.0f, 0.1f),
                   "a bipolar macro rests in the middle and swings both ways");
            check (near (mod::shaped (-1.0f, mod::srcLfo1, mod::polarityUnipolar), 0.0f)
                       && near (mod::shaped (1.0f, mod::srcLfo1, mod::polarityUnipolar), 1.0f)
                       && near (mod::shaped (-0.4f, mod::srcLfo1, mod::polarityAuto), -0.4f)
                       && near (mod::shaped (0.3f, mod::srcVelocity, mod::polarityAuto), 0.3f)
                       && mod::isBipolar (mod::srcLfo2, mod::polarityAuto) && ! mod::isBipolar (mod::srcMacro1, mod::polarityAuto),
                   "a unipolar LFO only pushes one way and Auto keeps each source as it is");
            check (modDestFor (id::oscAWarpAmount) == mod::dstOscAWarp && modDestFor (id::oscBWarpAmount) == mod::dstOscBWarp,
                   "the warp knobs are modulation destinations");
        }

        // Warp modes
        {
            EngineParams p;
            p.engineLevel = 0.0f;
            p.oscA.level = 0.8f;
            p.oscA.table = 0;
            p.oscA.position = 0.3f;
            const auto plain = render (p);
            bool finite = plain.finite && plain.peak > 0.01f, changes = true, silentTail = true;
            p.oscA.warpAmount = 0.7f;
            const auto offWithAmount = render (p);

            for (int mode = 1; mode < warp::numModes; ++mode)
            {
                p.oscA.warp = mode;
                p.slots[0] = {};
                const auto r = render (p);
                p.slots[0] = { mod::srcLfo1, mod::dstOscAWarp, 1.0f };
                const auto moving = render (p);
                finite = finite && r.finite && moving.finite && r.peak > 0.01f && r.peak < 4.0f && moving.peak < 4.0f;
                silentTail = silentTail && r.tailPeak < 0.001f;
                changes = changes && ! near (r.peak, plain.peak, 1.0e-4f);
                std::printf ("      warp %-8s peak %.4f (off %.4f)\n", getWarpModes()[(size_t) mode], (double) r.peak, (double) plain.peak);
            }
            check (near (offWithAmount.peak, plain.peak, 1.0e-6f), "the warp amount does nothing while warp is off");
            check (finite && silentTail, "every warp mode stays finite and bounded");
            check (changes, "every warp mode changes the wave");
        }

        // Drawn LFOs, the extra LFOs and envelope, and the destinations added with them.
        {
            const LfoShape ramp ({ { 1.0f, 0.5f, 0.0f }, { 0.0f, -1.0f, 0.0f }, { 0.5f, 0.0f, 0.0f } }); // given out of order
            check (ramp.getPoints().size() == 3 && near (ramp.at (0.0f), -1.0f) && near (ramp.at (0.25f), -0.5f, 0.01f) && near (ramp.at (0.5f), 0.0f, 0.01f)
                       && near (ramp.at (0.75f), 0.25f, 0.01f) && near (ramp.at (1.0f), 0.5f),
                   "a drawn LFO shape runs through its points in order");

            const LfoShape step ({ { 0.0f, 1.0f, 0.0f }, { 0.5f, 1.0f, 0.0f }, { 0.5f, -1.0f, 0.0f }, { 1.0f, -1.0f, 0.0f } });
            check (near (step.at (0.4f), 1.0f) && near (step.at (0.6f), -1.0f), "two points at the same place make a step");

            const LfoShape bent ({ { 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f, 0.0f } });
            const LfoShape again (LfoShape::pointsFromString (bent.toString()));
            const LfoShape empty (LfoShape::pointsFromString ("nonsense"));
            check (bent.at (0.5f) < 0.1f && near (again.at (0.5f), bent.at (0.5f), 1.0e-4f) && again.getPoints().size() == 2
                       && empty.getPoints().size() == 2 && near (empty.at (0.0f), 1.0f) && near (empty.at (1.0f), -1.0f),
                   "a curve bends a segment, and a shape survives being written as text");

            const LfoShape quarter ({ { 0.0f, 0.25f, 0.0f }, { 1.0f, 0.25f, 0.0f } });
            const LfoShape rise ({ { 0.0f, 0.0f, 0.0f }, { 1.0f, 0.25f, 0.0f } });

            auto p = open;
            p.lfos[0] = { lfoShapeCustom, 3.0f, false, 0, lfoMode::free, &quarter };
            p.slots[0] = { mod::srcLfo1, mod::dstOscAPitch, 1.0f };
            check (near (play (p, 1, {}).hz, 440.0f, 0.1f), "a drawn LFO modulates like the built-in shapes");

            p.lfos[0] = {};
            p.lfos[3] = { lfoShapeCustom, 3.0f, false, 0, lfoMode::free, &quarter };
            p.slots[0] = { mod::srcLfo4, mod::dstOscAPitch, 1.0f };
            check (near (play (p, 1, {}).hz, 440.0f, 0.1f), "LFO 4 is a modulation source");

            p.lfos[3] = { lfoShapeCustom, 20.0f, false, 0, lfoMode::oneShot, &rise };
            check (near (play (p, 1, {}).hz, 440.0f, 0.1f), "a one shot LFO runs once and stays at its end");

            p.lfos[3] = {};
            p.env3 = { 0.001f, 0.05f, 0.25f, 0.1f };
            p.slots[0] = { mod::srcEnv3, mod::dstOscAPitch, 1.0f };
            check (near (play (p, 1, {}).hz, 440.0f, 0.1f), "the third envelope is a modulation source");

            Lfo lfo;
            LfoParams lp;
            lp.rateHz = 1.0f;
            for (int i = 0; i < 10; ++i)
                lfo.next (4800, 48000.0, lp, {}, 0.0);
            const float running = lfo.getPhase();
            lfo.noteStarted (lp);
            const bool freeKeeps = near (lfo.getPhase(), running) && running > 0.5f;
            lp.mode = lfoMode::retrigger;
            lfo.noteStarted (lp);
            check (freeKeeps && near (lfo.getPhase(), 0.0f), "a retriggered LFO starts again with a note, a free one runs on");

            // Envelope times
            EngineParams slow;
            slow.amp.release = 6.0f;
            const auto ringing = render (slow);
            slow.modMacros[0] = 1.0f;
            slow.slots[0] = { mod::srcMacro1, mod::dstAmpRelease, -1.0f };
            const auto cut = render (slow);
            check (ringing.tailPeak > 0.01f && cut.tailPeak < 1.0e-4f && cut.finite, "modulation shortens the release of the amp envelope");

            // Effects are modulated from the shared sources.
            SynthEngine engine;
            engine.prepare (48000.0, 512);
            EngineParams fxp;
            fxp.modMacros[1] = 0.5f;
            fxp.slots[0] = { mod::srcMacro2, mod::dstDelayMix, 0.8f };
            fxp.slots[1] = { mod::srcMacro2, mod::dstCutoff, 1.0f };
            engine.setParams (fxp);
            juce::AudioBuffer<float> buffer (2, 512);
            buffer.clear();
            engine.process (buffer, {});
            check (near (engine.getGlobalModulation (mod::dstDelayMix), 0.4f) && near (engine.getGlobalModulation (mod::dstReverbMix), 0.0f)
                       && mod::isGlobalDest (mod::dstDistDrive) && ! mod::isGlobalDest (mod::dstCutoff) && ! mod::isGlobalDest (mod::dstFltRelease),
                   "the engine works out the modulation of the effects");
            check (modDestFor (id::ampA) == mod::dstAmpAttack && modDestFor (id::fltR) == mod::dstFltRelease && modDestFor (id::delayMix) == mod::dstDelayMix
                       && modDestFor (id::distDrive) == mod::dstDistDrive && modDestFor (id::reverbSize) == mod::dstReverbSize,
                   "envelope and effect knobs are modulation destinations");
            check (mod::lfoSource (0) == mod::srcLfo1 && mod::lfoSource (1) == mod::srcLfo2 && mod::lfoSource (2) == mod::srcLfo3 && mod::lfoSource (3) == mod::srcLfo4
                       && (int) mod::srcTimbre == 11 && (int) mod::dstGrainScan == 30,
                   "sources and destinations saved in older projects keep their numbers");
        }

        // Sampler
        {
            const double rate = 44100.0;
            const auto makeSample = [rate] (double seconds, const std::function<float (int, int)>& fn)
            {
                juce::AudioBuffer<float> b (1, (int) (seconds * rate));
                for (int i = 0; i < b.getNumSamples(); ++i)
                    b.setSample (0, i, fn (i, b.getNumSamples()));
                return SampleData::fromBuffer (b, rate, "test");
            };
            const auto sine = [] (float hz) { return [hz] (int i, int) { return 0.8f * std::sin (juce::MathConstants<float>::twoPi * hz * (float) i / 44100.0f); }; };

            // Plays A3 for `blocks` blocks of 512 samples at 48 kHz and returns the left channel.
            const auto playSample = [] (SamplerParams smp, int blocks, std::vector<float>* right = nullptr)
            {
                EngineParams p;
                p.engineLevel = 0.0f;
                p.cutoffHz = 20000.0f;
                p.resonance = 0.0f;
                p.filterEnv = 0.0f;
                p.vibDepthCents = 0.0f;
                p.amp = { 0.001f, 0.1f, 1.0f, 0.05f };
                p.sampler = smp;

                SynthEngine engine;
                engine.prepare (48000.0, 512);
                engine.setRandomSeed (3);
                engine.setParams (p);
                std::vector<float> out;
                for (int block = 0; block < blocks; ++block)
                {
                    juce::MidiBuffer midi;
                    if (block == 0)
                        midi.addEvent (juce::MidiMessage::noteOn (1, 57, 1.0f), 0);
                    juce::AudioBuffer<float> buffer (2, 512);
                    buffer.clear();
                    engine.process (buffer, midi);
                    out.insert (out.end(), buffer.getReadPointer (0), buffer.getReadPointer (0) + 512);
                    if (right != nullptr)
                        right->insert (right->end(), buffer.getReadPointer (1), buffer.getReadPointer (1) + 512);
                }
                return out;
            };
            const auto hzOf = [] (const std::vector<float>& out, size_t from, size_t to)
            {
                double first = -1.0, last = -1.0;
                int crossings = 0;
                for (size_t i = from; i + 1 < to; ++i)
                    if (out[i] < 0.0f && out[i + 1] >= 0.0f)
                    {
                        last = (double) i + (double) out[i] / ((double) out[i] - (double) out[i + 1]);
                        if (first < 0.0)
                            first = last;
                        else
                            ++crossings;
                    }
                return crossings > 0 ? (float) (48000.0 * crossings / (last - first)) : 0.0f;
            };
            const auto peakOf = [] (const std::vector<float>& out, size_t from, size_t to)
            {
                float peak = 0.0f;
                for (size_t i = from; i < to && i < out.size(); ++i)
                    peak = std::isfinite (out[i]) ? juce::jmax (peak, std::abs (out[i])) : 1.0e9f;
                return peak;
            };

            juce::AudioBuffer<float> tiny (1, 8);
            check (SampleData::fromBuffer (tiny, rate, "tiny") == nullptr, "a sample that is too short is refused");

            const auto tone = makeSample (1.0, sine (100.0f)); // 100 whole cycles, so the loop is seamless
            SamplerParams smp;
            smp.data = tone.get();
            smp.level = 1.0f;
            smp.rootNote = 57;
            smp.loop = sampler::loopForward;

            {
                const auto out = playSample (smp, 200);
                const float hz = hzOf (out, 4800, out.size());
                std::printf ("      sampler at its root: %.3f Hz, peak %.3f\n", (double) hz, (double) peakOf (out, 4800, out.size()));
                check (near (hz, 100.0f, 0.05f) && peakOf (out, 4800, out.size()) > 0.5f, "the sampler plays the sample at its own pitch on the root key, at any sample rate");

                auto up = smp;
                up.pitch = 12.0f;
                check (near (hzOf (playSample (up, 100), 4800, 51200), 200.0f, 0.1f), "sampler pitch transposes the sample");

                up.rootNote = 45; // the root an octave below the played key
                up.pitch = 0.0f;
                check (near (hzOf (playSample (up, 100), 4800, 51200), 200.0f, 0.1f), "the sampler follows the keyboard from its root note");

                up.keyTrack = false;
                check (near (hzOf (playSample (up, 100), 4800, 51200), 100.0f, 0.05f), "without key tracking every key plays the sample as it is");
            }

            // Without a loop the sample ends, with a loop it goes on.
            {
                auto once = smp;
                once.loop = sampler::loopOff;
                once.end = 0.25f;
                const auto out = playSample (once, 60);
                check (peakOf (out, 2400, 9600) > 0.5f && peakOf (out, 14400, out.size()) < 1.0e-5f, "a sample without a loop stops at END");

                auto looped = once;
                looped.loop = sampler::loopPingPong;
                looped.loopStart = 0.05f;
                looped.loopEnd = 0.2f;
                const auto pong = playSample (looped, 200);
                check (peakOf (pong, 60000, pong.size()) > 0.5f && peakOf (pong, 0, pong.size()) < 2.0f, "a ping pong loop keeps the sample going");
            }

            // Reverse starts at the end.
            {
                const auto late = makeSample (1.0, [] (int i, int n) { return i > n * 3 / 4 ? 0.8f * std::sin (0.05f * (float) i) : 0.0f; });
                auto p = smp;
                p.data = late.get();
                p.loop = sampler::loopOff;
                const auto forward = playSample (p, 20);
                p.reverse = true;
                const auto backward = playSample (p, 20);
                check (peakOf (forward, 0, forward.size()) < 1.0e-5f && peakOf (backward, 0, backward.size()) > 0.5f, "reverse plays the sample from its end");
            }

            // A loop that does not fit the wave clicks; the crossfade removes the jump.
            {
                const auto jump = [] (const std::vector<float>& out)
                {
                    float biggest = 0.0f;
                    for (size_t i = 4800; i + 1 < out.size(); ++i)
                        biggest = juce::jmax (biggest, std::abs (out[i + 1] - out[i]));
                    return biggest;
                };
                auto p = smp;
                p.loopStart = 0.3f;
                p.loopEnd = 0.5034f; // ends at another phase of the sine than it starts with
                const float hard = jump (playSample (p, 150));
                p.crossfade = 0.5f;
                const float soft = jump (playSample (p, 150));
                p.reverse = true;
                const float softReverse = jump (playSample (p, 150));
                std::printf ("      biggest step in the loop: %.4f without crossfade, %.4f with, %.4f reversed\n", (double) hard, (double) soft, (double) softReverse);
                check (hard > 0.1f && soft < 0.03f && softReverse < 0.03f, "the loop crossfade removes the click at the loop point");
            }

            // Granular
            {
                auto g = smp;
                g.mode = sampler::granular;
                g.loop = sampler::loopOff;
                g.position = 0.3f;
                g.spray = 0.0f;
                g.spread = 0.0f;
                const auto frozen = playSample (g, 400);
                const float hz = hzOf (frozen, 96000, frozen.size());
                std::printf ("      granular, frozen: %.2f Hz, peak %.3f\n", (double) hz, (double) peakOf (frozen, 96000, frozen.size()));
                check (peakOf (frozen, 150000, frozen.size()) > 0.2f && peakOf (frozen, 0, frozen.size()) < 2.0f,
                       "granular mode holds a sound for as long as the key is down");
                check (near (hz, 100.0f, 4.0f), "grains follow the pitch of the key");

                // Everything turned up stays finite and bounded, and the grains spread in stereo.
                g.spray = 1.0f;
                g.pitchSpray = 12.0f;
                g.spread = 1.0f;
                g.reverseChance = 0.5f;
                g.density = 100.0f;
                g.grainMs = 500.0f;
                g.scan = -2.0f;
                g.shape = 0.0f;
                std::vector<float> right;
                const auto wild = playSample (g, 300, &right);
                float difference = 0.0f;
                for (size_t i = 0; i < wild.size(); ++i)
                    difference = juce::jmax (difference, std::abs (wild[i] - right[i]));
                check (peakOf (wild, 0, wild.size()) > 0.05f && peakOf (wild, 0, wild.size()) < 3.0f && peakOf (right, 0, right.size()) < 3.0f && difference > 0.05f,
                       "dense, sprayed grains stay bounded and spread in stereo");

                // Scan at 1 walks through the sample at its own speed, whatever the key.
                const auto half = makeSample (1.0, [] (int i, int n) { return i > n / 2 ? 0.8f * std::sin (0.07f * (float) i) : 0.0f; });
                SamplerParams walk = smp;
                walk.data = half.get();
                walk.mode = sampler::granular;
                walk.loop = sampler::loopOff;
                walk.spray = 0.0f;
                walk.scan = 1.0f;
                walk.pitch = 12.0f;
                walk.grainMs = 40.0f;
                walk.density = 50.0f;
                const auto walked = playSample (walk, 90);
                check (peakOf (walked, 0, 19200) < 1.0e-4f && peakOf (walked, 31200, 43200) > 0.2f, "scan moves the grain position through the sample in time");
            }

            // No sample, modulation and the tools.
            {
                auto none = smp;
                none.data = nullptr;
                const auto silent = playSample (none, 20);
                check (peakOf (silent, 0, silent.size()) < 1.0e-9f, "the sampler is silent without a sample");

                EngineParams p;
                p.engineLevel = 0.0f;
                p.sampler = smp;
                p.sampler.level = 0.0f;
                const auto off = render (p);
                p.modMacros[0] = 1.0f;
                p.slots[0] = { mod::srcMacro1, mod::dstSamplerLevel, 1.0f };
                const auto on = render (p);
                check (off.peak < 1.0e-9f && on.finite && on.peak > 0.05f && on.tailPeak < 0.001f, "a macro brings the sampler in");
                check (modDestFor (id::smpGrainSize) == mod::dstGrainSize && modDestFor (id::smpPosition) == mod::dstGrainPosition
                           && modDestFor (id::smpLevel) == mod::dstSamplerLevel && modDestFor (id::smpScan) == mod::dstGrainScan,
                       "the sampler knobs are modulation destinations");

                const auto a3 = makeSample (0.5, sine (220.0f));
                juce::Random random (5);
                const auto hiss = makeSample (0.5, [&random] (int, int) { return random.nextFloat() - 0.5f; });
                std::printf ("      detected root: %d for a 220 Hz sine, %d for noise\n", detectRootNote (*a3), detectRootNote (*hiss));
                check (detectRootNote (*a3) == 57 && detectRootNote (*hiss) == -1, "the root note of a sample is found from its pitch");

                const auto frames = sampleToWavetable (*a3, 0.1f, 0.9f, 57, 16, 2048);
                bool cycles = frames.size() == 16 * 2048;
                for (int f = 0; cycles && f < 16; ++f)
                {
                    const float* frame = frames.data() + f * 2048;
                    int rising = 0;
                    float peak = 0.0f;
                    for (int i = 0; i < 2048; ++i)
                    {
                        rising += frame[i] < 0.0f && frame[(i + 1) % 2048] >= 0.0f ? 1 : 0;
                        peak = juce::jmax (peak, std::abs (frame[i]));
                    }
                    cycles = rising == 1 && near (peak, 0.8f, 0.02f) && std::abs (frame[2047] - frame[0]) < 0.01f;
                }
                check (cycles, "single cycles are cut from a sample for a wavetable");
            }
        }

        // FM, ring and amplitude modulation of the wavetable oscillators. The note is A3, a sine at 220 Hz.
        {
            // The level of single frequencies in the held note.
            const auto partials = [&] (const std::function<void (EngineParams&)>& setUp, std::initializer_list<float> frequencies)
            {
                auto p = open;
                p.engineLevel = 0.0f;
                p.oscA.table = 0;
                p.oscA.position = 0.0f;
                p.oscA.unison = 1;
                p.oscA.level = 1.0f;
                p.oscB.table = 0;
                p.oscB.position = 0.0f;
                p.vibDepthCents = 0.0f;
                p.filterEnv = 0.0f;
                p.amp = { 0.002f, 0.1f, 1.0f, 0.05f };
                setUp (p);

                SynthEngine engine;
                engine.prepare (48000.0, 512);
                engine.setParams (p);
                std::vector<float> out;
                for (int block = 0; block < 100; ++block)
                {
                    juce::MidiBuffer midi;
                    if (block == 0)
                        midi.addEvent (juce::MidiMessage::noteOn (1, 57, 0.9f), 1);
                    juce::AudioBuffer<float> buffer (2, 512);
                    buffer.clear();
                    engine.process (buffer, midi);
                    out.insert (out.end(), buffer.getReadPointer (0), buffer.getReadPointer (0) + 512);
                }

                std::vector<float> levels;
                const size_t from = 9600, count = 38400; // 0.8 s, a whole number of cycles of 110 Hz
                for (float hz : frequencies)
                {
                    double re = 0.0, im = 0.0;
                    for (size_t i = 0; i < count; ++i)
                    {
                        const double angle = juce::MathConstants<double>::twoPi * hz * (double) i / 48000.0;
                        const double v = std::isfinite (out[from + i]) ? out[from + i] : 1.0e6;
                        re += v * std::cos (angle);
                        im += v * std::sin (angle);
                    }
                    levels.push_back ((float) (2.0 * std::sqrt (re * re + im * im) / (double) count));
                }
                return levels;
            };

            const auto dry = partials ([] (EngineParams&) {}, { 110.0f, 220.0f, 330.0f, 440.0f });
            const float carrier = dry[1];
            check (carrier > 0.3f && dry[0] < carrier * 0.01f && dry[2] < carrier * 0.01f && dry[3] < carrier * 0.01f, "the test note is a clean sine");

            const auto bySub = [] (int type, float amount)
            {
                return [type, amount] (EngineParams& p)
                {
                    p.oscA.xmodType = type;
                    p.oscA.xmodSource = xmod::sub;
                    p.oscA.xmodAmount = amount;
                };
            };

            // Ring by the sub, an octave down: the note itself goes, 110 and 330 Hz take its place.
            const auto ring = partials (bySub (xmod::ring, 1.0f), { 110.0f, 220.0f, 330.0f });
            std::printf ("      ring by the sub: 110 Hz %.3f, 220 Hz %.3f, 330 Hz %.3f (dry 220 Hz %.3f)\n", (double) ring[0], (double) ring[1], (double) ring[2], (double) carrier);
            check (near (ring[0], carrier * 0.5f, carrier * 0.03f) && near (ring[2], carrier * 0.5f, carrier * 0.03f) && ring[1] < carrier * 0.02f,
                   "ring modulation replaces the note with its sum and difference tones");

            const auto am = partials (bySub (xmod::am, 1.0f), { 110.0f, 220.0f, 330.0f });
            check (near (am[1], carrier * 0.5f, carrier * 0.03f) && near (am[0], carrier * 0.25f, carrier * 0.03f) && near (am[2], carrier * 0.25f, carrier * 0.03f),
                   "amplitude modulation keeps the note and adds side tones");

            const auto none = partials (bySub (xmod::fm, 0.0f), { 220.0f, 440.0f });
            check (near (none[0], carrier, carrier * 0.001f) && none[1] < carrier * 0.01f, "a modulation amount of zero leaves the oscillator alone");

            // FM from oscillator B, which is turned down and still does its work.
            const auto fromB = partials ([] (EngineParams& p)
                                         {
                                             p.oscA.xmodType = xmod::fm;
                                             p.oscA.xmodSource = xmod::otherOsc;
                                             p.oscA.xmodAmount = 0.5f;
                                             p.oscB.level = 0.0f;
                                         }, { 220.0f, 440.0f, 660.0f });
            std::printf ("      FM from a silent oscillator B: 220 Hz %.3f, 440 Hz %.3f, 660 Hz %.3f\n", (double) fromB[0], (double) fromB[1], (double) fromB[2]);
            check (fromB[1] > carrier * 0.1f && fromB[2] > carrier * 0.02f, "FM from the other oscillator adds harmonics, even when that oscillator is turned down");

            const auto fromA = partials ([] (EngineParams& p)
                                         {
                                             p.oscA.level = 0.0f;
                                             p.oscB.level = 1.0f;
                                             p.oscB.xmodType = xmod::fm;
                                             p.oscB.xmodSource = xmod::otherOsc;
                                             p.oscB.xmodAmount = 0.5f;
                                         }, { 220.0f, 440.0f });
            check (fromA[0] > carrier * 0.2f && fromA[1] > carrier * 0.1f, "oscillator B is modulated by oscillator A in the same way");

            const auto fromEngine = partials ([] (EngineParams& p)
                                              {
                                                  p.oscA.xmodType = xmod::fm;
                                                  p.oscA.xmodSource = xmod::engine;
                                                  p.oscA.xmodAmount = 0.6f;
                                              }, { 220.0f, 440.0f });
            check (fromEngine[1] > carrier * 0.03f, "the Balkan engine can modulate an oscillator while its own level is down");

            // A sample that stands at one half turns the ringed oscillator down by half.
            juce::AudioBuffer<float> flat (1, 4410);
            for (int i = 0; i < flat.getNumSamples(); ++i)
                flat.setSample (0, i, 0.5f);
            const auto steady = SampleData::fromBuffer (flat, 44100.0, "steady");
            const auto bySample = partials ([&steady] (EngineParams& p)
                                            {
                                                p.sampler.data = steady.get();
                                                p.sampler.loop = sampler::loopForward;
                                                p.oscA.xmodType = xmod::ring;
                                                p.oscA.xmodSource = xmod::sampler;
                                                p.oscA.xmodAmount = 1.0f;
                                            }, { 220.0f });
            check (near (bySample[0], carrier * 0.5f, carrier * 0.01f), "the sampler can modulate an oscillator while its own level is down");

            const auto byMacro = partials ([] (EngineParams& p)
                                           {
                                               p.oscA.xmodType = xmod::fm;
                                               p.oscA.xmodSource = xmod::otherOsc;
                                               p.modMacros[0] = 0.5f;
                                               p.slots[0] = { mod::srcMacro1, mod::dstOscAFm, 1.0f };
                                           }, { 440.0f });
            check (byMacro[0] > carrier * 0.1f && modDestFor (id::oscAXmodAmount) == mod::dstOscAFm && modDestFor (id::oscBXmodAmount) == mod::dstOscBFm,
                   "the FM amount is a modulation destination");

            // Both oscillators modulating each other at full depth, with noise in the loop, stay bounded.
            EngineParams wild;
            wild.oscA.level = 0.8f;
            wild.oscB.level = 0.8f;
            wild.oscA.unison = 7;
            wild.oscA.width = 1.0f;
            wild.oscA.xmodType = xmod::fm;
            wild.oscA.xmodAmount = 1.0f;
            wild.oscB.xmodType = xmod::fm;
            wild.oscB.xmodAmount = 1.0f;
            const auto cross = render (wild);
            wild.oscA.xmodSource = xmod::noise;
            wild.oscB.xmodType = xmod::ring;
            wild.oscB.xmodSource = xmod::engine;
            const auto mixed = render (wild);
            check (cross.finite && mixed.finite && cross.peak > 0.02f && cross.peak < 4.0f && mixed.peak < 4.0f && cross.tailPeak < 0.001f && mixed.tailPeak < 0.001f,
                   "oscillators that modulate each other at full depth stay bounded");
        }

        // Formant and phaser types, the second filter and its routing. The note is A3, a sine at 220 Hz.
        {
            const float full = reference.peak;
            const auto level = [&] (const std::function<void (EngineParams&)>& setUp)
            {
                auto p = open;
                setUp (p);
                const auto r = play (p, 1, {});
                return r.finite ? r.peak : 1.0e9f;
            };

            // A (first formant near 730 Hz) against I (270 Hz) for a 220 Hz note.
            const float vowelA = level ([] (EngineParams& p) { p.filterType = filter::formant; p.cutoffHz = 200.0f; p.resonance = 0.6f; });
            const float vowelI = level ([] (EngineParams& p) { p.filterType = filter::formant; p.cutoffHz = 1265.0f; p.resonance = 0.6f; });
            std::printf ("      formant on a 220 Hz note: A %.4f, I %.4f, unfiltered %.4f\n", (double) vowelA, (double) vowelI, (double) full);
            check (vowelI > vowelA * 2.0f && vowelI < full * 4.0f, "the formant filter moves through the vowels with the cutoff");

            float deepest = full;
            for (float cutoff = 60.0f; cutoff < 1200.0f; cutoff *= 1.04f)
                deepest = juce::jmin (deepest, level ([cutoff] (EngineParams& p) { p.filterType = filter::phaser; p.cutoffHz = cutoff; p.resonance = 0.0f; }));
            const float away = level ([] (EngineParams& p) { p.filterType = filter::phaser; p.cutoffHz = 15000.0f; p.resonance = 0.0f; });
            check (deepest < full * 0.1f && near (away, full, full * 0.1f), "the phaser cuts a notch that the cutoff moves");

            // Filter 2
            const auto highPass2 = [] (EngineParams& p)
            {
                p.filter2On = true;
                p.filter2Type = filter::highPass24;
                p.cutoff2Hz = 6000.0f;
                p.resonance2 = 0.0f;
            };
            const float serial = level (highPass2);
            const float ignored = level ([&] (EngineParams& p) { highPass2 (p); p.filter2On = false; p.filterRoute = { 1, 1, 1, 1, 1, 1 }; });
            check (serial < full * 0.01f && near (ignored, full, full * 0.001f), "filter 2 follows filter 1 in series, and does nothing while it is off");

            const float parallel = level ([&] (EngineParams& p) { highPass2 (p); p.filtersParallel = true; });
            const float parallelTo2 = level ([&] (EngineParams& p) { highPass2 (p); p.filtersParallel = true; p.filterRoute[EngineParams::routeOscA] = 1; });
            check (near (parallel, full, full * 0.01f) && parallelTo2 < full * 0.01f, "in parallel each source is heard through its own filter only");

            // In series a source sent to filter 2 skips filter 1.
            const auto closedFirst = [] (EngineParams& p)
            {
                p.filterType = filter::highPass24;
                p.cutoffHz = 9000.0f;
                p.filter2On = true;
                p.filter2Type = filter::lowPass12;
                p.cutoff2Hz = 18000.0f;
                p.resonance2 = 0.0f;
            };
            const float through1 = level (closedFirst);
            const float skip1 = level ([&] (EngineParams& p) { closedFirst (p); p.filterRoute[EngineParams::routeOscA] = 1; });
            check (through1 < full * 0.01f && near (skip1, full, full * 0.05f), "a source routed to filter 2 skips filter 1");

            // Modulation reaches filter 2.
            const float opened = level ([&] (EngineParams& p)
                                        {
                                            p.filter2On = true;
                                            p.filter2Type = filter::lowPass24;
                                            p.cutoff2Hz = 30.0f;
                                            p.modMacros[0] = 1.0f;
                                            p.slots[0] = { mod::srcMacro1, mod::dstCutoff2, 1.0f };
                                        });
            const float shut = level ([] (EngineParams& p) { p.filter2On = true; p.filter2Type = filter::lowPass24; p.cutoff2Hz = 30.0f; });
            check (shut < full * 0.05f && opened > full * 0.5f && modDestFor (id::filter2Cutoff) == mod::dstCutoff2 && modDestFor (id::filter2Res) == mod::dstResonance2,
                   "the cutoff of filter 2 is a modulation destination");

            // Everything at once with both filters ringing stays bounded.
            EngineParams busy;
            busy.oscA.level = 0.7f;
            busy.oscB.level = 0.7f;
            busy.subLevel = 0.5f;
            busy.noiseLevel = 0.3f;
            busy.filter2On = true;
            busy.filter2Type = filter::formant;
            busy.resonance2 = 1.0f;
            busy.resonance = 1.0f;
            busy.filterRoute = { 0, 1, 0, 1, 0, 1 };
            bool bounded = true;
            for (bool side : { false, true })
            {
                busy.filtersParallel = side;
                const auto r = render (busy);
                bounded = bounded && r.finite && r.peak > 0.01f && r.peak < 30.0f && r.tailPeak < 0.001f;
            }
            check (bounded, "two resonant filters with split sources stay finite");
        }

        // Filter types
        {
            const auto with = [&] (int type, float cutoff, float resonance)
            {
                auto p = open;
                p.filterType = type;
                p.cutoffHz = cutoff;
                p.resonance = resonance;
                return play (p, 1, {});
            };

            const float full = reference.peak;
            bool finite = true;
            for (int type = 0; type < filter::numTypes; ++type)
                for (float cutoff : { 30.0f, 220.0f, 18000.0f })
                {
                    const auto r = with (type, cutoff, 1.0f);
                    finite = finite && r.finite && r.peak < 30.0f;
                }
            check (finite, "every filter type stays finite at full resonance");

            const float lp24 = with (filter::lowPass24, 60.0f, 0.0f).peak, lp12 = with (filter::lowPass12, 60.0f, 0.0f).peak;
            const float hp24 = with (filter::highPass24, 1500.0f, 0.0f).peak, hp12 = with (filter::highPass12, 1500.0f, 0.0f).peak;
            std::printf ("      sine level %.3f: LP12 %.4f, LP24 %.4f, HP12 %.4f, HP24 %.4f\n", (double) full, (double) lp12, (double) lp24,
                         (double) hp12, (double) hp24);
            check (lp12 < full * 0.2f && lp24 < lp12 * 0.3f, "low pass 24 is steeper than low pass 12");
            check (hp12 < full * 0.1f && hp24 < hp12 * 0.2f, "high pass 24 is steeper than high pass 12");
            check (near (with (filter::highPass12, 30.0f, 0.0f).peak, full, full * 0.1f), "high pass lets the note through above its cutoff");

            // Key tracking moves the cutoff a little below 220 Hz for this note, so tune the notch by search.
            float deepest = full;
            for (float cutoff = 200.0f; cutoff < 245.0f; cutoff += 1.0f)
                deepest = juce::jmin (deepest, with (filter::notch, cutoff, 0.0f).peak);
            const float band = with (filter::bandPass, 228.0f, 0.5f).peak, bandOff = with (filter::bandPass, 4000.0f, 0.5f).peak;
            check (deepest < full * 0.05f && near (with (filter::notch, 4000.0f, 0.0f).peak, full, full * 0.05f), "notch removes one frequency only");
            check (band > bandOff * 8.0f, "band pass keeps what is near its cutoff");

            // The comb rings when its pitch matches the note and cancels half way between.
            const float combOn = with (filter::comb, 228.0f, 0.6f).peak, combOff = with (filter::comb, 152.0f, 0.6f).peak;
            std::printf ("      comb: %.3f on the note, %.3f between its teeth\n", (double) combOn, (double) combOff);
            check (combOn > combOff * 2.0f, "comb filter boosts notes that match its pitch");
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

        bool listed = true, extras = true;
        for (const auto& preset : presets)
        {
            bool known = juce::String (preset.category) == "Init";
            for (const auto* category : getPresetCategories())
                known = known || juce::String (preset.category) == category;
            listed = listed && known;

            extras = extras && preset.sequence.size() <= (size_t) SeqPattern::maxSteps;
            for (const auto& step : preset.sequence)
                extras = extras && std::abs (step.note) <= SeqPattern::noteRange && step.velocity > 0.0f && step.velocity <= 1.0f && step.gate >= 0.0f && step.gate <= 1.0f;
            for (const auto& [lfo, points] : preset.lfoShapes)
                extras = extras && lfo >= 0 && lfo < mod::numLfos && points.size() >= 2 && points.size() <= (size_t) LfoShape::maxPoints;
        }
        check (listed, "every factory preset is in a category the browser lists");
        check (extras, "preset sequences and LFO shapes are in range");
        check (valid, "every preset value names a real parameter and is in range");

        // Applying a preset sets its values and resets everything else.
        applyPreset (host.apvts, presets[1]);
        const bool applied = near (host.apvts.getRawParameterValue (id::scale)->load(), 1.0f) && host.apvts.getRawParameterValue (id::delayOn)->load() > 0.5f;
        applyPreset (host.apvts, presets[0]);
        const bool reset = near (host.apvts.getRawParameterValue (id::scale)->load(), 0.0f) && host.apvts.getRawParameterValue (id::delayOn)->load() < 0.5f
                               && near (host.apvts.getRawParameterValue (id::cutoff)->load(), 3200.0f, 1.0f);
        check (applied && reset, "applying a preset sets its values and Init restores the defaults");

        // Tuning and MPE belong to the project and survive a preset change.
        auto* tune = host.apvts.getParameter (id::tuneA4);
        tune->setValueNotifyingHost (tune->convertTo0to1 (432.0f));
        applyPreset (host.apvts, presets[2]);
        check (near (host.apvts.getRawParameterValue (id::tuneA4)->load(), 432.0f, 0.01f), "a preset change keeps the reference pitch");
    }

    std::printf ("%d failure(s)\n", failures);
    return failures == 0 ? 0 : 1;
}
