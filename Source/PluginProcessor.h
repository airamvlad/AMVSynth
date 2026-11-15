#pragma once

#include <juce_audio_utils/juce_audio_utils.h>

#include "Arpeggiator.h"
#include "Effects.h"
#include "Parameters.h"
#include "Presets.h"
#include "SynthEngine.h"

class AMVSynthAudioProcessor final : public juce::AudioProcessor
{
public:
    AMVSynthAudioProcessor();
    ~AMVSynthAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "AMVSynth"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 8.0; }

    int getNumPrograms() override { return (int) amv::getFactoryPresets().size(); }
    int getCurrentProgram() override { return currentPreset; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    /** Writes the current sound to a preset file, or loads one. */
    bool savePresetFile (const juce::File&);
    bool loadPresetFile (const juce::File&);

    /** Highest output level since the editor last read it; the editor resets it to zero. */
    std::array<std::atomic<float>, 2> outputPeak {};

    juce::AudioProcessorValueTreeState apvts;
    juce::MidiKeyboardState keyboardState;

private:
    amv::EngineParams readParams() const;
    amv::FxParams readFxParams() const;
    float value (const char* paramId) const;

    amv::SynthEngine engine;
    amv::Arpeggiator arp;
    amv::FxChain fx;
    juce::AudioBuffer<float> stereo;
    juce::MidiBuffer arpMidi;
    juce::SmoothedValue<float> outputGain;
    int currentPreset = 0;

    // The numbered modulation parameters are looked up once, their names are built at run time.
    struct LfoRefs { std::atomic<float>* shape; std::atomic<float>* rate; std::atomic<float>* sync; std::atomic<float>* division; };
    struct SlotRefs { std::atomic<float>* source; std::atomic<float>* dest; std::atomic<float>* amount; };
    std::array<LfoRefs, amv::mod::numLfos> lfoRefs {};
    std::array<std::atomic<float>*, amv::mod::numMacros> macroRefs {};
    std::array<SlotRefs, amv::mod::numSlots> slotRefs {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AMVSynthAudioProcessor)
};
