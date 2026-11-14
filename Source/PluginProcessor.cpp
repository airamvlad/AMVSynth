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

    engine.setParams (readParams());
    engine.process (buffer, midi);

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
