#pragma once

#include <juce_core/juce_core.h>

#include <array>
#include <cmath>
#include <vector>

namespace amv
{
namespace filter
{
enum Type
{
    lowPass12 = 0,
    lowPass24,
    bandPass,
    highPass12,
    highPass24,
    notch,
    comb,
    formant,
    phaser,
    numTypes
};
} // namespace filter

inline const std::array<const char*, filter::numTypes>& getFilterTypes()
{
    static const std::array<const char*, filter::numTypes> names { "Low Pass 12", "Low Pass 24", "Band Pass", "High Pass 12",
                                                                   "High Pass 24", "Notch", "Comb", "Formant", "Phaser" };
    return names;
}

/** The filter of one voice, for two channels.

    The low, band and high pass and the notch are a state variable filter; the 24 dB types run a
    second, non-resonant stage after it. The comb is a tuned delay with feedback: its cutoff sets
    the pitch of the comb and the resonance how long it rings. The formant type is three band
    passes that the cutoff moves through the vowels A, E, I, O, U, with the resonance as their
    sharpness. The phaser is four all-pass stages around the cutoff with the resonance as feedback. */
class VoiceFilter
{
public:
    void prepare (double newSampleRate)
    {
        sampleRate = (float) newSampleRate;

        // Long enough for the lowest comb pitch.
        int size = 256;
        while ((float) size < sampleRate / minCombHz + 8.0f)
            size *= 2;

        for (auto& c : channels)
            c.delay.assign ((size_t) size, 0.0f);

        mask = size - 1;
        smoothCoeff = 1.0f - std::exp (-1.0f / (0.003f * sampleRate));
        reset();
    }

    void reset() noexcept
    {
        for (auto& c : channels)
        {
            c.s1 = c.s2 = c.t1 = c.t2 = c.damp = c.phaserFeedback = 0.0f;
            c.formant = {};
            c.allPass = {};
            c.write = 0;
            c.delayNow = -1.0f;
            std::fill (c.delay.begin(), c.delay.end(), 0.0f);
        }
    }

    /** `resonance` is 0..1. Call it once per chunk, before processSample. */
    void set (int newType, float cutoffHz, float resonance) noexcept
    {
        type = juce::jlimit (0, (int) filter::numTypes - 1, newType);
        cutoffHz = juce::jlimit (20.0f, 0.45f * sampleRate, cutoffHz);
        resonance = juce::jlimit (0.0f, 1.0f, resonance);

        g = std::tan (juce::MathConstants<float>::pi * cutoffHz / sampleRate);
        k = 1.0f / (0.5f + resonance * resonance * 9.0f);
        h = 1.0f / (1.0f + g * (g + k));
        h2 = 1.0f / (1.0f + g * (g + butterworth));
        bandGain = std::sqrt (k);

        combDelay = sampleRate / juce::jmax (minCombHz, cutoffHz);
        combFeedback = 0.97f * std::sqrt (resonance);
        combGain = 1.0f - 0.85f * combFeedback;

        if (type == filter::formant)
        {
            // The cutoff walks through the vowels: 200 Hz is A, 8 kHz is U.
            static constexpr float vowels[5][3] { { 730.0f, 1090.0f, 2440.0f }, { 530.0f, 1840.0f, 2480.0f }, { 270.0f, 2290.0f, 3010.0f },
                                                  { 570.0f, 840.0f, 2410.0f }, { 300.0f, 870.0f, 2240.0f } };
            const float place = juce::jlimit (0.0f, 3.999f, std::log2 (cutoffHz / 200.0f) / std::log2 (40.0f) * 4.0f);
            const int vowel = (int) place;
            const float mix = place - (float) vowel;
            formantK = 1.0f / (3.0f + resonance * 17.0f);

            for (size_t f = 0; f < 3; ++f)
            {
                const float hz = vowels[vowel][f] + (vowels[vowel + 1][f] - vowels[vowel][f]) * mix;
                formantG[f] = std::tan (juce::MathConstants<float>::pi * juce::jmin (hz, 0.45f * sampleRate) / sampleRate);
                formantH[f] = 1.0f / (1.0f + formantG[f] * (formantG[f] + formantK));
            }
        }
        else if (type == filter::phaser)
        {
            const float t = std::tan (juce::MathConstants<float>::pi * cutoffHz / sampleRate);
            allPassCoeff = (t - 1.0f) / (t + 1.0f);
            phaserAmount = 0.9f * resonance;
        }
    }

    float processSample (int channel, float x) noexcept
    {
        auto& c = channels[(size_t) channel];

        if (type == filter::comb)
            return processComb (c, x);
        if (type == filter::formant)
            return processFormant (c, x);
        if (type == filter::phaser)
            return processPhaser (c, x);

        const float hp = h * (x - c.s1 * (g + k) - c.s2);
        const float bp = hp * g + c.s1;
        c.s1 = hp * g + bp;
        const float lp = bp * g + c.s2;
        c.s2 = bp * g + lp;

        switch (type)
        {
            case filter::lowPass24:  return secondStage (c, lp, false);
            case filter::bandPass:   return bp * bandGain;
            case filter::highPass12: return hp;
            case filter::highPass24: return secondStage (c, hp, true);
            case filter::notch:      return x - k * bp;
            default:                 return lp;
        }
    }

private:
    struct Channel
    {
        float s1 = 0.0f, s2 = 0.0f, t1 = 0.0f, t2 = 0.0f;
        std::vector<float> delay;
        int write = 0;
        float delayNow = -1.0f, damp = 0.0f, phaserFeedback = 0.0f;
        std::array<std::array<float, 2>, 3> formant {};
        std::array<float, 4> allPass {};
    };

    float processFormant (Channel& c, float x) const noexcept
    {
        // Three band passes side by side; the higher formants are quieter, as in a voice.
        static constexpr float gains[3] { 1.0f, 0.6f, 0.35f };
        float y = 0.0f;

        for (size_t f = 0; f < 3; ++f)
        {
            auto& state = c.formant[f];
            const float fg = formantG[f];
            const float hp = formantH[f] * (x - state[0] * (fg + formantK) - state[1]);
            const float bp = hp * fg + state[0];
            state[0] = hp * fg + bp;
            state[1] = bp * fg + state[1] + bp * fg;
            y += bp * formantK * gains[f];
        }

        return y * 2.2f;
    }

    float processPhaser (Channel& c, float x) const noexcept
    {
        float y = x + c.phaserFeedback * phaserAmount;

        for (auto& z : c.allPass)
        {
            const float out = allPassCoeff * y + z;
            z = y - allPassCoeff * out;
            y = out;
        }

        c.phaserFeedback = y;
        return 0.5f * (x + y);
    }

    float secondStage (Channel& c, float x, bool highPass) const noexcept
    {
        const float hp = h2 * (x - c.t1 * (g + butterworth) - c.t2);
        const float bp = hp * g + c.t1;
        c.t1 = hp * g + bp;
        const float lp = bp * g + c.t2;
        c.t2 = bp * g + lp;
        return highPass ? hp : lp;
    }

    float processComb (Channel& c, float x) noexcept
    {
        // The delay length glides, so a moving cutoff bends the pitch of the comb without clicks.
        if (c.delayNow < 0.0f)
            c.delayNow = combDelay;
        else
            c.delayNow += (combDelay - c.delayNow) * smoothCoeff;

        const float pos = (float) c.write - c.delayNow;
        const float whole = std::floor (pos);
        const float frac = pos - whole;
        const int i0 = (int) whole & mask, i1 = (i0 + 1) & mask;
        const float delayed = c.delay[(size_t) i0] + (c.delay[(size_t) i1] - c.delay[(size_t) i0]) * frac;

        // A little damping in the loop keeps the top of the comb from getting harsh.
        c.damp += (delayed - c.damp) * 0.7f;
        const float y = x + combFeedback * c.damp;
        c.delay[(size_t) c.write] = y;
        c.write = (c.write + 1) & mask;
        return y * combGain;
    }

    static constexpr float minCombHz = 25.0f;
    static constexpr float butterworth = 1.41421356f;

    float sampleRate = 44100.0f;
    int type = filter::lowPass12, mask = 255;
    float g = 0.1f, k = 1.0f, h = 1.0f, h2 = 1.0f, bandGain = 1.0f;
    float combDelay = 100.0f, combFeedback = 0.0f, combGain = 1.0f, smoothCoeff = 0.01f;
    float formantK = 0.3f, allPassCoeff = 0.0f, phaserAmount = 0.0f;
    std::array<float, 3> formantG {}, formantH {};
    std::array<Channel, 2> channels;
};
} // namespace amv
