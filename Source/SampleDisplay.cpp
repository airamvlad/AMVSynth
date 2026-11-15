#include "SampleDisplay.h"
#include "PluginEditor.h"

SampleDisplay::SampleDisplay (AMVSynthAudioProcessor& p) : processor (p)
{
    using namespace amv;
    const char* ids[numMarkers] { id::smpStart, id::smpEnd, id::smpLoopStart, id::smpLoopEnd, id::smpPosition };
    for (int i = 0; i < numMarkers; ++i)
        params[(size_t) i] = processor.apvts.getParameter (ids[i]);

    mode = processor.apvts.getRawParameterValue (id::smpMode);
    loop = processor.apvts.getRawParameterValue (id::smpLoop);
    spray = processor.apvts.getRawParameterValue (id::smpSpray);
}

juce::Rectangle<float> SampleDisplay::waveArea() const
{
    return getLocalBounds().toFloat().reduced (8.0f, 4.0f).withTrimmedTop (12.0f).withTrimmedBottom (12.0f);
}

float SampleDisplay::xFor (float value) const
{
    const auto area = waveArea();
    return area.getX() + value * area.getWidth();
}

float SampleDisplay::valueAt (float x) const
{
    const auto area = waveArea();
    return juce::jlimit (0.0f, 1.0f, (x - area.getX()) / juce::jmax (1.0f, area.getWidth()));
}

bool SampleDisplay::markerShown (int marker) const
{
    if (marker == loopStart || marker == loopEnd)
        return shownMode == amv::sampler::classic && shownLoop != amv::sampler::loopOff;
    if (marker == position)
        return shownMode == amv::sampler::granular;
    return true;
}

int SampleDisplay::markerAt (juce::Point<float> p) const
{
    // In the lower half the loop handles win, everywhere else the start and end.
    const auto area = waveArea();
    const bool lower = p.y > area.getCentreY();
    const auto nearest = [&] (std::initializer_list<int> markers)
    {
        int best = none;
        float bestDistance = 9.0f;
        for (int m : markers)
        {
            const float distance = std::abs (p.x - xFor (shown[(size_t) m]));
            if (markerShown (m) && distance < bestDistance)
            {
                bestDistance = distance;
                best = m;
            }
        }
        return best;
    };

    int found = lower ? nearest ({ loopStart, loopEnd }) : (int) none;
    if (found == none)
        found = nearest ({ start, end });
    if (found == none && markerShown (position) && area.expanded (4.0f, 0.0f).contains (p))
        found = position;
    return found;
}

void SampleDisplay::setMarker (int marker, float value)
{
    // Start stays before end, and the loop inside the played part.
    const float smallest = 0.002f;
    switch (marker)
    {
        case start:     value = juce::jmin (value, shown[(size_t) end] - smallest); break;
        case end:       value = juce::jmax (value, shown[(size_t) start] + smallest); break;
        case loopStart: value = juce::jlimit (shown[(size_t) start], juce::jmax (shown[(size_t) start], shown[(size_t) loopEnd] - smallest), value); break;
        case loopEnd:   value = juce::jlimit (juce::jmin (shown[(size_t) end], shown[(size_t) loopStart] + smallest), shown[(size_t) end], value); break;
        case position:
        {
            // The grain position is counted inside the played part.
            const float span = juce::jmax (smallest, shown[(size_t) end] - shown[(size_t) start]);
            value = (value - shown[(size_t) start]) / span;
            break;
        }
        default: return;
    }

    if (auto* param = params[(size_t) marker])
        param->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, value));
}

void SampleDisplay::buildPeaks()
{
    const auto* data = processor.getSample();
    const int width = juce::jmax (1, juce::roundToInt (waveArea().getWidth()));
    peaksFor = data;
    peakLow.assign ((size_t) width, 0.0f);
    peakHigh.assign ((size_t) width, 0.0f);
    if (data == nullptr)
        return;

    const float* l = data->getChannel (0);
    const float* r = data->getChannel (1);
    const auto length = (double) data->getLength();

    for (int x = 0; x < width; ++x)
    {
        const int from = (int) (length * x / width);
        const int to = juce::jmax (from + 1, (int) (length * (x + 1) / width));
        const int step = juce::jmax (1, (to - from) / 256); // long samples are read sparsely
        float low = 1.0f, high = -1.0f;
        for (int i = from; i < to && i < data->getLength(); i += step)
        {
            const float v = 0.5f * (l[i] + r[i]);
            low = juce::jmin (low, v);
            high = juce::jmax (high, v);
        }
        peakLow[(size_t) x] = juce::jlimit (-1.0f, 1.0f, juce::jmin (low, high));
        peakHigh[(size_t) x] = juce::jlimit (-1.0f, 1.0f, juce::jmax (low, high));
    }
}

void SampleDisplay::refresh()
{
    bool changed = processor.getSample() != peaksFor;

    for (int m = 0; m < numMarkers; ++m)
        if (auto* param = params[(size_t) m])
        {
            const float value = param->getValue();
            changed = changed || std::abs (value - shown[(size_t) m]) > 1.0e-5f;
            shown[(size_t) m] = value;
        }

    const int newMode = juce::roundToInt (mode->load()), newLoop = juce::roundToInt (loop->load());
    const float newSpray = spray->load();
    changed = changed || newMode != shownMode || newLoop != shownLoop || std::abs (newSpray - shownSpray) > 1.0e-4f;
    shownMode = newMode;
    shownLoop = newLoop;
    shownSpray = newSpray;

    const int count = juce::jlimit (0, (int) heads.size(), processor.samplerPositionCount.load (std::memory_order_relaxed));
    changed = changed || count != numHeads || count > 0;
    numHeads = count;
    for (int i = 0; i < count; ++i)
        heads[(size_t) i] = processor.samplerPositions[(size_t) i].load (std::memory_order_relaxed);

    if (changed && isVisible())
        repaint();
}

void SampleDisplay::paint (juce::Graphics& g)
{
    using namespace amv;
    const auto bounds = getLocalBounds().toFloat();
    g.setColour (colour::background);
    g.fillRoundedRectangle (bounds, 6.0f);

    const auto area = waveArea();
    const auto* data = processor.getSample();

    if (data != peaksFor || peakLow.empty())
        buildPeaks();

    if (data == nullptr)
    {
        g.setColour (dropHover ? colour::teal : colour::dimText);
        g.setFont (juce::FontOptions (14.0f));
        g.drawText ("Drop an audio file here, or press LOAD", bounds, juce::Justification::centred);
        if (dropHover)
            g.drawRoundedRectangle (bounds.reduced (1.0f), 6.0f, 1.5f);
        return;
    }

    const float x0 = xFor (shown[(size_t) start]), x1 = xFor (shown[(size_t) end]);
    const float mid = area.getCentreY(), half = area.getHeight() * 0.5f;

    // The loop, or in granular mode the range the grains are spread over.
    if (markerShown (loopStart))
    {
        g.setColour (colour::forSource (mod::srcLfo1).withAlpha (0.13f));
        g.fillRect (juce::Rectangle<float>::leftTopRightBottom (xFor (shown[(size_t) loopStart]), area.getY(), xFor (shown[(size_t) loopEnd]), area.getBottom()));
    }

    const float grainX = x0 + shown[(size_t) position] * (x1 - x0);
    if (markerShown (position) && shownSpray > 0.0f)
    {
        const float reach = shownSpray * 0.25f * (x1 - x0);
        g.setColour (colour::forSource (mod::srcLfo2).withAlpha (0.16f));
        g.fillRect (juce::Rectangle<float>::leftTopRightBottom (juce::jmax (x0, grainX - reach), area.getY(), juce::jmin (x1, grainX + reach), area.getBottom()));
    }

    g.setColour (colour::track);
    g.drawHorizontalLine (juce::roundToInt (mid), area.getX(), area.getRight());

    // The wave: bright inside the played part, dim outside it.
    const int width = (int) peakLow.size();
    for (int pass = 0; pass < 2; ++pass)
    {
        juce::Path wave;
        for (int x = 0; x < width; ++x)
        {
            const float px = area.getX() + (float) x;
            if ((px >= x0 && px <= x1) != (pass == 1))
                continue;
            const float top = mid - peakHigh[(size_t) x] * half, bottom = mid - peakLow[(size_t) x] * half;
            wave.addRectangle (px, top, 1.0f, juce::jmax (1.0f, bottom - top));
        }
        g.setColour (pass == 1 ? colour::teal.withAlpha (0.9f) : colour::dimText.withAlpha (0.35f));
        g.fillPath (wave);
    }

    // Where the newest note is reading.
    g.setColour (colour::text.withAlpha (shownMode == sampler::granular ? 0.55f : 0.9f));
    for (int i = 0; i < numHeads; ++i)
        g.fillRect (xFor (juce::jlimit (0.0f, 1.0f, heads[(size_t) i])) - 0.5f, area.getY(), 1.0f, area.getHeight());

    const auto drawMarker = [&] (int marker, float x, juce::Colour c, bool top, const char* text)
    {
        const bool lit = hover == marker || dragging == marker;
        g.setColour (c.withAlpha (lit ? 1.0f : 0.8f));
        g.fillRect (x - (lit ? 1.0f : 0.5f), area.getY() - 2.0f, lit ? 2.0f : 1.0f, area.getHeight() + 4.0f);

        const bool flagLeft = marker == end || marker == loopEnd;
        auto flag = juce::Rectangle<float> (flagLeft ? x - 34.0f : x, top ? area.getY() - 14.0f : area.getBottom() + 2.0f, 34.0f, 12.0f);
        flag = flag.withX (juce::jlimit (bounds.getX(), bounds.getRight() - 34.0f, flag.getX()));
        g.fillRoundedRectangle (flag, 2.5f);
        g.setColour (colour::background);
        g.setFont (juce::FontOptions (9.0f, juce::Font::bold));
        g.drawText (text, flag, juce::Justification::centred);
    };

    drawMarker (start, x0, colour::teal, true, "START");
    drawMarker (end, x1, colour::teal, true, "END");
    if (markerShown (loopStart))
    {
        drawMarker (loopStart, xFor (shown[(size_t) loopStart]), colour::forSource (mod::srcLfo1), false, "LOOP");
        drawMarker (loopEnd, xFor (shown[(size_t) loopEnd]), colour::forSource (mod::srcLfo1), false, "LOOP");
    }
    if (markerShown (position))
        drawMarker (position, grainX, colour::forSource (mod::srcLfo2), false, "GRAIN");

    if (dropHover)
    {
        g.setColour (colour::teal);
        g.drawRoundedRectangle (bounds.reduced (1.0f), 6.0f, 1.5f);
    }
}

void SampleDisplay::mouseMove (const juce::MouseEvent& e)
{
    const int now = processor.getSample() != nullptr ? markerAt (e.position) : (int) none;
    if (now != hover)
    {
        hover = now;
        setMouseCursor (now == none ? juce::MouseCursor::NormalCursor : juce::MouseCursor::LeftRightResizeCursor);
        repaint();
    }
}

void SampleDisplay::mouseExit (const juce::MouseEvent&)
{
    hover = none;
    repaint();
}

void SampleDisplay::mouseDown (const juce::MouseEvent& e)
{
    dragging = processor.getSample() != nullptr ? markerAt (e.position) : (int) none;
    if (dragging == none)
        return;

    params[(size_t) dragging]->beginChangeGesture();
    setMarker (dragging, valueAt (e.position.x));
}

void SampleDisplay::mouseDrag (const juce::MouseEvent& e)
{
    if (dragging != none)
        setMarker (dragging, valueAt (e.position.x));
}

void SampleDisplay::mouseUp (const juce::MouseEvent&)
{
    if (dragging != none)
        params[(size_t) dragging]->endChangeGesture();
    dragging = none;
    repaint();
}

void SampleDisplay::filesDropped (const juce::StringArray& files, int, int)
{
    dropHover = false;
    const bool ok = files.size() > 0 && processor.loadSample (juce::File (files[0]));
    if (onFileDropped)
        onFileDropped (ok);
    repaint();
}
