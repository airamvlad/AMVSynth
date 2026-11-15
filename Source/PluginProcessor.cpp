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
        lfoRefs[(size_t) i] = { ref (id::lfo (i, "Shape")), ref (id::lfo (i, "Rate")), ref (id::lfo (i, "Sync")), ref (id::lfo (i, "Division")) };
    for (int i = 0; i < mod::numMacros; ++i)
        macroRefs[(size_t) i] = ref (id::macro (i));
    for (int i = 0; i < mod::numSlots; ++i)
        slotRefs[(size_t) i] = { ref (id::slot (i, "Source")), ref (id::slot (i, "Dest")), ref (id::slot (i, "Amount")) };
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
        case Model::juzniVetar:
        case Model::accordion: break;
    }

    p.engineLevel = value (id::engineLevel);
    p.oscA = { juce::roundToInt (value (id::oscATable)), value (id::oscAPos), juce::roundToInt (value (id::oscAUnison)),
               value (id::oscADetune), value (id::oscAPitch), value (id::oscAFine), value (id::oscALevel), value (id::oscAWidth) };
    p.oscB = { juce::roundToInt (value (id::oscBTable)), value (id::oscBPos), juce::roundToInt (value (id::oscBUnison)),
               value (id::oscBDetune), value (id::oscBPitch), value (id::oscBFine), value (id::oscBLevel), value (id::oscBWidth) };
    p.subLevel = value (id::subLevel);
    p.subOctave = juce::roundToInt (value (id::subOctave)) + 1;
    p.subShape = value (id::subShape);
    p.noiseLevel = value (id::noiseLevel);
    p.noiseColorHz = value (id::noiseColor);

    for (size_t i = 0; i < lfoRefs.size(); ++i)
        p.lfos[i] = { juce::roundToInt (lfoRefs[i].shape->load()), lfoRefs[i].rate->load(), lfoRefs[i].sync->load() > 0.5f,
                      juce::roundToInt (lfoRefs[i].division->load()) };
    for (size_t i = 0; i < macroRefs.size(); ++i)
        p.modMacros[i] = macroRefs[i]->load();
    for (size_t i = 0; i < slotRefs.size(); ++i)
        p.slots[i] = { juce::roundToInt (slotRefs[i].source->load()), juce::roundToInt (slotRefs[i].dest->load()), slotRefs[i].amount->load() };

    p.tone = value (id::tone);
    p.pulseWidth = value (id::pulseWidth);
    p.detuneCents = value (id::detune);
    p.vibDepthCents = value (id::vibDepth);
    p.vibRateHz = value (id::vibRate);
    p.vibDelaySec = value (id::vibDelay) * 0.001f;
    p.bendRange = value (id::bendRange);
    p.glideSec = value (id::glide) * 0.001f;
    p.mono = value (id::mono) > 0.5f;
    p.cutoffHz = value (id::cutoff);
    p.resonance = value (id::resonance);
    p.filterEnv = value (id::filterEnv);
    p.amp = { value (id::ampA) * 0.001f, value (id::ampD) * 0.001f, value (id::ampS), value (id::ampR) * 0.001f };
    p.flt = { value (id::fltA) * 0.001f, value (id::fltD) * 0.001f, value (id::fltS), value (id::fltR) * 0.001f };
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

    outputGain.setTargetValue (juce::Decibels::decibelsToGain (value (amv::id::level)));
    outputGain.applyGain (work, numSamples);

    fx.process (work, readFxParams(), transport);

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
        }
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
    apvts.replaceState (juce::ValueTree::fromXml (*xml));
    return true;
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AMVSynthAudioProcessor();
}
