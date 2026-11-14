#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "Tuning.h"

namespace amv
{
namespace id
{
inline constexpr auto model = "model";
inline constexpr auto tone = "tone";
inline constexpr auto sub = "sub";
inline constexpr auto accBellows = "accBellows";
inline constexpr auto accMusette = "accMusette";
inline constexpr auto accRegister = "accRegister";
inline constexpr auto accClick = "accClick";
inline constexpr auto scaleRoot = "scaleRoot";
inline constexpr auto scale = "scale";
inline constexpr auto snap = "snap";
inline constexpr auto ornGrace = "ornGrace";
inline constexpr auto ornMordent = "ornMordent";
inline constexpr auto ornTrill = "ornTrill";
inline constexpr auto ornFall = "ornFall";
inline constexpr auto ornSpeed = "ornSpeed";
inline constexpr auto ornAmount = "ornAmount";
inline constexpr auto ornChance = "ornChance";
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

    auto addBool = [&layout] (const char* paramId, const char* name, bool def)
    {
        layout.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { paramId, 1 }, name, def));
    };

    auto addChoice = [&layout] (const char* paramId, const char* name, const juce::StringArray& choices, int def)
    {
        layout.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { paramId, 1 }, name, choices, def));
    };

    addChoice (id::model, "Model", { juce::String (juce::CharPointer_UTF8 ("Ju\xc5\xbeni Vetar")), "Accordion" }, 0);

    // Juzni Vetar lead
    add (id::tone, "Lead Tone", Range (0.0f, 1.0f), 0.55f);
    add (id::pulseWidth, "Lead Pulse Width", Range (0.05f, 0.95f), 0.32f);
    add (id::detune, "Lead Detune", Range (0.0f, 50.0f), 9.0f, "ct");
    add (id::sub, "Lead Sub", Range (0.0f, 1.0f), 0.0f);

    // Accordion
    add (id::accBellows, "Accordion Bellows", Range (0.0f, 1.0f), 0.75f);
    add (id::accMusette, "Accordion Musette", Range (0.0f, 40.0f), 12.0f, "ct");
    add (id::accRegister, "Accordion Register", Range (0.0f, 1.0f), 0.4f);
    add (id::accClick, "Accordion Click", Range (0.0f, 1.0f), 0.3f);

    // Scale
    juce::StringArray roots, scaleNames;
    for (const auto* n : getNoteNames())
        roots.add (n);
    for (const auto& sc : getScales())
        scaleNames.add (sc.name);

    addChoice (id::scaleRoot, "Scale Root", roots, 2);
    addChoice (id::scale, "Scale", scaleNames, 0);
    addBool (id::snap, "Scale Snap", false);

    // Ornaments
    addBool (id::ornGrace, "Grace Note", false);
    addBool (id::ornMordent, "Mordent", false);
    addBool (id::ornTrill, "Trill", false);
    addBool (id::ornFall, "Fall", false);
    add (id::ornSpeed, "Ornament Speed", Range (25.0f, 150.0f), 60.0f, "ms");
    add (id::ornAmount, "Ornament Amount", Range (0.0f, 1.0f), 0.5f);
    add (id::ornChance, "Ornament Chance", Range (0.0f, 100.0f), 100.0f, "%");

    // Performance
    add (id::vibDepth, "Vibrato", Range (0.0f, 100.0f), 28.0f, "ct");
    add (id::vibRate, "Vib Rate", Range (2.0f, 12.0f), 6.2f, "Hz");
    add (id::vibDelay, "Vib Delay", skewed (0.0f, 1500.0f, 300.0f), 220.0f, "ms");
    add (id::bendRange, "Bend", Range (1.0f, 12.0f, 1.0f), 2.0f, "st");
    add (id::glide, "Glide", skewed (0.0f, 800.0f, 120.0f), 45.0f, "ms");
    addBool (id::mono, "Mono Legato", true);

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
