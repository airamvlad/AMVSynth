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
    using Values = std::vector<std::pair<juce::String, float>>;
    using Shapes = std::vector<std::pair<int, std::vector<LfoPoint>>>;

    Preset (const char* presetName, Values presetValues, const char* presetCategory, std::vector<SeqStep> steps = {}, Shapes shapes = {})
        : name (presetName), values (std::move (presetValues)), category (presetCategory), sequence (std::move (steps)), lfoShapes (std::move (shapes))
    {
    }

    const char* name;
    Values values;
    const char* category;            // the group it is listed under in the browser
    std::vector<SeqStep> sequence;   // steps of the sequencer; the rest keep their defaults
    Shapes lfoShapes;                // drawn shapes, by LFO (0 to 3)
    std::array<juce::String, mod::numMacros> macroNames; // what each macro does in this preset; empty for "MACRO 1" and so on
};

/** The name a macro has until a preset or the user gives it one. */
inline juce::String defaultMacroName (int index) { return "MACRO " + juce::String (index + 1); }

/** Gives a factory preset its four macros.

    A macro the preset already uses is named after what it moves. The others are wired in free
    modulation slots to the same four things in every preset, so the macros always do something
    and mean the same everywhere: BRIGHT opens the filter (DARK closes it instead, in a preset
    whose filter is already open), EDGE adds resonance, VIBRATO adds
    vibrato and TIMBRE moves the first control of the Balkan model, or the wavetable position of
    oscillator A when the model is silent. All of them rest at zero, so the preset itself sounds
    as it was made. */
inline void wirePresetMacros (Preset& preset)
{
    const auto find = [&preset] (const juce::String& paramId, float fallback)
    {
        for (const auto& [name, value] : preset.values)
            if (name == paramId)
                return value;
        return fallback;
    };

    std::array<bool, mod::numSlots> used {};
    std::array<bool, mod::numMacros> wired {};

    for (int slot = 0; slot < mod::numSlots; ++slot)
    {
        const int source = juce::roundToInt (find (id::slot (slot, "Source"), 0.0f));
        used[(size_t) slot] = source != mod::srcNone;

        const int macro = source - mod::srcMacro1;
        if (macro >= 0 && macro < mod::numMacros && ! wired[(size_t) macro])
        {
            // Slot 1 goes to the cutoff when nothing else is said, the others to pitch.
            const int dest = juce::roundToInt (find (id::slot (slot, "Dest"), slot == 0 ? (float) mod::dstCutoff : (float) mod::dstPitch));
            wired[(size_t) macro] = true;
            preset.macroNames[(size_t) macro] = juce::String (getModDests()[(size_t) juce::jlimit (0, (int) mod::numDests - 1, dest)]).toUpperCase();
        }
    }

    const bool engineSounds = find (id::engineLevel, 0.8f) > 0.0f;
    struct Wiring { const char* name; int dest; float amount; };
    const bool filterOpen = find (id::cutoff, 3200.0f) >= 6000.0f; // nothing left to open, so the macro closes it
    const Wiring wirings[mod::numMacros] { { filterOpen ? "DARK" : "BRIGHT", mod::dstCutoff, filterOpen ? -0.5f : 0.4f },
                                           { "EDGE", mod::dstResonance, 0.5f },
                                           { "VIBRATO", mod::dstVibrato, 0.5f },
                                           { "TIMBRE", engineSounds ? (int) mod::dstEngine1 : (int) mod::dstOscAPosition, 0.5f } };

    for (int macro = 0; macro < mod::numMacros; ++macro)
    {
        if (wired[(size_t) macro])
            continue;

        for (int slot = 0; slot < mod::numSlots; ++slot)
            if (! used[(size_t) slot])
            {
                used[(size_t) slot] = true;
                preset.values.emplace_back (id::slot (slot, "Source"), (float) (mod::srcMacro1 + macro));
                preset.values.emplace_back (id::slot (slot, "Dest"), (float) wirings[macro].dest);
                preset.values.emplace_back (id::slot (slot, "Amount"), wirings[macro].amount);
                preset.macroNames[(size_t) macro] = wirings[macro].name;
                break;
            }
    }
}

/** The groups of the factory presets, in the order the browser lists them. */
inline const std::array<const char*, 6>& getPresetCategories()
{
    static const std::array<const char*, 6> categories { "Folk", "Bass", "Lead", "Pad", "Arp", "Sequence" };
    return categories;
}

inline const std::vector<Preset>& getFactoryPresets()
{
    using namespace id;

    // Choice indices used below.
    enum { juzni = 0, accordion, gusle, frula, kaval, gajde, zurlaModel, tambura, tapan, dvojnice, clarinet, truba, violin, sargija, kanun, darbuka, voice };
    enum { chromatic = 0, hijaz, hijazkar, nikriz, harmonicMinor, kurdi, rast, ussak, huseyni, saba, naturalMinor, majorScale };
    enum { m44 = 0, m58, m78a, m78b, m98, m118 };
    enum { basic = 0, acid, pwm, vowels, kavalTable, zurla, reeds, bells };
    enum { lfo1 = 1, lfo2, fltEnv, velocity, wheel, macro1, macro2, macro3, macro4, pressure, timbre, lfo3, lfo4, env3 };
    enum { warpOff = 0, bendUp, bendDown, warpPwm, warpSync, warpMirror };
    enum { xmodOff = 0, fm, ring, am };
    enum { fromOtherOsc = 0, fromSub, fromNoise, fromEngine };
    enum { lp12 = 0, lp24, bandPass, hp12, hp24, notch, comb, formant, phaser };
    enum { sine = 0, triangle, saw, square, randomShape, custom };
    enum { serial = 0, parallel };
    enum { filter1 = 0, filter2 };
    enum { toPitch = 0, toCutoff, toRes, toPosA, toPosB, toLevelA, toLevelB, toEngine, toNoise, toVolume };

    static const std::vector<Preset> presets = []
    {
        std::vector<Preset> list {
        { "Init", {}, "Init" },

        { "Juzni Vetar Lead", {
            { level, -12.0f },
            { model, juzni }, { tone, 0.55f }, { detune, 11.0f }, { sub, 0.15f },
            { scale, hijaz }, { snap, 1 }, { ornGrace, 1 }, { ornMordent, 1 }, { ornChance, 60.0f },
            { glide, 60.0f }, { vibDepth, 32.0f }, { cutoff, 4200.0f }, { resonance, 0.3f },
            { delayOn, 1 }, { delayMix, 22.0f }, { reverbOn, 1 }, { reverbMix, 18.0f } }, "Folk" },

        { "Vetar Trill Solo", {
            { level, -13.0f },
            { model, juzni }, { tone, 0.8f }, { pulseWidth, 0.24f }, { detune, 6.0f },
            { scale, hijazkar }, { snap, 1 }, { ornTrill, 1 }, { ornFall, 1 }, { ornSpeed, 48.0f }, { ornAmount, 0.6f }, { ornChance, 45.0f },
            { glide, 90.0f }, { bendRange, 3.0f }, { cutoff, 5200.0f }, { filterEnv, 0.2f },
            { distOn, 1 }, { distDrive, 8.0f }, { distMix, 50.0f }, { delayOn, 1 }, { delayTime, 2 }, { delayMix, 28.0f } }, "Folk" },

        { "Harmonika Kolo", {
            { level, -16.0f },
            { model, accordion }, { accBellows, 0.8f }, { accMusette, 14.0f }, { accRegister, 0.45f }, { accClick, 0.4f },
            { mono, 0 }, { vibDepth, 0.0f }, { glide, 0.0f }, { cutoff, 9000.0f }, { filterEnv, 0.0f },
            { ampA, 12.0f }, { ampS, 0.95f }, { ampR, 90.0f }, { ornGrace, 1 }, { ornChance, 35.0f },
            { reverbOn, 1 }, { reverbSize, 0.35f }, { reverbMix, 14.0f } }, "Folk" },

        { "Harmonika Full Musette", {
            { level, -18.0f },
            { model, accordion }, { accBellows, 0.9f }, { accMusette, 22.0f }, { accRegister, 1.0f }, { accClick, 0.25f },
            { mono, 0 }, { vibDepth, 0.0f }, { glide, 0.0f }, { cutoff, 12000.0f }, { filterEnv, 0.0f },
            { ampA, 20.0f }, { ampS, 1.0f }, { ampR, 140.0f }, { reverbOn, 1 }, { reverbMix, 20.0f } }, "Folk" },

        { "Gusle Epic", {
            { model, gusle }, { gusBow, 0.75f }, { gusPosition, 0.45f }, { gusRasp, 0.45f }, { gusNasal, 0.7f },
            { scale, kurdi }, { snap, 1 }, { ornGrace, 1 }, { ornFall, 1 }, { ornChance, 50.0f },
            { glide, 110.0f }, { vibDepth, 20.0f }, { vibRate, 5.2f }, { ampA, 90.0f }, { ampR, 260.0f },
            { cutoff, 6000.0f }, { filterEnv, 0.1f }, { reverbOn, 1 }, { reverbSize, 0.8f }, { reverbMix, 30.0f } }, "Folk" },

        { "Frula Dance", {
            { level, -11.0f },
            { model, frula }, { fruBreath, 0.75f }, { fruOverblow, 0.2f }, { fruNoise, 0.3f }, { fruChiff, 0.55f },
            { scale, majorScale }, { snap, 1 }, { ornGrace, 1 }, { ornMordent, 1 }, { ornTrill, 1 }, { ornChance, 55.0f }, { ornSpeed, 45.0f },
            { glide, 20.0f }, { vibDepth, 14.0f }, { vibRate, 5.6f }, { ampA, 18.0f }, { cutoff, 9000.0f }, { filterEnv, 0.0f },
            { delayOn, 1 }, { delayMix, 18.0f }, { reverbOn, 1 }, { reverbMix, 22.0f } }, "Folk" },

        { "Kaval Mist", {
            { level, -10.0f },
            { model, kaval }, { kavBreath, 0.65f }, { kavOverblow, 0.1f }, { kavNoise, 0.65f }, { kavThroat, 0.45f },
            { scale, huseyni }, { snap, 1 }, { ornGrace, 1 }, { ornFall, 1 }, { ornChance, 40.0f },
            { glide, 140.0f }, { vibDepth, 16.0f }, { vibRate, 4.8f }, { vibDelay, 400.0f }, { ampA, 70.0f }, { ampR, 500.0f },
            { cutoff, 7000.0f }, { filterEnv, 0.0f }, { delayOn, 1 }, { delayTime, 4 }, { delayFeedback, 55.0f }, { delayMix, 25.0f },
            { reverbOn, 1 }, { reverbSize, 0.9f }, { reverbMix, 40.0f } }, "Folk" },

        { "Gajde Drone", {
            { model, gajde }, { gajDrone, 0.7f }, { gajBag, 0.75f }, { gajEdge, 0.55f },
            { scale, hijaz }, { snap, 1 }, { ornGrace, 1 }, { ornMordent, 1 }, { ornChance, 70.0f }, { ornSpeed, 40.0f },
            { glide, 15.0f }, { vibDepth, 0.0f }, { ampA, 25.0f }, { ampS, 1.0f }, { cutoff, 8000.0f }, { filterEnv, 0.0f },
            { reverbOn, 1 }, { reverbMix, 16.0f } }, "Folk" },

        { "Zurla Svadba", {
            { model, zurlaModel }, { zurPressure, 0.8f }, { zurBite, 0.55f }, { zurGrowl, 0.25f }, { zurNasal, 0.65f },
            { scale, hijaz }, { snap, 1 }, { ornGrace, 1 }, { ornTrill, 1 }, { ornFall, 1 }, { ornChance, 60.0f }, { ornSpeed, 42.0f },
            { glide, 35.0f }, { vibDepth, 26.0f }, { vibRate, 6.4f }, { ampA, 20.0f }, { cutoff, 11000.0f }, { filterEnv, 0.0f },
            { level, -12.0f }, { reverbOn, 1 }, { reverbMix, 18.0f } }, "Folk" },

        { "Tambura Sitna", {
            { model, tambura }, { tamPluck, 0.7f }, { tamDecay, 0.5f }, { tamDouble, 0.45f }, { tamTone, 0.65f },
            { scale, majorScale }, { snap, 1 }, { mono, 0 }, { vibDepth, 0.0f }, { glide, 0.0f },
            { cutoff, 12000.0f }, { filterEnv, 0.0f }, { ampA, 0.5f }, { ampS, 1.0f }, { ampR, 400.0f },
            { reverbOn, 1 }, { reverbSize, 0.4f }, { reverbMix, 14.0f } }, "Folk" },

        { "Tambura 7/8 Arp", {
            { model, tambura }, { tamPluck, 0.85f }, { tamDecay, 0.4f }, { tamDouble, 0.5f }, { tamTone, 0.75f },
            { scale, hijaz }, { snap, 1 }, { vibDepth, 0.0f }, { glide, 0.0f },
            { cutoff, 9000.0f }, { filterEnv, 0.2f }, { ampA, 0.5f }, { ampS, 1.0f }, { ampR, 200.0f },
            { arpOn, 1 }, { arpMeter, m78b }, { arpRate, 1 }, { arpMode, 0 }, { arpOctaves, 2.0f }, { arpGate, 70.0f }, { arpAccent, 70.0f },
            { delayOn, 1 }, { delayTime, 2 }, { delayMix, 26.0f }, { reverbOn, 1 }, { reverbMix, 16.0f } }, "Arp" },

        { "Tapan", {
            { model, tapan }, { tapThump, 0.55f }, { tapSnap, 0.5f }, { tapDecay, 0.45f }, { tapRing, 0.3f },
            { vibDepth, 0.0f }, { glide, 0.0f }, { cutoff, 14000.0f }, { filterEnv, 0.0f },
            { ampA, 0.5f }, { ampS, 1.0f }, { ampR, 300.0f }, { reverbOn, 1 }, { reverbSize, 0.3f }, { reverbMix, 10.0f } }, "Folk" },

        { "Dvojnice", {
            { model, dvojnice }, { dvoBreath, 0.7f }, { dvoSecond, 0.7f }, { dvoInterval, -3.0f }, { dvoBeat, 0.35f },
            { scale, majorScale }, { snap, 1 }, { ornGrace, 1 }, { ornMordent, 1 }, { ornChance, 50.0f }, { ornSpeed, 45.0f },
            { glide, 15.0f }, { vibDepth, 10.0f }, { ampA, 25.0f }, { cutoff, 9000.0f }, { filterEnv, 0.0f },
            { level, -11.0f }, { reverbOn, 1 }, { reverbMix, 22.0f } }, "Folk" },

        { "Klarinet Svadba", {
            { model, clarinet }, { claBreath, 0.7f }, { claReed, 0.35f }, { claGrowl, 0.15f }, { claBody, 0.55f },
            { scale, hijaz }, { snap, 1 }, { ornGrace, 1 }, { ornTrill, 1 }, { ornFall, 1 }, { ornChance, 55.0f }, { ornSpeed, 44.0f },
            { glide, 80.0f }, { bendRange, 3.0f }, { vibDepth, 26.0f }, { vibRate, 5.8f }, { ampA, 18.0f },
            { cutoff, 8000.0f }, { filterEnv, 0.0f }, { level, -11.0f },
            { delayOn, 1 }, { delayMix, 18.0f }, { reverbOn, 1 }, { reverbMix, 20.0f } }, "Folk" },

        { "Truba Guca", {
            { model, truba }, { truBlow, 0.7f }, { truBlare, 0.6f }, { truRasp, 0.2f }, { truMute, 0.0f },
            { scale, nikriz }, { snap, 1 }, { ornGrace, 1 }, { ornMordent, 1 }, { ornFall, 1 }, { ornChance, 50.0f }, { ornAmount, 0.6f },
            { glide, 25.0f }, { vibDepth, 22.0f }, { vibRate, 6.0f }, { vibDelay, 300.0f }, { ampA, 12.0f },
            { cutoff, 10000.0f }, { filterEnv, 0.0f }, { level, -11.0f }, { reverbOn, 1 }, { reverbSize, 0.5f }, { reverbMix, 16.0f } }, "Folk" },

        { "Violina Starogradska", {
            { model, violin }, { vioBow, 0.7f }, { vioPosition, 0.3f }, { vioScratch, 0.2f }, { vioBody, 0.6f },
            { scale, harmonicMinor }, { snap, 1 }, { ornGrace, 1 }, { ornChance, 35.0f },
            { glide, 90.0f }, { vibDepth, 30.0f }, { vibRate, 5.6f }, { vibDelay, 260.0f }, { ampA, 70.0f }, { ampR, 240.0f },
            { cutoff, 8000.0f }, { filterEnv, 0.0f }, { level, -11.0f }, { reverbOn, 1 }, { reverbSize, 0.7f }, { reverbMix, 26.0f } }, "Folk" },

        { "Sargija Sevdah", {
            { model, sargija }, { sarPluck, 0.75f }, { sarDecay, 0.55f }, { sarDrone, 0.5f }, { sarTone, 0.7f },
            { scale, hijaz }, { snap, 1 }, { ornGrace, 1 }, { ornMordent, 1 }, { ornChance, 45.0f }, { ornSpeed, 50.0f },
            { vibDepth, 0.0f }, { glide, 30.0f }, { cutoff, 11000.0f }, { filterEnv, 0.0f }, { ampA, 0.5f }, { ampS, 1.0f }, { ampR, 500.0f },
            { reverbOn, 1 }, { reverbSize, 0.5f }, { reverbMix, 18.0f } }, "Folk" },

        { "Kanun Tremolo", {
            { model, kanun }, { kanPluck, 0.75f }, { kanDecay, 0.6f }, { kanShimmer, 0.45f }, { kanTremolo, 0.55f },
            { scale, hijazkar }, { snap, 1 }, { mono, 0 }, { vibDepth, 0.0f }, { glide, 0.0f },
            { cutoff, 13000.0f }, { filterEnv, 0.0f }, { ampA, 0.5f }, { ampS, 1.0f }, { ampR, 700.0f },
            { delayOn, 1 }, { delayTime, 2 }, { delayMix, 20.0f }, { reverbOn, 1 }, { reverbSize, 0.7f }, { reverbMix, 24.0f } }, "Folk" },

        { "Darbuka", {
            { model, darbuka }, { darStrike, 0.2f }, { darSnap, 0.5f }, { darDecay, 0.5f }, { darRing, 0.3f },
            { vibDepth, 0.0f }, { glide, 0.0f }, { cutoff, 16000.0f }, { filterEnv, 0.0f },
            { ampA, 0.5f }, { ampS, 1.0f }, { ampR, 250.0f },
            { reverbOn, 1 }, { reverbSize, 0.25f }, { reverbMix, 10.0f } }, "Folk" },

        { "Glas Sevdah", {
            { model, voice }, { voxVowel, 0.0f }, { voxBreath, 0.25f }, { voxSize, 0.45f }, { voxTension, 0.5f },
            { scale, hijaz }, { snap, 1 }, { ornGrace, 1 }, { ornMordent, 1 }, { ornFall, 1 }, { ornChance, 50.0f }, { ornSpeed, 60.0f },
            { glide, 120.0f }, { vibDepth, 34.0f }, { vibRate, 5.4f }, { vibDelay, 350.0f }, { ampA, 60.0f }, { ampR, 300.0f },
            { cutoff, 9000.0f }, { filterEnv, 0.0f }, { lfo (1, "Rate"), 0.2f },
            { slot (0, "Source"), lfo2 }, { slot (0, "Dest"), toCutoff }, { slot (0, "Amount"), 0.05f },
            { reverbOn, 1 }, { reverbSize, 0.8f }, { reverbMix, 30.0f } }, "Folk" },

        { "Psy Bass", {
            { engineLevel, 0.0f }, { oscATable, basic }, { oscAPos, 0.66f }, { oscALevel, 0.9f }, { oscAPitch, -12.0f },
            { subLevel, 0.35f }, { vibDepth, 0.0f }, { glide, 0.0f },
            { cutoff, 260.0f }, { resonance, 0.3f }, { filterEnv, 0.55f },
            { fltA, 0.5f }, { fltD, 85.0f }, { fltS, 0.0f }, { fltR, 60.0f }, { ampA, 0.5f }, { ampD, 140.0f }, { ampS, 0.0f }, { ampR, 40.0f },
            { level, -6.0f } }, "Bass" },

        { "Acid 7/8 Arp", {
            { engineLevel, 0.0f }, { oscATable, acid }, { oscAPos, 0.35f }, { oscALevel, 0.9f }, { subLevel, 0.3f },
            { scale, hijaz }, { snap, 1 }, { vibDepth, 0.0f }, { glide, 25.0f },
            { cutoff, 700.0f }, { resonance, 0.6f }, { filterEnv, 0.55f }, { fltD, 130.0f }, { fltS, 0.0f }, { ampR, 70.0f },
            { arpOn, 1 }, { arpMeter, m78a }, { arpRate, 1 }, { arpMode, 2 }, { arpOctaves, 2.0f }, { arpGate, 55.0f }, { arpAccent, 80.0f },
            { lfo (1, "Sync"), 1 }, { lfo (1, "Division"), 0 }, { slot (0, "Source"), lfo2 }, { slot (0, "Dest"), toPosA }, { slot (0, "Amount"), 0.5f },
            { slot (1, "Source"), macro1 }, { slot (1, "Dest"), toCutoff }, { slot (1, "Amount"), 0.6f },
            { distOn, 1 }, { distDrive, 10.0f }, { distMix, 60.0f }, { delayOn, 1 }, { delayTime, 2 }, { delayMix, 24.0f } }, "Arp" },

        { "Supersaw Hijaz", {
            { level, -12.0f },
            { engineLevel, 0.0f }, { oscATable, basic }, { oscAPos, 0.66f }, { oscAUnison, 7.0f }, { oscADetune, 24.0f }, { oscALevel, 0.9f },
            { oscBTable, basic }, { oscBPos, 0.66f }, { oscBUnison, 5.0f }, { oscBDetune, 14.0f }, { oscBPitch, 12.0f }, { oscBLevel, 0.4f },
            { scale, hijaz }, { snap, 1 }, { mono, 0 }, { vibDepth, 0.0f }, { glide, 0.0f },
            { cutoff, 5000.0f }, { filterEnv, 0.25f }, { ampA, 8.0f }, { ampR, 320.0f },
            { duckOn, 1 }, { reverbOn, 1 }, { reverbSize, 0.75f }, { reverbMix, 28.0f }, { delayOn, 1 }, { delayMix, 20.0f } }, "Lead" },

        { "Vowel Gate 9/8", {
            { fxOrder, 1 },
            { level, -11.0f },
            { engineLevel, 0.0f }, { oscATable, vowels }, { oscAPos, 0.2f }, { oscAUnison, 3.0f }, { oscADetune, 12.0f }, { oscALevel, 0.9f },
            { mono, 0 }, { vibDepth, 0.0f }, { cutoff, 6000.0f }, { filterEnv, 0.0f }, { ampA, 30.0f }, { ampS, 1.0f }, { ampR, 250.0f },
            { lfo (0, "Sync"), 1 }, { lfo (0, "Division"), 1 }, { lfo (0, "Shape"), 1 },
            { slot (0, "Source"), lfo1 }, { slot (0, "Dest"), toPosA }, { slot (0, "Amount"), 0.8f },
            { gateOn, 1 }, { gatePattern, 5 }, { gateRate, 1 }, { gateLength, 60.0f },
            { delayOn, 1 }, { delayTime, 2 }, { delayMix, 26.0f }, { reverbOn, 1 }, { reverbMix, 22.0f } }, "Pad" },

        { "Zurla Scream", {
            { engineLevel, 0.0f }, { oscATable, zurla }, { oscAPos, 0.45f }, { oscALevel, 0.9f }, { noiseLevel, 0.06f }, { noiseColor, 9000.0f },
            { scale, hijazkar }, { snap, 1 }, { ornGrace, 1 }, { ornTrill, 1 }, { ornChance, 50.0f }, { ornSpeed, 42.0f },
            { glide, 45.0f }, { vibDepth, 24.0f }, { vibRate, 6.5f }, { cutoff, 9000.0f }, { filterEnv, 0.0f }, { ampA, 15.0f },
            { slot (0, "Source"), velocity }, { slot (0, "Dest"), toPosA }, { slot (0, "Amount"), 0.4f },
            { slot (1, "Source"), wheel }, { slot (1, "Dest"), toPosA }, { slot (1, "Amount"), 0.5f },
            { distOn, 1 }, { distDrive, 9.0f }, { distMix, 40.0f }, { reverbOn, 1 }, { reverbMix, 20.0f } }, "Lead" },

        { "Bells 11/8", {
            { engineLevel, 0.0f }, { oscATable, bells }, { oscAPos, 0.35f }, { oscALevel, 0.9f },
            { scale, nikriz }, { snap, 1 }, { mono, 0 }, { vibDepth, 0.0f }, { glide, 0.0f },
            { cutoff, 8000.0f }, { filterEnv, 0.3f }, { ampA, 1.0f }, { ampD, 600.0f }, { ampS, 0.0f }, { ampR, 600.0f },
            { slot (0, "Source"), fltEnv }, { slot (0, "Dest"), toPosA }, { slot (0, "Amount"), 0.5f },
            { arpOn, 1 }, { arpMeter, m118 }, { arpRate, 1 }, { arpOctaves, 2.0f }, { arpGate, 40.0f },
            { delayOn, 1 }, { delayTime, 2 }, { delayFeedback, 55.0f }, { delayMix, 32.0f }, { reverbOn, 1 }, { reverbSize, 0.8f }, { reverbMix, 30.0f } }, "Arp" },

        { "Kaval Pad", {
            { level, -11.0f },
            { model, kaval }, { engineLevel, 0.5f }, { kavNoise, 0.6f }, { oscATable, kavalTable }, { oscAPos, 0.2f }, { oscAUnison, 5.0f },
            { oscADetune, 16.0f }, { oscALevel, 0.6f }, { oscBTable, reeds }, { oscBPos, 0.2f }, { oscBPitch, -12.0f }, { oscBLevel, 0.3f },
            { scale, huseyni }, { snap, 1 }, { mono, 0 }, { vibDepth, 8.0f }, { glide, 0.0f },
            { cutoff, 3000.0f }, { filterEnv, 0.0f }, { ampA, 600.0f }, { ampS, 1.0f }, { ampR, 1500.0f },
            { lfo (1, "Rate"), 0.12f }, { slot (0, "Source"), lfo2 }, { slot (0, "Dest"), toPosA }, { slot (0, "Amount"), 0.4f },
            { slot (1, "Source"), lfo2 }, { slot (1, "Dest"), toCutoff }, { slot (1, "Amount"), 0.15f },
            { reverbOn, 1 }, { reverbSize, 0.95f }, { reverbMix, 45.0f } }, "Pad" },

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
            { delayOn, 1 }, { delayTime, 2 }, { delayMix, 30.0f }, { reverbOn, 1 }, { reverbMix, 16.0f } }, "Lead" },

        // Presets from here on were added later; programs are saved by number, so new ones go at the end.
        { "Acid Kolo 7/8 Seq", {
            { level, -9.0f },
            { engineLevel, 0.0f }, { oscATable, acid }, { oscAPos, 0.4f }, { oscALevel, 0.9f }, { subLevel, 0.25f },
            { scale, hijaz }, { snap, 1 }, { vibDepth, 0.0f }, { glide, 45.0f },
            { cutoff, 520.0f }, { resonance, 0.65f }, { filterEnv, 0.6f }, { fltD, 120.0f }, { fltS, 0.0f }, { ampR, 60.0f },
            { seqOn, 1 }, { seqMeter, m78a }, { seqRate, 1 }, { seqSteps, 7.0f }, { seqGate, 100.0f },
            { slot (0, "Source"), velocity }, { slot (0, "Dest"), toCutoff }, { slot (0, "Amount"), 0.35f },
            { slot (1, "Source"), macro1 }, { slot (1, "Dest"), toCutoff }, { slot (1, "Amount"), 0.6f },
            { distOn, 1 }, { distDrive, 10.0f }, { distMix, 55.0f }, { delayOn, 1 }, { delayTime, 2 }, { delayMix, 20.0f } },
          "Sequence",
          { { 0, 1.0f, 0.5f, false }, { 0, 0.5f, 0.35f, false }, { 1, 0.6f, 0.5f, true }, { 4, 1.0f, 0.5f, false },
            { 0, 0.5f, 0.35f, false }, { 5, 1.0f, 0.6f, true }, { 4, 0.6f, 0.4f, false } } },

        { "Cocek 9/8 Bass Seq", {
            { level, -6.0f },
            { engineLevel, 0.0f }, { oscATable, basic }, { oscAPos, 0.66f }, { oscALevel, 0.9f }, { oscAPitch, -12.0f }, { subLevel, 0.4f },
            { scale, hijaz }, { snap, 1 }, { vibDepth, 0.0f }, { glide, 30.0f },
            { cutoff, 300.0f }, { resonance, 0.35f }, { filterEnv, 0.55f }, { fltA, 0.5f }, { fltD, 90.0f }, { fltS, 0.0f },
            { ampA, 0.5f }, { ampD, 160.0f }, { ampS, 0.2f }, { ampR, 40.0f },
            { seqOn, 1 }, { seqMeter, m98 }, { seqRate, 1 }, { seqSteps, 9.0f }, { seqGate, 90.0f },
            { slot (0, "Source"), velocity }, { slot (0, "Dest"), toCutoff }, { slot (0, "Amount"), 0.3f } },
          "Sequence",
          { { 0, 1.0f, 0.6f, false }, { 0, 0.5f, 0.4f, false }, { 12, 0.9f, 0.5f, false }, { 0, 0.5f, 0.4f, false }, { 7, 1.0f, 0.6f, true },
            { 5, 0.6f, 0.4f, false }, { 0, 1.0f, 0.6f, false }, { 1, 0.6f, 0.5f, true }, { 0, 0.6f, 0.4f, false } } },

        { "Hijaz Pluck 11/8 Seq", {
            { level, -9.0f },
            { engineLevel, 0.0f }, { oscATable, bells }, { oscAPos, 0.25f }, { oscALevel, 0.8f }, { oscBTable, basic }, { oscBPos, 0.3f }, { oscBLevel, 0.3f },
            { oscBPitch, 12.0f }, { scale, hijaz }, { snap, 1 }, { mono, 0 }, { vibDepth, 0.0f }, { glide, 0.0f },
            { cutoff, 2400.0f }, { filterEnv, 0.45f }, { fltD, 220.0f }, { fltS, 0.0f }, { ampA, 1.0f }, { ampD, 380.0f }, { ampS, 0.0f }, { ampR, 380.0f },
            { seqOn, 1 }, { seqMeter, m118 }, { seqRate, 1 }, { seqSteps, 11.0f }, { seqGate, 80.0f }, { seqSwing, 12.0f },
            { delayOn, 1 }, { delayTime, 2 }, { delayFeedback, 50.0f }, { delayMix, 30.0f }, { reverbOn, 1 }, { reverbSize, 0.7f }, { reverbMix, 24.0f } },
          "Sequence",
          { { 0, 1.0f, 0.6f, false }, { 7, 0.5f, 0.5f, false }, { 4, 1.0f, 0.6f, false }, { 5, 0.5f, 0.5f, false }, { 7, 1.0f, 0.6f, false },
            { 8, 0.6f, 0.5f, false }, { 7, 0.5f, 0.5f, false }, { 12, 1.0f, 0.6f, false }, { 10, 0.5f, 0.5f, false }, { 5, 1.0f, 0.6f, false },
            { 1, 0.6f, 0.5f, false } } },

        { "Rolling Psy Bass Seq", {
            { level, -6.0f },
            { engineLevel, 0.0f }, { oscATable, basic }, { oscAPos, 0.66f }, { oscALevel, 0.9f }, { oscAPitch, -12.0f }, { subLevel, 0.3f },
            { vibDepth, 0.0f }, { glide, 0.0f },
            { cutoff, 240.0f }, { resonance, 0.3f }, { filterEnv, 0.6f }, { fltA, 0.5f }, { fltD, 80.0f }, { fltS, 0.0f }, { fltR, 50.0f },
            { ampA, 0.5f }, { ampD, 130.0f }, { ampS, 0.0f }, { ampR, 30.0f },
            { seqOn, 1 }, { seqMeter, m44 }, { seqRate, 1 }, { seqSteps, 16.0f }, { seqGate, 100.0f }, { seqFreeCycle, 0 },
            { slot (0, "Source"), macro1 }, { slot (0, "Dest"), toCutoff }, { slot (0, "Amount"), 0.5f } },
          "Sequence",
          // The kick has the first sixteenth of every beat to itself.
          { { 0, 1.0f, 0.0f, false }, { 0, 0.9f, 0.6f, false }, { 0, 0.8f, 0.6f, false }, { 0, 0.9f, 0.6f, false },
            { 0, 1.0f, 0.0f, false }, { 0, 0.9f, 0.6f, false }, { 0, 0.8f, 0.6f, false }, { 0, 0.9f, 0.6f, false },
            { 0, 1.0f, 0.0f, false }, { 0, 0.9f, 0.6f, false }, { 0, 0.8f, 0.6f, false }, { 0, 0.9f, 0.6f, false },
            { 0, 1.0f, 0.0f, false }, { 0, 0.9f, 0.6f, false }, { 1, 0.8f, 0.6f, false }, { 0, 0.9f, 0.6f, false } } },

        { "FM Growl Bass", {
            { level, -8.0f },
            { engineLevel, 0.0f }, { oscATable, basic }, { oscAPos, 0.0f }, { oscALevel, 0.9f }, { oscAPitch, -12.0f },
            { oscBTable, basic }, { oscBPos, 0.0f }, { oscBLevel, 0.0f }, { oscBPitch, 0.0f },
            { oscAXmodType, fm }, { oscAXmodSource, fromOtherOsc }, { oscAXmodAmount, 0.25f }, { subLevel, 0.3f },
            { vibDepth, 0.0f }, { glide, 35.0f }, { cutoff, 1400.0f }, { resonance, 0.25f }, { filterEnv, 0.3f }, { fltD, 260.0f }, { fltS, 0.2f },
            { lfo (0, "Sync"), 1 }, { lfo (0, "Division"), 7 }, { lfo (0, "Shape"), triangle },
            { slot (0, "Source"), fltEnv }, { slot (0, "Dest"), mod::dstOscAFm }, { slot (0, "Amount"), 0.45f },
            { slot (1, "Source"), lfo1 }, { slot (1, "Dest"), mod::dstOscAFm }, { slot (1, "Amount"), 0.2f },
            { slot (2, "Source"), wheel }, { slot (2, "Dest"), mod::dstOscAFm }, { slot (2, "Amount"), 0.4f },
            { distOn, 1 }, { distDrive, 6.0f }, { distMix, 35.0f } }, "Bass" },

        { "Sync Warp Bass", {
            { level, -8.0f },
            { engineLevel, 0.0f }, { oscATable, basic }, { oscAPos, 0.66f }, { oscALevel, 0.9f }, { oscAPitch, -12.0f },
            { oscAWarp, warpSync }, { oscAWarpAmount, 0.15f }, { subLevel, 0.35f }, { vibDepth, 0.0f }, { glide, 20.0f },
            { cutoff, 1800.0f }, { resonance, 0.2f }, { filterEnv, 0.35f }, { fltD, 200.0f }, { fltS, 0.1f }, { ampR, 60.0f },
            { slot (0, "Source"), fltEnv }, { slot (0, "Dest"), mod::dstOscAWarp }, { slot (0, "Amount"), 0.55f },
            { slot (1, "Source"), macro1 }, { slot (1, "Dest"), mod::dstOscAWarp }, { slot (1, "Amount"), 0.5f } }, "Bass" },

        { "Ring Zurla Lead", {
            { level, -10.0f },
            { engineLevel, 0.0f }, { oscATable, zurla }, { oscAPos, 0.4f }, { oscALevel, 0.9f },
            { oscBTable, basic }, { oscBPos, 0.0f }, { oscBLevel, 0.0f }, { oscBPitch, 7.0f },
            { oscAXmodType, ring }, { oscAXmodSource, fromOtherOsc }, { oscAXmodAmount, 0.35f },
            { scale, hijazkar }, { snap, 1 }, { ornGrace, 1 }, { ornTrill, 1 }, { ornChance, 45.0f },
            { glide, 55.0f }, { vibDepth, 22.0f }, { vibRate, 6.2f }, { cutoff, 7000.0f }, { filterEnv, 0.1f }, { ampA, 12.0f },
            { slot (0, "Source"), wheel }, { slot (0, "Dest"), mod::dstOscAFm }, { slot (0, "Amount"), 0.6f },
            { slot (1, "Source"), velocity }, { slot (1, "Dest"), toPosA }, { slot (1, "Amount"), 0.4f },
            { distOn, 1 }, { distDrive, 7.0f }, { distMix, 35.0f }, { delayOn, 1 }, { delayTime, 2 }, { delayMix, 22.0f }, { reverbOn, 1 }, { reverbMix, 18.0f } },
          "Lead" },

        { "Formant Vetar Lead", {
            { level, -8.0f },
            { model, juzni }, { tone, 0.7f }, { detune, 9.0f }, { sub, 0.1f },
            { scale, hijaz }, { snap, 1 }, { ornGrace, 1 }, { ornMordent, 1 }, { ornChance, 50.0f }, { glide, 70.0f }, { vibDepth, 26.0f },
            { filterType, formant }, { cutoff, 900.0f }, { resonance, 0.55f }, { filterEnv, 0.25f }, { fltA, 60.0f }, { fltD, 400.0f }, { fltS, 0.4f },
            { lfo (0, "Sync"), 1 }, { lfo (0, "Division"), 2 }, { lfo (0, "Shape"), custom },
            { slot (0, "Source"), lfo1 }, { slot (0, "Dest"), toCutoff }, { slot (0, "Amount"), 0.4f },
            { slot (1, "Source"), wheel }, { slot (1, "Dest"), toCutoff }, { slot (1, "Amount"), 0.5f },
            { delayOn, 1 }, { delayTime, 2 }, { delayMix, 24.0f }, { reverbOn, 1 }, { reverbMix, 20.0f } },
          "Lead", {},
          // A slow walk through the vowels and back.
          { { 0, { { 0.0f, -1.0f, 0.0f }, { 0.25f, 0.0f, 0.4f }, { 0.5f, 1.0f, 0.0f }, { 0.75f, 0.2f, -0.4f }, { 1.0f, -1.0f, 0.0f } } } } },

        { "Mirror PWM Lead", {
            { level, -11.0f },
            { engineLevel, 0.0f }, { oscATable, pwm }, { oscAPos, 0.4f }, { oscAUnison, 3.0f }, { oscADetune, 12.0f }, { oscALevel, 0.8f },
            { oscAWarp, warpMirror }, { oscAWarpAmount, 0.3f }, { oscBTable, basic }, { oscBPos, 0.66f }, { oscBPitch, -12.0f }, { oscBLevel, 0.35f },
            { scale, nikriz }, { snap, 1 }, { glide, 60.0f }, { vibDepth, 18.0f },
            { cutoff, 2600.0f }, { resonance, 0.35f }, { filterEnv, 0.35f }, { fltD, 300.0f },
            { lfo (2, "Rate"), 0.35f }, { lfo (2, "Shape"), triangle },
            { slot (0, "Source"), lfo3 }, { slot (0, "Dest"), mod::dstOscAWarp }, { slot (0, "Amount"), 0.4f },
            { slot (1, "Source"), wheel }, { slot (1, "Dest"), toCutoff }, { slot (1, "Amount"), 0.5f },
            { distOn, 1 }, { distDrive, 8.0f }, { distMix, 35.0f }, { delayOn, 1 }, { delayTime, 2 }, { delayMix, 26.0f }, { reverbOn, 1 }, { reverbMix, 16.0f } },
          "Lead" },

        { "Phaser Kaval Pad", {
            { level, -11.0f },
            { model, kaval }, { engineLevel, 0.55f }, { kavNoise, 0.5f }, { oscATable, reeds }, { oscAPos, 0.3f }, { oscAUnison, 5.0f },
            { oscADetune, 18.0f }, { oscALevel, 0.6f }, { oscAWidth, 0.8f },
            { scale, huseyni }, { snap, 1 }, { mono, 0 }, { vibDepth, 6.0f }, { glide, 0.0f },
            { cutoff, 3500.0f }, { filterEnv, 0.0f }, { ampA, 500.0f }, { ampS, 1.0f }, { ampR, 1400.0f },
            { filter2On, 1 }, { filter2Type, phaser }, { filter2Cutoff, 900.0f }, { filter2Res, 0.6f }, { filterRouting, serial },
            { lfo (1, "Rate"), 0.1f }, { slot (0, "Source"), lfo2 }, { slot (0, "Dest"), mod::dstCutoff2 }, { slot (0, "Amount"), 0.35f },
            { slot (1, "Source"), lfo2 }, { slot (1, "Dest"), toPosA }, { slot (1, "Amount"), 0.3f },
            { reverbOn, 1 }, { reverbSize, 0.9f }, { reverbMix, 40.0f } }, "Pad" },

        { "Two Filter Choir", {
            { level, -14.0f },
            { model, voice }, { voxVowel, 0.2f }, { voxBreath, 0.3f }, { engineLevel, 0.7f },
            { oscATable, vowels }, { oscAPos, 0.3f }, { oscAUnison, 5.0f }, { oscADetune, 14.0f }, { oscALevel, 0.6f }, { oscAWidth, 0.9f },
            { scale, kurdi }, { snap, 1 }, { mono, 0 }, { vibDepth, 10.0f }, { glide, 0.0f },
            { cutoff, 4200.0f }, { filterEnv, 0.0f }, { ampA, 350.0f }, { ampS, 1.0f }, { ampR, 1200.0f },
            { filter2On, 1 }, { filter2Type, formant }, { filter2Cutoff, 700.0f }, { filter2Res, 0.5f }, { filterRouting, parallel }, { routeOscA, filter2 },
            { lfo (1, "Rate"), 0.15f }, { slot (0, "Source"), lfo2 }, { slot (0, "Dest"), mod::dstCutoff2 }, { slot (0, "Amount"), 0.4f },
            { reverbOn, 1 }, { reverbSize, 0.85f }, { reverbMix, 38.0f } }, "Pad" },

        { "Env 3 Swell Pad", {
            { level, -11.0f },
            { engineLevel, 0.0f }, { oscATable, bells }, { oscAPos, 0.1f }, { oscAUnison, 5.0f }, { oscADetune, 16.0f }, { oscALevel, 0.7f }, { oscAWidth, 0.9f },
            { oscBTable, reeds }, { oscBPos, 0.2f }, { oscBPitch, -12.0f }, { oscBLevel, 0.4f },
            { scale, saba }, { snap, 1 }, { mono, 0 }, { vibDepth, 0.0f }, { glide, 0.0f },
            { cutoff, 900.0f }, { resonance, 0.2f }, { filterEnv, 0.0f }, { ampA, 300.0f }, { ampS, 1.0f }, { ampR, 1800.0f },
            { env3A, 2500.0f }, { env3D, 3000.0f }, { env3S, 0.5f }, { env3R, 1500.0f },
            { slot (0, "Source"), env3 }, { slot (0, "Dest"), toCutoff }, { slot (0, "Amount"), 0.45f },
            { slot (1, "Source"), env3 }, { slot (1, "Dest"), toPosA }, { slot (1, "Amount"), 0.6f },
            { slot (2, "Source"), env3 }, { slot (2, "Dest"), mod::dstReverbMix }, { slot (2, "Amount"), 0.3f },
            { reverbOn, 1 }, { reverbSize, 0.9f }, { reverbMix, 25.0f }, { delayOn, 1 }, { delayTime, 3 }, { delayMix, 18.0f } }, "Pad" },

        { "Pump Saw Pad", {
            { level, -16.0f },
            { engineLevel, 0.0f }, { oscATable, basic }, { oscAPos, 0.66f }, { oscAUnison, 7.0f }, { oscADetune, 22.0f }, { oscALevel, 0.8f }, { oscAWidth, 1.0f },
            { scale, hijaz }, { snap, 1 }, { mono, 0 }, { vibDepth, 0.0f }, { glide, 0.0f },
            { cutoff, 3200.0f }, { filterEnv, 0.0f }, { ampA, 40.0f }, { ampS, 1.0f }, { ampR, 500.0f },
            { lfo (0, "Sync"), 1 }, { lfo (0, "Division"), 5 }, { lfo (0, "Shape"), custom },
            { slot (0, "Source"), lfo1 }, { slot (0, "Dest"), toVolume }, { slot (0, "Amount"), 0.5f },
            { slot (1, "Source"), lfo1 }, { slot (1, "Dest"), toCutoff }, { slot (1, "Amount"), 0.2f },
            { reverbOn, 1 }, { reverbSize, 0.7f }, { reverbMix, 22.0f } },
          "Pad", {},
          // Ducked on the beat, back up before the next one.
          { { 0, { { 0.0f, -1.0f, 0.5f }, { 0.55f, 1.0f, 0.0f }, { 1.0f, 1.0f, 0.0f } } } } },

        { "Stairs Arp 7/8", {
            { level, -11.0f },
            { engineLevel, 0.0f }, { oscATable, pwm }, { oscAPos, 0.3f }, { oscALevel, 0.9f }, { subLevel, 0.2f },
            { scale, hijazkar }, { snap, 1 }, { vibDepth, 0.0f }, { glide, 0.0f },
            { cutoff, 900.0f }, { resonance, 0.5f }, { filterEnv, 0.3f }, { fltD, 110.0f }, { fltS, 0.0f }, { ampR, 80.0f },
            { arpOn, 1 }, { arpMeter, m78b }, { arpRate, 1 }, { arpMode, 0 }, { arpOctaves, 2.0f }, { arpGate, 50.0f }, { arpAccent, 70.0f },
            { lfo (0, "Sync"), 1 }, { lfo (0, "Division"), 1 }, { lfo (0, "Shape"), custom },
            { slot (0, "Source"), lfo1 }, { slot (0, "Dest"), toCutoff }, { slot (0, "Amount"), 0.4f },
            { delayOn, 1 }, { delayTime, 2 }, { delayMix, 24.0f }, { reverbOn, 1 }, { reverbMix, 14.0f } },
          "Arp", {},
          // Four steps up over two bars.
          { { 0, { { 0.0f, -1.0f, 0.0f }, { 0.25f, -1.0f, 0.0f }, { 0.25f, -0.33f, 0.0f }, { 0.5f, -0.33f, 0.0f }, { 0.5f, 0.33f, 0.0f },
                   { 0.75f, 0.33f, 0.0f }, { 0.75f, 1.0f, 0.0f }, { 1.0f, 1.0f, 0.0f } } } } },

        { "Gajde FM Drone", {
            { level, -10.0f },
            { model, gajde }, { engineLevel, 0.6f }, { gajDrone, 0.7f },
            { oscATable, basic }, { oscAPos, 0.0f }, { oscALevel, 0.5f }, { oscAPitch, -12.0f },
            { oscAXmodType, fm }, { oscAXmodSource, fromEngine }, { oscAXmodAmount, 0.3f },
            { scale, huseyni }, { snap, 1 }, { ornGrace, 1 }, { ornChance, 40.0f }, { glide, 40.0f }, { vibDepth, 0.0f },
            { cutoff, 5000.0f }, { filterEnv, 0.0f }, { ampA, 40.0f }, { ampS, 1.0f }, { ampR, 300.0f },
            { slot (0, "Source"), wheel }, { slot (0, "Dest"), mod::dstOscAFm }, { slot (0, "Amount"), 0.5f },
            { reverbOn, 1 }, { reverbSize, 0.6f }, { reverbMix, 20.0f } }, "Folk" },

        { "Tambura Tremolo AM", {
            { level, -13.0f },
            { model, tambura }, { tamDouble, 0.6f }, { engineLevel, 0.8f },
            { oscATable, bells }, { oscAPos, 0.2f }, { oscALevel, 0.45f }, { oscAPitch, 12.0f },
            { oscAXmodType, am }, { oscAXmodSource, fromEngine }, { oscAXmodAmount, 0.7f },
            { scale, harmonicMinor }, { snap, 1 }, { mono, 0 }, { vibDepth, 0.0f }, { glide, 0.0f },
            { cutoff, 7000.0f }, { filterEnv, 0.1f }, { ampA, 1.0f }, { ampD, 900.0f }, { ampS, 0.0f }, { ampR, 500.0f },
            { delayOn, 1 }, { delayTime, 2 }, { delayMix, 18.0f }, { reverbOn, 1 }, { reverbSize, 0.5f }, { reverbMix, 18.0f } }, "Folk" },

        { "Harmonika Chorus Wide", {
            { level, -19.0f },
            { model, accordion }, { accBellows, 0.85f }, { accMusette, 10.0f }, { accRegister, 0.6f }, { accClick, 0.3f },
            { mono, 0 }, { vibDepth, 0.0f }, { glide, 0.0f }, { cutoff, 10000.0f }, { filterEnv, 0.0f },
            { ampA, 15.0f }, { ampS, 1.0f }, { ampR, 120.0f }, { ornGrace, 1 }, { ornChance, 30.0f },
            { chorusOn, 1 }, { chorusRate, 0.5f }, { chorusDepth, 55.0f }, { chorusMix, 60.0f },
            { eqOn, 1 }, { eqLow, -3.0f }, { eqMid, 2.0f }, { eqMidFreq, 2200.0f }, { eqHigh, 3.0f },
            { reverbOn, 1 }, { reverbSize, 0.4f }, { reverbMix, 16.0f } }, "Folk" },

        { "Sargija Phaser", {
            { level, -11.0f },
            { model, sargija }, { sarPluck, 0.7f }, { sarDecay, 0.6f }, { sarDrone, 0.45f }, { sarTone, 0.75f },
            { scale, hijaz }, { snap, 1 }, { mono, 0 }, { vibDepth, 0.0f }, { glide, 0.0f }, { cutoff, 9000.0f }, { filterEnv, 0.0f },
            { phaserOn, 1 }, { phaserRate, 0.25f }, { phaserDepth, 75.0f }, { phaserFeedback, 60.0f }, { phaserMix, 55.0f },
            { delayOn, 1 }, { delayTime, 2 }, { delayMix, 20.0f }, { reverbOn, 1 }, { reverbSize, 0.55f }, { reverbMix, 20.0f } }, "Folk" },

        { "Flanger Acid 7/8 Seq", {
            { level, -10.0f },
            { engineLevel, 0.0f }, { oscATable, acid }, { oscAPos, 0.55f }, { oscALevel, 0.9f }, { subLevel, 0.2f },
            { scale, hijazkar }, { snap, 1 }, { vibDepth, 0.0f }, { glide, 35.0f },
            { cutoff, 800.0f }, { resonance, 0.6f }, { filterEnv, 0.5f }, { fltD, 140.0f }, { fltS, 0.0f }, { ampR, 70.0f },
            { seqOn, 1 }, { seqMeter, m78b }, { seqRate, 1 }, { seqSteps, 14.0f }, { seqGate, 100.0f },
            { flangerOn, 1 }, { flangerRate, 0.12f }, { flangerDepth, 70.0f }, { flangerFeedback, 70.0f }, { flangerMix, 55.0f },
            { compOn, 1 }, { compThreshold, -20.0f }, { compRatio, 4.0f }, { compMakeup, 4.0f },
            { distOn, 1 }, { distDrive, 9.0f }, { distMix, 50.0f }, { delayOn, 1 }, { delayTime, 2 }, { delayMix, 18.0f } },
          "Sequence",
          { { 0, 1.0f, 0.5f, false }, { 0, 0.5f, 0.4f, false }, { 4, 1.0f, 0.5f, true }, { 5, 0.6f, 0.4f, false }, { 7, 1.0f, 0.5f, false },
            { 5, 0.5f, 0.4f, false }, { 4, 0.6f, 0.4f, false }, { 0, 1.0f, 0.5f, false }, { 12, 0.6f, 0.4f, false }, { 1, 1.0f, 0.5f, true },
            { 0, 0.6f, 0.4f, false }, { -2, 1.0f, 0.6f, true }, { 0, 0.5f, 0.4f, false }, { 1, 0.6f, 0.4f, false } } },

        { "Crushed Kanun", {
            { level, -11.0f },
            { model, kanun }, { kanPluck, 0.7f }, { kanDecay, 0.6f }, { kanShimmer, 0.5f },
            { scale, nikriz }, { snap, 1 }, { mono, 0 }, { vibDepth, 0.0f }, { glide, 0.0f }, { cutoff, 12000.0f }, { filterEnv, 0.0f },
            { crushOn, 1 }, { crushBits, 6.0f }, { crushRate, 9000.0f }, { crushMix, 70.0f },
            { lfo (0, "Rate"), 0.3f }, { slot (0, "Source"), lfo1 }, { slot (0, "Dest"), mod::dstCrushRate }, { slot (0, "Amount"), 0.25f },
            { delayOn, 1 }, { delayTime, 2 }, { delayFeedback, 50.0f }, { delayMix, 26.0f }, { reverbOn, 1 }, { reverbMix, 20.0f } }, "Folk" },

        { "Punch Psy Bass", {
            { level, -9.0f },
            { engineLevel, 0.0f }, { oscATable, basic }, { oscAPos, 0.66f }, { oscALevel, 0.9f }, { oscAPitch, -12.0f }, { subLevel, 0.4f },
            { vibDepth, 0.0f }, { glide, 0.0f },
            { cutoff, 280.0f }, { resonance, 0.3f }, { filterEnv, 0.6f }, { fltA, 0.5f }, { fltD, 90.0f }, { fltS, 0.0f }, { fltR, 60.0f },
            { ampA, 0.5f }, { ampD, 150.0f }, { ampS, 0.0f }, { ampR, 40.0f },
            { compOn, 1 }, { compThreshold, -24.0f }, { compRatio, 6.0f }, { compAttack, 20.0f }, { compRelease, 80.0f }, { compMakeup, 6.0f },
            { eqOn, 1 }, { eqLow, 3.0f }, { eqMid, -4.0f }, { eqMidFreq, 400.0f }, { eqHigh, 2.0f } }, "Bass" },
        };

        // Init stays empty; every other preset gets its macros.
        for (size_t i = 1; i < list.size(); ++i)
            wirePresetMacros (list[i]);

        return list;
    }();

    return presets;
}

/** Sets every parameter to its default, then applies the preset's values. Tuning and MPE settings
    are kept. Message thread only. */
inline void applyPreset (juce::AudioProcessorValueTreeState& apvts, const Preset& preset)
{
    for (auto* p : apvts.processor.getParameters())
        if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (p))
            if (! isGlobalParameter (ranged->getParameterID()))
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
