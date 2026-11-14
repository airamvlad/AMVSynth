#include "PluginProcessor.h"
#include "PluginEditor.h"

AMVSynthAudioProcessor::AMVSynthAudioProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "AMVSynth", amv::createParameterLayout())
{
}

void AMVSynthAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    engine.prepare (sampleRate, samplesPerBlock);
    arp.prepare (sampleRate);
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
        case Model::juzniVetar:
        case Model::accordion: break;
    }

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

    engine.setParams (readParams());
    engine.process (buffer, arpMidi);

    outputGain.setTargetValue (juce::Decibels::decibelsToGain (value (amv::id::level)));
    outputGain.applyGain (buffer, buffer.getNumSamples());

    // Safety clip so a resonant patch can never send more than full scale to the DAW.
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        juce::FloatVectorOperations::clip (buffer.getWritePointer (ch), buffer.getReadPointer (ch),
                                           -1.0f, 1.0f, buffer.getNumSamples());
}

juce::AudioProcessorEditor* AMVSynthAudioProcessor::createEditor()
{
    return new AMVSynthAudioProcessorEditor (*this);
}

void AMVSynthAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void AMVSynthAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AMVSynthAudioProcessor();
}
