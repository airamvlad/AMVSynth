#pragma once

#include "PluginProcessor.h"

/** The grid of the step sequencer.

    Four lanes share the same columns, one column per step: the note as a block on a two octave
    roll around the held key, then velocity and gate as bars, then the slide switches. Values are
    drawn with the mouse, also across several steps in one drag; a right click turns a step into
    a rest and back. The columns are grouped like the chosen meter, and the step that is playing
    lights up. */
class StepEditor final : public juce::Component,
                         public juce::TooltipClient
{
public:
    explicit StepEditor (AMVSynthAudioProcessor&);

    /** Called from the editor's timer: repaints when the pattern, its length, the meter or the
        playing step changed. */
    void refresh();

    // The pattern tools.
    void fitToMeter (int bars);
    void accentsFromMeter();
    void randomise();
    void shift (int by);
    void reverse();
    void clear();

    void paint (juce::Graphics&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    juce::String getTooltip() override;

private:
    enum Lane { noLane = -1, noteLane, velocityLane, gateLane, slideLane, numLanes };

    juce::Rectangle<float> grid() const;
    juce::Rectangle<float> laneArea (int lane) const;
    float stepWidth() const;
    int stepAt (float x) const;
    int laneAt (float y) const;
    bool isAccent (int step) const;
    void edit (int lane, int step, float y);
    void commit (bool save);
    juce::String valueText (int lane, int step) const;

    AMVSynthAudioProcessor& processor;
    amv::SeqPattern pattern;
    const amv::SeqPattern* patternFrom = nullptr;
    std::atomic<float>* lengthParam = nullptr;
    std::atomic<float>* meterParam = nullptr;
    std::atomic<float>* onParam = nullptr;

    int length = 7, meter = 2, playing = -1;
    bool on = false;
    int hoverLane = noLane, hoverStep = -1, dragLane = noLane;
    bool slideValue = false, restoring = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StepEditor)
};
