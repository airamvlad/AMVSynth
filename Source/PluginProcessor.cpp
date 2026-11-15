#include "PluginProcessor.h"
#include "PluginEditor.h"

AMVSynthAudioProcessor::AMVSynthAudioProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "AMVSynth", amv::createParameterLayout())
{
    amv::WavetableBank::get(); // build the wavetables here, not on the audio thread

    using namespace amv;
    const auto ref = [this] (const juce::String& paramId) { return apvts.getRawParameterValue (paramId); };
    for (int i = 0; i < mod::numLfos; ++i)
        lfoRefs[(size_t) i] = { ref (id::lfo (i, "Shape")), ref (id::lfo (i, "Rate")), ref (id::lfo (i, "Sync")), ref (id::lfo (i, "Division")), ref (id::lfo (i, "Mode")) };
    for (int i = 0; i < mod::numMacros; ++i)
        macroRefs[(size_t) i] = ref (id::macro (i));
    for (int i = 0; i < mod::numSlots; ++i)
        slotRefs[(size_t) i] = { ref (id::slot (i, "Source")), ref (id::slot (i, "Dest")), ref (id::slot (i, "Amount")), ref (id::slot (i, "Polarity")) };

    for (int i = 0; i < mod::numLfos; ++i)
        setLfoShape (i, {}, false);

    const auto& knobIds = getEngineKnobIds();
    for (size_t m = 0; m < knobIds.size(); ++m)
        for (size_t k = 0; k < 4; ++k)
        {
            const auto range = apvts.getParameterRange (knobIds[m][k]);
            engineRanges[m][k] = { range.start, range.end };
        }
}

void AMVSynthAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    engine.prepare (sampleRate, samplesPerBlock);
    arp.prepare (sampleRate);
    fx.prepare (sampleRate, samplesPerBlock);
    stereo.setSize (2, juce::jmax (1, samplesPerBlock));
    arpMidi.ensureSize (4096);
    engine.setParams (readParams());
    outputGain.reset (sampleRate, 0.02);
    outputGain.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (value (amv::id::level)));
}

bool AMVSynthAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo();
}

float AMVSynthAudioProcessor::value (const char* paramId) const
{
    return apvts.getRawParameterValue (paramId)->load();
}

amv::EngineParams AMVSynthAudioProcessor::readParams() const
{
    using namespace amv;
    EngineParams p;
    p.model = (Model) juce::roundToInt (value (id::model));
    p.sub = value (id::sub);
    p.bellows = value (id::accBellows);
    p.musetteCents = value (id::accMusette);
    p.registerPos = value (id::accRegister);
    p.click = value (id::accClick);
    p.scaleRoot = juce::roundToInt (value (id::scaleRoot));
    p.scale = juce::roundToInt (value (id::scale));
    p.snap = value (id::snap) > 0.5f;
    p.grace = value (id::ornGrace) > 0.5f;
    p.mordent = value (id::ornMordent) > 0.5f;
    p.trill = value (id::ornTrill) > 0.5f;
    p.fall = value (id::ornFall) > 0.5f;
    p.ornSpeedSec = value (id::ornSpeed) * 0.001f;
    p.ornAmount = value (id::ornAmount);
    p.ornChance = value (id::ornChance) * 0.01f;

    switch (p.model)
    {
        case Model::gusle: p.macro = { value (id::gusBow), value (id::gusPosition), value (id::gusRasp), value (id::gusNasal) }; break;
        case Model::frula: p.macro = { value (id::fruBreath), value (id::fruOverblow), value (id::fruNoise), value (id::fruChiff) }; break;
        case Model::kaval: p.macro = { value (id::kavBreath), value (id::kavOverblow), value (id::kavNoise), value (id::kavThroat) }; break;
        case Model::gajde: p.macro = { value (id::gajDrone), value (id::gajDroneTune), value (id::gajBag), value (id::gajEdge) }; break;
        case Model::zurla: p.macro = { value (id::zurPressure), value (id::zurBite), value (id::zurGrowl), value (id::zurNasal) }; break;
        case Model::tambura: p.macro = { value (id::tamPluck), value (id::tamDecay), value (id::tamDouble), value (id::tamTone) }; break;
        case Model::tapan: p.macro = { value (id::tapThump), value (id::tapSnap), value (id::tapDecay), value (id::tapRing) }; break;
        case Model::dvojnice: p.macro = { value (id::dvoBreath), value (id::dvoSecond), value (id::dvoInterval), value (id::dvoBeat) }; break;
        case Model::clarinet: p.macro = { value (id::claBreath), value (id::claReed), value (id::claGrowl), value (id::claBody) }; break;
        case Model::truba: p.macro = { value (id::truBlow), value (id::truBlare), value (id::truRasp), value (id::truMute) }; break;
        case Model::violin: p.macro = { value (id::vioBow), value (id::vioPosition), value (id::vioScratch), value (id::vioBody) }; break;
        case Model::sargija: p.macro = { value (id::sarPluck), value (id::sarDecay), value (id::sarDrone), value (id::sarTone) }; break;
        case Model::kanun: p.macro = { value (id::kanPluck), value (id::kanDecay), value (id::kanShimmer), value (id::kanTremolo) }; break;
        case Model::darbuka: p.macro = { value (id::darStrike), value (id::darSnap), value (id::darDecay), value (id::darRing) }; break;
        case Model::voice: p.macro = { value (id::voxVowel), value (id::voxBreath), value (id::voxSize), value (id::voxTension) }; break;
        case Model::juzniVetar:
        case Model::accordion: break;
    }

    p.engineRange = engineRanges[(size_t) juce::jlimit (0, (int) engineRanges.size() - 1, (int) p.model)];
    p.engineLevel = value (id::engineLevel);
    p.oscA = { juce::roundToInt (value (id::oscATable)), value (id::oscAPos), juce::roundToInt (value (id::oscAUnison)),
               value (id::oscADetune), value (id::oscAPitch), value (id::oscAFine), value (id::oscALevel), value (id::oscAWidth) };
    p.oscB = { juce::roundToInt (value (id::oscBTable)), value (id::oscBPos), juce::roundToInt (value (id::oscBUnison)),
               value (id::oscBDetune), value (id::oscBPitch), value (id::oscBFine), value (id::oscBLevel), value (id::oscBWidth) };
    p.oscA.warp = juce::roundToInt (value (id::oscAWarp));
    p.oscA.warpAmount = value (id::oscAWarpAmount);
    p.oscB.warp = juce::roundToInt (value (id::oscBWarp));
    p.oscB.warpAmount = value (id::oscBWarpAmount);
    p.subLevel = value (id::subLevel);
    p.subOctave = juce::roundToInt (value (id::subOctave)) + 1;
    p.subShape = value (id::subShape);
    p.noiseLevel = value (id::noiseLevel);
    p.noiseColorHz = value (id::noiseColor);

    for (size_t i = 0; i < lfoRefs.size(); ++i)
        p.lfos[i] = { juce::roundToInt (lfoRefs[i].shape->load()), lfoRefs[i].rate->load(), lfoRefs[i].sync->load() > 0.5f,
                      juce::roundToInt (lfoRefs[i].division->load()), juce::roundToInt (lfoRefs[i].mode->load()),
                      activeLfoShape[i].load (std::memory_order_acquire) };
    for (size_t i = 0; i < macroRefs.size(); ++i)
        p.modMacros[i] = macroRefs[i]->load();
    for (size_t i = 0; i < slotRefs.size(); ++i)
        p.slots[i] = { juce::roundToInt (slotRefs[i].source->load()), juce::roundToInt (slotRefs[i].dest->load()), slotRefs[i].amount->load(),
                       juce::roundToInt (slotRefs[i].polarity->load()) };

    p.tone = value (id::tone);
    p.pulseWidth = value (id::pulseWidth);
    p.detuneCents = value (id::detune);
    p.vibDepthCents = value (id::vibDepth);
    p.vibRateHz = value (id::vibRate);
    p.vibDelaySec = value (id::vibDelay) * 0.001f;
    p.bendRange = value (id::bendRange);
    p.glideSec = value (id::glide) * 0.001f;
    p.mono = value (id::mono) > 0.5f;
    auto& smp = p.sampler;
    smp.data = activeSample.load (std::memory_order_acquire);
    smp.level = value (id::smpLevel);
    smp.mode = juce::roundToInt (value (id::smpMode));
    smp.pitch = value (id::smpPitch);
    smp.fineCents = value (id::smpFine);
    smp.rootNote = juce::roundToInt (value (id::smpRoot));
    smp.keyTrack = value (id::smpKeyTrack) > 0.5f;
    smp.pan = value (id::smpPan);
    smp.reverse = value (id::smpReverse) > 0.5f;
    smp.start = value (id::smpStart);
    smp.end = value (id::smpEnd);
    smp.loop = juce::roundToInt (value (id::smpLoop));
    smp.loopStart = value (id::smpLoopStart);
    smp.loopEnd = value (id::smpLoopEnd);
    smp.crossfade = value (id::smpCrossfade);
    smp.position = value (id::smpPosition);
    smp.grainMs = value (id::smpGrainSize);
    smp.density = value (id::smpDensity);
    smp.spray = value (id::smpSpray);
    smp.pitchSpray = value (id::smpPitchSpray);
    smp.scan = value (id::smpScan);
    smp.shape = value (id::smpShape);
    smp.spread = value (id::smpSpread);
    smp.reverseChance = value (id::smpRevChance);
    p.userWave = { activeUserWave[0].load (std::memory_order_acquire), activeUserWave[1].load (std::memory_order_acquire) };
    p.tuneHz = value (id::tuneA4);
    p.mpe = value (id::mpe) > 0.5f;
    p.mpeBendRange = value (id::mpeBend);
    p.filterType = juce::roundToInt (value (id::filterType));
    p.cutoffHz = value (id::cutoff);
    p.resonance = value (id::resonance);
    p.filterEnv = value (id::filterEnv);
    p.amp = { value (id::ampA) * 0.001f, value (id::ampD) * 0.001f, value (id::ampS), value (id::ampR) * 0.001f };
    p.flt = { value (id::fltA) * 0.001f, value (id::fltD) * 0.001f, value (id::fltS), value (id::fltR) * 0.001f };
    p.env3 = { value (id::env3A) * 0.001f, value (id::env3D) * 0.001f, value (id::env3S), value (id::env3R) * 0.001f };
    return p;
}

void AMVSynthAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    keyboardState.processNextMidiBuffer (midi, 0, buffer.getNumSamples(), true);

    amv::ArpParams ap;
    ap.on = value (amv::id::arpOn) > 0.5f;
    ap.meter = juce::roundToInt (value (amv::id::arpMeter));
    ap.rate = juce::roundToInt (value (amv::id::arpRate));
    ap.mode = juce::roundToInt (value (amv::id::arpMode));
    ap.octaves = juce::roundToInt (value (amv::id::arpOctaves));
    ap.gate = value (amv::id::arpGate) * 0.01f;
    ap.accent = value (amv::id::arpAccent) * 0.01f;
    ap.polymeter = value (amv::id::arpPolymeter) > 0.5f;

    amv::TransportInfo transport;
    if (auto* head = getPlayHead())
        if (const auto position = head->getPosition())
        {
            transport.playing = position->getIsPlaying();
            if (const auto bpm = position->getBpm())
                transport.bpm = *bpm;
            if (const auto ppq = position->getPpqPosition())
                transport.ppq = *ppq;
            else
                transport.playing = false;
            if (const auto sig = position->getTimeSignature())
                transport.barBeats = 4.0 * sig->numerator / juce::jmax (1, sig->denominator);
        }

    arp.process (midi, arpMidi, buffer.getNumSamples(), ap, transport);

    // The voices render in mono, the effects work in stereo.
    const int numSamples = buffer.getNumSamples();
    if (stereo.getNumSamples() < numSamples)
        stereo.setSize (2, numSamples, false, false, true);

    juce::AudioBuffer<float> work (stereo.getArrayOfWritePointers(), 2, numSamples);
    work.clear();

    engine.setParams (readParams());
    engine.process (work, arpMidi, transport);

    const auto& shown = engine.getShownSources();
    for (size_t i = 0; i < shown.size(); ++i)
        modSourceValues[i].store (shown[i], std::memory_order_relaxed);

    const auto& phases = engine.getShownLfoPhases();
    for (size_t i = 0; i < phases.size(); ++i)
        lfoPhases[i].store (phases[i], std::memory_order_relaxed);

    const float* positions = nullptr;
    const int positionCount = engine.getShownSamplerPositions (positions);
    for (int i = 0; i < positionCount; ++i)
        samplerPositions[(size_t) i].store (positions[i], std::memory_order_relaxed);
    samplerPositionCount.store (positionCount, std::memory_order_relaxed);

    outputGain.setTargetValue (juce::Decibels::decibelsToGain (value (amv::id::level)));
    outputGain.applyGain (work, numSamples);

    fx.process (work, readFxParams(), transport);

    // Peak level after the effects, for the meter in the editor.
    for (int ch = 0; ch < 2; ++ch)
    {
        const float peak = work.getMagnitude (ch, 0, numSamples);
        if (peak > outputPeak[(size_t) ch].load (std::memory_order_relaxed))
            outputPeak[(size_t) ch].store (peak, std::memory_order_relaxed);
    }

    if (buffer.getNumChannels() >= 2)
    {
        buffer.copyFrom (0, 0, work, 0, 0, numSamples);
        buffer.copyFrom (1, 0, work, 1, 0, numSamples);
    }
    else if (buffer.getNumChannels() == 1)
    {
        buffer.copyFrom (0, 0, work, 0, 0, numSamples);
        buffer.addFrom (0, 0, work, 1, 0, numSamples);
        buffer.applyGain (0.5f);
    }

    // The limiter in the effect chain keeps peaks down; this clip is only a last safety net.
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        juce::FloatVectorOperations::clip (buffer.getWritePointer (ch), buffer.getReadPointer (ch),
                                           -1.0f, 1.0f, numSamples);
}

amv::FxParams AMVSynthAudioProcessor::readFxParams() const
{
    using namespace amv;
    FxParams p;
    p.distOn = value (id::distOn) > 0.5f;
    p.distType = juce::roundToInt (value (id::distType));
    p.distDriveDb = value (id::distDrive);
    p.distMix = value (id::distMix) * 0.01f;
    p.gateOn = value (id::gateOn) > 0.5f;
    p.gateRate = juce::roundToInt (value (id::gateRate));
    p.gatePattern = juce::roundToInt (value (id::gatePattern));
    p.gateDepth = value (id::gateDepth) * 0.01f;
    p.gateLength = value (id::gateLength) * 0.01f;
    p.gateSmoothMs = value (id::gateSmooth);
    p.delayOn = value (id::delayOn) > 0.5f;
    p.delayTime = juce::roundToInt (value (id::delayTime));
    p.delayFeedback = value (id::delayFeedback) * 0.01f;
    p.delayToneHz = value (id::delayTone);
    p.delayMix = value (id::delayMix) * 0.01f;
    p.delayPingPong = value (id::delayPingPong) > 0.5f;
    p.reverbOn = value (id::reverbOn) > 0.5f;
    p.reverbSize = value (id::reverbSize);
    p.reverbDamp = value (id::reverbDamp);
    p.reverbMix = value (id::reverbMix) * 0.01f;
    p.duckOn = value (id::duckOn) > 0.5f;
    p.duckDepth = value (id::duckDepth) * 0.01f;
    p.duckRelease = value (id::duckRelease) * 0.01f;
    p.order = juce::roundToInt (value (id::fxOrder));

    // Modulation of the effects, worked out by the engine for this block.
    const auto moved = [this] (float v, int dest, float lowest, float highest)
    {
        return juce::jlimit (lowest, highest, v + engine.getGlobalModulation (dest) * (highest - lowest));
    };
    p.distDriveDb = moved (p.distDriveDb, mod::dstDistDrive, 0.0f, 36.0f);
    p.distMix = moved (p.distMix, mod::dstDistMix, 0.0f, 1.0f);
    p.gateDepth = moved (p.gateDepth, mod::dstGateDepth, 0.0f, 1.0f);
    p.delayFeedback = moved (p.delayFeedback, mod::dstDelayFeedback, 0.0f, 0.95f);
    p.delayToneHz = juce::jlimit (300.0f, 16000.0f, p.delayToneHz * std::exp2 (engine.getGlobalModulation (mod::dstDelayTone) * mod::timeOctaves));
    p.delayMix = moved (p.delayMix, mod::dstDelayMix, 0.0f, 1.0f);
    p.reverbSize = moved (p.reverbSize, mod::dstReverbSize, 0.0f, 1.0f);
    p.reverbMix = moved (p.reverbMix, mod::dstReverbMix, 0.0f, 1.0f);
    return p;
}

juce::AudioProcessorEditor* AMVSynthAudioProcessor::createEditor()
{
    return new AMVSynthAudioProcessorEditor (*this);
}

void AMVSynthAudioProcessor::setCurrentProgram (int index)
{
    const auto& presets = amv::getFactoryPresets();
    if (index < 0 || index >= (int) presets.size())
        return;

    currentPreset = index;
    amv::applyPreset (apvts, presets[(size_t) index]);
}

const juce::String AMVSynthAudioProcessor::getProgramName (int index)
{
    const auto& presets = amv::getFactoryPresets();
    return index >= 0 && index < (int) presets.size() ? juce::String (presets[(size_t) index].name) : juce::String();
}

void AMVSynthAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("preset", currentPreset, nullptr);
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void AMVSynthAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
        {
            const auto state = juce::ValueTree::fromXml (*xml);
            currentPreset = juce::jlimit (0, getNumPrograms() - 1, (int) state.getProperty ("preset", 0));
            apvts.replaceState (state);
            restoreUserWavetables();
            restoreSample();
            restoreLfoShapes();
        }
}

namespace
{
const char* const userWaveData[2] { "userWaveA", "userWaveB" };
const char* const userWaveName[2] { "userWaveAName", "userWaveBName" };
} // namespace

void AMVSynthAudioProcessor::setUserWavetable (int osc, std::unique_ptr<amv::UserWavetable> table)
{
    const auto now = juce::jmax<juce::uint32> (1, juce::Time::getMillisecondCounter());
    const auto* previous = activeUserWave[(size_t) osc].exchange (table.get(), std::memory_order_acq_rel);

    for (auto& stored : userWaveStore)
        if (stored.table.get() == previous && previous != nullptr)
            stored.retiredAt = now;

    if (table != nullptr)
        userWaveStore.push_back ({ std::move (table), 0 });

    // A replaced table is thrown away half a second later; the audio thread only holds one for a block.
    userWaveStore.erase (std::remove_if (userWaveStore.begin(), userWaveStore.end(),
                                         [now] (const StoredWave& stored) { return stored.retiredAt != 0 && now - stored.retiredAt > 500; }),
                         userWaveStore.end());
}

void AMVSynthAudioProcessor::setUserWavetableFrames (int osc, std::vector<float> frames, const juce::String& name, bool save)
{
    for (auto& v : frames)
        if (! std::isfinite (v))
            v = 0.0f;

    setUserWavetable (osc, std::make_unique<amv::UserWavetable> (std::move (frames), name));
    if (save)
        storeUserWavetables();
}

bool AMVSynthAudioProcessor::exportWavetable (int osc, const juce::File& file) const
{
    const auto* table = activeUserWave[(size_t) osc].load();
    if (table == nullptr)
        return false;

    // The table as it plays: without its offset and at full level.
    juce::AudioBuffer<float> audio (1, amv::UserWavetable::sourceSize);
    for (int frame = 0; frame < amv::UserWavetable::numFrames; ++frame)
        audio.copyFrom (0, frame * amv::UserWavetable::tableSize, table->wave (frame, 0), amv::UserWavetable::tableSize);

    file.deleteFile();
    juce::WavAudioFormat wav;
    if (auto stream = std::unique_ptr<juce::OutputStream> (file.createOutputStream()))
        if (auto writer = wav.createWriterFor (stream, juce::AudioFormatWriterOptions().withSampleRate (48000.0).withNumChannels (1).withBitsPerSample (32)
                                                           .withSampleFormat (juce::AudioFormatWriterOptions::SampleFormat::floatingPoint)))
            return writer->writeFromAudioSampleBuffer (audio, 0, audio.getNumSamples());

    return false;
}

bool AMVSynthAudioProcessor::importWavetable (int osc, const juce::File& file)
{
    juce::AudioFormatManager formats;
    formats.registerBasicFormats();
    const std::unique_ptr<juce::AudioFormatReader> reader (formats.createReaderFor (file));
    if (reader == nullptr || reader->lengthInSamples < 8)
        return false;

    // Wavetable files are short; anything longer than the largest table is cut off.
    const juce::int64 longest = (juce::int64) amv::UserWavetable::maxSourceFrames * amv::UserWavetable::tableSize;
    const int length = (int) (reader->lengthInSamples < longest ? reader->lengthInSamples : longest);
    juce::AudioBuffer<float> audio (1, length);
    if (! reader->read (&audio, 0, length, 0, true, false))
        return false;

    auto table = amv::UserWavetable::fromAudio (audio.getReadPointer (0), length, file.getFileNameWithoutExtension());
    if (table == nullptr)
        return false;

    setUserWavetable (osc, std::move (table));
    storeUserWavetables();
    return true;
}

void AMVSynthAudioProcessor::storeUserWavetables()
{
    // The table travels with the project and with saved presets, so the file is not needed again.
    for (int osc = 0; osc < 2; ++osc)
        if (const auto* table = activeUserWave[(size_t) osc].load())
        {
            const auto& source = table->getSource();
            apvts.state.setProperty (userWaveData[osc], juce::MemoryBlock (source.data(), source.size() * sizeof (float)).toBase64Encoding(), nullptr);
            apvts.state.setProperty (userWaveName[osc], table->getName(), nullptr);
        }
}

void AMVSynthAudioProcessor::restoreUserWavetables()
{
    for (int osc = 0; osc < 2; ++osc)
    {
        juce::MemoryBlock block;
        const auto text = apvts.state.getProperty (userWaveData[osc]).toString();
        const size_t expected = (size_t) amv::UserWavetable::sourceSize * sizeof (float);

        if (text.isNotEmpty() && block.fromBase64Encoding (text) && block.getSize() == expected)
        {
            std::vector<float> frames ((size_t) amv::UserWavetable::sourceSize);
            std::memcpy (frames.data(), block.getData(), expected);
            bool finite = true;
            for (float v : frames)
                finite = finite && std::isfinite (v);

            if (finite)
            {
                setUserWavetable (osc, std::make_unique<amv::UserWavetable> (std::move (frames), apvts.state.getProperty (userWaveName[osc]).toString()));
                continue;
            }
        }

        // A state without a table keeps the one that is loaded now.
        if (activeUserWave[(size_t) osc].load() != nullptr)
            storeUserWavetables();
    }
}

void AMVSynthAudioProcessor::setSample (std::unique_ptr<amv::SampleData> data)
{
    const auto now = juce::jmax<juce::uint32> (1, juce::Time::getMillisecondCounter());
    const auto* previous = activeSample.exchange (data.get(), std::memory_order_acq_rel);

    for (auto& stored : sampleStore)
        if (stored.data.get() == previous && previous != nullptr)
            stored.retiredAt = now;

    if (data != nullptr)
        sampleStore.push_back ({ std::move (data), 0 });

    // A replaced sample is thrown away half a second later; the audio thread only holds one for a block.
    sampleStore.erase (std::remove_if (sampleStore.begin(), sampleStore.end(),
                                       [now] (const StoredSample& stored) { return stored.retiredAt != 0 && now - stored.retiredAt > 500; }),
                       sampleStore.end());
    storeSample();
}

bool AMVSynthAudioProcessor::loadSample (const juce::File& file)
{
    juce::AudioFormatManager formats;
    formats.registerBasicFormats();
    const std::unique_ptr<juce::AudioFormatReader> reader (formats.createReaderFor (file));
    if (reader == nullptr || reader->lengthInSamples < 16 || reader->sampleRate < 1000.0)
        return false;

    const auto longest = (juce::int64) (amv::SampleData::maxSeconds * reader->sampleRate);
    const int length = (int) (reader->lengthInSamples < longest ? reader->lengthInSamples : longest);
    juce::AudioBuffer<float> audio (reader->numChannels > 1 ? 2 : 1, length);
    if (! reader->read (&audio, 0, length, 0, true, true))
        return false;

    auto data = amv::SampleData::fromBuffer (audio, reader->sampleRate, file.getFileNameWithoutExtension());
    if (data == nullptr)
        return false;

    setSample (std::move (data));
    return true;
}

void AMVSynthAudioProcessor::storeSample()
{
    // The sample travels with the project and with saved presets as FLAC, so the file is not needed again.
    const auto* data = activeSample.load();
    if (data == nullptr)
    {
        apvts.state.removeProperty ("sampleData", nullptr);
        apvts.state.removeProperty ("sampleName", nullptr);
        return;
    }

    juce::MemoryBlock block;
    {
        const auto audio = data->toBuffer();
        const bool isLong = (double) data->getLength() > 20.0 * data->getSampleRate();
        std::unique_ptr<juce::OutputStream> stream = std::make_unique<juce::MemoryOutputStream> (block, false);
        juce::FlacAudioFormat flac;
        if (auto writer = flac.createWriterFor (stream, juce::AudioFormatWriterOptions().withSampleRate (data->getSampleRate())
                                                            .withNumChannels (audio.getNumChannels()).withBitsPerSample (isLong ? 16 : 24)))
            writer->writeFromAudioSampleBuffer (audio, 0, audio.getNumSamples());
    }

    apvts.state.setProperty ("sampleData", block.toBase64Encoding(), nullptr);
    apvts.state.setProperty ("sampleName", data->getName(), nullptr);
}

void AMVSynthAudioProcessor::restoreSample()
{
    juce::MemoryBlock block;
    const auto text = apvts.state.getProperty ("sampleData").toString();

    if (text.isNotEmpty() && block.fromBase64Encoding (text) && block.getSize() > 0)
    {
        juce::FlacAudioFormat flac;
        const std::unique_ptr<juce::AudioFormatReader> reader (flac.createReaderFor (new juce::MemoryInputStream (block, false), true));
        if (reader != nullptr && reader->lengthInSamples >= 16)
        {
            juce::AudioBuffer<float> audio (reader->numChannels > 1 ? 2 : 1, (int) reader->lengthInSamples);
            if (reader->read (&audio, 0, audio.getNumSamples(), 0, true, true))
                if (auto data = amv::SampleData::fromBuffer (audio, reader->sampleRate, apvts.state.getProperty ("sampleName").toString()))
                {
                    const auto now = juce::jmax<juce::uint32> (1, juce::Time::getMillisecondCounter());
                    const auto* previous = activeSample.exchange (data.get(), std::memory_order_acq_rel);
                    for (auto& stored : sampleStore)
                        if (stored.data.get() == previous && previous != nullptr)
                            stored.retiredAt = now;
                    sampleStore.push_back ({ std::move (data), 0 });
                    return;
                }
        }
    }

    // A state without a sample keeps the one that is loaded now.
    if (activeSample.load() != nullptr)
        storeSample();
}

void AMVSynthAudioProcessor::setLfoShape (int lfo, std::vector<amv::LfoPoint> points, bool save)
{
    auto shape = std::make_unique<amv::LfoShape> (std::move (points));
    const auto text = shape->toString();
    const auto now = juce::jmax<juce::uint32> (1, juce::Time::getMillisecondCounter());
    const auto* previous = activeLfoShape[(size_t) lfo].exchange (shape.get(), std::memory_order_acq_rel);

    for (auto& stored : lfoShapeStore)
        if (stored.shape.get() == previous && previous != nullptr)
            stored.retiredAt = now;
    lfoShapeStore.push_back ({ std::move (shape), 0 });

    // A replaced shape is thrown away half a second later; the audio thread only holds one for a block.
    lfoShapeStore.erase (std::remove_if (lfoShapeStore.begin(), lfoShapeStore.end(),
                                         [now] (const StoredShape& stored) { return stored.retiredAt != 0 && now - stored.retiredAt > 500; }),
                         lfoShapeStore.end());

    if (save)
        apvts.state.setProperty ("lfoShape" + juce::String (lfo + 1), text, nullptr);
}

void AMVSynthAudioProcessor::restoreLfoShapes()
{
    // A state without a shape gets the default one, so a preset always sounds the same.
    for (int lfo = 0; lfo < amv::mod::numLfos; ++lfo)
        setLfoShape (lfo, amv::LfoShape::pointsFromString (apvts.state.getProperty ("lfoShape" + juce::String (lfo + 1)).toString()), false);
}

bool AMVSynthAudioProcessor::savePresetFile (const juce::File& file)
{
    const auto xml = apvts.copyState().createXml();
    return xml != nullptr && xml->writeTo (file);
}

bool AMVSynthAudioProcessor::loadPresetFile (const juce::File& file)
{
    const auto xml = juce::parseXML (file);
    if (xml == nullptr || ! xml->hasTagName (apvts.state.getType()))
        return false;

    // Start from the defaults so a file from an older version leaves no stale values behind.
    amv::applyPreset (apvts, amv::getFactoryPresets().front());

    // Tuning and MPE settings stay as they are in the project.
    std::vector<std::pair<juce::RangedAudioParameter*, float>> kept;
    for (auto* p : getParameters())
        if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (p))
            if (amv::isGlobalParameter (ranged->getParameterID()))
                kept.emplace_back (ranged, ranged->getValue());

    apvts.replaceState (juce::ValueTree::fromXml (*xml));
    restoreUserWavetables();
    restoreSample();
    restoreLfoShapes();

    for (const auto& [param, normalised] : kept)
        param->setValueNotifyingHost (normalised);

    return true;
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AMVSynthAudioProcessor();
}
