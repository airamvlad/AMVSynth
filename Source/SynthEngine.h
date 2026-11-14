#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

namespace amv
{
/** Values read from the plugin parameters once per audio block. */
struct EngineParams
{
    float tone = 0.55f;        // 0 = detuned saws, 1 = pulse
    float pulseWidth = 0.32f;
    float detuneCents = 9.0f;
    float vibDepthCents = 28.0f;
    float vibRateHz = 6.2f;
    float vibDelaySec = 0.22f;
    float bendRange = 2.0f;    // semitones
    float glideSec = 0.045f;
    bool mono = true;
    float cutoffHz = 3200.0f;
    float resonance = 0.25f;
    float filterEnv = 0.35f;   // -1..1, scaled to +/- 5 octaves
    juce::ADSR::Parameters amp { 0.004f, 0.25f, 0.8f, 0.16f };
    juce::ADSR::Parameters flt { 0.002f, 0.32f, 0.4f, 0.2f };
};

/** One voice of the Juzni Vetar style lead: two detuned saws blended with a pulse,
    delayed vibrato, glide, a low-pass filter with its own envelope. */
class LeadVoice
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

    void start (int midiNote, float vel, juce::uint64 newAge, const EngineParams& p, bool glideFromCurrent)
    {
        const bool wasActive = isActive();
        note = midiNote;
        velocity = vel;
        age = newAge;
        held = true;
        targetPitch = (float) midiNote;

        if (! (glideFromCurrent && wasActive))
            currentPitch = targetPitch;

        if (! wasActive)
        {
            filter.reset();
            phaseA = 0.0f;
            phaseB = 0.37f;
            phasePulse = 0.0f;
        }

        vibTime = 0.0f;
        ampEnv.setParameters (p.amp);
        fltEnv.setParameters (p.flt);
        ampEnv.noteOn();
        fltEnv.noteOn();
    }

    /** Legato: move to a new pitch without retriggering the envelopes. */
    void slideTo (int midiNote)
    {
        note = midiNote;
        targetPitch = (float) midiNote;
        held = true;
    }

    void release()
    {
        held = false;
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
        const float glideCoeff = p.glideSec > 0.0005f ? 1.0f - std::exp (-1.0f / (p.glideSec * sr)) : 1.0f;
        const float vibInc = p.vibRateHz / sr;
        const float detune = p.detuneCents * 0.01f;
        const float sawGain = std::cos (p.tone * juce::MathConstants<float>::halfPi);
        const float pulseGain = std::sin (p.tone * juce::MathConstants<float>::halfPi);

        // The filter is updated once per render call; the engine calls us in short chunks.
        const float envNow = fltEnv.getNextSample();
        for (int i = 1; i < numSamples; ++i)
            fltEnv.getNextSample();

        const float keyTrack = (currentPitch - 60.0f) / 12.0f * 0.35f;
        const float cutoff = juce::jlimit (20.0f, 0.45f * sr,
                                           p.cutoffHz * std::exp2 (p.filterEnv * 5.0f * envNow + keyTrack));
        filter.setCutoffFrequency (cutoff);
        filter.setResonance (0.5f + p.resonance * p.resonance * 9.0f);

        for (int i = 0; i < numSamples; ++i)
        {
            currentPitch += (targetPitch - currentPitch) * glideCoeff;

            const float fade = p.vibDelaySec > 0.001f ? juce::jlimit (0.0f, 1.0f, vibTime / p.vibDelaySec) : 1.0f;
            const float vib = std::sin (vibPhase * juce::MathConstants<float>::twoPi) * p.vibDepthCents * 0.01f * fade * fade;
            vibPhase += vibInc;
            if (vibPhase >= 1.0f)
                vibPhase -= 1.0f;
            vibTime += 1.0f / sr;

            const float pitch = currentPitch + bend + vib;
            const float incA = midiToHz (pitch - detune) / sr;
            const float incB = midiToHz (pitch + detune) / sr;
            const float incP = midiToHz (pitch) / sr;

            const float saws = 0.5f * (saw (phaseA, incA) + saw (phaseB, incB));
            const float pls = pulse (phasePulse, incP, p.pulseWidth);

            advance (phaseA, incA);
            advance (phaseB, incB);
            advance (phasePulse, incP);

            float s = saws * sawGain + pls * pulseGain;
            s = filter.processSample (0, s);
            out[i] += s * ampEnv.getNextSample() * (0.35f + 0.65f * velocity);
        }

        if (! isActive())
            note = -1;
    }

private:
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

    int note = -1;
    bool held = false;
    float velocity = 1.0f;
    juce::uint64 age = 0;
    float currentPitch = 60.0f, targetPitch = 60.0f;
    float phaseA = 0.0f, phaseB = 0.37f, phasePulse = 0.0f;
    float vibPhase = 0.0f, vibTime = 0.0f;
};

/** Voice allocation, MIDI handling and rendering. */
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
        if (newParams.mono != params.mono)
            allNotesOff();

        params = newParams;
    }

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

    void noteOn (int note, float velocity)
    {
        ++counter;

        if (params.mono)
        {
            auto& v = voices[0];
            const bool legato = ! heldNotes.empty() && v.isActive();
            heldNotes.erase (std::remove (heldNotes.begin(), heldNotes.end(), note), heldNotes.end());
            heldNotes.push_back (note);

            if (legato)
                v.slideTo (note);
            else
                v.start (note, velocity, counter, params, true);

            return;
        }

        LeadVoice* target = nullptr;
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

        target->start (note, velocity, counter, params, false);
    }

    void noteOff (int note)
    {
        if (params.mono)
        {
            heldNotes.erase (std::remove (heldNotes.begin(), heldNotes.end(), note), heldNotes.end());

            if (heldNotes.empty())
                voices[0].release();
            else if (voices[0].getNote() == note)
                voices[0].slideTo (heldNotes.back());

            return;
        }

        for (auto& v : voices)
            if (v.isHeld() && v.getNote() == note)
                v.release();
    }

    std::array<LeadVoice, maxVoices> voices;
    std::vector<float> mix;
    std::vector<int> heldNotes;
    EngineParams params;
    juce::uint64 counter = 0;
    float bendSmooth = 0.0f, bendTarget = 0.0f, bendCoeff = 1.0f;
};
} // namespace amv
