#include "WavetableEditor.h"
#include "PluginEditor.h"

namespace amv
{
void drawWavetableStack (juce::Graphics& g, juce::Rectangle<float> area, const std::function<const float* (int)>& getFrame,
                         int numFrames, float position, juce::Colour tint, int selectedFrame)
{
    // Every frame sits a little higher and further right than the one before it.
    const float waveWidth = area.getWidth() * 0.74f, shiftX = area.getWidth() - waveWidth;
    const float amplitude = area.getHeight() * 0.17f;
    const float front = area.getBottom() - amplitude - 2.0f, back = area.getY() + amplitude + 2.0f;
    const int points = juce::jlimit (32, 192, (int) waveWidth / 2);
    const int size = UserWavetable::tableSize;

    const auto outline = [&] (float depth, const std::function<float (int)>& sample)
    {
        const float x0 = area.getX() + depth * shiftX, y0 = front + depth * (back - front);
        juce::Path path;
        for (int i = 0; i < points; ++i)
        {
            const int index = juce::jmin (size - 1, i * size / (points - 1));
            const float x = x0 + waveWidth * (float) i / (float) (points - 1);
            const float y = y0 - juce::jlimit (-1.0f, 1.0f, sample (index)) * amplitude;
            if (i == 0)
                path.startNewSubPath (x, y);
            else
                path.lineTo (x, y);
        }
        return path;
    };

    // From the back to the front, each frame hiding what lies behind it.
    for (int f = numFrames - 1; f >= 0; --f)
    {
        const float* data = getFrame (f);
        if (data == nullptr)
            continue;

        const float depth = numFrames > 1 ? (float) f / (float) (numFrames - 1) : 0.0f;
        const auto path = outline (depth, [data] (int i) { return data[i]; });

        juce::Path cover (path);
        const float x0 = area.getX() + depth * shiftX, floorY = front + depth * (back - front) + amplitude + 2.0f;
        cover.lineTo (x0 + waveWidth, floorY);
        cover.lineTo (x0, floorY);
        cover.closeSubPath();
        g.setColour (colour::background.withAlpha (0.86f));
        g.fillPath (cover);

        const bool selected = f == selectedFrame;
        g.setColour (selected ? colour::text : tint.withAlpha (0.28f + 0.3f * (1.0f - depth)));
        g.strokePath (path, juce::PathStrokeType (selected ? 1.8f : 1.1f));
    }

    if (position < 0.0f || numFrames < 2)
        return;

    // The shape that is playing: the two frames around the position, mixed.
    const float framePos = juce::jlimit (0.0f, 1.0f, position) * (float) (numFrames - 1);
    const int f0 = juce::jmin ((int) framePos, numFrames - 2);
    const float mix = framePos - (float) f0;
    const float* a = getFrame (f0);
    const float* b = getFrame (f0 + 1);
    if (a == nullptr || b == nullptr)
        return;

    const auto path = outline (framePos / (float) (numFrames - 1), [a, b, mix] (int i) { return a[i] + (b[i] - a[i]) * mix; });
    g.setColour (tint.withAlpha (0.3f));
    g.strokePath (path, juce::PathStrokeType (5.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (tint.brighter (0.25f));
    g.strokePath (path, juce::PathStrokeType (1.9f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}
} // namespace amv

namespace
{
constexpr float twoPi = juce::MathConstants<float>::twoPi;
const char* const shapeNames[5] { "SINE", "SAW", "SQUARE", "TRI", "NOISE" };

float shapeAt (int shape, float phase, juce::Random& random)
{
    switch (shape)
    {
        case 1:  return 1.0f - 2.0f * phase;
        case 2:  return phase < 0.5f ? 1.0f : -1.0f;
        case 3:  return phase < 0.25f ? 4.0f * phase : phase < 0.75f ? 2.0f - 4.0f * phase : 4.0f * phase - 4.0f;
        case 4:  return random.nextFloat() * 2.0f - 1.0f;
        default: return std::sin (twoPi * phase);
    }
}

void paintPanel (juce::Graphics& g, juce::Rectangle<float> bounds, const juce::String& title)
{
    g.setColour (amv::colour::background);
    g.fillRoundedRectangle (bounds, 8.0f);
    g.setColour (amv::colour::panelBorder);
    g.drawRoundedRectangle (bounds.reduced (0.5f), 8.0f, 1.0f);
    g.setColour (amv::colour::dimText);
    g.setFont (juce::FontOptions (10.5f, juce::Font::bold));
    g.drawText (title, bounds.reduced (10.0f, 6.0f).removeFromTop (12.0f), juce::Justification::centredLeft);
}
} // namespace

//==============================================================================
WavetableEditor::WavetableEditor (AMVSynthAudioProcessor& p)
    : processor (p)
{
    using namespace amv;
    frames.assign ((size_t) numFrames * (size_t) tableSize, 0.0f);

    const auto style = [this] (juce::TextButton& button, const juce::String& tooltip)
    {
        button.setColour (juce::TextButton::buttonColourId, colour::knob);
        button.setColour (juce::TextButton::buttonOnColourId, colour::teal.withAlpha (0.55f));
        button.setColour (juce::TextButton::textColourOffId, colour::text);
        button.setColour (juce::TextButton::textColourOnId, colour::text);
        button.setTooltip (tooltip);
        addAndMakeVisible (button);
    };

    for (size_t i = 0; i < oscButtons.size(); ++i)
    {
        oscButtons[i].setButtonText (i == 0 ? "OSC A" : "OSC B");
        style (oscButtons[i], "Edit the User wavetable of this oscillator.");
        oscButtons[i].onClick = [this, i] { setOscillator ((int) i); };
    }

    nameLabel.setEditable (true);
    nameLabel.setFont (juce::FontOptions (14.0f, juce::Font::bold));
    nameLabel.setColour (juce::Label::textColourId, colour::text);
    nameLabel.setColour (juce::Label::backgroundWhenEditingColourId, colour::background);
    nameLabel.setColour (juce::Label::textWhenEditingColourId, colour::text);
    nameLabel.setTooltip ("The name of the table. Click to change it.");
    nameLabel.onTextChange = [this]
    {
        name = nameLabel.getText().trim().isEmpty() ? juce::String ("My Table") : nameLabel.getText().trim();
        changed (true);
    };
    addAndMakeVisible (nameLabel);

    style (newButton, "Start a new table from a basic shape or from a copy of a built-in table.");
    style (importButton, "Load a WAV, AIFF or FLAC file: a row of 2048-sample frames, or a single cycle.");
    style (exportButton, "Save this table as a WAV file of 16 frames.");
    style (undoButton, "Take back the last change.");
    newButton.onClick = [this] { showNewMenu(); };
    importButton.onClick = [this] { importFile(); };
    exportButton.onClick = [this] { exportFile(); };
    undoButton.onClick = [this]
    {
        if (undoFrames.size() == frames.size())
        {
            std::swap (frames, undoFrames);
            updateHarmonics();
            changed (true);
        }
    };

    for (size_t i = 0; i < shapeButtons.size(); ++i)
    {
        shapeButtons[i].setButtonText (shapeNames[i]);
        style (shapeButtons[i], "Replace the selected frame with this shape.");
        shapeButtons[i].onClick = [this, i] { setShape ((int) i); };
    }

    style (smoothButton, "Round off the selected frame.");
    style (normaliseButton, "Centre the selected frame and bring it to full level.");
    style (invertButton, "Turn the selected frame upside down.");
    style (reverseButton, "Play the selected frame backwards.");
    style (copyButton, "Copy the selected frame.");
    style (pasteButton, "Replace the selected frame with the copied one.");
    style (fillButton, "Copy the selected frame to all 16 frames.");
    style (morphButton, "Fill the frames between the first frame and the selected frame with a gradual blend. With the first frame selected, blend from the first to the last.");

    smoothButton.onClick = [this]
    {
        applyToFrame ([] (float* data)
        {
            std::vector<float> copy ((size_t) tableSize);
            for (int pass = 0; pass < 12; ++pass)
            {
                std::copy (data, data + tableSize, copy.begin());
                for (int i = 0; i < tableSize; ++i)
                    data[i] = 0.25f * copy[(size_t) ((i + tableSize - 1) % tableSize)] + 0.5f * copy[(size_t) i] + 0.25f * copy[(size_t) ((i + 1) % tableSize)];
            }
        });
    };
    normaliseButton.onClick = [this]
    {
        applyToFrame ([] (float* data)
        {
            float sum = 0.0f, peak = 0.0f;
            for (int i = 0; i < tableSize; ++i)
                sum += data[i];
            const float mean = sum / (float) tableSize;
            for (int i = 0; i < tableSize; ++i)
                peak = juce::jmax (peak, std::abs (data[i] - mean));
            for (int i = 0; i < tableSize; ++i)
                data[i] = peak > 1.0e-6f ? (data[i] - mean) / peak : 0.0f;
        });
    };
    invertButton.onClick = [this]
    {
        applyToFrame ([] (float* data)
        {
            for (int i = 0; i < tableSize; ++i)
                data[i] = -data[i];
        });
    };
    reverseButton.onClick = [this] { applyToFrame ([] (float* data) { std::reverse (data, data + tableSize); }); };
    copyButton.onClick = [this] { clipboard.assign (frameData (frame), frameData (frame) + tableSize); };
    pasteButton.onClick = [this]
    {
        if (clipboard.size() == (size_t) tableSize)
            applyToFrame ([this] (float* data) { std::copy (clipboard.begin(), clipboard.end(), data); });
    };
    fillButton.onClick = [this]
    {
        beginEdit();
        for (int f = 0; f < numFrames; ++f)
            if (f != frame)
                std::copy (frameData (frame), frameData (frame) + tableSize, frameData (f));
        changed (true);
    };
    morphButton.onClick = [this]
    {
        beginEdit();
        const int last = frame == 0 ? numFrames - 1 : frame;
        for (int f = 1; f < last; ++f)
        {
            const float mix = (float) f / (float) last;
            for (int i = 0; i < tableSize; ++i)
                frameData (f)[i] = frameData (0)[i] + (frameData (last)[i] - frameData (0)[i]) * mix;
        }
        changed (true);
    };

    gridBox.addItemList ({ "Draw: Free", "Draw: 8 Steps", "Draw: 16 Steps", "Draw: 32 Steps", "Draw: 64 Steps" }, 1);
    gridBox.setSelectedId (1, juce::dontSendNotification);
    gridBox.setTooltip ("Free draws a line under the mouse. Steps draws flat blocks, for stepped and square shapes.");
    gridBox.onChange = [this] { drawArea.repaint(); };
    addAndMakeVisible (gridBox);

    addAndMakeVisible (drawArea);
    addAndMakeVisible (harmonicArea);
    addAndMakeVisible (stackView);
    addAndMakeVisible (frameStrip);

    setOscillator (0);
}

void WavetableEditor::setOscillator (int newOsc)
{
    osc = juce::jlimit (0, 1, newOsc);
    for (size_t i = 0; i < oscButtons.size(); ++i)
        oscButtons[i].setToggleState ((int) i == osc, juce::dontSendNotification);

    load();
}

void WavetableEditor::visibilityChanged()
{
    // The table can have changed while the page was hidden, for example with a preset.
    if (isVisible())
        load();
}

void WavetableEditor::load()
{
    using namespace amv;

    if (const auto* table = processor.getUserWavetable (osc))
    {
        frames = table->getSource();
        name = table->getName().isEmpty() ? juce::String ("My Table") : table->getName();
    }
    else
    {
        // Nothing of the user's yet: start from the table the oscillator plays now. It only
        // becomes the User table with the first change.
        const int builtIn = juce::roundToInt (processor.apvts.getRawParameterValue (osc == 0 ? id::oscATable : id::oscBTable)->load());
        const int source = builtIn < WavetableBank::numTables ? juce::jmax (0, builtIn) : 0;
        for (int f = 0; f < numFrames; ++f)
            std::copy (WavetableBank::get().wave (source, f, 0), WavetableBank::get().wave (source, f, 0) + tableSize, frameData (f));
        name = juce::String (getWavetableNames()[(size_t) source]) + " Edit";
    }

    frames.resize ((size_t) numFrames * (size_t) tableSize, 0.0f);
    undoFrames.clear();
    frame = juce::jlimit (0, numFrames - 1, frame);
    nameLabel.setText (name, juce::dontSendNotification);
    updateHarmonics();
    repaint();
}

void WavetableEditor::selectFrame (int index)
{
    index = juce::jlimit (0, numFrames - 1, index);
    if (index == frame)
        return;

    frame = index;
    updateHarmonics();
    repaint();
}

void WavetableEditor::beginEdit()
{
    undoFrames = frames;
}

void WavetableEditor::changed (bool finished)
{
    // While the mouse is moving the synth gets the table a few times a second; the last one always goes.
    const auto now = juce::Time::getMillisecondCounter();
    if (finished || now - lastSent > 60)
    {
        lastSent = now;
        processor.setUserWavetableFrames (osc, frames, name, finished);
    }

    if (finished)
    {
        // The oscillator plays what is being edited.
        if (auto* param = processor.apvts.getParameter (osc == 0 ? amv::id::oscATable : amv::id::oscBTable))
            if (param->getValue() < 1.0f)
            {
                param->beginChangeGesture();
                param->setValueNotifyingHost (1.0f);
                param->endChangeGesture();
            }

        updateHarmonics();
    }

    repaint();
}

void WavetableEditor::applyToFrame (const std::function<void (float*)>& operation)
{
    beginEdit();
    operation (frameData (frame));
    changed (true);
}

void WavetableEditor::setShape (int shape)
{
    applyToFrame ([shape] (float* data)
    {
        juce::Random random;
        for (int i = 0; i < tableSize; ++i)
            data[i] = shapeAt (shape, (float) i / (float) tableSize, random);
    });
}

void WavetableEditor::newTable (int choice)
{
    using namespace amv;
    beginEdit();
    juce::Random random;

    if (choice < 5)
    {
        // One shape in every frame.
        for (int f = 0; f < numFrames; ++f)
            for (int i = 0; i < tableSize; ++i)
                frameData (f)[i] = shapeAt (choice, (float) i / (float) tableSize, random);
        name = juce::String (shapeNames[choice]).toLowerCase() + " table";
    }
    else if (choice == 5)
    {
        // A sine that turns into a saw, as a first table to play with.
        for (int f = 0; f < numFrames; ++f)
        {
            const float mix = (float) f / (float) (numFrames - 1);
            for (int i = 0; i < tableSize; ++i)
            {
                const float phase = (float) i / (float) tableSize;
                frameData (f)[i] = shapeAt (0, phase, random) * (1.0f - mix) + shapeAt (1, phase, random) * mix;
            }
        }
        name = "sine to saw";
    }
    else
    {
        const int source = juce::jlimit (0, WavetableBank::numTables - 1, choice - 10);
        for (int f = 0; f < numFrames; ++f)
            std::copy (WavetableBank::get().wave (source, f, 0), WavetableBank::get().wave (source, f, 0) + tableSize, frameData (f));
        name = juce::String (getWavetableNames()[(size_t) source]) + " Edit";
    }

    nameLabel.setText (name, juce::dontSendNotification);
    changed (true);
}

void WavetableEditor::showNewMenu()
{
    juce::PopupMenu menu, copies;
    menu.addSectionHeader ("New table");
    menu.addItem (1, "Sine");
    menu.addItem (2, "Saw");
    menu.addItem (3, "Square");
    menu.addItem (4, "Triangle");
    menu.addItem (6, "Sine to saw");

    for (int t = 0; t < amv::WavetableBank::numTables; ++t)
        copies.addItem (11 + t, amv::getWavetableNames()[(size_t) t]);
    menu.addSubMenu ("Copy of a built-in table", copies);

    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&newButton),
                        [safe = juce::Component::SafePointer<WavetableEditor> (this)] (int result)
                        {
                            if (safe != nullptr && result > 0)
                                safe->newTable (result - 1);
                        });
}

void WavetableEditor::importFile()
{
    const auto folder = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory);
    fileChooser = std::make_unique<juce::FileChooser> ("Import wavetable", folder, "*.wav;*.aif;*.aiff;*.flac");
    fileChooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                              [this] (const juce::FileChooser& chooser)
                              {
                                  const auto file = chooser.getResult();
                                  if (file == juce::File())
                                      return;

                                  const auto before = frames;
                                  if (! processor.importWavetable (osc, file))
                                  {
                                      juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "Import wavetable",
                                                                              "This file could not be read as audio.");
                                      return;
                                  }

                                  load();
                                  undoFrames = before;
                                  changed (true);
                              });
}

void WavetableEditor::exportFile()
{
    // Make sure the synth has exactly what is on screen.
    processor.setUserWavetableFrames (osc, frames, name, true);

    const auto folder = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile (juce::File::createLegalFileName (name) + ".wav");
    fileChooser = std::make_unique<juce::FileChooser> ("Export wavetable", folder, "*.wav");
    fileChooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles | juce::FileBrowserComponent::warnAboutOverwriting,
                              [this] (const juce::FileChooser& chooser)
                              {
                                  const auto file = chooser.getResult();
                                  if (file != juce::File() && ! processor.exportWavetable (osc, file.withFileExtension ("wav")))
                                      juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "Export wavetable",
                                                                              "The file could not be written.");
                              });
}

void WavetableEditor::updateHarmonics()
{
    spectrum.assign ((size_t) tableSize * 2, 0.0f);
    std::copy (frameData (frame), frameData (frame) + tableSize, spectrum.begin());
    fft.performRealOnlyForwardTransform (spectrum.data());

    for (int h = 1; h <= numBars; ++h)
        harmonics[(size_t) (h - 1)] = std::hypot (spectrum[(size_t) h * 2], spectrum[(size_t) h * 2 + 1]) / (float) (tableSize / 2);
}

void WavetableEditor::setHarmonic (int harmonic, float level)
{
    // `spectrum` holds the frame as it was when the drag began, changed bar by bar since.
    const float target = juce::jlimit (0.0f, 1.0f, level) * (float) (tableSize / 2);
    const auto bin = (size_t) harmonic * 2, mirror = (size_t) (tableSize - harmonic) * 2;
    const float current = std::hypot (spectrum[bin], spectrum[bin + 1]);

    if (current > 1.0e-4f)
    {
        const float scale = target / current;
        for (const auto index : { bin, bin + 1, mirror, mirror + 1 })
            spectrum[index] *= scale;
    }
    else
    {
        // A harmonic that was silent comes in as a sine.
        spectrum[bin] = 0.0f;
        spectrum[bin + 1] = -target;
        spectrum[mirror] = 0.0f;
        spectrum[mirror + 1] = target;
    }

    harmonics[(size_t) (harmonic - 1)] = juce::jlimit (0.0f, 1.0f, level);

    std::vector<float> buffer (spectrum);
    fft.performRealOnlyInverseTransform (buffer.data());
    std::copy (buffer.begin(), buffer.begin() + tableSize, frameData (frame));
}

//==============================================================================
void WavetableEditor::paint (juce::Graphics& g)
{
    g.setColour (amv::colour::dimText);
    g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
    g.drawText ("FRAME " + juce::String (frame + 1) + " / " + juce::String (numFrames), shapeButtons[0].getBounds().translated (-88, 0).withWidth (84),
                juce::Justification::centredLeft);
}

void WavetableEditor::resized()
{
    auto area = getLocalBounds();
    const int gap = 8;

    // Top row: which oscillator, the name of the table, and the file buttons.
    auto top = area.removeFromTop (30);
    area.removeFromTop (gap);
    oscButtons[0].setBounds (top.removeFromLeft (64));
    top.removeFromLeft (4);
    oscButtons[1].setBounds (top.removeFromLeft (64));
    top.removeFromLeft (12);
    undoButton.setBounds (top.removeFromRight (62));
    top.removeFromRight (gap);
    exportButton.setBounds (top.removeFromRight (104));
    top.removeFromRight (4);
    importButton.setBounds (top.removeFromRight (104));
    top.removeFromRight (4);
    newButton.setBounds (top.removeFromRight (62));
    top.removeFromRight (gap);
    gridBox.setBounds (top.removeFromRight (140));
    top.removeFromRight (gap);
    nameLabel.setBounds (top);

    // Second row: the tools for the selected frame, then the ones for the whole table.
    auto tools = area.removeFromTop (28);
    area.removeFromTop (gap);
    tools.removeFromLeft (88);
    const auto place = [&tools] (juce::Component& c, int width, int space = 4)
    {
        c.setBounds (tools.removeFromLeft (width));
        tools.removeFromLeft (space);
    };
    static const int shapeWidths[5] { 54, 54, 72, 48, 62 };
    for (size_t i = 0; i < shapeButtons.size(); ++i)
        place (shapeButtons[i], shapeWidths[i]);
    tools.removeFromLeft (8);
    place (smoothButton, 74);
    place (normaliseButton, 96);
    place (invertButton, 68);
    place (reverseButton, 78, 12);
    place (copyButton, 58);
    place (pasteButton, 62, 12);
    place (fillButton, 74);
    place (morphButton, 70);

    // Left: the frame and its harmonics. Right: the whole table.
    auto left = area.removeFromLeft (area.getWidth() * 62 / 100);
    area.removeFromLeft (gap);
    harmonicArea.setBounds (left.removeFromBottom (left.getHeight() * 32 / 100));
    left.removeFromBottom (gap);
    drawArea.setBounds (left);
    frameStrip.setBounds (area.removeFromBottom (area.getHeight() * 30 / 100));
    area.removeFromBottom (gap);
    stackView.setBounds (area);
}

//==============================================================================
void WavetableEditor::DrawArea::paint (juce::Graphics& g)
{
    using namespace amv;
    const auto bounds = getLocalBounds().toFloat();
    paintPanel (g, bounds, "DRAW THE FRAME");
    const auto area = bounds.reduced (12.0f, 24.0f);

    // Grid: the steps when drawing in steps, otherwise eight divisions.
    const int steps = owner.gridBox.getSelectedId() <= 1 ? 8 : 4 << (owner.gridBox.getSelectedId() - 1);
    g.setColour (colour::track.withAlpha (0.45f));
    for (int i = 0; i <= steps; ++i)
        g.drawVerticalLine (juce::roundToInt (area.getX() + area.getWidth() * (float) i / (float) steps), area.getY(), area.getBottom());
    for (int i = 0; i <= 4; ++i)
        g.drawHorizontalLine (juce::roundToInt (area.getY() + area.getHeight() * (float) i / 4.0f), area.getX(), area.getRight());
    g.setColour (colour::track);
    g.fillRect (area.getX(), area.getCentreY() - 0.5f, area.getWidth(), 1.0f);

    const float* data = owner.frameData (owner.frame);
    juce::Path path;
    const int points = juce::jmax (2, (int) area.getWidth());
    for (int i = 0; i < points; ++i)
    {
        const int index = juce::jmin (tableSize - 1, i * tableSize / (points - 1));
        const float x = area.getX() + (float) i, y = area.getCentreY() - juce::jlimit (-1.0f, 1.0f, data[index]) * area.getHeight() * 0.5f;
        if (i == 0)
            path.startNewSubPath (x, y);
        else
            path.lineTo (x, y);
    }

    juce::Path fill (path);
    fill.lineTo (area.getRight(), area.getCentreY());
    fill.lineTo (area.getX(), area.getCentreY());
    fill.closeSubPath();
    g.setColour (colour::teal.withAlpha (0.12f));
    g.fillPath (fill);
    g.setColour (colour::teal);
    g.strokePath (path, juce::PathStrokeType (1.8f));
}

void WavetableEditor::DrawArea::draw (juce::Point<float> from, juce::Point<float> to)
{
    const auto area = getLocalBounds().toFloat().reduced (12.0f, 24.0f);
    const auto valueAt = [&area] (float y) { return juce::jlimit (-1.0f, 1.0f, (area.getCentreY() - y) / (area.getHeight() * 0.5f)); };
    const auto unitAt = [&area] (float x) { return juce::jlimit (0.0f, 0.99999f, (x - area.getX()) / area.getWidth()); };
    float* data = owner.frameData (owner.frame);

    const int steps = owner.gridBox.getSelectedId() <= 1 ? 0 : 4 << (owner.gridBox.getSelectedId() - 1);
    if (steps > 0)
    {
        // Every step the mouse passed over becomes flat, at the height the mouse had there.
        const int c0 = (int) (unitAt (from.x) * (float) steps), c1 = (int) (unitAt (to.x) * (float) steps);
        for (int c = juce::jmin (c0, c1); c <= juce::jmax (c0, c1); ++c)
        {
            const float t = c1 == c0 ? 1.0f : (float) (c - c0) / (float) (c1 - c0);
            const float value = valueAt (from.y + (to.y - from.y) * t);
            for (int i = c * tableSize / steps; i < (c + 1) * tableSize / steps; ++i)
                data[i] = value;
        }
        return;
    }

    // A straight line between the last two mouse positions.
    const int i0 = (int) (unitAt (from.x) * (float) tableSize), i1 = (int) (unitAt (to.x) * (float) tableSize);
    for (int i = juce::jmin (i0, i1); i <= juce::jmax (i0, i1); ++i)
    {
        const float t = i1 == i0 ? 1.0f : (float) (i - i0) / (float) (i1 - i0);
        data[i] = valueAt (from.y + (to.y - from.y) * t);
    }
}

void WavetableEditor::DrawArea::mouseDown (const juce::MouseEvent& e)
{
    owner.beginEdit();
    last = e.position;
    draw (last, last);
    owner.changed (false);
}

void WavetableEditor::DrawArea::mouseDrag (const juce::MouseEvent& e)
{
    draw (last, e.position);
    last = e.position;
    owner.changed (false);
}

void WavetableEditor::DrawArea::mouseUp (const juce::MouseEvent&)
{
    owner.changed (true);
}

//==============================================================================
void WavetableEditor::HarmonicArea::paint (juce::Graphics& g)
{
    using namespace amv;
    const auto bounds = getLocalBounds().toFloat();
    paintPanel (g, bounds, "HARMONICS 1 TO " + juce::String (numBars) + "  (DRAG THE BARS)");
    const auto area = bounds.reduced (12.0f, 8.0f).withTrimmedTop (16.0f);
    const float width = area.getWidth() / (float) numBars;

    for (int i = 0; i < numBars; ++i)
    {
        const auto slot = juce::Rectangle<float> (area.getX() + width * (float) i, area.getY(), width, area.getHeight()).reduced (1.5f, 0.0f);
        g.setColour (colour::track.withAlpha (0.5f));
        g.fillRoundedRectangle (slot, 2.0f);

        // The square root makes quiet harmonics tall enough to see and grab.
        const float height = std::sqrt (juce::jlimit (0.0f, 1.0f, owner.harmonics[(size_t) i])) * slot.getHeight();
        g.setColour ((i % 8 == 0 ? colour::red : colour::teal).withAlpha (0.9f));
        g.fillRoundedRectangle (slot.withTop (slot.getBottom() - juce::jmax (0.0f, height)), 2.0f);
    }
}

void WavetableEditor::HarmonicArea::mouseDown (const juce::MouseEvent& e)
{
    owner.beginEdit();
    owner.updateHarmonics();
    mouseDrag (e);
}

void WavetableEditor::HarmonicArea::mouseDrag (const juce::MouseEvent& e)
{
    const auto area = getLocalBounds().toFloat().reduced (12.0f, 8.0f).withTrimmedTop (16.0f);
    const int bar = juce::jlimit (0, numBars - 1, (int) ((e.position.x - area.getX()) / area.getWidth() * (float) numBars));
    const float height = juce::jlimit (0.0f, 1.0f, (area.getBottom() - e.position.y) / area.getHeight());
    owner.setHarmonic (bar + 1, height * height);
    owner.changed (false);
}

void WavetableEditor::HarmonicArea::mouseUp (const juce::MouseEvent&)
{
    owner.changed (true);
}

//==============================================================================
void WavetableEditor::StackView::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    paintPanel (g, bounds, "ALL 16 FRAMES");

    const float position = owner.processor.apvts.getRawParameterValue (owner.osc == 0 ? amv::id::oscAPos : amv::id::oscBPos)->load();
    amv::drawWavetableStack (g, bounds.reduced (14.0f, 10.0f).withTrimmedTop (16.0f), [this] (int f) { return owner.frameData (f); }, numFrames,
                             position, amv::colour::teal, owner.frame);
}

juce::Rectangle<float> WavetableEditor::FrameStrip::cell (int index) const
{
    const auto area = getLocalBounds().toFloat().reduced (10.0f, 8.0f).withTrimmedTop (16.0f);
    const float width = area.getWidth() / 8.0f, height = area.getHeight() / 2.0f;
    return juce::Rectangle<float> (area.getX() + width * (float) (index % 8), area.getY() + height * (float) (index / 8), width, height).reduced (2.0f);
}

void WavetableEditor::FrameStrip::paint (juce::Graphics& g)
{
    using namespace amv;
    paintPanel (g, getLocalBounds().toFloat(), "FRAMES  (CLICK TO SELECT)");

    for (int f = 0; f < numFrames; ++f)
    {
        const auto box = cell (f);
        const bool selected = f == owner.frame;
        g.setColour (selected ? colour::teal.withAlpha (0.18f) : colour::panel);
        g.fillRoundedRectangle (box, 4.0f);
        g.setColour (selected ? colour::teal : colour::panelBorder);
        g.drawRoundedRectangle (box.reduced (0.5f), 4.0f, 1.0f);

        const float* data = owner.frameData (f);
        const auto inner = box.reduced (4.0f, 5.0f);
        juce::Path path;
        const int points = juce::jmax (2, (int) inner.getWidth());
        for (int i = 0; i < points; ++i)
        {
            const float x = inner.getX() + (float) i;
            const float y = inner.getCentreY() - juce::jlimit (-1.0f, 1.0f, data[juce::jmin (tableSize - 1, i * tableSize / (points - 1))]) * inner.getHeight() * 0.5f;
            if (i == 0)
                path.startNewSubPath (x, y);
            else
                path.lineTo (x, y);
        }
        g.setColour (selected ? colour::text : colour::teal.withAlpha (0.75f));
        g.strokePath (path, juce::PathStrokeType (1.1f));
    }
}

void WavetableEditor::FrameStrip::mouseDown (const juce::MouseEvent& e)
{
    for (int f = 0; f < numFrames; ++f)
        if (cell (f).contains (e.position))
        {
            owner.selectFrame (f);
            return;
        }
}
