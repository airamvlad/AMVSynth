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

    /** Sets the drawn shape of an LFO (0 to 3), used when its shape is Custom. With `save` off the
        project state is not updated yet; call it again with `save` on when the edit is finished.
        Message thread only. */
    void setLfoShape (int lfo, std::vector<amv::LfoPoint> points, bool save);

    /** The points of the drawn shape of an LFO. Message thread only. */
    const std::vector<amv::LfoPoint>& getLfoPoints (int lfo) const noexcept { return activeLfoShape[(size_t) lfo].load()->getPoints(); }

    /** Sets one of the four patterns of the step sequencer (0 to 3). With `save` off the project
        state is not updated yet; call it again with `save` on when the edit is finished. Message
        thread only. */
    void setSeqPattern (int index, const amv::SeqPattern&, bool save);

    /** One of the patterns of the step sequencer. Message thread only. */
    const amv::SeqPattern& getSeqPattern (int index) const noexcept { return seqBank.patterns[(size_t) index]; }

    /** The order the chain plays the patterns in: up to eight numbers from 0 to 3. Message thread only. */
    void setSeqChain (const std::vector<int>& chain);
    std::vector<int> getSeqChain() const;

    /** Counts up whenever a pattern or the chain was changed, so a display knows to read them again. */
    int getSeqVersion() const noexcept { return seqVersion; }

    /** The pattern the step that played last came from, for the display. */
    std::atomic<int> seqPatternPlaying { 0 };

    /** The step the sequencer played last, or -1 when it is silent, for the display. */
    std::atomic<int> seqStep { -1 };

    /** Where each LFO is in its cycle, 0..1, for the display. */
    std::array<std::atomic<float>, amv::mod::numLfos> lfoPhases {};

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
    amv::StepSequencer sequencer;
    amv::FxChain fx;
    juce::AudioBuffer<float> stereo;
    juce::MidiBuffer arpMidi, seqMidi;
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

    // Drawn LFO shapes, handed over like the wavetables. There is always one per LFO.
    void restoreLfoShapes();
    std::array<std::atomic<const amv::LfoShape*>, amv::mod::numLfos> activeLfoShape {};
    struct StoredShape
    {
        std::unique_ptr<amv::LfoShape> shape;
        juce::uint32 retiredAt = 0;
    };
    std::vector<StoredShape> lfoShapeStore;

    // The sequencer patterns and the chain, handed over the same way. `seqBank` is the copy the
    // message thread edits; every change publishes a new copy to the audio thread.
    void publishSeqBank();
    void restoreSeqPattern();
    amv::SeqBank seqBank;
    int seqVersion = 0;
    std::atomic<const amv::SeqBank*> activeSeqBank { nullptr };
    struct StoredBank
    {
        std::unique_ptr<amv::SeqBank> bank;
        juce::uint32 retiredAt = 0;
    };
    std::vector<StoredBank> seqBankStore;

    // The numbered modulation parameters are looked up once, their names are built at run time.
    struct LfoRefs { std::atomic<float>* shape; std::atomic<float>* rate; std::atomic<float>* sync; std::atomic<float>* division; std::atomic<float>* mode; };
    struct SlotRefs { std::atomic<float>* source; std::atomic<float>* dest; std::atomic<float>* amount; std::atomic<float>* polarity; };
    std::array<LfoRefs, amv::mod::numLfos> lfoRefs {};
    std::array<std::atomic<float>*, amv::mod::numMacros> macroRefs {};
    std::array<SlotRefs, amv::mod::numSlots> slotRefs {};
    std::array<std::array<std::array<float, 2>, 4>, 17> engineRanges {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AMVSynthAudioProcessor)
};
