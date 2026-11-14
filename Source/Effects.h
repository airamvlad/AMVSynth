#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

#include "Arpeggiator.h"

#include <array>
#include <cmath>
#include <vector>

namespace amv
{
/** A trance gate pattern: which steps are open. */
struct GatePatternDef
{
    const char* name;
    std::vector<bool> steps;
};

inline const std::vector<GatePatternDef>& getGatePatterns()
{
    static const std::vector<GatePatternDef> patterns {
        { "Straight", { true } },
        { "Offbeat", { false, true } },
        { "Gallop", { true, false, true, true } },
        { "3+3+2", { true, true, false, true, true, false, true, false } },
        { "7/8  (3+2+2)", { true, true, false, true, false, true, false } },
        { "9/8  (2+2+2+3)", { true, false, true, false, true, false, true, true, false } },
        { "11/8  (2+2+3+2+2)", { true, false, true, false, true, true, false, true, false, true, false } },
    };
    return patterns;
}

inline const std::array<const char*, 3>& getDistortionTypes()
{
    static const std::array<const char*, 3> types { "Tube", "Fold", "Crush" };
    return types;
}

struct DelayTimeDef
{
    const char* name;
    double beats; // quarter notes
};

inline const std::array<DelayTimeDef, 5>& getDelayTimes()
{
    static const std::array<DelayTimeDef, 5> times { { { "1/16", 0.25 }, { "1/8", 0.5 }, { "3/16", 0.75 }, { "1/4", 1.0 }, { "3/8", 1.5 } } };
    return times;
}

struct FxParams
{
    bool distOn = false;
    int distType = 0;
    float distDriveDb = 12.0f, distMix = 1.0f;

    bool gateOn = false;
    int gateRate = 1;      // index into getArpRates(): 1/8, 1/16, 1/32
    int gatePattern = 0;
    float gateDepth = 1.0f, gateLength = 0.5f, gateSmoothMs = 4.0f;

    bool delayOn = false;
    int delayTime = 2;
    float delayFeedback = 0.45f, delayToneHz = 4000.0f, delayMix = 0.3f;
    bool delayPingPong = true;

    bool reverbOn = false;
    float reverbSize = 0.6f, reverbDamp = 0.4f, reverbMix = 0.25f;

    bool duckOn = false;
    float duckDepth = 0.8f, duckRelease = 0.5f; // release as a fraction of a beat
};

/** The effect chain after the voices: distortion, trance gate, delay, reverb, kick duck, limiter.
    The tempo-based effects follow the host position when it plays and run freely otherwise. */
class FxChain
{
public:
    void prepare (double newSampleRate, int maxBlockSize)
    {
        sampleRate = newSampleRate;

        const auto delaySize = (size_t) (sampleRate * 3.0) + 8;
        for (auto& line : delayLines)
            line.assign (delaySize, 0.0f);

        reverb.setSampleRate (sampleRate);
        limiter.prepare ({ sampleRate, (juce::uint32) juce::jmax (1, maxBlockSize), 2 });
        limiter.setThreshold (-0.3f);
        limiter.setRelease (80.0f);
        reset();
    }

    void reset()
    {
        for (auto& line : delayLines)
            std::fill (line.begin(), line.end(), 0.0f);

        delayWrite = 0;
        delayLp = { 0.0f, 0.0f };
        delaySamples = 0.0f;
        gateGain = 1.0f;
        duckGain = 1.0f;
        crushHold = { 0.0f, 0.0f };
        crushCounter = 0.0f;
        freeBeat = 0.0;
        reverb.reset();
        limiter.reset();
    }

    /** Processes a stereo buffer in place. */
    void process (juce::AudioBuffer<float>& buffer, const FxParams& p, const TransportInfo& t)
    {
        const int numSamples = buffer.getNumSamples();
        if (buffer.getNumChannels() < 2 || numSamples == 0)
            return;

        float* left = buffer.getWritePointer (0);
        float* right = buffer.getWritePointer (1);

        const double bpm = juce::jmax (20.0, t.bpm);
        const double beatsPerSample = bpm / 60.0 / sampleRate;
        const double startBeat = t.playing ? t.ppq : freeBeat;

        if (p.distOn)
            distort (left, right, numSamples, p);

        if (p.gateOn)
            gate (left, right, numSamples, p, startBeat, beatsPerSample);
        else
            gateGain = 1.0f;

        if (p.delayOn)
            delay (left, right, numSamples, p, bpm);

        if (p.reverbOn)
        {
            juce::Reverb::Parameters rp;
            rp.roomSize = p.reverbSize;
            rp.damping = p.reverbDamp;
            rp.wetLevel = p.reverbMix;
            rp.dryLevel = 1.0f - 0.5f * p.reverbMix;
            rp.width = 1.0f;
            reverb.setParameters (rp);
            reverb.processStereo (left, right, numSamples);
        }

        if (p.duckOn)
            duck (left, right, numSamples, p, startBeat, beatsPerSample);
        else
            duckGain = 1.0f;

        juce::dsp::AudioBlock<float> block (buffer);
        limiter.process (juce::dsp::ProcessContextReplacing<float> (block));

        freeBeat = t.playing ? t.ppq + beatsPerSample * numSamples : freeBeat + beatsPerSample * numSamples;
    }

private:
    void distort (float* left, float* right, int numSamples, const FxParams& p)
    {
        const float drive = juce::Decibels::decibelsToGain (p.distDriveDb);
        const float makeup = 1.0f / std::sqrt (drive);
        const float mix = p.distMix;
        float* channels[2] { left, right };

        if (p.distType == 2)
        {
            // Crush: fewer bits and a lower sample rate as the drive goes up.
            const float amount = juce::jlimit (0.0f, 1.0f, p.distDriveDb / 36.0f);
            const float levels = std::exp2 (12.0f - 9.0f * amount);
            const float holdSamples = 1.0f + amount * amount * 40.0f;

            for (int i = 0; i < numSamples; ++i)
            {
                crushCounter += 1.0f;
                if (crushCounter >= holdSamples)
                {
                    crushCounter -= holdSamples;
                    crushHold[0] = std::round (left[i] * levels) / levels;
                    crushHold[1] = std::round (right[i] * levels) / levels;
                }

                left[i] += (crushHold[0] - left[i]) * mix;
                right[i] += (crushHold[1] - right[i]) * mix;
            }

            return;
        }

        for (auto* data : channels)
            for (int i = 0; i < numSamples; ++i)
            {
                const float x = data[i] * drive;
                float y;

                if (p.distType == 1)
                    y = std::sin (juce::MathConstants<float>::halfPi * juce::jlimit (-8.0f, 8.0f, x)); // wavefolder
                else
                    y = (std::tanh (x + 0.2f) - 0.19738f) * 1.04f;                                    // slightly asymmetric tube

                data[i] += (y * (p.distType == 1 ? 0.8f : juce::jmax (makeup, 0.5f)) - data[i]) * mix;
            }
    }

    void gate (float* left, float* right, int numSamples, const FxParams& p, double startBeat, double beatsPerSample)
    {
        const auto& pattern = getGatePatterns()[(size_t) juce::jlimit (0, (int) getGatePatterns().size() - 1, p.gatePattern)].steps;
        const auto length = (juce::int64) pattern.size();
        const double stepBeats = 0.5 / (double) (1 << juce::jlimit (0, 2, p.gateRate));
        const float coeff = 1.0f - std::exp (-1.0f / (juce::jmax (0.5f, p.gateSmoothMs) * 0.001f * (float) sampleRate));
        const float closed = 1.0f - p.gateDepth;

        for (int i = 0; i < numSamples; ++i)
        {
            const double pos = (startBeat + beatsPerSample * i) / stepBeats;
            const double whole = std::floor (pos);
            const auto index = (size_t) ((((juce::int64) whole % length) + length) % length);
            const bool open = pattern[index] && (pos - whole) < (double) p.gateLength;

            gateGain += ((open ? 1.0f : closed) - gateGain) * coeff;
            left[i] *= gateGain;
            right[i] *= gateGain;
        }
    }

    void delay (float* left, float* right, int numSamples, const FxParams& p, double bpm)
    {
        const auto size = delayLines[0].size();
        const double beats = getDelayTimes()[(size_t) juce::jlimit (0, (int) getDelayTimes().size() - 1, p.delayTime)].beats;
        const float target = (float) juce::jlimit (16.0, (double) size - 4.0, beats * 60.0 / bpm * sampleRate);
        if (delaySamples <= 0.0f)
            delaySamples = target;

        const float lpCoeff = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * juce::jmin (p.delayToneHz, 0.45f * (float) sampleRate) / (float) sampleRate);
        const float feedback = juce::jmin (0.95f, p.delayFeedback);

        for (int i = 0; i < numSamples; ++i)
        {
            // Glide to a new delay time instead of jumping, so tempo changes do not click.
            delaySamples += (target - delaySamples) * 0.0005f;

            float readPos = (float) delayWrite - delaySamples;
            if (readPos < 0.0f)
                readPos += (float) size;

            const auto i0 = (size_t) readPos % size;
            const auto i1 = (i0 + 1) % size;
            const float frac = readPos - std::floor (readPos);
            const float tapL = delayLines[0][i0] + (delayLines[0][i1] - delayLines[0][i0]) * frac;
            const float tapR = delayLines[1][i0] + (delayLines[1][i1] - delayLines[1][i0]) * frac;

            delayLp[0] += (tapL - delayLp[0]) * lpCoeff;
            delayLp[1] += (tapR - delayLp[1]) * lpCoeff;

            if (p.delayPingPong)
            {
                // The input goes in on the left and the echoes bounce between the sides.
                delayLines[0][delayWrite] = 0.5f * (left[i] + right[i]) + delayLp[1] * feedback;
                delayLines[1][delayWrite] = delayLp[0] * feedback;
            }
            else
            {
                delayLines[0][delayWrite] = left[i] + delayLp[0] * feedback;
                delayLines[1][delayWrite] = right[i] + delayLp[1] * feedback;
            }

            left[i] += tapL * p.delayMix;
            right[i] += tapR * p.delayMix;
            delayWrite = (delayWrite + 1) % size;
        }
    }

    void duck (float* left, float* right, int numSamples, const FxParams& p, double startBeat, double beatsPerSample)
    {
        // Volume dips on every quarter note and recovers, like sidechaining to a four-on-the-floor kick.
        const float release = juce::jlimit (0.05f, 1.0f, p.duckRelease);
        const float coeff = 1.0f - std::exp (-1.0f / (0.002f * (float) sampleRate));

        for (int i = 0; i < numSamples; ++i)
        {
            const double beat = startBeat + beatsPerSample * i;
            const float phase = (float) (beat - std::floor (beat));
            const float x = juce::jmin (1.0f, phase / release);
            const float target = 1.0f - p.duckDepth * (1.0f - x) * (1.0f - x);

            duckGain += (target - duckGain) * coeff;
            left[i] *= duckGain;
            right[i] *= duckGain;
        }
    }

    double sampleRate = 44100.0;
    std::array<std::vector<float>, 2> delayLines;
    size_t delayWrite = 0;
    std::array<float, 2> delayLp { 0.0f, 0.0f };
    float delaySamples = 0.0f;
    float gateGain = 1.0f, duckGain = 1.0f;
    std::array<float, 2> crushHold { 0.0f, 0.0f };
    float crushCounter = 0.0f;
    double freeBeat = 0.0;
    juce::Reverb reverb;
    juce::dsp::Limiter<float> limiter;
};
} // namespace amv
