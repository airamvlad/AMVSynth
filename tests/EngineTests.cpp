// Offline checks for the tuning tables and the synth engine. Exits non-zero on failure.
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

    std::printf ("%d failure(s)\n", failures);
    return failures == 0 ? 0 : 1;
}
