#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

#include "Tuning.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

namespace amv
{
enum class Model
{
    juzniVetar = 0,
    accordion
};

enum class Ornament
{
    none = 0,
    grace,
    mordent,
    trill
};

/** Values read from the plugin parameters once per audio block. */
struct EngineParams
{
    Model model = Model::juzniVetar;

    // Juzni Vetar lead
    float tone = 0.55f;        // 0 = detuned saws, 1 = pulse
    float pulseWidth = 0.32f;
    float detuneCents = 9.0f;
    float sub = 0.0f;

    // Accordion
    float bellows = 0.75f;
    float musetteCents = 12.0f;
    float registerPos = 0.4f;
    float click = 0.3f;

    // Performance
    float vibDepthCents = 28.0f;
    float vibRateHz = 6.2f;
    float vibDelaySec = 0.22f;
    float bendRange = 2.0f;    // semitones
    float glideSec = 0.045f;
    bool mono = true;

    // Scale
    int scaleRoot = 2;
    int scale = 0;
    bool snap = false;

    // Ornaments
    bool grace = false, mordent = false, trill = false, fall = false;
    float ornSpeedSec = 0.06f; // length of one ornament step
    float ornAmount = 0.5f;    // trill length and fall depth
    float ornChance = 1.0f;

    // Filter and envelopes
    float cutoffHz = 3200.0f;
    float resonance = 0.25f;
    float filterEnv = 0.35f;   // -1..1, scaled to +/- 5 octaves
    juce::ADSR::Parameters amp { 0.004f, 0.25f, 0.8f, 0.16f };
    juce::ADSR::Parameters flt { 0.002f, 0.32f, 0.4f, 0.2f };
};

/** What the engine decides for a note before handing it to a voice. */
struct NoteEvent
{
    int note = 60;
    float pitch = 60.0f;       // tuned pitch as a fractional MIDI note
    float stepAbove = 1.0f;    // semitones up to the next scale degree
    float velocity = 1.0f;
    Ornament ornament = Ornament::none;
    bool fall = false;
};

/** One synth voice. The oscillator section depends on the selected model, everything after it
    (glide, vibrato, ornaments, filter, envelopes) is shared. */
class Voice
{
public:
    void prepare (double newSampleRate)
    {
        sampleRate = newSampleRate;
        ampEnv.setSampleRate (newSampleRate);
        fltEnv.setSampleRate (newSampleRate);
        filter.prepare ({ newSampleRate, 64, 1 });
        filter.setType (juce::dsp::StateVariableTPTFilterType::lowpass);
        reset();
    }

    void reset()
    {
        ampEnv.reset();
        fltEnv.reset();
        filter.reset();
        note = -1;
        held = false;
    }

    bool isActive() const noexcept { return ampEnv.isActive(); }
    bool isHeld() const noexcept { return held; }
    int getNote() const noexcept { return note; }
    juce::uint64 getAge() const noexcept { return age; }

    void start (const NoteEvent& e, juce::uint64 newAge, const EngineParams& p, bool glideFromCurrent)
    {
        const bool wasActive = isActive();
        setNote (e);
        velocity = e.velocity;
        age = newAge;

        if (! (glideFromCurrent && wasActive))
            currentPitch = targetPitch;

        if (! wasActive)
        {
            filter.reset();
            phases = { 0.0f, 0.37f, 0.71f, 0.13f, 0.52f };
            reedLp = 0.0f;
            clickLp = 0.0f;
            clickHp = 0.0f;
        }

        vibTime = 0.0f;
        clickEnv = 1.0f;
        ampEnv.setParameters (p.amp);
        fltEnv.setParameters (p.flt);
        ampEnv.noteOn();
        fltEnv.noteOn();
    }

    /** Legato: move to a new pitch without retriggering the envelopes. */
    void slideTo (const NoteEvent& e)
    {
        setNote (e);
    }

    void release()
    {
        held = false;
        releaseTime = 0.0f;
        ampEnv.noteOff();
        fltEnv.noteOff();
    }

    /** Adds this voice to `out` (mono). `bend` is the pitch wheel offset in semitones. */
    void render (float* out, int numSamples, const EngineParams& p, float bend)
    {
        if (! isActive())
            return;

        ampEnv.setParameters (p.amp);
        fltEnv.setParameters (p.flt);

        const auto sr = (float) sampleRate;
        const float dt = 1.0f / sr;
        const float glideCoeff = p.glideSec > 0.0005f ? 1.0f - std::exp (-1.0f / (p.glideSec * sr)) : 1.0f;
        const float vibInc = p.vibRateHz / sr;

        // The filter is updated once per render call; the engine calls us in short chunks.
        const float envNow = fltEnv.getNextSample();
        for (int i = 1; i < numSamples; ++i)
            fltEnv.getNextSample();

        const float keyTrack = (currentPitch - 60.0f) / 12.0f * 0.35f;
        const float cutoff = juce::jlimit (20.0f, 0.45f * sr,
                                           p.cutoffHz * std::exp2 (p.filterEnv * 5.0f * envNow + keyTrack));
        filter.setCutoffFrequency (cutoff);
        filter.setResonance (0.5f + p.resonance * p.resonance * 9.0f);

        ModelCoeffs mc;
        prepareModel (mc, p, sr);

        const float fallDepth = fall ? p.ornAmount * 3.0f : 0.0f;
        const float fallTime = juce::jlimit (0.04f, 0.4f, p.amp.release);

        for (int i = 0; i < numSamples; ++i)
        {
            currentPitch += (targetPitch - currentPitch) * glideCoeff;

            const float fade = p.vibDelaySec > 0.001f ? juce::jlimit (0.0f, 1.0f, vibTime / p.vibDelaySec) : 1.0f;
            const float vib = std::sin (vibPhase * juce::MathConstants<float>::twoPi) * p.vibDepthCents * 0.01f * fade * fade;
            vibPhase += vibInc;
            if (vibPhase >= 1.0f)
                vibPhase -= 1.0f;
            vibTime += dt;

            float pitch = currentPitch + bend + vib + ornamentOffset (p);
            ornTime += dt;

            if (! held)
            {
                pitch -= fallDepth * juce::jmin (1.0f, releaseTime / fallTime);
                releaseTime += dt;
            }

            float s = p.model == Model::accordion ? renderAccordion (pitch, sr, mc) : renderLead (pitch, sr, p, mc);
            s = filter.processSample (0, s);
            out[i] += s * ampEnv.getNextSample() * (0.35f + 0.65f * velocity);
        }

        if (! isActive())
            note = -1;
    }

private:
    struct ModelCoeffs
    {
        // lead
        float sawGain = 1.0f, pulseGain = 0.0f, detune = 0.0f;
        // accordion
        float gains[5] {};     // low, mid, mid sharp, mid flat, high
        float musette = 0.0f, reedLpCoeff = 1.0f, reedLevel = 1.0f, clickLevel = 0.0f, clickDecay = 0.0f;
    };

    void setNote (const NoteEvent& e)
    {
        note = e.note;
        targetPitch = e.pitch;
        stepAbove = e.stepAbove;
        ornament = e.ornament;
        fall = e.fall;
        ornTime = 0.0f;
        held = true;
    }

    float ornamentOffset (const EngineParams& p) const noexcept
    {
        if (ornament == Ornament::none)
            return 0.0f;

        const float step = juce::jmax (0.01f, p.ornSpeedSec);
        const int n = (int) (ornTime / step);

        switch (ornament)
        {
            case Ornament::grace:   return n < 1 ? stepAbove : 0.0f;
            case Ornament::mordent: return n == 1 ? stepAbove : 0.0f;
            case Ornament::trill:   return (n < 2 * (2 + juce::roundToInt (p.ornAmount * 4.0f)) && (n & 1) == 1) ? stepAbove : 0.0f;
            case Ornament::none:    break;
        }

        return 0.0f;
    }

    void prepareModel (ModelCoeffs& mc, const EngineParams& p, float sr) const
    {
        mc.sawGain = std::cos (p.tone * juce::MathConstants<float>::halfPi);
        mc.pulseGain = std::sin (p.tone * juce::MathConstants<float>::halfPi);
        mc.detune = p.detuneCents * 0.01f;

        if (p.model != Model::accordion)
            return;

        // The register knob sweeps through the classic reed combinations:
        // single reed, two detuned reeds, with the low octave, full musette, then the high reed on top.
        const float r = p.registerPos;
        const auto ramp = [r] (float from, float to) { return juce::jlimit (0.0f, 1.0f, (r - from) / (to - from)); };
        mc.gains[1] = 1.0f;
        mc.gains[2] = ramp (0.05f, 0.25f);
        mc.gains[0] = ramp (0.3f, 0.5f);
        mc.gains[3] = ramp (0.55f, 0.75f);
        mc.gains[4] = ramp (0.8f, 1.0f) * 0.7f;

        float power = 0.0f;
        for (float g : mc.gains)
            power += g * g;

        const float pressure = 0.15f + 0.85f * p.bellows;
        mc.reedLevel = pressure * std::sqrt (pressure) / std::sqrt (power) * 1.5f;
        mc.musette = p.musetteCents * 0.01f;
        const float brightHz = 900.0f * std::exp2 (pressure * 3.4f);
        mc.reedLpCoeff = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * juce::jmin (brightHz, 0.45f * sr) / sr);
        mc.clickLevel = p.click * 0.6f;
        mc.clickDecay = std::exp (-1.0f / (0.006f * sr));
    }

    float renderLead (float pitch, float sr, const EngineParams& p, const ModelCoeffs& mc) noexcept
    {
        const float incA = midiToHz (pitch - mc.detune) / sr;
        const float incB = midiToHz (pitch + mc.detune) / sr;
        const float incP = midiToHz (pitch) / sr;
        const float incS = incP * 0.5f;

        const float saws = 0.5f * (saw (phases[0], incA) + saw (phases[1], incB));
        const float pls = pulse (phases[2], incP, p.pulseWidth);
        const float subOsc = p.sub > 0.0f ? pulse (phases[3], incS, 0.5f) * p.sub * 0.7f : 0.0f;

        advance (phases[0], incA);
        advance (phases[1], incB);
        advance (phases[2], incP);
        advance (phases[3], incS);

        return saws * mc.sawGain + pls * mc.pulseGain + subOsc;
    }

    float renderAccordion (float pitch, float sr, const ModelCoeffs& mc) noexcept
    {
        // Pitch offsets of the five reeds in semitones.
        const float offsets[5] { -12.0f, 0.0f, mc.musette, -mc.musette * 0.85f, 12.0f };
        float sum = 0.0f;

        for (size_t r = 0; r < 5; ++r)
        {
            if (mc.gains[r] <= 0.0f)
                continue;

            const float inc = midiToHz (pitch + offsets[r]) / sr;
            // A free reed chops the air stream, which gives a narrow pulse with some saw in it.
            sum += mc.gains[r] * (0.6f * pulse (phases[r], inc, 0.3f) + 0.4f * saw (phases[r], inc));
            advance (phases[r], inc);
        }

        reedLp += (sum - reedLp) * mc.reedLpCoeff;

        // Key click: a short band of noise when the note starts.
        float clickOut = 0.0f;
        if (clickEnv > 0.0005f)
        {
            const float noise = random.nextFloat() * 2.0f - 1.0f;
            clickLp += (noise - clickLp) * 0.45f;
            clickHp += (clickLp - clickHp) * 0.12f;
            clickOut = (clickLp - clickHp) * clickEnv * mc.clickLevel;
            clickEnv *= mc.clickDecay;
        }

        return reedLp * mc.reedLevel + clickOut;
    }

    static float midiToHz (float pitch) noexcept
    {
        return 440.0f * std::exp2 ((pitch - 69.0f) / 12.0f);
    }

    static void advance (float& phase, float inc) noexcept
    {
        phase += inc;
        if (phase >= 1.0f)
            phase -= 1.0f;
    }

    static float polyBlep (float t, float dt) noexcept
    {
        if (t < dt)
        {
            t /= dt;
            return t + t - t * t - 1.0f;
        }

        if (t > 1.0f - dt)
        {
            t = (t - 1.0f) / dt;
            return t * t + t + t + 1.0f;
        }

        return 0.0f;
    }

    static float saw (float phase, float inc) noexcept
    {
        return 2.0f * phase - 1.0f - polyBlep (phase, inc);
    }

    static float pulse (float phase, float inc, float width) noexcept
    {
        float shifted = phase + 1.0f - width;
        if (shifted >= 1.0f)
            shifted -= 1.0f;

        return (phase < width ? 1.0f : -1.0f) + polyBlep (phase, inc) - polyBlep (shifted, inc);
    }

    double sampleRate = 44100.0;
    juce::ADSR ampEnv, fltEnv;
    juce::dsp::StateVariableTPTFilter<float> filter;
    juce::Random random;

    int note = -1;
    bool held = false;
    float velocity = 1.0f;
    juce::uint64 age = 0;
    float currentPitch = 60.0f, targetPitch = 60.0f;
    std::array<float, 5> phases { 0.0f, 0.37f, 0.71f, 0.13f, 0.52f };
    float vibPhase = 0.0f, vibTime = 0.0f;

    Ornament ornament = Ornament::none;
    bool fall = false;
    float stepAbove = 1.0f, ornTime = 0.0f, releaseTime = 0.0f;

    float reedLp = 0.0f, clickEnv = 0.0f, clickLp = 0.0f, clickHp = 0.0f;
};

/** Voice allocation, tuning, ornaments, MIDI handling and rendering. */
class SynthEngine
{
public:
    static constexpr int maxVoices = 16;
    static constexpr int chunkSize = 32;

    void prepare (double sampleRate, int maxBlockSize)
    {
        for (auto& v : voices)
            v.prepare (sampleRate);

        mix.assign ((size_t) juce::jmax (maxBlockSize, chunkSize), 0.0f);
        bendSmooth = 0.0f;
        bendTarget = 0.0f;
        bendCoeff = 1.0f - std::exp (-1.0f / (0.004f * (float) sampleRate / (float) chunkSize));
        heldNotes.clear();
        heldNotes.reserve (64);
    }

    void setParams (const EngineParams& newParams)
    {
        if (newParams.mono != params.mono || newParams.model != params.model)
            allNotesOff();

        params = newParams;
        tuning.set (params.scaleRoot, params.scale, params.snap);
    }

    /** Fixes the random sequence used for ornament chance, for repeatable tests. */
    void setRandomSeed (juce::int64 seed) { random.setSeed (seed); }

    const Tuning& getTuning() const noexcept { return tuning; }

    void process (juce::AudioBuffer<float>& buffer, const juce::MidiBuffer& midi)
    {
        const int numSamples = buffer.getNumSamples();
        if ((int) mix.size() < numSamples)
            mix.resize ((size_t) numSamples);

        std::fill (mix.begin(), mix.begin() + numSamples, 0.0f);

        int pos = 0;
        for (const auto meta : midi)
        {
            const int eventPos = juce::jlimit (0, numSamples, meta.samplePosition);
            renderRange (pos, eventPos);
            pos = eventPos;
            handleMidi (meta.getMessage());
        }

        renderRange (pos, numSamples);

        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            buffer.copyFrom (ch, 0, mix.data(), numSamples);
    }

    void allNotesOff()
    {
        heldNotes.clear();
        for (auto& v : voices)
            if (v.isHeld())
                v.release();
    }

private:
    void renderRange (int start, int end)
    {
        while (start < end)
        {
            const int n = juce::jmin (chunkSize, end - start);
            bendSmooth += (bendTarget - bendSmooth) * bendCoeff;

            for (auto& v : voices)
                v.render (mix.data() + start, n, params, bendSmooth);

            start += n;
        }
    }

    void handleMidi (const juce::MidiMessage& m)
    {
        if (m.isNoteOn())
            noteOn (m.getNoteNumber(), m.getFloatVelocity());
        else if (m.isNoteOff())
            noteOff (m.getNoteNumber());
        else if (m.isPitchWheel())
            bendTarget = ((float) m.getPitchWheelValue() - 8192.0f) / 8192.0f * params.bendRange;
        else if (m.isAllNotesOff() || m.isAllSoundOff())
            allNotesOff();
    }

    /** Builds the note a voice should play: tuned pitch plus the ornament chosen for it. */
    NoteEvent makeEvent (int note, float velocity, bool allowOrnament)
    {
        NoteEvent e;
        e.note = note;
        e.velocity = velocity;
        e.pitch = tuning.pitchFor (note);
        e.stepAbove = tuning.stepAbove (note);
        e.fall = params.fall && random.nextFloat() < params.ornChance;

        if (! allowOrnament)
            return e;

        Ornament enabled[3];
        int count = 0;
        if (params.grace)
            enabled[count++] = Ornament::grace;
        if (params.mordent)
            enabled[count++] = Ornament::mordent;
        if (params.trill)
            enabled[count++] = Ornament::trill;

        if (count > 0 && random.nextFloat() < params.ornChance)
            e.ornament = enabled[random.nextInt (count)];

        return e;
    }

    void noteOn (int note, float velocity)
    {
        ++counter;
        const auto e = makeEvent (note, velocity, true);

        if (params.mono)
        {
            auto& v = voices[0];
            const bool legato = ! heldNotes.empty() && v.isActive();
            heldNotes.erase (std::remove (heldNotes.begin(), heldNotes.end(), note), heldNotes.end());
            heldNotes.push_back (note);

            if (legato)
                v.slideTo (e);
            else
                v.start (e, counter, params, true);

            return;
        }

        Voice* target = nullptr;
        for (auto& v : voices)
            if (! v.isActive())
            {
                target = &v;
                break;
            }

        if (target == nullptr)
        {
            target = &voices[0];
            for (auto& v : voices)
                if (v.getAge() < target->getAge())
                    target = &v;
        }

        target->start (e, counter, params, false);
    }

    void noteOff (int note)
    {
        if (params.mono)
        {
            heldNotes.erase (std::remove (heldNotes.begin(), heldNotes.end(), note), heldNotes.end());

            if (heldNotes.empty())
                voices[0].release();
            else if (voices[0].getNote() == note)
                voices[0].slideTo (makeEvent (heldNotes.back(), 1.0f, false)); // falling back to a held note is never ornamented

            return;
        }

        for (auto& v : voices)
            if (v.isHeld() && v.getNote() == note)
                v.release();
    }

    std::array<Voice, maxVoices> voices;
    std::vector<float> mix;
    std::vector<int> heldNotes;
    EngineParams params;
    Tuning tuning;
    juce::Random random;
    juce::uint64 counter = 0;
    float bendSmooth = 0.0f, bendTarget = 0.0f, bendCoeff = 1.0f;
};
} // namespace amv
