#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

#include "Filter.h"
#include "Modulation.h"
#include "Sampler.h"
#include "Tuning.h"
#include "Wavetables.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

namespace amv
{
enum class Model
{
    juzniVetar = 0,
    accordion,
    gusle,
    frula,
    kaval,
    gajde,
    zurla,
    tambura,
    tapan,
    dvojnice,
    clarinet,
    truba,
    violin,
    sargija,
    kanun,
    darbuka,
    voice
};

/** Drums are struck again on every note, even in mono legato. */
inline bool isDrum (Model m) noexcept { return m == Model::tapan || m == Model::darbuka; }

enum class Ornament
{
    none = 0,
    grace,
    mordent,
    trill
};

/** One wavetable oscillator. */
struct OscParams
{
    int table = 0;
    float position = 0.0f;     // 0..1 through the frames of the table
    int unison = 1;            // 1..7 stacked copies
    float detuneCents = 18.0f; // spread of the outermost copies
    float pitch = 0.0f;        // semitones
    float fineCents = 0.0f;
    float level = 0.0f;
    float width = 0.0f;        // 0 = mono, 1 = unison copies spread fully left and right
    int warp = warp::off;      // how the phase is bent before the table is read
    float warpAmount = 0.0f;   // 0..1
};

/** Values read from the plugin parameters once per audio block. */
struct EngineParams
{
    Model model = Model::juzniVetar;
    float engineLevel = 1.0f;  // level of the Balkan model in the mixer

    // Wavetable oscillators, sub oscillator and noise, mixed with the model before the filter
    OscParams oscA, oscB;
    float subLevel = 0.0f;
    int subOctave = 1;         // octaves below the note: 1 or 2
    float subShape = 0.0f;     // 0 = sine, 1 = square
    float noiseLevel = 0.0f;
    float noiseColorHz = 6000.0f;
    SamplerParams sampler;

    // Modulation
    std::array<LfoParams, mod::numLfos> lfos;
    std::array<float, mod::numMacros> modMacros {};
    std::array<ModSlot, mod::numSlots> slots;

    // Juzni Vetar lead
    float tone = 0.55f;        // 0 = detuned saws, 1 = pulse
    float pulseWidth = 0.32f;
    float detuneCents = 9.0f;
    float sub = 0.0f;

    // Accordion
    float bellows = 0.75f;
    float musetteCents = 12.0f;
    float registerPos = 0.4f;
    float click = 0.3f;

    // The four controls of the selected model, for the models added after the accordion:
    //   gusle: bow, position, rasp, nasal
    //   frula: breath, overblow, noise, chiff
    //   kaval: breath, overblow, noise, throat
    //   gajde: drone level, drone tuning in semitones, bag pressure, edge
    //   zurla: pressure, bite, growl, nasal
    //   tambura: pluck, decay, double string, tone
    //   tapan: thump, snap, decay, ring
    //   dvojnice: breath, second pipe level, interval in semitones, beating
    //   clarinet: breath, reed, growl, body
    //   truba: blow, blare, rasp, mute
    //   violin: bow, position, scratch, body
    //   sargija: pluck, decay, drone string level, tone
    //   kanun: pluck, decay, shimmer, tremolo
    //   darbuka: strike (dum to tek), snap, decay, ring
    //   voice: vowel, breath, size, tension
    std::array<float, 4> macro { 0.7f, 0.4f, 0.3f, 0.5f };

    // Lowest and highest value of the four controls of the selected model, so modulation can
    // move them over their own range.
    std::array<std::array<float, 2>, 4> engineRange { { { 0.0f, 1.0f }, { 0.0f, 1.0f }, { 0.0f, 1.0f }, { 0.0f, 1.0f } } };

    // Performance
    float vibDepthCents = 28.0f;
    float vibRateHz = 6.2f;
    float vibDelaySec = 0.22f;
    float bendRange = 2.0f;    // semitones
    float tuneHz = 440.0f;     // frequency of the A above middle C
    bool mpe = true;           // channels 2 to 16 carry one note each, with their own bend, pressure and timbre
    float mpeBendRange = 48.0f; // semitones, per-note bend in MPE mode
    float glideSec = 0.045f;
    bool mono = true;

    // Scale
    int scaleRoot = 2;
    int scale = 0;
    bool snap = false;

    // Ornaments
    bool grace = false, mordent = false, trill = false, fall = false;
    float ornSpeedSec = 0.06f; // length of one ornament step
    float ornAmount = 0.5f;    // trill length and fall depth
    float ornChance = 1.0f;

    // Filter and envelopes
    // Imported wavetables of oscillator A and B, used when its table is set to "User". Not owned.
    std::array<const UserWavetable*, 2> userWave {};

    // Filter 2 and where each source goes. With filter 2 off everything runs through filter 1.
    enum FilterSource { routeEngine = 0, routeOscA, routeOscB, routeSub, routeNoise, routeSampler, numFilterSources };
    bool filter2On = false;
    bool filtersParallel = false; // off: filter 1 runs into filter 2; on: they sit side by side
    int filter2Type = filter::highPass12;
    float cutoff2Hz = 200.0f;
    float resonance2 = 0.2f;
    float filter2Env = 0.0f;
    std::array<int, numFilterSources> filterRoute {}; // 0 = filter 1, 1 = filter 2

    int filterType = filter::lowPass12;
    float cutoffHz = 3200.0f;
    float resonance = 0.25f;
    float filterEnv = 0.35f;   // -1..1, scaled to +/- 5 octaves
    juce::ADSR::Parameters amp { 0.004f, 0.25f, 0.8f, 0.16f };
    juce::ADSR::Parameters flt { 0.002f, 0.32f, 0.4f, 0.2f };
    juce::ADSR::Parameters env3 { 0.01f, 0.3f, 0.0f, 0.3f }; // a free envelope, only a modulation source
};

/** What the engine decides for a note before handing it to a voice. */
struct NoteEvent
{
    int note = 60;
    int channel = 1;           // MIDI channel, 1 to 16
    float pitch = 60.0f;       // tuned pitch as a fractional MIDI note
    float stepAbove = 1.0f;    // semitones up to the next scale degree
    float velocity = 1.0f;
    Ornament ornament = Ornament::none;
    bool fall = false;
};

/** One synth voice. The oscillator section depends on the selected model, everything after it
    (glide, vibrato, ornaments, filter, envelopes) is shared. */
class Voice
{
public:
    void prepare (double newSampleRate)
    {
        sampleRate = newSampleRate;
        ampEnv.setSampleRate (newSampleRate);
        fltEnv.setSampleRate (newSampleRate);
        env3.setSampleRate (newSampleRate);
        filter.prepare (newSampleRate);
        filter2.prepare (newSampleRate);

        // Plucked strings: long enough for the lowest note at this sample rate.
        size_t length = 1024;
        while ((double) length < newSampleRate / 14.0)
            length *= 2;
        for (auto& s : strings)
            s.buffer.assign (length, 0.0f);

        reset();
    }

    void reset()
    {
        ampEnv.reset();
        fltEnv.reset();
        env3.reset();
        filter.reset();
            filter2.reset();
        note = -1;
        held = false;
    }

    bool isActive() const noexcept { return ampEnv.isActive(); }
    bool isHeld() const noexcept { return held; }
    int getNote() const noexcept { return note; }
    int getChannel() const noexcept { return channel; }

    /** Where the sampler of this voice is reading, 0..1 through the sample. Returns how many positions. */
    int getSamplerPositions (const SamplerParams& p, float* out, int capacity) const noexcept
    {
        return samplerVoice.getPositions (p, out, capacity);
    }

    /** Writes this voice's own modulation sources, for the display. */
    void fillSources (float* src) const noexcept
    {
        src[mod::srcFilterEnv] = lastFilterEnv;
        src[mod::srcEnv3] = lastEnv3;
        src[mod::srcVelocity] = velocity;
        src[mod::srcPressure] = notePressure;
        src[mod::srcTimbre] = noteTimbre;
    }

    /** One of the four controls of the selected model, wherever that model keeps it. */
    static float& engineControl (EngineParams& p, int index) noexcept
    {
        if (p.model == Model::juzniVetar)
            return index == 0 ? p.tone : index == 1 ? p.pulseWidth : index == 2 ? p.detuneCents : p.sub;
        if (p.model == Model::accordion)
            return index == 0 ? p.bellows : index == 1 ? p.musetteCents : index == 2 ? p.registerPos : p.click;
        return p.macro[(size_t) index];
    }

    /** Per-note expression from the note's MIDI channel: bend in semitones, pressure 0..1 and
        timbre -1..1. Called before every chunk; the values glide, except right after a note starts. */
    void setExpression (float bendTarget, float pressureTarget, float timbreTarget, float coeff) noexcept
    {
        if (expressionSnap)
            coeff = 1.0f;

        noteBend += (bendTarget - noteBend) * coeff;
        notePressure += (pressureTarget - notePressure) * coeff;
        noteTimbre += (timbreTarget - noteTimbre) * coeff;
        expressionSnap = false;
    }
    juce::uint64 getAge() const noexcept { return age; }

    void start (const NoteEvent& e, juce::uint64 newAge, const EngineParams& p, bool glideFromCurrent)
    {
        const bool wasActive = isActive();
        setNote (e);
        velocity = e.velocity;
        age = newAge;
        expressionSnap = true;

        if (! (glideFromCurrent && wasActive))
            currentPitch = targetPitch;

        if (! wasActive)
        {
            filter.reset();
            filter2.reset();
            phases = { 0.0f, 0.37f, 0.71f, 0.13f, 0.52f };
            for (auto* osc : { &oscPhasesA, &oscPhasesB })
            {
                // The first copy always starts at zero so a single oscillator has a repeatable attack.
                for (auto& ph : *osc)
                    ph = random.nextFloat();
                (*osc)[0] = 0.0f;
            }
            subPhase = 0.0f;
            noiseLp = 0.0f;
            lastFilterEnv = 0.0f;
            lastEnv3 = 0.0f;
            modSnap = true;
            reedLp = 0.0f;
            clickLp = 0.0f;
            clickHp = 0.0f;
            slowNoise = 0.0f;
            fastNoise = 0.0f;
            for (auto& b : bands)
                b.reset();
        }

        // A new note plucks the strings and hits the drum again; a legato slide does not.
        for (auto& s : strings)
            s.clear();
        exciteLeft = juce::jmax (2, (int) ((float) sampleRate / midiToHz (targetPitch)));
        exciteLp = 0.0f;
        drumTime = 0.0f;
        if (! wasActive)
            drumPhase = { 0.0f, 0.0f };

        // A legato slide keeps the sample running, a new note starts it again.
        samplerVoice.start (p.sampler, (juce::uint32) random.nextInt());

        vibTime = 0.0f;
        clickEnv = 1.0f;
        ampEnv.setParameters (p.amp);
        fltEnv.setParameters (p.flt);
        env3.setParameters (p.env3);
        ampEnv.noteOn();
        fltEnv.noteOn();
        env3.noteOn();
    }

    /** Legato: move to a new pitch without retriggering the envelopes. */
    void slideTo (const NoteEvent& e)
    {
        setNote (e);
    }

    void release()
    {
        held = false;
        releaseTime = 0.0f;
        ampEnv.noteOff();
        fltEnv.noteOff();
        env3.noteOff();
    }

    /** Adds this voice to `out` and `outRight`. `globalBend` is the pitch wheel and tuning offset in semitones.
        `sources` holds the shared modulation sources (LFOs, mod wheel, macros) indexed by
        mod::Source, or is null when no modulation slot is in use. */
    void render (float* out, float* outRight, int numSamples, const EngineParams& base, float globalBend, const float* sources = nullptr)
    {
        if (! isActive())
            return;

        const float bend = globalBend + noteBend;

        // Apply the modulation matrix to a copy of the parameters for this chunk.
        EngineParams modulated;
        const EngineParams* current = &base;
        float pitchTarget = 0.0f, gainTarget = 1.0f;

        if (sources != nullptr)
        {
            float src[mod::numSources];
            std::copy (sources, sources + mod::numSources, src);
            src[mod::srcNone] = 0.0f;
            src[mod::srcFilterEnv] = lastFilterEnv;
            src[mod::srcEnv3] = lastEnv3;
            src[mod::srcVelocity] = velocity;
            src[mod::srcPressure] = notePressure;
            src[mod::srcTimbre] = noteTimbre;

            float dest[mod::numDests] {};
            for (const auto& slot : base.slots)
                if (slot.source > mod::srcNone && slot.source < mod::numSources && slot.dest >= 0 && slot.dest < mod::numDests)
                    dest[slot.dest] += mod::shaped (src[slot.source], slot.source, slot.polarity) * slot.amount;

            const auto unit = [] (float v) { return juce::jlimit (0.0f, 1.0f, v); };
            modulated = base;
            modulated.cutoffHz = juce::jlimit (20.0f, 20000.0f, base.cutoffHz * std::exp2 (dest[mod::dstCutoff] * 5.0f));
            modulated.resonance = unit (base.resonance + dest[mod::dstResonance]);
            modulated.cutoff2Hz = juce::jlimit (20.0f, 20000.0f, base.cutoff2Hz * std::exp2 (dest[mod::dstCutoff2] * 5.0f));
            modulated.resonance2 = unit (base.resonance2 + dest[mod::dstResonance2]);
            modulated.oscA.position = unit (base.oscA.position + dest[mod::dstOscAPosition]);
            modulated.oscB.position = unit (base.oscB.position + dest[mod::dstOscBPosition]);
            modulated.oscA.level = unit (base.oscA.level + dest[mod::dstOscALevel]);
            modulated.oscB.level = unit (base.oscB.level + dest[mod::dstOscBLevel]);
            modulated.engineLevel = unit (base.engineLevel + dest[mod::dstEngineLevel]);
            modulated.noiseLevel = unit (base.noiseLevel + dest[mod::dstNoiseLevel]);
            modulated.subLevel = unit (base.subLevel + dest[mod::dstSubLevel]);

            // The full amount moves a control across its whole range.
            const auto ranged = [] (float value, float amount, float lowest, float highest)
            {
                return juce::jlimit (lowest, highest, value + amount * (highest - lowest));
            };
            modulated.filterEnv = ranged (base.filterEnv, dest[mod::dstFilterEnv], -1.0f, 1.0f);
            modulated.vibDepthCents = ranged (base.vibDepthCents, dest[mod::dstVibrato], 0.0f, 100.0f);
            modulated.vibRateHz = ranged (base.vibRateHz, dest[mod::dstVibRate], 2.0f, 12.0f);
            modulated.oscA.detuneCents = ranged (base.oscA.detuneCents, dest[mod::dstOscADetune], 0.0f, 100.0f);
            modulated.oscB.detuneCents = ranged (base.oscB.detuneCents, dest[mod::dstOscBDetune], 0.0f, 100.0f);
            modulated.oscA.pitch = ranged (base.oscA.pitch, dest[mod::dstOscAPitch], -24.0f, 24.0f);
            modulated.oscB.pitch = ranged (base.oscB.pitch, dest[mod::dstOscBPitch], -24.0f, 24.0f);
            modulated.oscA.warpAmount = unit (base.oscA.warpAmount + dest[mod::dstOscAWarp]);
            modulated.oscB.warpAmount = unit (base.oscB.warpAmount + dest[mod::dstOscBWarp]);
            modulated.sampler.level = unit (base.sampler.level + dest[mod::dstSamplerLevel]);
            modulated.sampler.pitch = ranged (base.sampler.pitch, dest[mod::dstSamplerPitch], -24.0f, 24.0f);
            modulated.sampler.position = unit (base.sampler.position + dest[mod::dstGrainPosition]);
            modulated.sampler.grainMs = ranged (base.sampler.grainMs, dest[mod::dstGrainSize], 5.0f, 500.0f);
            modulated.sampler.density = ranged (base.sampler.density, dest[mod::dstGrainDensity], 1.0f, 100.0f);
            modulated.sampler.spray = unit (base.sampler.spray + dest[mod::dstGrainSpray]);
            modulated.sampler.scan = ranged (base.sampler.scan, dest[mod::dstGrainScan], -2.0f, 2.0f);

            // Envelope times move by octaves, so a modulation works the same on a short and a long time.
            const auto timed = [] (float seconds, float amount)
            {
                return juce::jlimit (0.0005f, 20.0f, seconds * std::exp2 (amount * mod::timeOctaves));
            };
            modulated.amp.attack = timed (base.amp.attack, dest[mod::dstAmpAttack]);
            modulated.amp.decay = timed (base.amp.decay, dest[mod::dstAmpDecay]);
            modulated.amp.release = timed (base.amp.release, dest[mod::dstAmpRelease]);
            modulated.flt.attack = timed (base.flt.attack, dest[mod::dstFltAttack]);
            modulated.flt.decay = timed (base.flt.decay, dest[mod::dstFltDecay]);
            modulated.flt.release = timed (base.flt.release, dest[mod::dstFltRelease]);

            for (int i = 0; i < 4; ++i)
                if (std::abs (dest[mod::dstEngine1 + i]) > 0.0f)
                {
                    float& control = engineControl (modulated, i);
                    control = ranged (control, dest[mod::dstEngine1 + i], base.engineRange[(size_t) i][0], base.engineRange[(size_t) i][1]);
                }
            current = &modulated;

            // Small amounts give fine pitch control, the full amount reaches two octaves.
            const float pm = juce::jlimit (-1.0f, 1.0f, dest[mod::dstPitch]);
            pitchTarget = pm * std::abs (pm) * 24.0f;
            gainTarget = juce::jlimit (0.0f, 2.0f, 1.0f + dest[mod::dstVolume]);
        }

        if (modSnap)
        {
            modPitch = pitchTarget;
            modGain = gainTarget;
            modSnap = false;
        }

        // Pitch and volume glide to their new values over the chunk, so fast LFOs do not click.
        const float pitchStep = (pitchTarget - modPitch) / (float) numSamples;
        const float gainStep = (gainTarget - modGain) / (float) numSamples;
        const EngineParams& p = *current;

        ampEnv.setParameters (p.amp);
        fltEnv.setParameters (p.flt);
        env3.setParameters (p.env3);

        const auto sr = (float) sampleRate;
        const float dt = 1.0f / sr;
        const float glideCoeff = p.glideSec > 0.0005f ? 1.0f - std::exp (-1.0f / (p.glideSec * sr)) : 1.0f;
        const float vibInc = p.vibRateHz / sr;

        // The filter is updated once per render call; the engine calls us in short chunks.
        const float envNow = fltEnv.getNextSample();
        lastFilterEnv = envNow;
        lastEnv3 = env3.getNextSample();
        for (int i = 1; i < numSamples; ++i)
        {
            fltEnv.getNextSample();
            env3.getNextSample();
        }

        const float keyTrack = (currentPitch - 60.0f) / 12.0f * 0.35f;
        const float cutoff = juce::jlimit (20.0f, 0.45f * sr,
                                           p.cutoffHz * std::exp2 (p.filterEnv * 5.0f * envNow + keyTrack));
        filter.set (p.filterType, cutoff, p.resonance);

        const bool twoFilters = p.filter2On;
        if (twoFilters)
            filter2.set (p.filter2Type, juce::jlimit (20.0f, 0.45f * sr, p.cutoff2Hz * std::exp2 (p.filter2Env * 5.0f * envNow + keyTrack)), p.resonance2);

        // The bus each source feeds: 0 is filter 1, 1 is filter 2.
        std::array<size_t, EngineParams::numFilterSources> route {};
        if (twoFilters)
            for (size_t r = 0; r < route.size(); ++r)
                route[r] = p.filterRoute[r] != 0 ? 1u : 0u;

        ModelCoeffs mc;
        if (p.engineLevel > 0.0f)
            prepareModel (mc, p, sr);

        const float chunkInc = midiToHz (currentPitch + bend + juce::jmax (modPitch, pitchTarget) + 1.0f) / sr;
        OscCoeffs oscA, oscB;
        prepareOsc (oscA, p.oscA, chunkInc, p.userWave[0]);
        prepareOsc (oscB, p.oscB, chunkInc, p.userWave[1]);

        const bool subOn = p.subLevel > 0.0f, noiseOn = p.noiseLevel > 0.0f;
        const bool extras = oscA.on || oscB.on || subOn;

        // The sampler follows the key from its root note, or plays every key at the sample's own pitch.
        const bool samplerOn = p.sampler.level > 0.0f && p.sampler.data != nullptr;
        const float samplerTune = std::exp2 ((p.sampler.pitch + p.sampler.fineCents * 0.01f) / 12.0f);
        const float samplerRootHz = midiToHz ((float) p.sampler.rootNote);

        // Only spread unison and the sampler need a second filter; everything else is the same on both sides.
        const bool wide = (oscA.on && oscA.wide) || (oscB.on && oscB.wide) || samplerOn;
        const float subRatio = p.subOctave >= 2 ? 0.25f : 0.5f;
        const float noiseCoeff = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * juce::jmin (p.noiseColorHz, 0.45f * sr) / sr);

        const float fallDepth = fall ? p.ornAmount * 3.0f : 0.0f;
        const float fallTime = juce::jlimit (0.04f, 0.4f, p.amp.release);

        for (int i = 0; i < numSamples; ++i)
        {
            currentPitch += (targetPitch - currentPitch) * glideCoeff;

            const float fade = p.vibDelaySec > 0.001f ? juce::jlimit (0.0f, 1.0f, vibTime / p.vibDelaySec) : 1.0f;
            const float vib = std::sin (vibPhase * juce::MathConstants<float>::twoPi) * p.vibDepthCents * 0.01f * fade * fade;
            vibPhase += vibInc;
            if (vibPhase >= 1.0f)
                vibPhase -= 1.0f;
            vibTime += dt;

            modPitch += pitchStep;
            modGain += gainStep;
            float pitch = currentPitch + bend + vib + ornamentOffset (p) + modPitch;
            ornTime += dt;

            if (! held)
            {
                pitch -= fallDepth * juce::jmin (1.0f, releaseTime / fallTime);
                releaseTime += dt;
            }

            float s = 0.0f;
            float bus[2][2] {}; // [filter][left, right]
            if (p.engineLevel > 0.0f)
            {
                switch (p.model)
                {
                    case Model::juzniVetar: s = renderLead (pitch, sr, p, mc); break;
                    case Model::accordion:  s = renderAccordion (pitch, sr, mc); break;
                    case Model::gusle:      s = renderGusle (pitch, sr, p, mc); break;
                    case Model::frula:      s = renderPipe (pitch, sr, p, mc, false); break;
                    case Model::kaval:      s = renderPipe (pitch, sr, p, mc, true); break;
                    case Model::gajde:      s = renderGajde (pitch, sr, p, mc); break;
                    case Model::zurla:      s = renderZurla (pitch, sr, p, mc); break;
                    case Model::tambura:    s = renderTambura (pitch, sr, p, mc); break;
                    case Model::tapan:      s = renderTapan (pitch, sr, p, mc); break;
                    case Model::dvojnice:   s = renderDvojnice (pitch, sr, p, mc); break;
                    case Model::clarinet:   s = renderClarinet (pitch, sr, p, mc); break;
                    case Model::truba:      s = renderTruba (pitch, sr, p, mc); break;
                    case Model::violin:     s = renderViolin (pitch, sr, p, mc); break;
                    case Model::sargija:    s = renderSargija (pitch, sr, p, mc); break;
                    case Model::kanun:      s = renderKanun (pitch, sr, p, mc); break;
                    case Model::darbuka:    s = renderDarbuka (pitch, sr, p, mc); break;
                    case Model::voice:      s = renderVoice (pitch, sr, p, mc); break;
                }

                s *= p.engineLevel;
                bus[route[EngineParams::routeEngine]][0] += s;
                bus[route[EngineParams::routeEngine]][1] += s;
            }

            if (extras)
            {
                const float inc = midiToHz (pitch) / sr;

                if (oscA.on)
                    renderOsc (oscPhasesA, oscA, inc, bus[route[EngineParams::routeOscA]][0], bus[route[EngineParams::routeOscA]][1]);
                if (oscB.on)
                    renderOsc (oscPhasesB, oscB, inc, bus[route[EngineParams::routeOscB]][0], bus[route[EngineParams::routeOscB]][1]);

                if (subOn)
                {
                    const float subInc = inc * subRatio;
                    const float sine = std::sin (subPhase * juce::MathConstants<float>::twoPi);
                    const float sub = (sine + (pulse (subPhase, subInc, 0.5f) * 0.7f - sine) * p.subShape) * p.subLevel * 0.8f;
                    bus[route[EngineParams::routeSub]][0] += sub;
                    bus[route[EngineParams::routeSub]][1] += sub;
                    advance (subPhase, subInc);
                }
            }

            if (samplerOn)
            {
                // Without key tracking only bends, vibrato and modulation move the pitch.
                const float ratio = p.sampler.keyTrack ? midiToHz (pitch) / samplerRootHz : std::exp2 ((pitch - currentPitch) / 12.0f);
                samplerVoice.process (p.sampler, ratio * samplerTune, sr, bus[route[EngineParams::routeSampler]][0], bus[route[EngineParams::routeSampler]][1]);
            }

            if (noiseOn)
            {
                noiseLp += (noise() - noiseLp) * noiseCoeff;
                const float hiss = noiseLp * p.noiseLevel * 0.7f;
                bus[route[EngineParams::routeNoise]][0] += hiss;
                bus[route[EngineParams::routeNoise]][1] += hiss;
            }

            const float gain = ampEnv.getNextSample() * (0.35f + 0.65f * velocity) * modGain;
            float left = filter.processSample (0, bus[0][0]);
            float right = wide ? filter.processSample (1, bus[0][1]) : left;

            if (twoFilters)
            {
                // In series what leaves filter 1 joins the sources of filter 2; in parallel the two outputs add.
                const float out2 = filter2.processSample (0, p.filtersParallel ? bus[1][0] : left + bus[1][0]);
                const float out2Right = wide ? filter2.processSample (1, p.filtersParallel ? bus[1][1] : right + bus[1][1]) : out2;
                left = p.filtersParallel ? left + out2 : out2;
                right = p.filtersParallel ? right + out2Right : out2Right;
            }

            out[i] += left * gain;
            outRight[i] += right * gain;
        }

        if (! isActive())
            note = -1;
    }

private:
    /** A band-pass resonator (state variable filter) with unity gain at its centre. */
    struct Band
    {
        void set (float freq, float q, float sr) noexcept
        {
            const float g = std::tan (juce::MathConstants<float>::pi * juce::jlimit (20.0f, 0.45f * sr, freq) / sr);
            k = 1.0f / q;
            a1 = 1.0f / (1.0f + g * (g + k));
            a2 = g * a1;
            a3 = g * a2;
        }

        float process (float x) noexcept
        {
            const float v3 = x - ic2;
            const float v1 = a1 * ic1 + a2 * v3;
            const float v2 = ic2 + a2 * ic1 + a3 * v3;
            ic1 = 2.0f * v1 - ic1;
            ic2 = 2.0f * v2 - ic2;
            return v1 * k;
        }

        void reset() noexcept { ic1 = ic2 = 0.0f; }

        float k = 1.0f, a1 = 0.0f, a2 = 0.0f, a3 = 0.0f, ic1 = 0.0f, ic2 = 0.0f;
    };

    /** A plucked string: a delay line with a low-pass in its feedback path. */
    struct StringLine
    {
        void clear() noexcept
        {
            std::fill (buffer.begin(), buffer.end(), 0.0f);
            lp = 0.0f;
        }

        float process (float delay, float input, float lpCoeff, float loss) noexcept
        {
            const auto size = buffer.size();
            if (size == 0)
                return 0.0f;

            delay = juce::jlimit (2.0f, (float) size - 2.0f, delay);
            const int whole = (int) delay;
            const float frac = delay - (float) whole;
            const size_t mask = size - 1;
            const float a = buffer[(write + size - (size_t) whole) & mask];
            const float b = buffer[(write + size - (size_t) whole - 1) & mask];
            const float out = a + (b - a) * frac;

            lp += (out - lp) * lpCoeff;
            buffer[write] = input + lp * loss;
            write = (write + 1) & mask;
            return out;
        }

        std::vector<float> buffer;
        size_t write = 0;
        float lp = 0.0f;
    };

    struct ModelCoeffs
    {
        float loss = 0.0f;

        // shared by the newer models
        float level = 1.0f, brightCoeff = 1.0f, slowCoeff = 0.0f, fastCoeff = 0.0f;

        // lead
        float sawGain = 1.0f, pulseGain = 0.0f, detune = 0.0f;
        // accordion
        float gains[5] {};     // low, mid, mid sharp, mid flat, high
        float musette = 0.0f, reedLpCoeff = 1.0f, reedLevel = 1.0f, clickLevel = 0.0f, clickDecay = 0.0f;
    };

    static constexpr int maxUnison = 7;
    using OscPhases = std::array<float, maxUnison>;

    struct OscCoeffs
    {
        bool on = false;
        int count = 1;
        const float* waveA = nullptr; // the two frames around the position
        const float* waveB = nullptr;
        float frameMix = 0.0f, gain = 0.0f;
        float ratio[maxUnison] {};    // frequency of each copy relative to the note
        float panLeft[maxUnison] {}, panRight[maxUnison] {};
        bool wide = false;
        int warp = warp::off;
        float warpValue = 0.0f;       // exponent, width or ratio, depending on the mode
    };

    /** `noteInc` is the phase increment of the note itself, with a little headroom. */
    static void prepareOsc (OscCoeffs& oc, const OscParams& op, float noteInc, const UserWavetable* user)
    {
        oc.on = op.level > 0.0f;
        if (! oc.on)
            return;

        oc.count = juce::jlimit (1, maxUnison, op.unison);
        const float offset = op.pitch + op.fineCents * 0.01f;
        float highest = 0.0f;

        for (int i = 0; i < oc.count; ++i)
        {
            const float spread = oc.count > 1 ? 2.0f * (float) i / (float) (oc.count - 1) - 1.0f : 0.0f;
            oc.ratio[i] = std::exp2 ((offset + spread * op.detuneCents * 0.01f) / 12.0f);
            highest = juce::jmax (highest, oc.ratio[i]);

            // Neighbouring copies go to opposite sides, the outer ones furthest out. Equal power,
            // scaled so a centred copy has a gain of one on each side.
            const float pan = ((i & 1) == 0 ? -1.0f : 1.0f) * std::abs (spread) * juce::jlimit (0.0f, 1.0f, op.width);
            const float angle = (pan + 1.0f) * juce::MathConstants<float>::pi * 0.25f;
            oc.panLeft[i] = std::cos (angle) * juce::MathConstants<float>::sqrt2;
            oc.panRight[i] = std::sin (angle) * juce::MathConstants<float>::sqrt2;
        }

        oc.wide = oc.count > 1 && op.width > 0.001f;

        // Warping squeezes the wave, which raises its harmonics; `steepest` is by how much at most,
        // so a table with fewer harmonics is picked.
        const float amount = juce::jlimit (0.0f, 1.0f, op.warpAmount);
        float steepest = 1.0f;
        oc.warp = amount > 0.0005f ? juce::jlimit (0, (int) warp::numModes - 1, op.warp) : (int) warp::off;
        switch (oc.warp)
        {
            case warp::bendUp:
            case warp::bendDown: oc.warpValue = std::exp2 (2.0f * amount); steepest = oc.warpValue; break;
            case warp::pwm:      oc.warpValue = 1.0f - 0.9f * amount; steepest = 1.0f / oc.warpValue; break;
            case warp::sync:     oc.warpValue = 1.0f + 7.0f * amount; steepest = oc.warpValue; break;
            case warp::mirror:   oc.warpValue = amount; steepest = 1.0f + amount; break;
            default: break;
        }

        const auto& bank = WavetableBank::get();
        const int level = WavetableBank::levelFor (noteInc * highest * steepest);
        const float framePos = juce::jlimit (0.0f, 1.0f, op.position) * (float) (WavetableBank::numFrames - 1);
        const int frame = juce::jmin ((int) framePos, WavetableBank::numFrames - 2);

        // The table after the built-in ones is the imported one; without an import it is a sine.
        if (op.table >= WavetableBank::numTables && user != nullptr)
        {
            oc.waveA = user->wave (frame, level);
            oc.waveB = user->wave (frame + 1, level);
        }
        else
        {
            const int table = op.table >= WavetableBank::numTables ? 0 : juce::jmax (0, op.table);
            const int builtInFrame = op.table >= WavetableBank::numTables ? 0 : frame;
            oc.waveA = bank.wave (table, builtInFrame, level);
            oc.waveB = bank.wave (table, op.table >= WavetableBank::numTables ? 0 : frame + 1, level);
        }

        oc.frameMix = framePos - (float) frame;
        oc.gain = op.level * 0.75f / std::sqrt ((float) oc.count);
    }

    /** Where in the table a warped oscillator reads for a phase of 0..1. `window` is a gain that
        fades the end of the cycle where the mode would otherwise jump. */
    static float warpPhase (float phase, int mode, float value, float& window) noexcept
    {
        switch (mode)
        {
            case warp::bendUp:   return std::pow (phase, value);                 // the wave is pushed towards the end
            case warp::bendDown: return 1.0f - std::pow (1.0f - phase, value);   // and towards the start
            case warp::pwm:      return juce::jmin (1.0f, phase / value);        // squeezed, then held
            case warp::mirror:   return phase + ((phase < 0.5f ? 2.0f * phase : 2.0f - 2.0f * phase) - phase) * value;
            case warp::sync:
            {
                // The wave restarts with every cycle of the note, like a synced oscillator.
                const float p2 = phase * phase, p4 = p2 * p2;
                window = 1.0f - p4 * p4;
                const float fast = phase * value;
                return fast - std::floor (fast);
            }
            default: return phase;
        }
    }

    static void renderOsc (OscPhases& phases, const OscCoeffs& oc, float inc, float& left, float& right) noexcept
    {
        float sumLeft = 0.0f, sumRight = 0.0f;

        for (int i = 0; i < oc.count; ++i)
        {
            float& phase = phases[(size_t) i];
            float window = 1.0f;
            const float read = oc.warp == warp::off ? phase : warpPhase (phase, oc.warp, oc.warpValue, window);
            const float pos = read * (float) WavetableBank::tableSize;
            const int index = juce::jlimit (0, WavetableBank::tableSize - 1, (int) pos);
            const float frac = pos - (float) index;
            const float a = oc.waveA[index] + (oc.waveA[index + 1] - oc.waveA[index]) * frac;
            const float b = oc.waveB[index] + (oc.waveB[index + 1] - oc.waveB[index]) * frac;
            const float v = (a + (b - a) * oc.frameMix) * window;
            sumLeft += v * oc.panLeft[i];
            sumRight += v * oc.panRight[i];

            phase += inc * oc.ratio[i];
            if (phase >= 1.0f)
                phase -= 1.0f;
        }

        left += sumLeft * oc.gain;
        right += sumRight * oc.gain;
    }

    void setNote (const NoteEvent& e)
    {
        note = e.note;
        channel = juce::jlimit (1, 16, e.channel);
        targetPitch = e.pitch;
        stepAbove = e.stepAbove;
        ornament = e.ornament;
        fall = e.fall;
        ornTime = 0.0f;
        held = true;
    }

    float ornamentOffset (const EngineParams& p) const noexcept
    {
        if (ornament == Ornament::none)
            return 0.0f;

        const float step = juce::jmax (0.01f, p.ornSpeedSec);
        const int n = (int) (ornTime / step);

        switch (ornament)
        {
            case Ornament::grace:   return n < 1 ? stepAbove : 0.0f;
            case Ornament::mordent: return n == 1 ? stepAbove : 0.0f;
            case Ornament::trill:   return (n < 2 * (2 + juce::roundToInt (p.ornAmount * 4.0f)) && (n & 1) == 1) ? stepAbove : 0.0f;
            case Ornament::none:    break;
        }

        return 0.0f;
    }

    void prepareModel (ModelCoeffs& mc, const EngineParams& p, float sr)
    {
        mc.sawGain = std::cos (p.tone * juce::MathConstants<float>::halfPi);
        mc.pulseGain = std::sin (p.tone * juce::MathConstants<float>::halfPi);
        mc.detune = p.detuneCents * 0.01f;

        const auto onePole = [sr] (float hz) { return 1.0f - std::exp (-juce::MathConstants<float>::twoPi * juce::jmin (hz, 0.45f * sr) / sr); };
        const float hz = midiToHz (currentPitch);

        switch (p.model)
        {
            case Model::gusle:
            {
                const float bow = 0.2f + 0.8f * p.macro[0];
                mc.level = bow * 1.6f;
                mc.brightCoeff = onePole (1100.0f * std::exp2 (bow * 3.0f));
                mc.slowCoeff = onePole (18.0f);
                mc.fastCoeff = onePole (2200.0f);
                bands[0].set (430.0f, 4.0f, sr);
                bands[1].set (1150.0f, 6.0f, sr);
                bands[2].set (2500.0f, 5.0f, sr);
                return;
            }

            case Model::frula:
            case Model::kaval:
            {
                const bool isKaval = p.model == Model::kaval;
                const float breath = 0.25f + 0.75f * p.macro[0];
                mc.level = breath * 1.8f;
                mc.slowCoeff = onePole (9.0f);
                mc.clickDecay = std::exp (-1.0f / (0.03f * sr));
                bands[0].set (hz * (isKaval ? 1.5f : 2.0f), isKaval ? 1.6f : 3.0f, sr); // breath noise around the tone
                bands[1].set (hz * 4.0f, 2.0f, sr);                                       // chiff at the attack
                return;
            }

            case Model::gajde:
            {
                const float bag = 0.3f + 0.7f * p.macro[2];
                mc.level = bag * 1.1f;
                mc.slowCoeff = onePole (2.5f);
                mc.brightCoeff = onePole (1500.0f * std::exp2 (p.macro[3] * 2.6f));
                bands[0].set (1400.0f, 3.0f, sr);
                bands[1].set (2900.0f, 4.0f, sr);
                return;
            }

            case Model::zurla:
            {
                const float pressure = 0.3f + 0.7f * p.macro[0];
                mc.level = pressure * 1.25f;
                mc.brightCoeff = onePole (2200.0f * std::exp2 (pressure * 2.4f));
                mc.slowCoeff = onePole (3.0f);
                mc.fastCoeff = onePole (55.0f);
                bands[0].set (1250.0f, 3.5f, sr);
                bands[1].set (2700.0f, 4.5f, sr);
                bands[2].set (4300.0f, 5.0f, sr);
                return;
            }

            case Model::tambura:
            {
                // Seconds for the string to die away, and the loss per trip round the string that gives it.
                const float t60 = 0.35f * std::pow (25.0f, p.macro[1]);
                mc.loss = std::pow (10.0f, -3.0f / (juce::jmax (20.0f, hz) * t60));
                mc.brightCoeff = 0.3f + 0.65f * p.macro[3];                       // damping inside the string loop
                mc.fastCoeff = onePole (700.0f * std::exp2 (p.macro[0] * 4.0f));  // pick hardness
                mc.detune = p.macro[2] * 0.14f;
                mc.slowCoeff = onePole (25.0f);
                mc.level = 0.9f;
                bands[0].set (310.0f, 3.0f, sr);
                bands[1].set (1900.0f, 2.5f, sr);
                return;
            }

            case Model::tapan:
            {
                mc.level = 0.9f;
                mc.loss = std::exp (-1.0f / ((0.08f + 0.9f * p.macro[2] * p.macro[2]) * sr)); // decay of the skin
                mc.clickDecay = std::exp (-1.0f / (0.035f * sr));                             // decay of the stick crack
                mc.slowCoeff = std::exp (-1.0f / ((0.012f + 0.03f * p.macro[0]) * sr));       // how fast the pitch drops
                bands[0].set (2400.0f, 1.2f, sr);
                bands[1].set (5200.0f, 2.0f, sr);
                return;
            }

            case Model::dvojnice:
            {
                const float breath = 0.25f + 0.75f * p.macro[0];
                mc.level = breath * 1.5f;
                mc.slowCoeff = onePole (7.0f);
                bands[0].set (hz * 2.0f, 2.5f, sr);
                return;
            }

            case Model::clarinet:
            {
                const float breath = 0.25f + 0.75f * p.macro[0];
                mc.level = breath * 1.3f;
                mc.brightCoeff = onePole (700.0f * std::exp2 (breath * 3.2f));
                mc.slowCoeff = onePole (4.0f);
                mc.fastCoeff = onePole (45.0f);
                bands[0].set (1500.0f, 2.5f, sr);
                bands[1].set (3100.0f, 3.5f, sr);
                return;
            }

            case Model::truba:
            {
                mc.level = (0.3f + 0.7f * p.macro[0]) * 1.4f;
                mc.slowCoeff = onePole (5.0f);
                mc.fastCoeff = onePole (70.0f);
                mc.clickDecay = std::exp (-1.0f / (0.07f * sr)); // how long the attack stays extra bright
                bands[0].set (1250.0f, 2.0f, sr);                // the bell
                bands[1].set (1900.0f, 5.0f, sr);                // the mute
                return;
            }

            case Model::violin:
            {
                const float bow = 0.2f + 0.8f * p.macro[0];
                mc.level = bow * 1.5f;
                mc.brightCoeff = onePole (1600.0f * std::exp2 (bow * 2.6f));
                mc.slowCoeff = onePole (12.0f);
                mc.fastCoeff = onePole (3000.0f);
                bands[0].set (290.0f, 5.0f, sr);
                bands[1].set (520.0f, 4.0f, sr);
                bands[2].set (2900.0f, 2.5f, sr);
                return;
            }

            case Model::sargija:
            case Model::kanun:
            {
                const bool isKanun = p.model == Model::kanun;
                const float t60 = (isKanun ? 0.6f : 0.3f) * std::pow (isKanun ? 20.0f : 18.0f, p.macro[1]);
                mc.loss = std::pow (10.0f, -3.0f / (juce::jmax (20.0f, hz) * t60));
                mc.brightCoeff = isKanun ? 0.75f : 0.35f + 0.6f * p.macro[3];
                mc.fastCoeff = onePole ((isKanun ? 1500.0f : 900.0f) * std::exp2 (p.macro[0] * 3.5f));
                mc.detune = isKanun ? p.macro[2] * 0.1f : 0.0f;
                mc.slowCoeff = onePole (25.0f);
                mc.level = isKanun ? 0.65f : 0.95f;
                bands[0].set (isKanun ? 520.0f : 420.0f, 2.5f, sr);
                bands[1].set (isKanun ? 3200.0f : 2400.0f, 2.0f, sr);
                return;
            }

            case Model::darbuka:
            {
                mc.level = 0.9f;
                mc.loss = std::exp (-1.0f / ((0.03f + 0.35f * p.macro[2] * p.macro[2]) * sr));
                mc.clickDecay = std::exp (-1.0f / (0.012f * sr));
                mc.slowCoeff = std::exp (-1.0f / (0.008f * sr));
                bands[0].set (3500.0f, 1.5f, sr);
                bands[1].set (7500.0f, 2.0f, sr);
                return;
            }

            case Model::voice:
            {
                // A E I O U, three formants each, for a voice of average size.
                static const float vowels[5][3] { { 730.0f, 1090.0f, 2440.0f }, { 530.0f, 1840.0f, 2480.0f }, { 270.0f, 2290.0f, 3010.0f },
                                                  { 570.0f, 840.0f, 2410.0f }, { 300.0f, 870.0f, 2240.0f } };
                const float x = juce::jlimit (0.0f, 1.0f, p.macro[0]) * 4.0f;
                const int seg = juce::jmin (3, (int) x);
                const float f = x - (float) seg;
                const float size = std::exp2 ((0.5f - p.macro[2]) * 0.8f); // small voices have higher formants
                const float qs[3] { 7.0f, 9.0f, 10.0f };

                for (size_t k = 0; k < 3; ++k)
                    bands[k].set ((vowels[seg][k] + (vowels[seg + 1][k] - vowels[seg][k]) * f) * size, qs[k], sr);

                mc.level = 6.0f;
                mc.brightCoeff = onePole (900.0f * std::exp2 (p.macro[3] * 3.0f));
                mc.slowCoeff = onePole (4.0f);
                mc.fastCoeff = onePole (2500.0f);
                return;
            }

            case Model::juzniVetar:
                return;

            case Model::accordion:
                break;
        }

        // The register knob sweeps through the classic reed combinations:
        // single reed, two detuned reeds, with the low octave, full musette, then the high reed on top.
        const float r = p.registerPos;
        const auto ramp = [r] (float from, float to) { return juce::jlimit (0.0f, 1.0f, (r - from) / (to - from)); };
        mc.gains[1] = 1.0f;
        mc.gains[2] = ramp (0.05f, 0.25f);
        mc.gains[0] = ramp (0.3f, 0.5f);
        mc.gains[3] = ramp (0.55f, 0.75f);
        mc.gains[4] = ramp (0.8f, 1.0f) * 0.7f;

        float power = 0.0f;
        for (float g : mc.gains)
            power += g * g;

        const float pressure = 0.15f + 0.85f * p.bellows;
        mc.reedLevel = pressure * std::sqrt (pressure) / std::sqrt (power) * 1.5f;
        mc.musette = p.musetteCents * 0.01f;
        const float brightHz = 900.0f * std::exp2 (pressure * 3.4f);
        mc.reedLpCoeff = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * juce::jmin (brightHz, 0.45f * sr) / sr);
        mc.clickLevel = p.click * 0.6f;
        mc.clickDecay = std::exp (-1.0f / (0.006f * sr));
    }

    float renderLead (float pitch, float sr, const EngineParams& p, const ModelCoeffs& mc) noexcept
    {
        const float incA = midiToHz (pitch - mc.detune) / sr;
        const float incB = midiToHz (pitch + mc.detune) / sr;
        const float incP = midiToHz (pitch) / sr;
        const float incS = incP * 0.5f;

        const float saws = 0.5f * (saw (phases[0], incA) + saw (phases[1], incB));
        const float pls = pulse (phases[2], incP, p.pulseWidth);
        const float subOsc = p.sub > 0.0f ? pulse (phases[3], incS, 0.5f) * p.sub * 0.7f : 0.0f;

        advance (phases[0], incA);
        advance (phases[1], incB);
        advance (phases[2], incP);
        advance (phases[3], incS);

        return saws * mc.sawGain + pls * mc.pulseGain + subOsc;
    }

    float renderAccordion (float pitch, float sr, const ModelCoeffs& mc) noexcept
    {
        // Pitch offsets of the five reeds in semitones.
        const float offsets[5] { -12.0f, 0.0f, mc.musette, -mc.musette * 0.85f, 12.0f };
        float sum = 0.0f;

        for (size_t r = 0; r < 5; ++r)
        {
            if (mc.gains[r] <= 0.0f)
                continue;

            const float inc = midiToHz (pitch + offsets[r]) / sr;
            // A free reed chops the air stream, which gives a narrow pulse with some saw in it.
            sum += mc.gains[r] * (0.6f * pulse (phases[r], inc, 0.3f) + 0.4f * saw (phases[r], inc));
            advance (phases[r], inc);
        }

        reedLp += (sum - reedLp) * mc.reedLpCoeff;

        // Key click: a short band of noise when the note starts.
        float clickOut = 0.0f;
        if (clickEnv > 0.0005f)
        {
            const float noise = random.nextFloat() * 2.0f - 1.0f;
            clickLp += (noise - clickLp) * 0.45f;
            clickHp += (clickLp - clickHp) * 0.12f;
            clickOut = (clickLp - clickHp) * clickEnv * mc.clickLevel;
            clickEnv *= mc.clickDecay;
        }

        return reedLp * mc.reedLevel + clickOut;
    }

    float noise() noexcept { return random.nextFloat() * 2.0f - 1.0f; }

    /** Gusle: one bowed horsehair string over a skin-covered body. A bowed string moves in a
        sawtooth-like shape; the bow adds irregular scratch, the body adds nasal resonances. */
    float renderGusle (float pitch, float sr, const EngineParams& p, const ModelCoeffs& mc) noexcept
    {
        const float position = p.macro[1], rasp = p.macro[2], nasal = p.macro[3];

        slowNoise += (noise() - slowNoise) * mc.slowCoeff;
        fastNoise += (noise() - fastNoise) * mc.fastCoeff;

        const float inc = midiToHz (pitch + rasp * 6.0f * slowNoise) / sr;
        float s = saw (phases[0], inc) * (1.0f - 0.5f * position)
                  + pulse (phases[0], inc, 0.08f + 0.25f * position) * position * 0.7f;
        advance (phases[0], inc);

        s *= 1.0f + rasp * 2.5f * fastNoise;
        s += rasp * 0.25f * (noise() - fastNoise);

        reedLp += (s - reedLp) * mc.brightCoeff;
        const float body = bands[0].process (reedLp) * 1.6f + bands[1].process (reedLp) * 2.2f + bands[2].process (reedLp) * 1.2f;
        return (reedLp * (1.0f - 0.75f * nasal) + body * nasal) * mc.level;
    }

    /** Frula and kaval: an air jet exciting a pipe. A nearly pure tone whose second harmonic takes
        over when overblown, with breath noise around it. The kaval adds its throaty low register. */
    float renderPipe (float pitch, float sr, const EngineParams& p, const ModelCoeffs& mc, bool isKaval) noexcept
    {
        const float breath = p.macro[0], overblow = p.macro[1], noiseAmount = p.macro[2], fourth = p.macro[3];
        const float twoPi = juce::MathConstants<float>::twoPi;

        slowNoise += (noise() - slowNoise) * mc.slowCoeff;
        const float inc = midiToHz (pitch + slowNoise * 8.0f * noiseAmount) / sr;
        const float ph = phases[0] * twoPi;

        const float h2 = (isKaval ? 0.15f : 0.25f) + overblow * 0.9f;
        const float h3 = isKaval ? 0.3f : 0.12f;
        float tone = std::sin (ph) * (1.0f - 0.6f * overblow) + h2 * std::sin (2.0f * ph) + h3 * std::sin (3.0f * ph)
                     + breath * 0.15f * std::sin (4.0f * ph);
        advance (phases[0], inc);

        const float white = noise();
        float air = bands[0].process (white) * noiseAmount * (isKaval ? 1.4f : 0.9f);

        if (isKaval)
        {
            // Kaba register: a tone an octave below that also roughens the main tone.
            const float low = std::sin (phases[1] * twoPi);
            tone = tone * (1.0f - 0.35f * fourth + 0.35f * fourth * low) + fourth * 0.6f * low;
            advance (phases[1], inc * 0.5f);
        }
        else if (clickEnv > 0.0005f)
        {
            // Chiff: a puff of air before the tone settles.
            air += bands[1].process (white) * clickEnv * fourth * 2.5f;
            clickEnv *= mc.clickDecay;
        }

        tone *= 1.0f + slowNoise * 12.0f * noiseAmount;
        return (tone * 0.6f + air) * mc.level;
    }

    /** Gajde chanter: a single reed in a narrow pipe, bright and nasal, fed from a bag so the
        pressure and pitch wander slightly. The drone is added by the engine. */
    float renderGajde (float pitch, float sr, const EngineParams& p, const ModelCoeffs& mc) noexcept
    {
        const float bag = p.macro[2], edge = p.macro[3];

        slowNoise += (noise() - slowNoise) * mc.slowCoeff;
        const float sag = (1.0f - bag) * (0.25f + 25.0f * slowNoise);
        const float inc = midiToHz (pitch - sag) / sr;

        const float s = pulse (phases[0], inc, 0.32f - 0.2f * edge) * 0.7f + saw (phases[0], inc) * 0.3f;
        advance (phases[0], inc);

        reedLp += (s - reedLp) * mc.brightCoeff;
        const float nasal = bands[0].process (reedLp) * 1.8f + bands[1].process (reedLp) * (0.6f + edge);
        return (reedLp * 0.45f + nasal) * mc.level;
    }

    /** Zurla: a double reed on a wide conical pipe. Very bright and loud, with a strong nasal
        formant and a rough edge when it is pushed. */
    float renderZurla (float pitch, float sr, const EngineParams& p, const ModelCoeffs& mc) noexcept
    {
        const float bite = p.macro[1], growl = p.macro[2], nasal = p.macro[3];

        slowNoise += (noise() - slowNoise) * mc.slowCoeff;
        fastNoise += (noise() - fastNoise) * mc.fastCoeff;

        const float inc = midiToHz (pitch + slowNoise * 3.0f + growl * 4.0f * fastNoise) / sr;
        float s = pulse (phases[0], inc, 0.3f - 0.22f * bite) * 0.65f + saw (phases[0], inc) * 0.5f;
        advance (phases[0], inc);

        s *= 1.0f + growl * 9.0f * fastNoise;

        reedLp += (s - reedLp) * mc.brightCoeff;
        const float formants = bands[0].process (reedLp) * 2.0f + bands[1].process (reedLp) * 1.6f + bands[2].process (reedLp) * 1.0f;
        return (reedLp * (1.0f - 0.6f * nasal) + formants * nasal) * mc.level;
    }

    /** Dvojnice: a double flute carved from one piece of wood. One pipe plays the tune, the other
        holds a second voice a fixed interval away; slightly mistuned pipes beat against each other. */
    float renderDvojnice (float pitch, float sr, const EngineParams& p, const ModelCoeffs& mc) noexcept
    {
        const float breath = p.macro[0], second = p.macro[1], interval = p.macro[2], beating = p.macro[3];
        const float twoPi = juce::MathConstants<float>::twoPi;

        slowNoise += (noise() - slowNoise) * mc.slowCoeff;
        const float wobble = slowNoise * 5.0f;
        const float incA = midiToHz (pitch + wobble) / sr;
        const float incB = juce::jmin (0.45f, midiToHz (pitch + interval + beating * 0.35f - wobble) / sr);

        const auto pipe = [twoPi, breath] (float phase)
        {
            const float ph = phase * twoPi;
            return std::sin (ph) + 0.3f * std::sin (2.0f * ph) + (0.1f + 0.15f * breath) * std::sin (3.0f * ph);
        };

        const float tone = pipe (phases[0]) + second * pipe (phases[1]);
        advance (phases[0], incA);
        advance (phases[1], incB);

        const float air = bands[0].process (noise()) * (0.25f + 0.5f * breath);
        return (tone * 0.5f * (1.0f + slowNoise * 4.0f) + air) * mc.level / (1.0f + 0.6f * second);
    }

    /** Clarinet: a single reed on a straight pipe, so the odd harmonics dominate. Dark and woody
        when soft, with a growl when pushed, as in Balkan wedding bands. */
    float renderClarinet (float pitch, float sr, const EngineParams& p, const ModelCoeffs& mc) noexcept
    {
        const float reed = p.macro[1], growl = p.macro[2], body = p.macro[3];

        slowNoise += (noise() - slowNoise) * mc.slowCoeff;
        fastNoise += (noise() - fastNoise) * mc.fastCoeff;

        const float inc = midiToHz (pitch + slowNoise * 2.5f) / sr;
        // A square wave has only odd harmonics; the reed control brings the even ones in.
        float s = pulse (phases[0], inc, 0.5f - 0.14f * reed) * 0.8f + saw (phases[0], inc) * 0.35f * reed;
        advance (phases[0], inc);

        s *= 1.0f + growl * 10.0f * fastNoise;
        s += (noise() - fastNoise) * 0.03f;

        reedLp += (s - reedLp) * mc.brightCoeff;
        const float formants = bands[0].process (reedLp) * 1.3f + bands[1].process (reedLp) * 0.8f;
        return (reedLp * (1.0f - 0.4f * body) + formants * body) * mc.level;
    }

    /** Truba: the trumpet of the brass bands. The lips buzz like a sawtooth, the tone gets
        brighter the harder it is blown, and every note starts with a short extra bright blare. */
    float renderTruba (float pitch, float sr, const EngineParams& p, const ModelCoeffs& mc) noexcept
    {
        const float blow = p.macro[0], blare = p.macro[1], rasp = p.macro[2], mute = p.macro[3];

        slowNoise += (noise() - slowNoise) * mc.slowCoeff;
        fastNoise += (noise() - fastNoise) * mc.fastCoeff;

        // The pitch scoops up into the note a little while the blare lasts.
        const float inc = midiToHz (pitch + slowNoise * 2.0f - clickEnv * blare * 0.35f) / sr;
        float s = saw (phases[0], inc) * 0.8f + pulse (phases[0], inc, 0.18f) * 0.4f;
        advance (phases[0], inc);
        s *= 1.0f + rasp * 8.0f * fastNoise;

        const float brightHz = 600.0f * std::exp2 (blow * 3.0f + clickEnv * blare * 2.0f);
        const float coeff = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * juce::jmin (brightHz, 0.45f * sr) / sr);
        reedLp += (s - reedLp) * coeff;
        clickEnv *= mc.clickDecay;

        const float bell = bands[0].process (reedLp);
        const float muted = bands[1].process (reedLp) * 2.2f;
        return ((reedLp + bell * 0.8f) * (1.0f - mute) + muted * mute) * mc.level;
    }

    /** Violin: a bowed string like the gusle, but with a smoother bow and the resonances of a
        thin wooden body, as played in the old town and Vojvodina styles. */
    float renderViolin (float pitch, float sr, const EngineParams& p, const ModelCoeffs& mc) noexcept
    {
        const float position = p.macro[1], scratch = p.macro[2], body = p.macro[3];

        slowNoise += (noise() - slowNoise) * mc.slowCoeff;
        fastNoise += (noise() - fastNoise) * mc.fastCoeff;

        const float inc = midiToHz (pitch + scratch * 2.0f * slowNoise) / sr;
        float s = saw (phases[0], inc) * (1.0f - 0.4f * position) + pulse (phases[0], inc, 0.12f + 0.2f * position) * position * 0.5f;
        advance (phases[0], inc);

        s *= 1.0f + scratch * 1.5f * fastNoise;
        s += scratch * 0.12f * (noise() - fastNoise);

        reedLp += (s - reedLp) * mc.brightCoeff;
        const float resonances = bands[0].process (reedLp) * 1.2f + bands[1].process (reedLp) * 1.4f + bands[2].process (reedLp) * 1.1f;
        return (reedLp * (1.0f - 0.55f * body) + resonances * body) * mc.level;
    }

    /** Tambura: a pair of metal strings plucked with a pick. Each string is a delay line one
        period long that feeds back through a gentle low-pass, so high harmonics die first. */
    float renderTambura (float pitch, float sr, const EngineParams& p, const ModelCoeffs& mc) noexcept
    {
        float excite = 0.0f;
        if (exciteLeft > 0)
        {
            exciteLp += (noise() - exciteLp) * mc.fastCoeff;
            excite = exciteLp * 1.4f;
            --exciteLeft;
        }

        // The low-pass in the loop delays the wave a little; shorten the string to stay in tune.
        const float filterDelay = (1.0f - mc.brightCoeff) / mc.brightCoeff;
        const float second = p.macro[2] > 0.001f ? 0.8f : 0.0f;

        float s = strings[0].process (sr / midiToHz (pitch - mc.detune) - filterDelay, excite, mc.brightCoeff, mc.loss);
        if (second > 0.0f)
            s += second * strings[1].process (sr / midiToHz (pitch + mc.detune) - filterDelay, excite, mc.brightCoeff, mc.loss);

        // The pluck leaves a constant offset circulating in the string; keep it out of the output.
        slowNoise += (s - slowNoise) * mc.slowCoeff;
        s -= slowNoise;

        const float body = bands[0].process (s) * 0.5f + bands[1].process (s) * 0.35f;
        return (s * 0.75f + body) * mc.level;
    }

    /** Sargija: a long-necked lute with thin metal strings. The tune is played on one string
        while another rings along as a drone on the root of the scale. */
    float renderSargija (float pitch, float sr, const EngineParams& p, const ModelCoeffs& mc) noexcept
    {
        float excite = 0.0f;
        if (exciteLeft > 0)
        {
            exciteLp += (noise() - exciteLp) * mc.fastCoeff;
            excite = exciteLp * 1.4f;
            --exciteLeft;
        }

        const float filterDelay = (1.0f - mc.brightCoeff) / mc.brightCoeff;
        float s = strings[0].process (sr / midiToHz (pitch) - filterDelay, excite, mc.brightCoeff, mc.loss);

        if (p.macro[2] > 0.001f)
        {
            // The drone: the scale root in the octave below middle C, with a longer ring.
            const float dronePitch = 48.0f + (float) p.scaleRoot;
            s += p.macro[2] * 0.8f * strings[1].process (sr / midiToHz (dronePitch) - filterDelay, excite * 0.7f, mc.brightCoeff,
                                                            0.5f + 0.5f * mc.loss);
        }

        slowNoise += (s - slowNoise) * mc.slowCoeff;
        s -= slowNoise;

        const float body = bands[0].process (s) * 0.4f + bands[1].process (s) * 0.5f;
        return (s * 0.7f + body) * mc.level;
    }

    /** Kanun: a plucked zither with several strings per note. Bright and long ringing; the
        tremolo control picks the note again and again while it is held. */
    float renderKanun (float pitch, float sr, const EngineParams& p, const ModelCoeffs& mc) noexcept
    {
        const float tremolo = p.macro[3];

        drumTime += 1.0f / sr;
        if (tremolo > 0.02f && held && drumTime >= 1.0f / (6.0f + 12.0f * tremolo))
        {
            drumTime = 0.0f;
            exciteLeft = juce::jmax (2, (int) (sr / midiToHz (pitch)));
        }

        float excite = 0.0f;
        if (exciteLeft > 0)
        {
            exciteLp += (noise() - exciteLp) * mc.fastCoeff;
            excite = exciteLp * 1.3f;
            --exciteLeft;
        }

        const float filterDelay = (1.0f - mc.brightCoeff) / mc.brightCoeff;
        float s = strings[0].process (sr / midiToHz (pitch - mc.detune) - filterDelay, excite, mc.brightCoeff, mc.loss)
                  + 0.8f * strings[1].process (sr / midiToHz (pitch + mc.detune) - filterDelay, excite, mc.brightCoeff, mc.loss);

        slowNoise += (s - slowNoise) * mc.slowCoeff;
        s -= slowNoise;

        const float body = bands[0].process (s) * 0.3f + bands[1].process (s) * 0.45f;
        return (s * 0.6f + body) * mc.level;
    }

    /** Darbuka: a goblet drum. Struck in the middle it gives the deep "dum", at the rim the
        short high "tek"; the strike control moves between the two. */
    float renderDarbuka (float pitch, float sr, const EngineParams& p, const ModelCoeffs& mc) noexcept
    {
        const float strike = p.macro[0], snap = p.macro[1], ring = p.macro[3];
        const float twoPi = juce::MathConstants<float>::twoPi;

        if (drumTime <= 0.0f)
        {
            drumEnv = 1.0f;
            drumDrop = 1.0f;
            clickEnv = 1.0f;
        }
        drumTime += 1.0f / sr;

        const float inc = midiToHz (pitch + drumDrop * 3.0f) / sr;
        const float low = std::sin (drumPhase[0] * twoPi);
        const float high = std::sin (drumPhase[1] * twoPi) + 0.6f * std::sin (phases[0] * twoPi);
        advance (drumPhase[0], inc);
        advance (drumPhase[1], juce::jmin (0.45f, inc * 2.3f));
        advance (phases[0], juce::jmin (0.45f, inc * 3.6f));

        // The rim modes die much faster than the middle of the skin unless the shell rings.
        const float highEnv = std::pow (drumEnv, 3.5f - 2.5f * ring);
        float s = low * drumEnv * (1.0f - 0.85f * strike) + high * highEnv * (0.15f + 0.6f * strike);
        s *= juce::jmin (1.0f, drumTime * 1500.0f);

        const float white = noise();
        const float slap = (bands[0].process (white) + bands[1].process (white) * (0.3f + strike)) * clickEnv * snap * (0.5f + strike)
                           * (0.4f + 0.8f * velocity);

        drumEnv *= mc.loss;
        drumDrop *= mc.slowCoeff;
        clickEnv *= mc.clickDecay;
        return (s + slap) * mc.level;
    }

    /** Voice: a sung vowel. The vocal folds buzz, three resonances of the mouth and throat shape
        the vowel, and some breath leaks through. */
    float renderVoice (float pitch, float sr, const EngineParams& p, const ModelCoeffs& mc) noexcept
    {
        const float breath = p.macro[1];

        slowNoise += (noise() - slowNoise) * mc.slowCoeff;
        fastNoise += (noise() - fastNoise) * mc.fastCoeff;

        const float inc = midiToHz (pitch + slowNoise * 4.0f) / sr;
        float s = saw (phases[0], inc) * 0.7f + pulse (phases[0], inc, 0.3f) * 0.4f;
        advance (phases[0], inc);

        reedLp += (s - reedLp) * mc.brightCoeff;
        const float source = reedLp * (1.0f - 0.5f * breath) + fastNoise * breath * 1.2f;

        const float mouth = bands[0].process (source) * 1.0f + bands[1].process (source) * 0.7f + bands[2].process (source) * 0.45f;
        return (mouth + source * 0.08f) * mc.level;
    }

    /** Tapan: a large drum with two skins. The heavy beater gives a low thump whose pitch drops
        as it rings, the thin switch on the other side gives a sharp crack. */
    float renderTapan (float pitch, float sr, const EngineParams& p, const ModelCoeffs& mc) noexcept
    {
        const float thump = p.macro[0], snap = p.macro[1], ring = p.macro[3];
        const float twoPi = juce::MathConstants<float>::twoPi;

        if (drumTime <= 0.0f)
        {
            drumEnv = 1.0f;
            drumDrop = 1.0f;
            clickEnv = 1.0f;
        }
        drumTime += 1.0f / sr;

        const float inc = midiToHz (pitch + drumDrop * (4.0f + 14.0f * thump)) / sr;
        float s = std::sin (drumPhase[0] * twoPi) + ring * 0.5f * std::sin (drumPhase[1] * twoPi);
        advance (drumPhase[0], inc);
        advance (drumPhase[1], juce::jmin (0.45f, inc * 1.59f));

        // Soft attack of a few milliseconds, so the thump does not click.
        s *= drumEnv * juce::jmin (1.0f, drumTime * 600.0f);

        const float white = noise();
        const float crack = (bands[0].process (white) + bands[1].process (white) * 0.6f) * clickEnv * snap * (0.4f + 0.8f * velocity);

        drumEnv *= mc.loss;
        drumDrop *= mc.slowCoeff;
        clickEnv *= mc.clickDecay;
        return (s + crack) * mc.level;
    }

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
    juce::ADSR ampEnv, fltEnv, env3;
    VoiceFilter filter, filter2;
    SamplerVoice samplerVoice;
    juce::Random random;

    int note = -1, channel = 1;
    float noteBend = 0.0f, notePressure = 0.0f, noteTimbre = 0.0f;
    bool expressionSnap = true;
    bool held = false;
    float velocity = 1.0f;
    juce::uint64 age = 0;
    float currentPitch = 60.0f, targetPitch = 60.0f;
    std::array<float, 5> phases { 0.0f, 0.37f, 0.71f, 0.13f, 0.52f };
    float vibPhase = 0.0f, vibTime = 0.0f;

    Ornament ornament = Ornament::none;
    bool fall = false;
    float stepAbove = 1.0f, ornTime = 0.0f, releaseTime = 0.0f;

    float reedLp = 0.0f, clickEnv = 0.0f, clickLp = 0.0f, clickHp = 0.0f;
    float slowNoise = 0.0f, fastNoise = 0.0f;
    std::array<Band, 3> bands;

    std::array<StringLine, 2> strings;
    int exciteLeft = 0;
    float exciteLp = 0.0f;
    std::array<float, 2> drumPhase {};
    float drumTime = 0.0f, drumEnv = 0.0f, drumDrop = 0.0f;

    OscPhases oscPhasesA {}, oscPhasesB {};
    float subPhase = 0.0f, noiseLp = 0.0f;

    float lastFilterEnv = 0.0f, lastEnv3 = 0.0f, modPitch = 0.0f, modGain = 1.0f;
    bool modSnap = true;
};

/** Voice allocation, tuning, ornaments, MIDI handling and rendering. */
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
        mixRight.assign (mix.size(), 0.0f);
        engineSampleRate = (float) sampleRate;
        droneLevel = 0.0f;
        droneLp = 0.0f;
        bendSmooth = 0.0f;
        bendTarget = 0.0f;
        bendCoeff = 1.0f - std::exp (-1.0f / (0.004f * (float) sampleRate / (float) chunkSize));
        wheelSmooth = 0.0f;
        wheelTarget = 0.0f;
        clearExpression();
        for (auto& lfo : lfos)
            lfo.reset();
        heldNotes.clear();
        heldNotes.reserve (64);
    }

    void setParams (const EngineParams& newParams)
    {
        if (newParams.mono != params.mono || newParams.model != params.model)
            allNotesOff();

        if (newParams.mpe != params.mpe)
            clearExpression();

        params = newParams;
        tuning.set (params.scaleRoot, params.scale, params.snap);
        tuneOffset = 12.0f * std::log2 (juce::jlimit (200.0f, 900.0f, params.tuneHz) / 440.0f);

        modActive = false;
        for (const auto& slot : params.slots)
            modActive = modActive || (slot.source != mod::srcNone && std::abs (slot.amount) > 1.0e-6f);
    }

    /** Fixes the random sequence used for ornament chance, for repeatable tests. */
    void setRandomSeed (juce::int64 seed) { random.setSeed (seed); }

    const Tuning& getTuning() const noexcept { return tuning; }

    /** The modulation sources as they were at the end of the last block, indexed by mod::Source. */
    const std::array<float, mod::numSources>& getShownSources() const noexcept { return shownSources; }

    /** The modulation sum of a destination that belongs to the effects (see mod::isGlobalDest). */
    float getGlobalModulation (int dest) const noexcept { return globalDests[(size_t) dest]; }

    /** Where each LFO is in its cycle, 0..1, for the display. */
    const std::array<float, mod::numLfos>& getShownLfoPhases() const noexcept { return shownLfoPhases; }

    /** Where the sampler of the newest note is reading, 0..1 through the sample, for the display. */
    int getShownSamplerPositions (const float*& positions) const noexcept
    {
        positions = shownSamplerPositions.data();
        return shownSamplerCount;
    }

    /** `transport` is only needed for tempo-synced LFOs. */
    void process (juce::AudioBuffer<float>& buffer, const juce::MidiBuffer& midi, const TransportInfo& transport = {})
    {
        const int numSamples = buffer.getNumSamples();
        blockTransport = transport;
        if ((int) mix.size() < numSamples)
        {
            mix.resize ((size_t) numSamples);
            mixRight.resize ((size_t) numSamples);
        }

        std::fill (mix.begin(), mix.begin() + numSamples, 0.0f);
        std::fill (mixRight.begin(), mixRight.begin() + numSamples, 0.0f);

        int pos = 0;
        for (const auto meta : midi)
        {
            const int eventPos = juce::jlimit (0, numSamples, meta.samplePosition);
            renderRange (pos, eventPos);
            pos = eventPos;
            handleMidi (meta.getMessage());
        }

        renderRange (pos, numSamples);

        // Left and right for a stereo buffer; a mono buffer gets the left side.
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            buffer.copyFrom (ch, 0, ch == 1 ? mixRight.data() : mix.data(), numSamples);
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
            wheelSmooth += (wheelTarget - wheelSmooth) * bendCoeff;

            // The LFOs always run, so they keep their place when a slot is switched on.
            float sources[mod::numSources] {};
            const double ppq = blockTransport.ppq + (double) start / (double) engineSampleRate * blockTransport.bpm / 60.0;
            for (size_t i = 0; i < lfos.size(); ++i)
                sources[mod::lfoSource ((int) i)] = lfos[i].next (n, (double) engineSampleRate, params.lfos[i], blockTransport, ppq);
            sources[mod::srcModWheel] = wheelSmooth;
            for (size_t i = 0; i < params.modMacros.size(); ++i)
                sources[mod::srcMacro1 + (int) i] = params.modMacros[i];

            // What the display shows: the shared sources, and the per-note ones of the newest note.
            const Voice* newest = nullptr;
            for (const auto& v : voices)
                if (v.isActive() && (newest == nullptr || v.getAge() > newest->getAge()))
                    newest = &v;
            std::copy (sources, sources + mod::numSources, shownSources.begin());
            if (newest != nullptr)
                newest->fillSources (shownSources.data());

            // The effects are modulated from what the display shows: the newest note stands in for all.
            globalDests.fill (0.0f);
            if (modActive)
                for (const auto& slot : params.slots)
                    if (slot.source > mod::srcNone && slot.source < mod::numSources && mod::isGlobalDest (slot.dest))
                        globalDests[(size_t) slot.dest] += mod::shaped (shownSources[(size_t) slot.source], slot.source, slot.polarity) * slot.amount;
            for (size_t i = 0; i < lfos.size(); ++i)
                shownLfoPhases[i] = lfos[i].getPhase();

            shownSamplerCount = newest != nullptr && params.sampler.level > 0.0f
                                    ? newest->getSamplerPositions (params.sampler, shownSamplerPositions.data(), (int) shownSamplerPositions.size())
                                    : 0;

            bool anyHeld = false;
            for (auto& v : voices)
            {
                if (v.isActive())
                {
                    // Channel 1 is the MPE master channel: what it sends applies to every note.
                    const auto ch = (size_t) (v.getChannel() - 1);
                    const bool own = params.mpe && ch > 0;
                    v.setExpression (own ? channelBend[ch] : 0.0f,
                                     juce::jmin (1.0f, channelPressure[0] + (own ? channelPressure[ch] : 0.0f)),
                                     juce::jlimit (-1.0f, 1.0f, channelTimbre[0] + (own ? channelTimbre[ch] : 0.0f)),
                                     bendCoeff);
                }

                v.render (mix.data() + start, mixRight.data() + start, n, params, bendSmooth + tuneOffset, modActive ? sources : nullptr);
                anyHeld = anyHeld || v.isHeld();
            }

            if (params.model == Model::gajde)
                renderDrone (mix.data() + start, mixRight.data() + start, n, anyHeld);

            start += n;
        }
    }

    /** The gajde drone: one steady pipe on the scale root, two octaves below middle C, sounding
        for as long as any note is held. */
    void renderDrone (float* out, float* outRight, int numSamples, bool anyHeld)
    {
        const float sr = engineSampleRate;
        const float target = anyHeld ? params.macro[0] * 0.3f * params.engineLevel : 0.0f;
        if (target <= 0.0f && droneLevel < 1.0e-5f)
        {
            droneLevel = 0.0f;
            return;
        }

        const float levelCoeff = 1.0f - std::exp (-1.0f / ((anyHeld ? 0.08f : 0.35f) * sr));
        const float pitch = 36.0f + (float) params.scaleRoot + params.macro[1] + tuneOffset;
        const float inc = 440.0f * std::exp2 ((pitch - 69.0f) / 12.0f) / sr;
        const float lpCoeff = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * 1400.0f / sr);

        for (int i = 0; i < numSamples; ++i)
        {
            droneLevel += (target - droneLevel) * levelCoeff;
            const float raw = (dronePhase < 0.25f ? 1.0f : -1.0f) * 0.6f + (2.0f * dronePhase - 1.0f) * 0.4f;
            droneLp += (raw - droneLp) * lpCoeff;
            out[i] += droneLp * droneLevel;
            outRight[i] += droneLp * droneLevel;

            dronePhase += inc;
            if (dronePhase >= 1.0f)
                dronePhase -= 1.0f;
        }
    }

    void handleMidi (const juce::MidiMessage& m)
    {
        // Without MPE every channel is treated like channel 1.
        const auto ch = (size_t) (params.mpe ? juce::jlimit (1, 16, m.getChannel()) - 1 : 0);

        if (m.isNoteOn())
            noteOn (m.getNoteNumber(), m.getFloatVelocity(), (int) ch + 1);
        else if (m.isNoteOff())
            noteOff (m.getNoteNumber());
        else if (m.isPitchWheel())
        {
            const float amount = ((float) m.getPitchWheelValue() - 8192.0f) / 8192.0f;
            if (ch == 0)
                bendTarget = amount * params.bendRange;
            else
                channelBend[ch] = amount * params.mpeBendRange;
        }
        else if (m.isChannelPressure())
            channelPressure[ch] = (float) m.getChannelPressureValue() / 127.0f;
        else if (m.isController() && m.getControllerNumber() == 74)
            channelTimbre[ch] = juce::jlimit (-1.0f, 1.0f, ((float) m.getControllerValue() - 64.0f) / 63.0f);
        else if (m.isController() && m.getControllerNumber() == 1)
            wheelTarget = (float) m.getControllerValue() / 127.0f;
        else if (m.isAllNotesOff() || m.isAllSoundOff())
            allNotesOff();
    }

    /** Builds the note a voice should play: tuned pitch plus the ornament chosen for it. */
    NoteEvent makeEvent (int note, float velocity, bool allowOrnament, int channel)
    {
        NoteEvent e;
        e.note = note;
        e.channel = channel;
        e.velocity = velocity;
        e.pitch = tuning.pitchFor (note);
        e.stepAbove = tuning.stepAbove (note);
        e.fall = params.fall && random.nextFloat() < params.ornChance;

        if (! allowOrnament)
            return e;

        Ornament enabled[3];
        int count = 0;
        if (params.grace)
            enabled[count++] = Ornament::grace;
        if (params.mordent)
            enabled[count++] = Ornament::mordent;
        if (params.trill)
            enabled[count++] = Ornament::trill;

        if (count > 0 && random.nextFloat() < params.ornChance)
            e.ornament = enabled[random.nextInt (count)];

        return e;
    }

    void noteOn (int note, float velocity, int channel)
    {
        ++counter;
        const auto e = makeEvent (note, velocity, true, channel);
        noteChannel[(size_t) juce::jlimit (0, 127, note)] = channel;

        if (params.mono)
        {
            auto& v = voices[0];
            const bool legato = ! heldNotes.empty() && v.isActive();
            heldNotes.erase (std::remove (heldNotes.begin(), heldNotes.end(), note), heldNotes.end());
            heldNotes.push_back (note);

            // A drum is struck again on every note; the other models slide.
            if (legato && ! isDrum (params.model))
            {
                v.slideTo (e);
            }
            else
            {
                retriggerLfos();
                v.start (e, counter, params, true);
            }

            return;
        }

        Voice* target = nullptr;
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

        retriggerLfos();
        target->start (e, counter, params, false);
    }

    void retriggerLfos() noexcept
    {
        for (size_t i = 0; i < lfos.size(); ++i)
            lfos[i].noteStarted (params.lfos[i]);
    }

    void noteOff (int note)
    {
        if (params.mono)
        {
            heldNotes.erase (std::remove (heldNotes.begin(), heldNotes.end(), note), heldNotes.end());

            if (heldNotes.empty())
                voices[0].release();
            else if (voices[0].getNote() == note)
                voices[0].slideTo (makeEvent (heldNotes.back(), 1.0f, false, noteChannel[(size_t) juce::jlimit (0, 127, heldNotes.back())])); // falling back to a held note is never ornamented

            return;
        }

        for (auto& v : voices)
            if (v.isHeld() && v.getNote() == note)
                v.release();
    }

    std::array<Voice, maxVoices> voices;
    std::vector<float> mix, mixRight;
    std::vector<int> heldNotes;
    EngineParams params;
    Tuning tuning;
    juce::Random random;
    juce::uint64 counter = 0;
    float bendSmooth = 0.0f, bendTarget = 0.0f, bendCoeff = 1.0f;
    float tuneOffset = 0.0f; // semitones, from the A4 reference
    std::array<float, mod::numSources> shownSources {};
    std::array<float, mod::numDests> globalDests {};
    std::array<float, mod::numLfos> shownLfoPhases {};
    std::array<float, 16> shownSamplerPositions {};
    int shownSamplerCount = 0;

    // Per-channel expression for MPE; index 0 is channel 1, the master channel.
    std::array<float, 16> channelBend {}, channelPressure {}, channelTimbre {};
    std::array<int, 128> noteChannel {};

    void clearExpression() noexcept
    {
        channelBend.fill (0.0f);
        channelPressure.fill (0.0f);
        channelTimbre.fill (0.0f);
    }
    float engineSampleRate = 44100.0f, droneLevel = 0.0f, droneLp = 0.0f, dronePhase = 0.0f;

    std::array<Lfo, mod::numLfos> lfos;
    TransportInfo blockTransport;
    float wheelSmooth = 0.0f, wheelTarget = 0.0f;
    bool modActive = false;
};
} // namespace amv
