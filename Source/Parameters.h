#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "Arpeggiator.h"
#include "Effects.h"
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
inline constexpr auto engineLevel = "engineLevel";
inline constexpr auto oscATable = "oscATable";
inline constexpr auto oscAPos = "oscAPosition";
inline constexpr auto oscAUnison = "oscAUnison";
inline constexpr auto oscADetune = "oscADetune";
inline constexpr auto oscAPitch = "oscAPitch";
inline constexpr auto oscAFine = "oscAFine";
inline constexpr auto oscALevel = "oscALevel";
inline constexpr auto oscAWidth = "oscAWidth";
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

    addChoice (id::model, "Model", { juce::String (juce::CharPointer_UTF8 ("Ju\xc5\xbeni Vetar")), "Accordion", "Gusle", "Frula", "Kaval", "Gajde", "Zurla", "Tambura", "Tapan", "Dvojnice", "Clarinet", "Truba", "Violin" }, 0);

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

    // Wavetable oscillators, sub, noise and the mixer
    juce::StringArray tableNames;
    for (const auto* n : getWavetableNames())
        tableNames.add (n);

    add (id::engineLevel, "Engine Level", Range (0.0f, 1.0f), 1.0f);

    addChoice (id::oscATable, "Osc A Table", tableNames, 0);
    add (id::oscAPos, "Osc A Position", Range (0.0f, 1.0f), 0.66f);
    add (id::oscAUnison, "Osc A Unison", Range (1.0f, 7.0f, 1.0f), 1.0f);
    add (id::oscADetune, "Osc A Detune", Range (0.0f, 100.0f), 18.0f, "ct");
    add (id::oscAPitch, "Osc A Pitch", Range (-24.0f, 24.0f, 1.0f), 0.0f, "st");
    add (id::oscAFine, "Osc A Fine", Range (-100.0f, 100.0f), 0.0f, "ct");
    add (id::oscALevel, "Osc A Level", Range (0.0f, 1.0f), 0.0f);
    add (id::oscAWidth, "Osc A Width", Range (0.0f, 1.0f), 0.7f);

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

    for (int i = 0; i < mod::numLfos; ++i)
    {
        const auto name = "LFO " + juce::String (i + 1) + " ";
        addChoice (id::lfo (i, "Shape").toRawUTF8(), (name + "Shape").toRawUTF8(), lfoShapes, 0);
        add (id::lfo (i, "Rate").toRawUTF8(), (name + "Rate").toRawUTF8(), skewed (0.02f, 40.0f, 3.0f), i == 0 ? 5.0f : 0.5f, "Hz");
        addBool (id::lfo (i, "Sync").toRawUTF8(), (name + "Sync").toRawUTF8(), false);
        addChoice (id::lfo (i, "Division").toRawUTF8(), (name + "Division").toRawUTF8(), lfoDivisions, i == 0 ? 7 : 2);
    }

    for (int i = 0; i < mod::numMacros; ++i)
        add (id::macro (i).toRawUTF8(), ("Macro " + juce::String (i + 1)).toRawUTF8(), Range (0.0f, 1.0f), 0.0f);

    for (int i = 0; i < mod::numSlots; ++i)
    {
        const auto name = "Mod " + juce::String (i + 1) + " ";
        addChoice (id::slot (i, "Source").toRawUTF8(), (name + "Source").toRawUTF8(), modSources, 0);
        addChoice (id::slot (i, "Dest").toRawUTF8(), (name + "Destination").toRawUTF8(), modDests, i == 0 ? 1 : 0);
        add (id::slot (i, "Amount").toRawUTF8(), (name + "Amount").toRawUTF8(), Range (-1.0f, 1.0f), 0.0f);
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
