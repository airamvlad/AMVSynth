#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace amv
{
namespace id
{
inline constexpr auto tone = "tone";
inline constexpr auto pulseWidth = "pulseWidth";
inline constexpr auto detune = "detune";
inline constexpr auto vibDepth = "vibDepth";
inline constexpr auto vibRate = "vibRate";
inline constexpr auto vibDelay = "vibDelay";
inline constexpr auto bendRange = "bendRange";
inline constexpr auto glide = "glide";
inline constexpr auto mono = "mono";
inline constexpr auto cutoff = "cutoff";
inline constexpr auto resonance = "resonance";
inline constexpr auto filterEnv = "filterEnv";
inline constexpr auto ampA = "ampAttack";
inline constexpr auto ampD = "ampDecay";
inline constexpr auto ampS = "ampSustain";
inline constexpr auto ampR = "ampRelease";
inline constexpr auto fltA = "fltAttack";
inline constexpr auto fltD = "fltDecay";
inline constexpr auto fltS = "fltSustain";
inline constexpr auto fltR = "fltRelease";
inline constexpr auto level = "level";
} // namespace id

inline juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    using Range = juce::NormalisableRange<float>;
    using Float = juce::AudioParameterFloat;
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    auto add = [&layout] (const char* paramId, const char* name, Range range, float def, const char* unit = "")
    {
        layout.add (std::make_unique<Float> (juce::ParameterID { paramId, 1 }, name, range, def,
                                             juce::AudioParameterFloatAttributes().withLabel (unit)));
    };

    auto skewed = [] (float lo, float hi, float centre)
    {
        Range r (lo, hi);
        r.setSkewForCentre (centre);
        return r;
    };

    // Lead voice
    add (id::tone, "Tone", Range (0.0f, 1.0f), 0.55f);
    add (id::pulseWidth, "Pulse Width", Range (0.05f, 0.95f), 0.32f);
    add (id::detune, "Detune", Range (0.0f, 50.0f), 9.0f, "ct");
    add (id::vibDepth, "Vibrato", Range (0.0f, 100.0f), 28.0f, "ct");
    add (id::vibRate, "Vib Rate", Range (2.0f, 12.0f), 6.2f, "Hz");
    add (id::vibDelay, "Vib Delay", skewed (0.0f, 1500.0f, 300.0f), 220.0f, "ms");
    add (id::bendRange, "Bend", Range (1.0f, 12.0f, 1.0f), 2.0f, "st");
    add (id::glide, "Glide", skewed (0.0f, 800.0f, 120.0f), 45.0f, "ms");
    layout.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { id::mono, 1 }, "Mono Legato", true));

    // Filter
    add (id::cutoff, "Cutoff", skewed (30.0f, 18000.0f, 1200.0f), 3200.0f, "Hz");
    add (id::resonance, "Resonance", Range (0.0f, 1.0f), 0.25f);
    add (id::filterEnv, "Filter Env", Range (-1.0f, 1.0f), 0.35f);

    // Envelopes
    add (id::ampA, "Amp Attack", skewed (0.5f, 4000.0f, 200.0f), 4.0f, "ms");
    add (id::ampD, "Amp Decay", skewed (1.0f, 6000.0f, 400.0f), 250.0f, "ms");
    add (id::ampS, "Amp Sustain", Range (0.0f, 1.0f), 0.8f);
    add (id::ampR, "Amp Release", skewed (1.0f, 8000.0f, 500.0f), 160.0f, "ms");
    add (id::fltA, "Filter Attack", skewed (0.5f, 4000.0f, 200.0f), 2.0f, "ms");
    add (id::fltD, "Filter Decay", skewed (1.0f, 6000.0f, 400.0f), 320.0f, "ms");
    add (id::fltS, "Filter Sustain", Range (0.0f, 1.0f), 0.4f);
    add (id::fltR, "Filter Release", skewed (1.0f, 8000.0f, 500.0f), 200.0f, "ms");

    add (id::level, "Level", Range (-48.0f, 6.0f), -9.0f, "dB");

    return layout;
}
} // namespace amv
