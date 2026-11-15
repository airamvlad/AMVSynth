#pragma once

#include "Parameters.h"

#include <utility>
#include <vector>

namespace amv
{
/** A factory preset: every parameter starts at its default, then the listed ones are set.
    Values are in the units the parameter shows (ms, Hz, %, choice index, 0 or 1 for switches). */
struct Preset
{
    const char* name;
    std::vector<std::pair<juce::String, float>> values;
};

inline const std::vector<Preset>& getFactoryPresets()
{
    using namespace id;

    // Choice indices used below.
    enum { juzni = 0, accordion, gusle, frula, kaval, gajde, zurlaModel, tambura, tapan, dvojnice, clarinet, truba, violin, sargija, kanun, darbuka, voice };
    enum { chromatic = 0, hijaz, hijazkar, nikriz, harmonicMinor, kurdi, rast, ussak, huseyni, saba, naturalMinor, majorScale };
    enum { m44 = 0, m58, m78a, m78b, m98, m118 };
    enum { basic = 0, acid, pwm, vowels, kavalTable, zurla, reeds, bells };
    enum { lfo1 = 1, lfo2, fltEnv, velocity, wheel, macro1, macro2 };
    enum { toPitch = 0, toCutoff, toRes, toPosA, toPosB, toLevelA, toLevelB, toEngine, toNoise, toVolume };

    static const std::vector<Preset> presets {
        { "Init", {} },

        { "Juzni Vetar Lead", {
            { level, -12.0f },
            { model, juzni }, { tone, 0.55f }, { detune, 11.0f }, { sub, 0.15f },
            { scale, hijaz }, { snap, 1 }, { ornGrace, 1 }, { ornMordent, 1 }, { ornChance, 60.0f },
            { glide, 60.0f }, { vibDepth, 32.0f }, { cutoff, 4200.0f }, { resonance, 0.3f },
            { delayOn, 1 }, { delayMix, 22.0f }, { reverbOn, 1 }, { reverbMix, 18.0f } } },

        { "Vetar Trill Solo", {
            { level, -13.0f },
            { model, juzni }, { tone, 0.8f }, { pulseWidth, 0.24f }, { detune, 6.0f },
            { scale, hijazkar }, { snap, 1 }, { ornTrill, 1 }, { ornFall, 1 }, { ornSpeed, 48.0f }, { ornAmount, 0.6f }, { ornChance, 45.0f },
            { glide, 90.0f }, { bendRange, 3.0f }, { cutoff, 5200.0f }, { filterEnv, 0.2f },
            { distOn, 1 }, { distDrive, 8.0f }, { distMix, 50.0f }, { delayOn, 1 }, { delayTime, 2 }, { delayMix, 28.0f } } },

        { "Harmonika Kolo", {
            { level, -16.0f },
            { model, accordion }, { accBellows, 0.8f }, { accMusette, 14.0f }, { accRegister, 0.45f }, { accClick, 0.4f },
            { mono, 0 }, { vibDepth, 0.0f }, { glide, 0.0f }, { cutoff, 9000.0f }, { filterEnv, 0.0f },
            { ampA, 12.0f }, { ampS, 0.95f }, { ampR, 90.0f }, { ornGrace, 1 }, { ornChance, 35.0f },
            { reverbOn, 1 }, { reverbSize, 0.35f }, { reverbMix, 14.0f } } },

        { "Harmonika Full Musette", {
            { level, -18.0f },
            { model, accordion }, { accBellows, 0.9f }, { accMusette, 22.0f }, { accRegister, 1.0f }, { accClick, 0.25f },
            { mono, 0 }, { vibDepth, 0.0f }, { glide, 0.0f }, { cutoff, 12000.0f }, { filterEnv, 0.0f },
            { ampA, 20.0f }, { ampS, 1.0f }, { ampR, 140.0f }, { reverbOn, 1 }, { reverbMix, 20.0f } } },

        { "Gusle Epic", {
            { model, gusle }, { gusBow, 0.75f }, { gusPosition, 0.45f }, { gusRasp, 0.45f }, { gusNasal, 0.7f },
            { scale, kurdi }, { snap, 1 }, { ornGrace, 1 }, { ornFall, 1 }, { ornChance, 50.0f },
            { glide, 110.0f }, { vibDepth, 20.0f }, { vibRate, 5.2f }, { ampA, 90.0f }, { ampR, 260.0f },
            { cutoff, 6000.0f }, { filterEnv, 0.1f }, { reverbOn, 1 }, { reverbSize, 0.8f }, { reverbMix, 30.0f } } },

        { "Frula Dance", {
            { level, -11.0f },
            { model, frula }, { fruBreath, 0.75f }, { fruOverblow, 0.2f }, { fruNoise, 0.3f }, { fruChiff, 0.55f },
            { scale, majorScale }, { snap, 1 }, { ornGrace, 1 }, { ornMordent, 1 }, { ornTrill, 1 }, { ornChance, 55.0f }, { ornSpeed, 45.0f },
            { glide, 20.0f }, { vibDepth, 14.0f }, { vibRate, 5.6f }, { ampA, 18.0f }, { cutoff, 9000.0f }, { filterEnv, 0.0f },
            { delayOn, 1 }, { delayMix, 18.0f }, { reverbOn, 1 }, { reverbMix, 22.0f } } },

        { "Kaval Mist", {
            { level, -10.0f },
            { model, kaval }, { kavBreath, 0.65f }, { kavOverblow, 0.1f }, { kavNoise, 0.65f }, { kavThroat, 0.45f },
            { scale, huseyni }, { snap, 1 }, { ornGrace, 1 }, { ornFall, 1 }, { ornChance, 40.0f },
            { glide, 140.0f }, { vibDepth, 16.0f }, { vibRate, 4.8f }, { vibDelay, 400.0f }, { ampA, 70.0f }, { ampR, 500.0f },
            { cutoff, 7000.0f }, { filterEnv, 0.0f }, { delayOn, 1 }, { delayTime, 4 }, { delayFeedback, 55.0f }, { delayMix, 25.0f },
            { reverbOn, 1 }, { reverbSize, 0.9f }, { reverbMix, 40.0f } } },

        { "Gajde Drone", {
            { model, gajde }, { gajDrone, 0.7f }, { gajBag, 0.75f }, { gajEdge, 0.55f },
            { scale, hijaz }, { snap, 1 }, { ornGrace, 1 }, { ornMordent, 1 }, { ornChance, 70.0f }, { ornSpeed, 40.0f },
            { glide, 15.0f }, { vibDepth, 0.0f }, { ampA, 25.0f }, { ampS, 1.0f }, { cutoff, 8000.0f }, { filterEnv, 0.0f },
            { reverbOn, 1 }, { reverbMix, 16.0f } } },

        { "Zurla Svadba", {
            { model, zurlaModel }, { zurPressure, 0.8f }, { zurBite, 0.55f }, { zurGrowl, 0.25f }, { zurNasal, 0.65f },
            { scale, hijaz }, { snap, 1 }, { ornGrace, 1 }, { ornTrill, 1 }, { ornFall, 1 }, { ornChance, 60.0f }, { ornSpeed, 42.0f },
            { glide, 35.0f }, { vibDepth, 26.0f }, { vibRate, 6.4f }, { ampA, 20.0f }, { cutoff, 11000.0f }, { filterEnv, 0.0f },
            { level, -12.0f }, { reverbOn, 1 }, { reverbMix, 18.0f } } },

        { "Tambura Sitna", {
            { model, tambura }, { tamPluck, 0.7f }, { tamDecay, 0.5f }, { tamDouble, 0.45f }, { tamTone, 0.65f },
            { scale, majorScale }, { snap, 1 }, { mono, 0 }, { vibDepth, 0.0f }, { glide, 0.0f },
            { cutoff, 12000.0f }, { filterEnv, 0.0f }, { ampA, 0.5f }, { ampS, 1.0f }, { ampR, 400.0f },
            { reverbOn, 1 }, { reverbSize, 0.4f }, { reverbMix, 14.0f } } },

        { "Tambura 7/8 Arp", {
            { model, tambura }, { tamPluck, 0.85f }, { tamDecay, 0.4f }, { tamDouble, 0.5f }, { tamTone, 0.75f },
            { scale, hijaz }, { snap, 1 }, { vibDepth, 0.0f }, { glide, 0.0f },
            { cutoff, 9000.0f }, { filterEnv, 0.2f }, { ampA, 0.5f }, { ampS, 1.0f }, { ampR, 200.0f },
            { arpOn, 1 }, { arpMeter, m78b }, { arpRate, 1 }, { arpMode, 0 }, { arpOctaves, 2.0f }, { arpGate, 70.0f }, { arpAccent, 70.0f },
            { delayOn, 1 }, { delayTime, 2 }, { delayMix, 26.0f }, { reverbOn, 1 }, { reverbMix, 16.0f } } },

        { "Tapan", {
            { model, tapan }, { tapThump, 0.55f }, { tapSnap, 0.5f }, { tapDecay, 0.45f }, { tapRing, 0.3f },
            { vibDepth, 0.0f }, { glide, 0.0f }, { cutoff, 14000.0f }, { filterEnv, 0.0f },
            { ampA, 0.5f }, { ampS, 1.0f }, { ampR, 300.0f }, { reverbOn, 1 }, { reverbSize, 0.3f }, { reverbMix, 10.0f } } },

        { "Dvojnice", {
            { model, dvojnice }, { dvoBreath, 0.7f }, { dvoSecond, 0.7f }, { dvoInterval, -3.0f }, { dvoBeat, 0.35f },
            { scale, majorScale }, { snap, 1 }, { ornGrace, 1 }, { ornMordent, 1 }, { ornChance, 50.0f }, { ornSpeed, 45.0f },
            { glide, 15.0f }, { vibDepth, 10.0f }, { ampA, 25.0f }, { cutoff, 9000.0f }, { filterEnv, 0.0f },
            { level, -11.0f }, { reverbOn, 1 }, { reverbMix, 22.0f } } },

        { "Klarinet Svadba", {
            { model, clarinet }, { claBreath, 0.7f }, { claReed, 0.35f }, { claGrowl, 0.15f }, { claBody, 0.55f },
            { scale, hijaz }, { snap, 1 }, { ornGrace, 1 }, { ornTrill, 1 }, { ornFall, 1 }, { ornChance, 55.0f }, { ornSpeed, 44.0f },
            { glide, 80.0f }, { bendRange, 3.0f }, { vibDepth, 26.0f }, { vibRate, 5.8f }, { ampA, 18.0f },
            { cutoff, 8000.0f }, { filterEnv, 0.0f }, { level, -11.0f },
            { delayOn, 1 }, { delayMix, 18.0f }, { reverbOn, 1 }, { reverbMix, 20.0f } } },

        { "Truba Guca", {
            { model, truba }, { truBlow, 0.7f }, { truBlare, 0.6f }, { truRasp, 0.2f }, { truMute, 0.0f },
            { scale, nikriz }, { snap, 1 }, { ornGrace, 1 }, { ornMordent, 1 }, { ornFall, 1 }, { ornChance, 50.0f }, { ornAmount, 0.6f },
            { glide, 25.0f }, { vibDepth, 22.0f }, { vibRate, 6.0f }, { vibDelay, 300.0f }, { ampA, 12.0f },
            { cutoff, 10000.0f }, { filterEnv, 0.0f }, { level, -11.0f }, { reverbOn, 1 }, { reverbSize, 0.5f }, { reverbMix, 16.0f } } },

        { "Violina Starogradska", {
            { model, violin }, { vioBow, 0.7f }, { vioPosition, 0.3f }, { vioScratch, 0.2f }, { vioBody, 0.6f },
            { scale, harmonicMinor }, { snap, 1 }, { ornGrace, 1 }, { ornChance, 35.0f },
            { glide, 90.0f }, { vibDepth, 30.0f }, { vibRate, 5.6f }, { vibDelay, 260.0f }, { ampA, 70.0f }, { ampR, 240.0f },
            { cutoff, 8000.0f }, { filterEnv, 0.0f }, { level, -11.0f }, { reverbOn, 1 }, { reverbSize, 0.7f }, { reverbMix, 26.0f } } },

        { "Sargija Sevdah", {
            { model, sargija }, { sarPluck, 0.75f }, { sarDecay, 0.55f }, { sarDrone, 0.5f }, { sarTone, 0.7f },
            { scale, hijaz }, { snap, 1 }, { ornGrace, 1 }, { ornMordent, 1 }, { ornChance, 45.0f }, { ornSpeed, 50.0f },
            { vibDepth, 0.0f }, { glide, 30.0f }, { cutoff, 11000.0f }, { filterEnv, 0.0f }, { ampA, 0.5f }, { ampS, 1.0f }, { ampR, 500.0f },
            { reverbOn, 1 }, { reverbSize, 0.5f }, { reverbMix, 18.0f } } },

        { "Kanun Tremolo", {
            { model, kanun }, { kanPluck, 0.75f }, { kanDecay, 0.6f }, { kanShimmer, 0.45f }, { kanTremolo, 0.55f },
            { scale, hijazkar }, { snap, 1 }, { mono, 0 }, { vibDepth, 0.0f }, { glide, 0.0f },
            { cutoff, 13000.0f }, { filterEnv, 0.0f }, { ampA, 0.5f }, { ampS, 1.0f }, { ampR, 700.0f },
            { delayOn, 1 }, { delayTime, 2 }, { delayMix, 20.0f }, { reverbOn, 1 }, { reverbSize, 0.7f }, { reverbMix, 24.0f } } },

        { "Darbuka", {
            { model, darbuka }, { darStrike, 0.2f }, { darSnap, 0.5f }, { darDecay, 0.5f }, { darRing, 0.3f },
            { vibDepth, 0.0f }, { glide, 0.0f }, { cutoff, 16000.0f }, { filterEnv, 0.0f },
            { ampA, 0.5f }, { ampS, 1.0f }, { ampR, 250.0f },
            { reverbOn, 1 }, { reverbSize, 0.25f }, { reverbMix, 10.0f } } },

        { "Glas Sevdah", {
            { model, voice }, { voxVowel, 0.0f }, { voxBreath, 0.25f }, { voxSize, 0.45f }, { voxTension, 0.5f },
            { scale, hijaz }, { snap, 1 }, { ornGrace, 1 }, { ornMordent, 1 }, { ornFall, 1 }, { ornChance, 50.0f }, { ornSpeed, 60.0f },
            { glide, 120.0f }, { vibDepth, 34.0f }, { vibRate, 5.4f }, { vibDelay, 350.0f }, { ampA, 60.0f }, { ampR, 300.0f },
            { cutoff, 9000.0f }, { filterEnv, 0.0f }, { lfo (1, "Rate"), 0.2f },
            { slot (0, "Source"), lfo2 }, { slot (0, "Dest"), toCutoff }, { slot (0, "Amount"), 0.05f },
            { reverbOn, 1 }, { reverbSize, 0.8f }, { reverbMix, 30.0f } } },

        { "Psy Bass", {
            { engineLevel, 0.0f }, { oscATable, basic }, { oscAPos, 0.66f }, { oscALevel, 0.9f }, { oscAPitch, -12.0f },
            { subLevel, 0.35f }, { vibDepth, 0.0f }, { glide, 0.0f },
            { cutoff, 260.0f }, { resonance, 0.3f }, { filterEnv, 0.55f },
            { fltA, 0.5f }, { fltD, 85.0f }, { fltS, 0.0f }, { fltR, 60.0f }, { ampA, 0.5f }, { ampD, 140.0f }, { ampS, 0.0f }, { ampR, 40.0f },
            { level, -6.0f } } },

        { "Acid 7/8 Arp", {
            { engineLevel, 0.0f }, { oscATable, acid }, { oscAPos, 0.35f }, { oscALevel, 0.9f }, { subLevel, 0.3f },
            { scale, hijaz }, { snap, 1 }, { vibDepth, 0.0f }, { glide, 25.0f },
            { cutoff, 700.0f }, { resonance, 0.6f }, { filterEnv, 0.55f }, { fltD, 130.0f }, { fltS, 0.0f }, { ampR, 70.0f },
            { arpOn, 1 }, { arpMeter, m78a }, { arpRate, 1 }, { arpMode, 2 }, { arpOctaves, 2.0f }, { arpGate, 55.0f }, { arpAccent, 80.0f },
            { lfo (1, "Sync"), 1 }, { lfo (1, "Division"), 0 }, { slot (0, "Source"), lfo2 }, { slot (0, "Dest"), toPosA }, { slot (0, "Amount"), 0.5f },
            { slot (1, "Source"), macro1 }, { slot (1, "Dest"), toCutoff }, { slot (1, "Amount"), 0.6f },
            { distOn, 1 }, { distDrive, 10.0f }, { distMix, 60.0f }, { delayOn, 1 }, { delayTime, 2 }, { delayMix, 24.0f } } },

        { "Supersaw Hijaz", {
            { level, -12.0f },
            { engineLevel, 0.0f }, { oscATable, basic }, { oscAPos, 0.66f }, { oscAUnison, 7.0f }, { oscADetune, 24.0f }, { oscALevel, 0.9f },
            { oscBTable, basic }, { oscBPos, 0.66f }, { oscBUnison, 5.0f }, { oscBDetune, 14.0f }, { oscBPitch, 12.0f }, { oscBLevel, 0.4f },
            { scale, hijaz }, { snap, 1 }, { mono, 0 }, { vibDepth, 0.0f }, { glide, 0.0f },
            { cutoff, 5000.0f }, { filterEnv, 0.25f }, { ampA, 8.0f }, { ampR, 320.0f },
            { duckOn, 1 }, { reverbOn, 1 }, { reverbSize, 0.75f }, { reverbMix, 28.0f }, { delayOn, 1 }, { delayMix, 20.0f } } },

        { "Vowel Gate 9/8", {
            { level, -11.0f },
            { engineLevel, 0.0f }, { oscATable, vowels }, { oscAPos, 0.2f }, { oscAUnison, 3.0f }, { oscADetune, 12.0f }, { oscALevel, 0.9f },
            { mono, 0 }, { vibDepth, 0.0f }, { cutoff, 6000.0f }, { filterEnv, 0.0f }, { ampA, 30.0f }, { ampS, 1.0f }, { ampR, 250.0f },
            { lfo (0, "Sync"), 1 }, { lfo (0, "Division"), 1 }, { lfo (0, "Shape"), 1 },
            { slot (0, "Source"), lfo1 }, { slot (0, "Dest"), toPosA }, { slot (0, "Amount"), 0.8f },
            { gateOn, 1 }, { gatePattern, 5 }, { gateRate, 1 }, { gateLength, 60.0f },
            { delayOn, 1 }, { delayTime, 2 }, { delayMix, 26.0f }, { reverbOn, 1 }, { reverbMix, 22.0f } } },

        { "Zurla Scream", {
            { engineLevel, 0.0f }, { oscATable, zurla }, { oscAPos, 0.45f }, { oscALevel, 0.9f }, { noiseLevel, 0.06f }, { noiseColor, 9000.0f },
            { scale, hijazkar }, { snap, 1 }, { ornGrace, 1 }, { ornTrill, 1 }, { ornChance, 50.0f }, { ornSpeed, 42.0f },
            { glide, 45.0f }, { vibDepth, 24.0f }, { vibRate, 6.5f }, { cutoff, 9000.0f }, { filterEnv, 0.0f }, { ampA, 15.0f },
            { slot (0, "Source"), velocity }, { slot (0, "Dest"), toPosA }, { slot (0, "Amount"), 0.4f },
            { slot (1, "Source"), wheel }, { slot (1, "Dest"), toPosA }, { slot (1, "Amount"), 0.5f },
            { distOn, 1 }, { distDrive, 9.0f }, { distMix, 40.0f }, { reverbOn, 1 }, { reverbMix, 20.0f } } },

        { "Bells 11/8", {
            { engineLevel, 0.0f }, { oscATable, bells }, { oscAPos, 0.35f }, { oscALevel, 0.9f },
            { scale, nikriz }, { snap, 1 }, { mono, 0 }, { vibDepth, 0.0f }, { glide, 0.0f },
            { cutoff, 8000.0f }, { filterEnv, 0.3f }, { ampA, 1.0f }, { ampD, 600.0f }, { ampS, 0.0f }, { ampR, 600.0f },
            { slot (0, "Source"), fltEnv }, { slot (0, "Dest"), toPosA }, { slot (0, "Amount"), 0.5f },
            { arpOn, 1 }, { arpMeter, m118 }, { arpRate, 1 }, { arpOctaves, 2.0f }, { arpGate, 40.0f },
            { delayOn, 1 }, { delayTime, 2 }, { delayFeedback, 55.0f }, { delayMix, 32.0f }, { reverbOn, 1 }, { reverbSize, 0.8f }, { reverbMix, 30.0f } } },

        { "Kaval Pad", {
            { level, -11.0f },
            { model, kaval }, { engineLevel, 0.5f }, { kavNoise, 0.6f }, { oscATable, kavalTable }, { oscAPos, 0.2f }, { oscAUnison, 5.0f },
            { oscADetune, 16.0f }, { oscALevel, 0.6f }, { oscBTable, reeds }, { oscBPos, 0.2f }, { oscBPitch, -12.0f }, { oscBLevel, 0.3f },
            { scale, huseyni }, { snap, 1 }, { mono, 0 }, { vibDepth, 8.0f }, { glide, 0.0f },
            { cutoff, 3000.0f }, { filterEnv, 0.0f }, { ampA, 600.0f }, { ampS, 1.0f }, { ampR, 1500.0f },
            { lfo (1, "Rate"), 0.12f }, { slot (0, "Source"), lfo2 }, { slot (0, "Dest"), toPosA }, { slot (0, "Amount"), 0.4f },
            { slot (1, "Source"), lfo2 }, { slot (1, "Dest"), toCutoff }, { slot (1, "Amount"), 0.15f },
            { reverbOn, 1 }, { reverbSize, 0.95f }, { reverbMix, 45.0f } } },

        { "Full-On Lead", {
            { level, -11.0f },
            { model, juzni }, { tone, 0.3f }, { detune, 14.0f }, { engineLevel, 0.7f },
            { oscATable, pwm }, { oscAPos, 0.5f }, { oscAUnison, 3.0f }, { oscADetune, 10.0f }, { oscALevel, 0.5f },
            { scale, hijaz }, { snap, 1 }, { ornGrace, 1 }, { ornChance, 40.0f }, { glide, 70.0f }, { vibDepth, 22.0f },
            { cutoff, 1800.0f }, { resonance, 0.45f }, { filterEnv, 0.4f },
            { lfo (0, "Sync"), 1 }, { lfo (0, "Division"), 7 }, { lfo (0, "Shape"), 2 },
            { slot (0, "Source"), lfo1 }, { slot (0, "Dest"), toCutoff }, { slot (0, "Amount"), 0.25f },
            { slot (1, "Source"), lfo2 }, { slot (1, "Dest"), toPosA }, { slot (1, "Amount"), 0.4f },
            { slot (2, "Source"), wheel }, { slot (2, "Dest"), toCutoff }, { slot (2, "Amount"), 0.5f },
            { distOn, 1 }, { distDrive, 12.0f }, { distMix, 45.0f }, { duckOn, 1 }, { duckDepth, 50.0f },
            { delayOn, 1 }, { delayTime, 2 }, { delayMix, 30.0f }, { reverbOn, 1 }, { reverbMix, 16.0f } } },
    };

    return presets;
}

/** Sets every parameter to its default, then applies the preset's values. Message thread only. */
inline void applyPreset (juce::AudioProcessorValueTreeState& apvts, const Preset& preset)
{
    for (auto* p : apvts.processor.getParameters())
        if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (p))
            ranged->setValueNotifyingHost (ranged->getDefaultValue());

    for (const auto& [paramId, value] : preset.values)
    {
        if (auto* p = apvts.getParameter (paramId))
            p->setValueNotifyingHost (p->convertTo0to1 (value));
        else
            jassertfalse; // a preset names a parameter that does not exist
    }
}
} // namespace amv
