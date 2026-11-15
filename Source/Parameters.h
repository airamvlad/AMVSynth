#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "Arpeggiator.h"
#include "Effects.h"
#include "Filter.h"
#include "Modulation.h"
#include "Tuning.h"
#include "Wavetables.h"

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
inline constexpr auto gusBow = "gusBow";
inline constexpr auto gusPosition = "gusPosition";
inline constexpr auto gusRasp = "gusRasp";
inline constexpr auto gusNasal = "gusNasal";
inline constexpr auto fruBreath = "fruBreath";
inline constexpr auto fruOverblow = "fruOverblow";
inline constexpr auto fruNoise = "fruNoise";
inline constexpr auto fruChiff = "fruChiff";
inline constexpr auto kavBreath = "kavBreath";
inline constexpr auto kavOverblow = "kavOverblow";
inline constexpr auto kavNoise = "kavNoise";
inline constexpr auto kavThroat = "kavThroat";
inline constexpr auto gajDrone = "gajDrone";
inline constexpr auto gajDroneTune = "gajDroneTune";
inline constexpr auto gajBag = "gajBag";
inline constexpr auto gajEdge = "gajEdge";
inline constexpr auto zurPressure = "zurPressure";
inline constexpr auto zurBite = "zurBite";
inline constexpr auto zurGrowl = "zurGrowl";
inline constexpr auto zurNasal = "zurNasal";
inline constexpr auto tamPluck = "tamPluck";
inline constexpr auto tamDecay = "tamDecay";
inline constexpr auto tamDouble = "tamDouble";
inline constexpr auto tamTone = "tamTone";
inline constexpr auto tapThump = "tapThump";
inline constexpr auto tapSnap = "tapSnap";
inline constexpr auto tapDecay = "tapDecay";
inline constexpr auto tapRing = "tapRing";
inline constexpr auto dvoBreath = "dvoBreath";
inline constexpr auto dvoSecond = "dvoSecond";
inline constexpr auto dvoInterval = "dvoInterval";
inline constexpr auto dvoBeat = "dvoBeat";
inline constexpr auto claBreath = "claBreath";
inline constexpr auto claReed = "claReed";
inline constexpr auto claGrowl = "claGrowl";
inline constexpr auto claBody = "claBody";
inline constexpr auto truBlow = "truBlow";
inline constexpr auto truBlare = "truBlare";
inline constexpr auto truRasp = "truRasp";
inline constexpr auto truMute = "truMute";
inline constexpr auto vioBow = "vioBow";
inline constexpr auto vioPosition = "vioPosition";
inline constexpr auto vioScratch = "vioScratch";
inline constexpr auto vioBody = "vioBody";
inline constexpr auto sarPluck = "sarPluck";
inline constexpr auto sarDecay = "sarDecay";
inline constexpr auto sarDrone = "sarDrone";
inline constexpr auto sarTone = "sarTone";
inline constexpr auto kanPluck = "kanPluck";
inline constexpr auto kanDecay = "kanDecay";
inline constexpr auto kanShimmer = "kanShimmer";
inline constexpr auto kanTremolo = "kanTremolo";
inline constexpr auto darStrike = "darStrike";
inline constexpr auto darSnap = "darSnap";
inline constexpr auto darDecay = "darDecay";
inline constexpr auto darRing = "darRing";
inline constexpr auto voxVowel = "voxVowel";
inline constexpr auto voxBreath = "voxBreath";
inline constexpr auto voxSize = "voxSize";
inline constexpr auto voxTension = "voxTension";
inline constexpr auto arpOn = "arpOn";
inline constexpr auto arpMeter = "arpMeter";
inline constexpr auto arpRate = "arpRate";
inline constexpr auto arpMode = "arpMode";
inline constexpr auto arpOctaves = "arpOctaves";
inline constexpr auto arpGate = "arpGate";
inline constexpr auto arpAccent = "arpAccent";
inline constexpr auto arpPolymeter = "arpPolymeter";
inline constexpr auto distOn = "distOn";
inline constexpr auto distType = "distType";
inline constexpr auto distDrive = "distDrive";
inline constexpr auto distMix = "distMix";
inline constexpr auto gateOn = "gateOn";
inline constexpr auto gateRate = "gateRate";
inline constexpr auto gatePattern = "gatePattern";
inline constexpr auto gateDepth = "gateDepth";
inline constexpr auto gateLength = "gateLength";
inline constexpr auto gateSmooth = "gateSmooth";
inline constexpr auto delayOn = "delayOn";
inline constexpr auto delayTime = "delayTime";
inline constexpr auto delayFeedback = "delayFeedback";
inline constexpr auto delayTone = "delayTone";
inline constexpr auto delayMix = "delayMix";
inline constexpr auto delayPingPong = "delayPingPong";
inline constexpr auto reverbOn = "reverbOn";
inline constexpr auto reverbSize = "reverbSize";
inline constexpr auto reverbDamp = "reverbDamp";
inline constexpr auto reverbMix = "reverbMix";
inline constexpr auto duckOn = "duckOn";
inline constexpr auto duckDepth = "duckDepth";
inline constexpr auto duckRelease = "duckRelease";
inline constexpr auto fxOrder = "fxOrder";
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
inline constexpr auto tuneA4 = "tuneA4";
inline constexpr auto mpe = "mpe";
inline constexpr auto mpeBend = "mpeBend";
inline constexpr auto filterType = "filterType";
inline constexpr auto filter2On = "filter2On";
inline constexpr auto filter2Type = "filter2Type";
inline constexpr auto filter2Cutoff = "filter2Cutoff";
inline constexpr auto filter2Res = "filter2Resonance";
inline constexpr auto filter2Env = "filter2Env";
inline constexpr auto filterRouting = "filterRouting";
inline constexpr auto routeEngine = "routeEngine";
inline constexpr auto routeOscA = "routeOscA";
inline constexpr auto routeOscB = "routeOscB";
inline constexpr auto routeSub = "routeSub";
inline constexpr auto routeNoise = "routeNoise";
inline constexpr auto routeSampler = "routeSampler";
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
inline constexpr auto engineLevel = "engineLevel";
inline constexpr auto oscATable = "oscATable";
inline constexpr auto oscAPos = "oscAPosition";
inline constexpr auto oscAUnison = "oscAUnison";
inline constexpr auto oscADetune = "oscADetune";
inline constexpr auto oscAPitch = "oscAPitch";
inline constexpr auto oscAFine = "oscAFine";
inline constexpr auto oscALevel = "oscALevel";
inline constexpr auto oscAWidth = "oscAWidth";
inline constexpr auto env3A = "env3Attack";
inline constexpr auto env3D = "env3Decay";
inline constexpr auto env3S = "env3Sustain";
inline constexpr auto env3R = "env3Release";
inline constexpr auto smpLevel = "smpLevel";
inline constexpr auto smpMode = "smpMode";
inline constexpr auto smpPitch = "smpPitch";
inline constexpr auto smpFine = "smpFine";
inline constexpr auto smpRoot = "smpRoot";
inline constexpr auto smpKeyTrack = "smpKeyTrack";
inline constexpr auto smpPan = "smpPan";
inline constexpr auto smpReverse = "smpReverse";
inline constexpr auto smpStart = "smpStart";
inline constexpr auto smpEnd = "smpEnd";
inline constexpr auto smpLoop = "smpLoop";
inline constexpr auto smpLoopStart = "smpLoopStart";
inline constexpr auto smpLoopEnd = "smpLoopEnd";
inline constexpr auto smpCrossfade = "smpCrossfade";
inline constexpr auto smpPosition = "smpPosition";
inline constexpr auto smpGrainSize = "smpGrainSize";
inline constexpr auto smpDensity = "smpDensity";
inline constexpr auto smpSpray = "smpSpray";
inline constexpr auto smpPitchSpray = "smpPitchSpray";
inline constexpr auto smpScan = "smpScan";
inline constexpr auto smpShape = "smpShape";
inline constexpr auto smpSpread = "smpSpread";
inline constexpr auto smpRevChance = "smpRevChance";
inline constexpr auto oscAWarp = "oscAWarp";
inline constexpr auto oscAWarpAmount = "oscAWarpAmount";
inline constexpr auto oscBWarp = "oscBWarp";
inline constexpr auto oscBWarpAmount = "oscBWarpAmount";
inline constexpr auto oscBTable = "oscBTable";
inline constexpr auto oscBPos = "oscBPosition";
inline constexpr auto oscBUnison = "oscBUnison";
inline constexpr auto oscBDetune = "oscBDetune";
inline constexpr auto oscBPitch = "oscBPitch";
inline constexpr auto oscBFine = "oscBFine";
inline constexpr auto oscBLevel = "oscBLevel";
inline constexpr auto oscBWidth = "oscBWidth";
inline constexpr auto subLevel = "subLevel";
inline constexpr auto subOctave = "subOctave";
inline constexpr auto subShape = "subShape";
inline constexpr auto noiseLevel = "noiseLevel";
inline constexpr auto noiseColor = "noiseColor";

// Modulation parameters are numbered from 1: lfo1Rate, macro3, mod2Amount and so on.
inline juce::String lfo (int index, const char* suffix) { return "lfo" + juce::String (index + 1) + suffix; }
inline juce::String macro (int index) { return "macro" + juce::String (index + 1); }
inline juce::String slot (int index, const char* suffix) { return "mod" + juce::String (index + 1) + suffix; }
} // namespace id

/** The four controls of every Balkan model, in the order of the model list. */
inline const std::array<std::array<const char*, 4>, 17>& getEngineKnobIds()
{
    static const std::array<std::array<const char*, 4>, 17> ids { {
        { id::tone, id::pulseWidth, id::detune, id::sub },
        { id::accBellows, id::accMusette, id::accRegister, id::accClick },
        { id::gusBow, id::gusPosition, id::gusRasp, id::gusNasal },
        { id::fruBreath, id::fruOverblow, id::fruNoise, id::fruChiff },
        { id::kavBreath, id::kavOverblow, id::kavNoise, id::kavThroat },
        { id::gajDrone, id::gajDroneTune, id::gajBag, id::gajEdge },
        { id::zurPressure, id::zurBite, id::zurGrowl, id::zurNasal },
        { id::tamPluck, id::tamDecay, id::tamDouble, id::tamTone },
        { id::tapThump, id::tapSnap, id::tapDecay, id::tapRing },
        { id::dvoBreath, id::dvoSecond, id::dvoInterval, id::dvoBeat },
        { id::claBreath, id::claReed, id::claGrowl, id::claBody },
        { id::truBlow, id::truBlare, id::truRasp, id::truMute },
        { id::vioBow, id::vioPosition, id::vioScratch, id::vioBody },
        { id::sarPluck, id::sarDecay, id::sarDrone, id::sarTone },
        { id::kanPluck, id::kanDecay, id::kanShimmer, id::kanTremolo },
        { id::darStrike, id::darSnap, id::darDecay, id::darRing },
        { id::voxVowel, id::voxBreath, id::voxSize, id::voxTension },
    } };
    return ids;
}

/** The modulation destination a parameter belongs to, or -1 when it cannot be modulated. */
inline int modDestFor (const juce::String& paramId)
{
    static const std::pair<const char*, int> direct[] {
        { id::cutoff, mod::dstCutoff }, { id::resonance, mod::dstResonance }, { id::filterEnv, mod::dstFilterEnv },
        { id::oscAPos, mod::dstOscAPosition }, { id::oscBPos, mod::dstOscBPosition },
        { id::oscALevel, mod::dstOscALevel }, { id::oscBLevel, mod::dstOscBLevel },
        { id::oscADetune, mod::dstOscADetune }, { id::oscBDetune, mod::dstOscBDetune },
        { id::oscAPitch, mod::dstOscAPitch }, { id::oscBPitch, mod::dstOscBPitch },
        { id::oscAWarpAmount, mod::dstOscAWarp }, { id::oscBWarpAmount, mod::dstOscBWarp },
        { id::smpLevel, mod::dstSamplerLevel }, { id::smpPitch, mod::dstSamplerPitch }, { id::smpPosition, mod::dstGrainPosition },
        { id::smpGrainSize, mod::dstGrainSize }, { id::smpDensity, mod::dstGrainDensity }, { id::smpSpray, mod::dstGrainSpray },
        { id::smpScan, mod::dstGrainScan },
        { id::ampA, mod::dstAmpAttack }, { id::ampD, mod::dstAmpDecay }, { id::ampR, mod::dstAmpRelease },
        { id::fltA, mod::dstFltAttack }, { id::fltD, mod::dstFltDecay }, { id::fltR, mod::dstFltRelease },
        { id::distDrive, mod::dstDistDrive }, { id::distMix, mod::dstDistMix }, { id::gateDepth, mod::dstGateDepth },
        { id::delayFeedback, mod::dstDelayFeedback }, { id::delayTone, mod::dstDelayTone }, { id::delayMix, mod::dstDelayMix },
        { id::reverbSize, mod::dstReverbSize }, { id::reverbMix, mod::dstReverbMix },
        { id::filter2Cutoff, mod::dstCutoff2 }, { id::filter2Res, mod::dstResonance2 },
        { id::engineLevel, mod::dstEngineLevel }, { id::noiseLevel, mod::dstNoiseLevel }, { id::subLevel, mod::dstSubLevel },
        { id::vibDepth, mod::dstVibrato }, { id::vibRate, mod::dstVibRate }, { id::level, mod::dstVolume },
    };

    for (const auto& [name, dest] : direct)
        if (paramId == name)
            return dest;

    for (const auto& model : getEngineKnobIds())
        for (size_t i = 0; i < model.size(); ++i)
            if (paramId == model[i])
                return mod::dstEngine1 + (int) i;

    return -1;
}

/** Settings of the project rather than of a sound; loading a preset does not change them. */
inline bool isGlobalParameter (const juce::String& paramId)
{
    return paramId == id::tuneA4 || paramId == id::mpe || paramId == id::mpeBend;
}

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

    addChoice (id::model, "Model", { juce::String (juce::CharPointer_UTF8 ("Ju\xc5\xbeni Vetar")), "Accordion", "Gusle", "Frula", "Kaval", "Gajde", "Zurla", "Tambura", "Tapan", "Dvojnice", "Clarinet", "Truba", "Violin",
                                   juce::String (juce::CharPointer_UTF8 ("\xc5\xa0""argija")), "Kanun", "Darbuka", "Voice" }, 0);

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

    // Gusle
    add (id::gusBow, "Gusle Bow", Range (0.0f, 1.0f), 0.7f);
    add (id::gusPosition, "Gusle Position", Range (0.0f, 1.0f), 0.4f);
    add (id::gusRasp, "Gusle Rasp", Range (0.0f, 1.0f), 0.35f);
    add (id::gusNasal, "Gusle Nasal", Range (0.0f, 1.0f), 0.6f);

    // Frula
    add (id::fruBreath, "Frula Breath", Range (0.0f, 1.0f), 0.7f);
    add (id::fruOverblow, "Frula Overblow", Range (0.0f, 1.0f), 0.15f);
    add (id::fruNoise, "Frula Noise", Range (0.0f, 1.0f), 0.3f);
    add (id::fruChiff, "Frula Chiff", Range (0.0f, 1.0f), 0.4f);

    // Kaval
    add (id::kavBreath, "Kaval Breath", Range (0.0f, 1.0f), 0.7f);
    add (id::kavOverblow, "Kaval Overblow", Range (0.0f, 1.0f), 0.1f);
    add (id::kavNoise, "Kaval Noise", Range (0.0f, 1.0f), 0.5f);
    add (id::kavThroat, "Kaval Throat", Range (0.0f, 1.0f), 0.3f);

    // Gajde
    add (id::gajDrone, "Gajde Drone", Range (0.0f, 1.0f), 0.6f);
    add (id::gajDroneTune, "Gajde Drone Tune", Range (-12.0f, 12.0f, 1.0f), 0.0f, "st");
    add (id::gajBag, "Gajde Bag", Range (0.0f, 1.0f), 0.8f);
    add (id::gajEdge, "Gajde Edge", Range (0.0f, 1.0f), 0.5f);

    // Zurla
    add (id::zurPressure, "Zurla Pressure", Range (0.0f, 1.0f), 0.75f);
    add (id::zurBite, "Zurla Bite", Range (0.0f, 1.0f), 0.5f);
    add (id::zurGrowl, "Zurla Growl", Range (0.0f, 1.0f), 0.2f);
    add (id::zurNasal, "Zurla Nasal", Range (0.0f, 1.0f), 0.6f);

    // Tambura
    add (id::tamPluck, "Tambura Pluck", Range (0.0f, 1.0f), 0.65f);
    add (id::tamDecay, "Tambura Decay", Range (0.0f, 1.0f), 0.55f);
    add (id::tamDouble, "Tambura Double", Range (0.0f, 1.0f), 0.4f);
    add (id::tamTone, "Tambura Tone", Range (0.0f, 1.0f), 0.6f);

    // Tapan
    add (id::tapThump, "Tapan Thump", Range (0.0f, 1.0f), 0.5f);
    add (id::tapSnap, "Tapan Snap", Range (0.0f, 1.0f), 0.5f);
    add (id::tapDecay, "Tapan Decay", Range (0.0f, 1.0f), 0.45f);
    add (id::tapRing, "Tapan Ring", Range (0.0f, 1.0f), 0.3f);

    // Dvojnice
    add (id::dvoBreath, "Dvojnice Breath", Range (0.0f, 1.0f), 0.7f);
    add (id::dvoSecond, "Dvojnice Second Pipe", Range (0.0f, 1.0f), 0.7f);
    add (id::dvoInterval, "Dvojnice Interval", Range (-12.0f, 12.0f, 1.0f), -3.0f, "st");
    add (id::dvoBeat, "Dvojnice Beating", Range (0.0f, 1.0f), 0.3f);

    // Clarinet
    add (id::claBreath, "Clarinet Breath", Range (0.0f, 1.0f), 0.65f);
    add (id::claReed, "Clarinet Reed", Range (0.0f, 1.0f), 0.3f);
    add (id::claGrowl, "Clarinet Growl", Range (0.0f, 1.0f), 0.1f);
    add (id::claBody, "Clarinet Body", Range (0.0f, 1.0f), 0.5f);

    // Truba
    add (id::truBlow, "Truba Blow", Range (0.0f, 1.0f), 0.65f);
    add (id::truBlare, "Truba Blare", Range (0.0f, 1.0f), 0.5f);
    add (id::truRasp, "Truba Rasp", Range (0.0f, 1.0f), 0.15f);
    add (id::truMute, "Truba Mute", Range (0.0f, 1.0f), 0.0f);

    // Violin
    add (id::vioBow, "Violin Bow", Range (0.0f, 1.0f), 0.7f);
    add (id::vioPosition, "Violin Position", Range (0.0f, 1.0f), 0.3f);
    add (id::vioScratch, "Violin Scratch", Range (0.0f, 1.0f), 0.2f);
    add (id::vioBody, "Violin Body", Range (0.0f, 1.0f), 0.6f);

    // Sargija
    add (id::sarPluck, "Sargija Pluck", Range (0.0f, 1.0f), 0.75f);
    add (id::sarDecay, "Sargija Decay", Range (0.0f, 1.0f), 0.5f);
    add (id::sarDrone, "Sargija Drone", Range (0.0f, 1.0f), 0.5f);
    add (id::sarTone, "Sargija Tone", Range (0.0f, 1.0f), 0.7f);

    // Kanun
    add (id::kanPluck, "Kanun Pluck", Range (0.0f, 1.0f), 0.7f);
    add (id::kanDecay, "Kanun Decay", Range (0.0f, 1.0f), 0.6f);
    add (id::kanShimmer, "Kanun Shimmer", Range (0.0f, 1.0f), 0.4f);
    add (id::kanTremolo, "Kanun Tremolo", Range (0.0f, 1.0f), 0.0f);

    // Darbuka
    add (id::darStrike, "Darbuka Strike", Range (0.0f, 1.0f), 0.2f);
    add (id::darSnap, "Darbuka Snap", Range (0.0f, 1.0f), 0.5f);
    add (id::darDecay, "Darbuka Decay", Range (0.0f, 1.0f), 0.5f);
    add (id::darRing, "Darbuka Ring", Range (0.0f, 1.0f), 0.3f);

    // Voice
    add (id::voxVowel, "Voice Vowel", Range (0.0f, 1.0f), 0.0f);
    add (id::voxBreath, "Voice Breath", Range (0.0f, 1.0f), 0.2f);
    add (id::voxSize, "Voice Size", Range (0.0f, 1.0f), 0.5f);
    add (id::voxTension, "Voice Tension", Range (0.0f, 1.0f), 0.5f);

    // Wavetable oscillators, sub, noise and the mixer
    juce::StringArray tableNames;
    for (const auto* n : getWavetableNames())
        tableNames.add (n);
    tableNames.add ("User"); // the wavetable imported for that oscillator

    add (id::engineLevel, "Engine Level", Range (0.0f, 1.0f), 1.0f);

    addChoice (id::oscATable, "Osc A Table", tableNames, 0);
    add (id::oscAPos, "Osc A Position", Range (0.0f, 1.0f), 0.66f);
    add (id::oscAUnison, "Osc A Unison", Range (1.0f, 7.0f, 1.0f), 1.0f);
    add (id::oscADetune, "Osc A Detune", Range (0.0f, 100.0f), 18.0f, "ct");
    add (id::oscAPitch, "Osc A Pitch", Range (-24.0f, 24.0f, 1.0f), 0.0f, "st");
    add (id::oscAFine, "Osc A Fine", Range (-100.0f, 100.0f), 0.0f, "ct");
    add (id::oscALevel, "Osc A Level", Range (0.0f, 1.0f), 0.0f);
    add (id::oscAWidth, "Osc A Width", Range (0.0f, 1.0f), 0.7f);

    juce::StringArray warpModes;
    for (const auto* n : getWarpModes())
        warpModes.add (n);
    addChoice (id::oscAWarp, "Osc A Warp Mode", warpModes, 0);
    add (id::oscAWarpAmount, "Osc A Warp", Range (0.0f, 1.0f), 0.0f);
    addChoice (id::oscBWarp, "Osc B Warp Mode", warpModes, 0);
    add (id::oscBWarpAmount, "Osc B Warp", Range (0.0f, 1.0f), 0.0f);

    // Sampler
    add (id::smpLevel, "Sampler Level", Range (0.0f, 1.0f), 0.0f);
    addChoice (id::smpMode, "Sampler Mode", { "Classic", "Granular" }, 0);
    add (id::smpPitch, "Sampler Pitch", Range (-24.0f, 24.0f, 1.0f), 0.0f, "st");
    add (id::smpFine, "Sampler Fine", Range (-100.0f, 100.0f), 0.0f, "ct");
    layout.add (std::make_unique<Float> (juce::ParameterID { id::smpRoot, 1 }, "Sampler Root Note", Range (0.0f, 127.0f, 1.0f), 60.0f,
                                         juce::AudioParameterFloatAttributes()
                                             .withStringFromValueFunction ([] (float v, int) { return juce::MidiMessage::getMidiNoteName (juce::roundToInt (v), true, true, 3); })));
    addBool (id::smpKeyTrack, "Sampler Key Track", true);
    add (id::smpPan, "Sampler Pan", Range (-1.0f, 1.0f), 0.0f);
    addBool (id::smpReverse, "Sampler Reverse", false);
    add (id::smpStart, "Sampler Start", Range (0.0f, 1.0f), 0.0f);
    add (id::smpEnd, "Sampler End", Range (0.0f, 1.0f), 1.0f);
    addChoice (id::smpLoop, "Sampler Loop", { "Loop Off", "Loop", "Ping Pong" }, 0);
    add (id::smpLoopStart, "Sampler Loop Start", Range (0.0f, 1.0f), 0.0f);
    add (id::smpLoopEnd, "Sampler Loop End", Range (0.0f, 1.0f), 1.0f);
    add (id::smpCrossfade, "Sampler Loop Crossfade", Range (0.0f, 1.0f), 0.0f);
    add (id::smpPosition, "Grain Position", Range (0.0f, 1.0f), 0.0f);
    add (id::smpGrainSize, "Grain Size", skewed (5.0f, 500.0f, 80.0f), 80.0f, "ms");
    add (id::smpDensity, "Grain Density", skewed (1.0f, 100.0f, 20.0f), 20.0f, "/s");
    add (id::smpSpray, "Grain Spray", Range (0.0f, 1.0f), 0.1f);
    add (id::smpPitchSpray, "Grain Pitch Spray", skewed (0.0f, 12.0f, 2.0f), 0.0f, "st");
    add (id::smpScan, "Grain Scan", Range (-2.0f, 2.0f), 0.0f);
    add (id::smpShape, "Grain Shape", Range (0.0f, 1.0f), 0.5f);
    add (id::smpSpread, "Grain Spread", Range (0.0f, 1.0f), 0.5f);
    add (id::smpRevChance, "Grain Reverse", Range (0.0f, 1.0f), 0.0f);

    addChoice (id::oscBTable, "Osc B Table", tableNames, 1);
    add (id::oscBPos, "Osc B Position", Range (0.0f, 1.0f), 0.4f);
    add (id::oscBUnison, "Osc B Unison", Range (1.0f, 7.0f, 1.0f), 1.0f);
    add (id::oscBDetune, "Osc B Detune", Range (0.0f, 100.0f), 18.0f, "ct");
    add (id::oscBPitch, "Osc B Pitch", Range (-24.0f, 24.0f, 1.0f), 0.0f, "st");
    add (id::oscBFine, "Osc B Fine", Range (-100.0f, 100.0f), 0.0f, "ct");
    add (id::oscBLevel, "Osc B Level", Range (0.0f, 1.0f), 0.0f);
    add (id::oscBWidth, "Osc B Width", Range (0.0f, 1.0f), 0.7f);

    add (id::subLevel, "Sub Level", Range (0.0f, 1.0f), 0.0f);
    addChoice (id::subOctave, "Sub Octave", { "-1 OCT", "-2 OCT" }, 0);
    add (id::subShape, "Sub Shape", Range (0.0f, 1.0f), 0.0f);
    add (id::noiseLevel, "Noise Level", Range (0.0f, 1.0f), 0.0f);
    add (id::noiseColor, "Noise Color", skewed (200.0f, 18000.0f, 2500.0f), 6000.0f, "Hz");

    // Modulation
    juce::StringArray lfoShapes, lfoDivisions, modSources, modDests;
    for (const auto* n : getLfoShapes())
        lfoShapes.add (n);
    for (const auto& d : getLfoDivisions())
        lfoDivisions.add (d.name);
    for (const auto* n : getModSources())
        modSources.add (n);
    for (const auto* n : getModDests())
        modDests.add (n);
    juce::StringArray polarities;
    for (const auto* n : getModPolarities())
        polarities.add (n);

    for (int i = 0; i < mod::numLfos; ++i)
    {
        const auto name = "LFO " + juce::String (i + 1) + " ";
        addChoice (id::lfo (i, "Shape").toRawUTF8(), (name + "Shape").toRawUTF8(), lfoShapes, 0);
        add (id::lfo (i, "Rate").toRawUTF8(), (name + "Rate").toRawUTF8(), skewed (0.02f, 40.0f, 3.0f), i == 0 ? 5.0f : 0.5f, "Hz");
        addBool (id::lfo (i, "Sync").toRawUTF8(), (name + "Sync").toRawUTF8(), false);
        addChoice (id::lfo (i, "Division").toRawUTF8(), (name + "Division").toRawUTF8(), lfoDivisions, i == 0 ? 7 : 2);
        addChoice (id::lfo (i, "Mode").toRawUTF8(), (name + "Mode").toRawUTF8(), { "Free", "Retrigger", "One Shot" }, 0);
    }

    for (int i = 0; i < mod::numMacros; ++i)
        add (id::macro (i).toRawUTF8(), ("Macro " + juce::String (i + 1)).toRawUTF8(), Range (0.0f, 1.0f), 0.0f);

    for (int i = 0; i < mod::numSlots; ++i)
    {
        const auto name = "Mod " + juce::String (i + 1) + " ";
        addChoice (id::slot (i, "Source").toRawUTF8(), (name + "Source").toRawUTF8(), modSources, 0);
        addChoice (id::slot (i, "Dest").toRawUTF8(), (name + "Destination").toRawUTF8(), modDests, i == 0 ? 1 : 0);
        add (id::slot (i, "Amount").toRawUTF8(), (name + "Amount").toRawUTF8(), Range (-1.0f, 1.0f), 0.0f);
        addChoice (id::slot (i, "Polarity").toRawUTF8(), (name + "Polarity").toRawUTF8(), polarities, 0);
    }

    // Arpeggiator
    juce::StringArray meterNames, rateNames, modeNames;
    for (const auto& m : getMeters())
        meterNames.add (m.name);
    for (const auto* r : getArpRates())
        rateNames.add (r);
    for (const auto* m : getArpModes())
        modeNames.add (m);

    addBool (id::arpOn, "Arp On", false);
    addChoice (id::arpMeter, "Arp Meter", meterNames, 2);
    addChoice (id::arpRate, "Arp Rate", rateNames, 1);
    addChoice (id::arpMode, "Arp Mode", modeNames, 0);
    add (id::arpOctaves, "Arp Octaves", Range (1.0f, 3.0f, 1.0f), 1.0f);
    add (id::arpGate, "Arp Gate", Range (10.0f, 150.0f), 60.0f, "%");
    add (id::arpAccent, "Arp Accent", Range (0.0f, 100.0f), 60.0f, "%");
    addBool (id::arpPolymeter, "Arp Polymeter", true);

    // Effects
    juce::StringArray distTypes, gatePatterns, delayTimes;
    for (const auto* t : getDistortionTypes())
        distTypes.add (t);
    for (const auto& g : getGatePatterns())
        gatePatterns.add (g.name);
    for (const auto& d : getDelayTimes())
        delayTimes.add (d.name);

    addBool (id::distOn, "Distortion On", false);
    addChoice (id::distType, "Distortion Type", distTypes, 0);
    add (id::distDrive, "Distortion Drive", Range (0.0f, 36.0f), 12.0f, "dB");
    add (id::distMix, "Distortion Mix", Range (0.0f, 100.0f), 100.0f, "%");

    addBool (id::gateOn, "Gate On", false);
    addChoice (id::gateRate, "Gate Rate", rateNames, 1);
    addChoice (id::gatePattern, "Gate Pattern", gatePatterns, 0);
    add (id::gateDepth, "Gate Depth", Range (0.0f, 100.0f), 100.0f, "%");
    add (id::gateLength, "Gate Length", Range (5.0f, 100.0f), 50.0f, "%");
    add (id::gateSmooth, "Gate Smooth", Range (0.5f, 40.0f), 4.0f, "ms");

    addBool (id::delayOn, "Delay On", false);
    addChoice (id::delayTime, "Delay Time", delayTimes, 2);
    add (id::delayFeedback, "Delay Feedback", Range (0.0f, 95.0f), 45.0f, "%");
    add (id::delayTone, "Delay Tone", skewed (300.0f, 16000.0f, 3000.0f), 4000.0f, "Hz");
    add (id::delayMix, "Delay Mix", Range (0.0f, 100.0f), 30.0f, "%");
    addBool (id::delayPingPong, "Delay Ping-Pong", true);

    addBool (id::reverbOn, "Reverb On", false);
    add (id::reverbSize, "Reverb Size", Range (0.0f, 1.0f), 0.6f);
    add (id::reverbDamp, "Reverb Damp", Range (0.0f, 1.0f), 0.4f);
    add (id::reverbMix, "Reverb Mix", Range (0.0f, 100.0f), 25.0f, "%");

    juce::StringArray fxOrders;
    for (const auto& o : getFxOrders())
        fxOrders.add (o.name);
    addChoice (id::fxOrder, "Effect Order", fxOrders, 0);

    addBool (id::duckOn, "Kick Duck On", false);
    add (id::duckDepth, "Kick Duck Depth", Range (0.0f, 100.0f), 80.0f, "%");
    add (id::duckRelease, "Kick Duck Release", Range (5.0f, 100.0f), 50.0f, "%");

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

    // Tuning and MPE. These belong to the project, not to a sound: presets leave them alone.
    add (id::tuneA4, "A4 Tuning", Range (400.0f, 480.0f, 0.1f), 440.0f, "Hz");
    addBool (id::mpe, "MPE", true);
    add (id::mpeBend, "MPE Bend", Range (1.0f, 96.0f, 1.0f), 48.0f, "st");

    // Filter
    juce::StringArray filterTypes;
    for (const auto* n : getFilterTypes())
        filterTypes.add (n);
    addChoice (id::filterType, "Filter Type", filterTypes, 0);

    // Filter 2
    addBool (id::filter2On, "Filter 2 On", false);
    addChoice (id::filter2Type, "Filter 2 Type", filterTypes, 3);
    add (id::filter2Cutoff, "Filter 2 Cutoff", skewed (30.0f, 18000.0f, 1200.0f), 200.0f, "Hz");
    add (id::filter2Res, "Filter 2 Resonance", Range (0.0f, 1.0f), 0.2f);
    add (id::filter2Env, "Filter 2 Env Amount", Range (-1.0f, 1.0f), 0.0f);
    addChoice (id::filterRouting, "Filter Routing", { "Serial", "Parallel" }, 0);
    addChoice (id::routeEngine, "Engine Filter", { "Filter 1", "Filter 2" }, 0);
    addChoice (id::routeOscA, "Osc A Filter", { "Filter 1", "Filter 2" }, 0);
    addChoice (id::routeOscB, "Osc B Filter", { "Filter 1", "Filter 2" }, 0);
    addChoice (id::routeSub, "Sub Filter", { "Filter 1", "Filter 2" }, 0);
    addChoice (id::routeNoise, "Noise Filter", { "Filter 1", "Filter 2" }, 0);
    addChoice (id::routeSampler, "Sampler Filter", { "Filter 1", "Filter 2" }, 0);
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

    add (id::env3A, "Env 3 Attack", skewed (0.5f, 4000.0f, 200.0f), 10.0f, "ms");
    add (id::env3D, "Env 3 Decay", skewed (1.0f, 8000.0f, 500.0f), 300.0f, "ms");
    add (id::env3S, "Env 3 Sustain", Range (0.0f, 1.0f), 0.0f);
    add (id::env3R, "Env 3 Release", skewed (1.0f, 8000.0f, 500.0f), 300.0f, "ms");

    add (id::level, "Level", Range (-48.0f, 6.0f), -9.0f, "dB");

    return layout;
}
} // namespace amv
