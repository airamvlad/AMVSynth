#pragma once

#include "PluginProcessor.h"

/** The waveform of the sampler with its markers.

    The start and end of the played part and of the loop are dragged by their handles; in
    granular mode a click or drag in the wave moves the grain position. The lines that run
    through it show where the newest note is reading. An audio file dropped here is loaded. */
class SampleDisplay final : public juce::Component,
                            public juce::FileDragAndDropTarget
{
public:
    explicit SampleDisplay (AMVSynthAudioProcessor&);

    /** Called from the editor's timer: repaints when the sample, a marker or a playhead moved. */
    void refresh();

    /** Called after a file was dropped, with whether it could be loaded. */
    std::function<void (bool)> onFileDropped;

    void paint (juce::Graphics&) override;
    void resized() override { peaksFor = nullptr; }
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    bool isInterestedInFileDrag (const juce::StringArray&) override { return true; }
    void fileDragEnter (const juce::StringArray&, int, int) override { dropHover = true; repaint(); }
    void fileDragExit (const juce::StringArray&) override { dropHover = false; repaint(); }
    void filesDropped (const juce::StringArray&, int, int) override;

private:
    enum Marker { none = -1, start, end, loopStart, loopEnd, position, numMarkers };

    juce::Rectangle<float> waveArea() const;
    float xFor (float value) const;
    float valueAt (float x) const;
    int markerAt (juce::Point<float>) const;
    bool markerShown (int marker) const;
    void setMarker (int marker, float value);
    void buildPeaks();

    AMVSynthAudioProcessor& processor;
    std::array<juce::RangedAudioParameter*, numMarkers> params {};
    std::array<float, numMarkers> shown {};
    std::atomic<float>* mode = nullptr;
    std::atomic<float>* loop = nullptr;
    std::atomic<float>* spray = nullptr;
    int shownMode = -1, shownLoop = -1;
    float shownSpray = -1.0f;

    const amv::SampleData* peaksFor = nullptr;
    std::vector<float> peakLow, peakHigh;
    std::array<float, 16> heads {};
    int numHeads = 0;
    int hover = none, dragging = none;
    bool dropHover = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SampleDisplay)
};
