#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

#include "Arpeggiator.h"

#include <array>
#include <memory>
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

/** The four effects that can change places. The compressor and the bitcrusher always come first,
    the chorus, flanger and phaser sit right before the delay, and the equaliser, the kick duck
    and the limiter come last. */
enum class FxStage { distortion, gate, delay, reverb };

struct FxOrderDef
{
    const char* name;
    std::array<FxStage, 4> stages;
};

inline const std::array<FxOrderDef, 4>& getFxOrders()
{
    using S = FxStage;
    static const std::array<FxOrderDef, 4> orders { {
        { "Dist > Gate > Delay > Reverb", { S::distortion, S::gate, S::delay, S::reverb } },
        { "Dist > Delay > Reverb > Gate", { S::distortion, S::delay, S::reverb, S::gate } }, // the gate chops the echoes and the tail too
        { "Gate > Dist > Delay > Reverb", { S::gate, S::distortion, S::delay, S::reverb } },
        { "Delay > Reverb > Dist > Gate", { S::delay, S::reverb, S::distortion, S::gate } }, // everything goes through the distortion
    } };
    return orders;
}

struct FxParams
{
    int order = 0;         // index into getFxOrders()

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

    bool compOn = false;
    float compThresholdDb = -18.0f, compRatio = 4.0f, compAttackMs = 10.0f, compReleaseMs = 120.0f, compMakeupDb = 0.0f;

    bool crushOn = false;
    float crushBits = 8.0f, crushRateHz = 8000.0f, crushMix = 1.0f;

    bool chorusOn = false;
    float chorusRateHz = 0.6f, chorusDepth = 0.5f, chorusMix = 0.4f;

    bool flangerOn = false;
    float flangerRateHz = 0.25f, flangerDepth = 0.6f, flangerFeedback = 0.5f, flangerMix = 0.5f; // feedback -0.95..0.95

    bool phaserOn = false;
    float phaserRateHz = 0.4f, phaserDepth = 0.7f, phaserFeedback = 0.5f, phaserMix = 0.5f;

    bool eqOn = false;
    float eqLowDb = 0.0f, eqMidDb = 0.0f, eqMidHz = 1000.0f, eqHighDb = 0.0f;
};

/** A second-order filter section for the equaliser, after the Audio EQ Cookbook. */
struct EqBand
{
    enum Type { lowShelf, peak, highShelf };

    void set (Type type, float hz, float gainDb, float q, float sampleRate) noexcept
    {
        const float a = std::pow (10.0f, gainDb / 40.0f);
        const float w = juce::MathConstants<float>::twoPi * juce::jlimit (20.0f, 0.45f * sampleRate, hz) / sampleRate;
        const float cs = std::cos (w), alpha = std::sin (w) / (2.0f * q);
        float a0 = 1.0f;

        if (type == peak)
        {
            b0 = 1.0f + alpha * a;
            b1 = -2.0f * cs;
            b2 = 1.0f - alpha * a;
            a0 = 1.0f + alpha / a;
            a1 = -2.0f * cs;
            a2 = 1.0f - alpha / a;
        }
        else
        {
            const float sign = type == lowShelf ? 1.0f : -1.0f;
            const float root = 2.0f * std::sqrt (a) * alpha;
            b0 = a * ((a + 1.0f) - sign * (a - 1.0f) * cs + root);
            b1 = sign * 2.0f * a * ((a - 1.0f) - sign * (a + 1.0f) * cs);
            b2 = a * ((a + 1.0f) - sign * (a - 1.0f) * cs - root);
            a0 = (a + 1.0f) + sign * (a - 1.0f) * cs + root;
            a1 = -sign * 2.0f * ((a - 1.0f) + sign * (a + 1.0f) * cs);
            a2 = (a + 1.0f) + sign * (a - 1.0f) * cs - root;
        }

        b0 /= a0;
        b1 /= a0;
        b2 /= a0;
        a1 /= a0;
        a2 /= a0;
    }

    float process (float x, int channel) noexcept
    {
        auto& s = state[(size_t) channel];
        const float y = b0 * x + s[0];
        s[0] = b1 * x - a1 * y + s[1];
        s[1] = b2 * x - a2 * y;
        return y;
    }

    void reset() noexcept { state = {}; }

    float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f, a1 = 0.0f, a2 = 0.0f;
    std::array<std::array<float, 2>, 2> state {};
};

/** A short stereo delay line that is read at a moving point, for the chorus and the flanger. */
struct ModDelay
{
    void prepare (double sampleRate, double maxSeconds)
    {
        for (auto& line : lines)
            line.assign ((size_t) (sampleRate * maxSeconds) + 8, 0.0f);
        write = 0;
    }

    void reset() noexcept
    {
        for (auto& line : lines)
            std::fill (line.begin(), line.end(), 0.0f);
    }

    float read (int channel, float delaySamples) const noexcept
    {
        const auto& line = lines[(size_t) channel];
        const auto size = line.size();
        float pos = (float) write - juce::jlimit (1.0f, (float) size - 4.0f, delaySamples);
        if (pos < 0.0f)
            pos += (float) size;

        const auto i0 = (size_t) pos % size;
        const float frac = pos - std::floor (pos);
        return line[i0] + (line[(i0 + 1) % size] - line[i0]) * frac;
    }

    void push (float left, float right) noexcept
    {
        lines[0][write] = left;
        lines[1][write] = right;
        write = (write + 1) % lines[0].size();
    }

    std::array<std::vector<float>, 2> lines;
    size_t write = 0;
};

/** The effect chain after the voices: compressor, bitcrusher, distortion, trance gate, chorus,
    flanger, phaser, delay, reverb, equaliser, kick duck, limiter.
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

        // The tube and fold curves add harmonics far above the input; running them at four times
        // the sample rate keeps those from folding back as aliasing.
        oversampleBlock = juce::jmax (16, maxBlockSize);
        oversampling = std::make_unique<juce::dsp::Oversampling<float>> (2, 2, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true);
        oversampling->initProcessing ((size_t) oversampleBlock);

        chorusLine.prepare (sampleRate, 0.05);
        flangerLine.prepare (sampleRate, 0.02);
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
        chorusLine.reset();
        flangerLine.reset();
        chorusPhase = flangerPhase = phaserPhase = 0.0f;
        flangerLast = phaserLast = { 0.0f, 0.0f };
        phaserState = {};
        compEnvelope = 0.0f;
        bitHold = { 0.0f, 0.0f };
        bitPhase = 1.0f;
        for (auto& band : eqBands)
            band.reset();
        if (oversampling != nullptr)
            oversampling->reset();
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

        if (! p.gateOn)
            gateGain = 1.0f;

        if (p.compOn)
            compress (left, right, numSamples, p);
        else
            compEnvelope = 0.0f;

        if (p.crushOn)
            bitcrush (left, right, numSamples, p);

        for (const auto stage : getFxOrders()[(size_t) juce::jlimit (0, (int) getFxOrders().size() - 1, p.order)].stages)
        {
            switch (stage)
            {
                case FxStage::distortion:
                    if (p.distOn)
                        distort (left, right, numSamples, p);
                    break;

                case FxStage::gate:
                    if (p.gateOn)
                        gate (left, right, numSamples, p, startBeat, beatsPerSample);
                    break;

                case FxStage::delay:
                    if (p.chorusOn)
                        chorus (left, right, numSamples, p);
                    if (p.flangerOn)
                        flanger (left, right, numSamples, p);
                    if (p.phaserOn)
                        phaser (left, right, numSamples, p);
                    if (p.delayOn)
                        delay (left, right, numSamples, p, bpm);
                    break;

                case FxStage::reverb:
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
                    break;
            }
        }

        if (p.eqOn)
            equalise (left, right, numSamples, p);

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

        // Work through the buffer in pieces the oversampler was prepared for.
        for (int start = 0; start < numSamples; start += oversampleBlock)
        {
            const int n = juce::jmin (oversampleBlock, numSamples - start);
            float* piece[2] { left + start, right + start };
            juce::dsp::AudioBlock<float> block (piece, 2, (size_t) n);
            auto up = oversampling->processSamplesUp (block);

            for (size_t ch = 0; ch < 2; ++ch)
            {
                float* data = up.getChannelPointer (ch);
                for (size_t i = 0; i < up.getNumSamples(); ++i)
                {
                    const float x = data[i] * drive;
                    float y;

                    if (p.distType == 1)
                        y = std::sin (juce::MathConstants<float>::halfPi * juce::jlimit (-8.0f, 8.0f, x)); // wavefolder
                    else
                        y = (std::tanh (x + 0.2f) - 0.19738f) * 1.04f;                                    // slightly asymmetric tube

                    // The dry part goes through the same filters, so the mix stays in phase.
                    data[i] += (y * (p.distType == 1 ? 0.8f : juce::jmax (makeup, 0.5f)) - data[i]) * mix;
                }
            }

            oversampling->processSamplesDown (block);
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

    void compress (float* left, float* right, int numSamples, const FxParams& p)
    {
        // Both sides are turned down together by the louder one, with a soft knee of 6 dB.
        const float attack = 1.0f - std::exp (-1.0f / (juce::jmax (0.05f, p.compAttackMs) * 0.001f * (float) sampleRate));
        const float release = 1.0f - std::exp (-1.0f / (juce::jmax (1.0f, p.compReleaseMs) * 0.001f * (float) sampleRate));
        const float slope = 1.0f - 1.0f / juce::jmax (1.0f, p.compRatio);
        const float makeup = juce::Decibels::decibelsToGain (p.compMakeupDb);
        const float knee = 6.0f;

        for (int i = 0; i < numSamples; ++i)
        {
            const float level = juce::jmax (std::abs (left[i]), std::abs (right[i]));
            compEnvelope += (level - compEnvelope) * (level > compEnvelope ? attack : release);

            const float over = juce::Decibels::gainToDecibels (compEnvelope, -100.0f) - p.compThresholdDb;
            float reduction = 0.0f;
            if (over > knee * 0.5f)
                reduction = over * slope;
            else if (over > -knee * 0.5f)
                reduction = slope * (over + knee * 0.5f) * (over + knee * 0.5f) / (2.0f * knee);

            const float gain = juce::Decibels::decibelsToGain (-reduction) * makeup;
            left[i] *= gain;
            right[i] *= gain;
        }
    }

    void bitcrush (float* left, float* right, int numSamples, const FxParams& p)
    {
        // Fewer steps for the level and a new value only at the lowered sample rate.
        const float levels = std::exp2 (juce::jlimit (1.0f, 16.0f, p.crushBits) - 1.0f);
        const float step = juce::jlimit (0.001f, 1.0f, p.crushRateHz / (float) sampleRate);

        for (int i = 0; i < numSamples; ++i)
        {
            bitPhase += step;
            if (bitPhase >= 1.0f)
            {
                bitPhase -= 1.0f;
                bitHold[0] = std::round (left[i] * levels) / levels;
                bitHold[1] = std::round (right[i] * levels) / levels;
            }

            left[i] += (bitHold[0] - left[i]) * p.crushMix;
            right[i] += (bitHold[1] - right[i]) * p.crushMix;
        }
    }

    void chorus (float* left, float* right, int numSamples, const FxParams& p)
    {
        // Two copies per side, each a little late and drifting; the sides drift a quarter cycle apart.
        const float step = p.chorusRateHz / (float) sampleRate;
        const float centre = 0.014f * (float) sampleRate, sweep = p.chorusDepth * 0.006f * (float) sampleRate;
        const float twoPi = juce::MathConstants<float>::twoPi;

        for (int i = 0; i < numSamples; ++i)
        {
            chorusLine.push (left[i], right[i]);
            chorusPhase += step;
            chorusPhase -= std::floor (chorusPhase);

            const float a = twoPi * chorusPhase;
            const float wetL = 0.5f * (chorusLine.read (0, centre + sweep * std::sin (a)) + chorusLine.read (0, centre * 1.6f + sweep * std::sin (a + 2.1f)));
            const float wetR = 0.5f * (chorusLine.read (1, centre + sweep * std::sin (a + 1.571f)) + chorusLine.read (1, centre * 1.6f + sweep * std::sin (a + 3.67f)));

            left[i] += (wetL - left[i]) * p.chorusMix * 0.7f;
            right[i] += (wetR - right[i]) * p.chorusMix * 0.7f;
        }
    }

    void flanger (float* left, float* right, int numSamples, const FxParams& p)
    {
        // A very short delay that sweeps, fed back into itself: a comb that moves through the sound.
        const float step = p.flangerRateHz / (float) sampleRate;
        const float shortest = 0.0004f * (float) sampleRate, sweep = p.flangerDepth * 0.007f * (float) sampleRate;
        const float feedback = juce::jlimit (-0.95f, 0.95f, p.flangerFeedback);
        const float twoPi = juce::MathConstants<float>::twoPi;

        for (int i = 0; i < numSamples; ++i)
        {
            flangerLine.push (left[i] + flangerLast[0] * feedback, right[i] + flangerLast[1] * feedback);
            flangerPhase += step;
            flangerPhase -= std::floor (flangerPhase);

            flangerLast[0] = flangerLine.read (0, shortest + sweep * (0.5f + 0.5f * std::sin (twoPi * flangerPhase)));
            flangerLast[1] = flangerLine.read (1, shortest + sweep * (0.5f + 0.5f * std::sin (twoPi * flangerPhase + 1.571f)));

            left[i] += flangerLast[0] * p.flangerMix;
            right[i] += flangerLast[1] * p.flangerMix;
            left[i] *= 1.0f - 0.3f * p.flangerMix;
            right[i] *= 1.0f - 0.3f * p.flangerMix;
        }
    }

    void phaser (float* left, float* right, int numSamples, const FxParams& p)
    {
        // Six all-pass stages whose corner sweeps up and down; mixed with the dry sound they cut moving notches.
        const float step = p.phaserRateHz / (float) sampleRate;
        const float feedback = juce::jlimit (0.0f, 0.95f, p.phaserFeedback);
        const float twoPi = juce::MathConstants<float>::twoPi;
        float* io[2] { left, right };

        for (int i = 0; i < numSamples; ++i)
        {
            phaserPhase += step;
            phaserPhase -= std::floor (phaserPhase);

            for (size_t ch = 0; ch < 2; ++ch)
            {
                const float lfo = 0.5f + 0.5f * std::sin (twoPi * phaserPhase + (ch == 0 ? 0.0f : 1.571f));
                const float hz = juce::jmin (0.45f * (float) sampleRate, 300.0f * std::exp2 (lfo * p.phaserDepth * 4.5f));
                const float t = std::tan (juce::MathConstants<float>::pi * hz / (float) sampleRate);
                const float c = (t - 1.0f) / (t + 1.0f);

                float x = io[ch][i] + phaserLast[ch] * feedback;
                for (auto& z : phaserState[ch])
                {
                    const float y = c * x + z;
                    z = x - c * y;
                    x = y;
                }

                phaserLast[ch] = x;
                io[ch][i] = (io[ch][i] + x * p.phaserMix) * (1.0f - 0.3f * p.phaserMix);
            }
        }
    }

    void equalise (float* left, float* right, int numSamples, const FxParams& p)
    {
        eqBands[0].set (EqBand::lowShelf, 150.0f, p.eqLowDb, 0.707f, (float) sampleRate);
        eqBands[1].set (EqBand::peak, p.eqMidHz, p.eqMidDb, 0.9f, (float) sampleRate);
        eqBands[2].set (EqBand::highShelf, 6000.0f, p.eqHighDb, 0.707f, (float) sampleRate);

        for (int i = 0; i < numSamples; ++i)
            for (auto& band : eqBands)
            {
                left[i] = band.process (left[i], 0);
                right[i] = band.process (right[i], 1);
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
    ModDelay chorusLine, flangerLine;
    float chorusPhase = 0.0f, flangerPhase = 0.0f, phaserPhase = 0.0f;
    std::array<float, 2> flangerLast { 0.0f, 0.0f }, phaserLast { 0.0f, 0.0f };
    std::array<std::array<float, 6>, 2> phaserState {};
    float compEnvelope = 0.0f;
    std::array<float, 2> bitHold { 0.0f, 0.0f };
    float bitPhase = 1.0f;
    std::array<EqBand, 3> eqBands;
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampling;
    int oversampleBlock = 512;
    juce::Reverb reverb;
    juce::dsp::Limiter<float> limiter;
};
} // namespace amv
