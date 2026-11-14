// Offline checks for the tuning tables and the synth engine. Exits non-zero on failure.
#include "Arpeggiator.h"
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

    std::printf ("%d failure(s)\n", failures);
    return failures == 0 ? 0 : 1;
}
