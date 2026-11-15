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
    bool supportsMPE() const override { return true; }
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

    /** Reads an audio file as the "User" wavetable of oscillator 0 (A) or 1 (B). Message thread only. */
    bool importWavetable (int osc, const juce::File&);

    /** Makes 16 cycles of 2048 samples the "User" wavetable of an oscillator, as the wavetable
        editor does while drawing. With `save` off the project state is not updated yet; call
        it again with `save` on when the edit is finished. Message thread only. */
    void setUserWavetableFrames (int osc, std::vector<float> frames, const juce::String& name, bool save);

    /** Writes the User wavetable of an oscillator as a WAV file of 16 frames. */
    bool exportWavetable (int osc, const juce::File&) const;

    /** The imported wavetable of an oscillator, or null. Message thread only. */
    const amv::UserWavetable* getUserWavetable (int osc) const noexcept { return activeUserWave[(size_t) osc].load(); }

    /** Loads an audio file into the sampler; at most the first 60 seconds are used. Message thread only. */
    bool loadSample (const juce::File&);

    /** Makes `data` the sample of the sampler, or removes it when null. The sample is saved
        with the project. Message thread only. */
    void setSample (std::unique_ptr<amv::SampleData> data);

    /** The loaded sample, or null. Message thread only. */
    const amv::SampleData* getSample() const noexcept { return activeSample.load(); }

    /** Where the sampler of the newest note is reading, 0..1 through the sample, for the display. */
    std::array<std::atomic<float>, 16> samplerPositions {};
    std::atomic<int> samplerPositionCount { 0 };

    /** The modulation sources as they are now, indexed by amv::mod::Source, for the display. */
    std::array<std::atomic<float>, amv::mod::numSources> modSourceValues {};

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

    // Imported wavetables. The audio thread only reads the pointers; the tables themselves are
    // made and thrown away on the message thread, a few imports after they were replaced.
    void setUserWavetable (int osc, std::unique_ptr<amv::UserWavetable>);
    void storeUserWavetables();
    void restoreUserWavetables();
    std::array<std::atomic<const amv::UserWavetable*>, 2> activeUserWave {};
    struct StoredWave
    {
        std::unique_ptr<amv::UserWavetable> table;
        juce::uint32 retiredAt = 0; // 0 while it is in use
    };
    std::vector<StoredWave> userWaveStore;

    // The sample is handed to the audio thread like the wavetables.
    void storeSample();
    void restoreSample();
    std::atomic<const amv::SampleData*> activeSample { nullptr };
    struct StoredSample
    {
        std::unique_ptr<amv::SampleData> data;
        juce::uint32 retiredAt = 0;
    };
    std::vector<StoredSample> sampleStore;

    // The numbered modulation parameters are looked up once, their names are built at run time.
    struct LfoRefs { std::atomic<float>* shape; std::atomic<float>* rate; std::atomic<float>* sync; std::atomic<float>* division; };
    struct SlotRefs { std::atomic<float>* source; std::atomic<float>* dest; std::atomic<float>* amount; std::atomic<float>* polarity; };
    std::array<LfoRefs, amv::mod::numLfos> lfoRefs {};
    std::array<std::atomic<float>*, amv::mod::numMacros> macroRefs {};
    std::array<SlotRefs, amv::mod::numSlots> slotRefs {};
    std::array<std::array<std::array<float, 2>, 4>, 17> engineRanges {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AMVSynthAudioProcessor)
};
