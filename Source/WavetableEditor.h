#pragma once

#include "PluginProcessor.h"

#include <functional>

namespace amv
{
/** Draws the frames of a wavetable stacked behind one another, the first frame in front.

    `getFrame` returns the tableSize samples of a frame. `position` (0..1) marks where the
    oscillator reads the table, or is negative for no mark; `selectedFrame` is drawn brighter. */
void drawWavetableStack (juce::Graphics&, juce::Rectangle<float> area, const std::function<const float* (int)>& getFrame,
                         int numFrames, float position, juce::Colour, int selectedFrame = -1);
} // namespace amv

/** The WAVE page: draws and edits the User wavetable of oscillator A or B.

    A table is 16 frames. The selected frame is drawn by hand or built from its harmonics, and
    the tools shape one frame or the whole table. Every change is heard at once. */
class WavetableEditor final : public juce::Component
{
public:
    explicit WavetableEditor (AMVSynthAudioProcessor&);

    /** Chooses the oscillator to edit, 0 for A and 1 for B, and loads its table. */
    void setOscillator (int osc);

    void paint (juce::Graphics&) override;
    void resized() override;
    void visibilityChanged() override;

private:
    static constexpr int numFrames = amv::UserWavetable::numFrames;
    static constexpr int tableSize = amv::UserWavetable::tableSize;
    static constexpr int numBars = 48;

    /** The selected frame as a curve that is drawn on with the mouse. */
    struct DrawArea final : public juce::Component
    {
        explicit DrawArea (WavetableEditor& e) : owner (e) {}
        void paint (juce::Graphics&) override;
        void mouseDown (const juce::MouseEvent&) override;
        void mouseDrag (const juce::MouseEvent&) override;
        void mouseUp (const juce::MouseEvent&) override;
        void draw (juce::Point<float> from, juce::Point<float> to);
        WavetableEditor& owner;
        juce::Point<float> last;
    };

    /** The harmonics of the selected frame as bars that are dragged up and down. */
    struct HarmonicArea final : public juce::Component
    {
        explicit HarmonicArea (WavetableEditor& e) : owner (e) {}
        void paint (juce::Graphics&) override;
        void mouseDown (const juce::MouseEvent&) override;
        void mouseDrag (const juce::MouseEvent&) override;
        void mouseUp (const juce::MouseEvent&) override;
        WavetableEditor& owner;
    };

    /** All frames stacked in depth. */
    struct StackView final : public juce::Component
    {
        explicit StackView (WavetableEditor& e) : owner (e) {}
        void paint (juce::Graphics&) override;
        WavetableEditor& owner;
    };

    /** One small picture per frame; a click selects the frame. */
    struct FrameStrip final : public juce::Component
    {
        explicit FrameStrip (WavetableEditor& e) : owner (e) {}
        void paint (juce::Graphics&) override;
        void mouseDown (const juce::MouseEvent&) override;
        void mouseDrag (const juce::MouseEvent& e) override { mouseDown (e); }
        juce::Rectangle<float> cell (int frame) const;
        WavetableEditor& owner;
    };

    float* frameData (int index) noexcept { return frames.data() + (size_t) index * (size_t) tableSize; }
    const float* frameData (int index) const noexcept { return frames.data() + (size_t) index * (size_t) tableSize; }

    void load();
    void selectFrame (int index);
    void beginEdit();                 // remembers the table for UNDO
    void changed (bool finished);     // sends the table to the synth; `finished` also saves it
    void applyToFrame (const std::function<void (float*)>&);
    void setShape (int shape);
    void newTable (int choice);
    void showNewMenu();
    void importFile();
    void exportFile();
    void updateHarmonics();
    void setHarmonic (int harmonic, float level);

    AMVSynthAudioProcessor& processor;
    int osc = 0, frame = 0;
    std::vector<float> frames, undoFrames, clipboard, spectrum;
    std::array<float, numBars> harmonics {};
    juce::String name { "My Table" };
    juce::uint32 lastSent = 0;
    juce::dsp::FFT fft { 11 };

    DrawArea drawArea { *this };
    HarmonicArea harmonicArea { *this };
    StackView stackView { *this };
    FrameStrip frameStrip { *this };

    std::array<juce::TextButton, 2> oscButtons;
    juce::Label nameLabel;
    juce::TextButton newButton { "NEW" }, importButton { "IMPORT WAV" }, exportButton { "EXPORT WAV" }, undoButton { "UNDO" };
    std::array<juce::TextButton, 5> shapeButtons;
    juce::TextButton smoothButton { "SMOOTH" }, normaliseButton { "NORMALIZE" }, invertButton { "INVERT" }, reverseButton { "REVERSE" };
    juce::TextButton copyButton { "COPY" }, pasteButton { "PASTE" }, fillButton { "FILL ALL" }, morphButton { "MORPH" };
    juce::ComboBox gridBox;
    std::unique_ptr<juce::FileChooser> fileChooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WavetableEditor)
};
