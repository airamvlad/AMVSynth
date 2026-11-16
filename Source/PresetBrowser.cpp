#include "PresetBrowser.h"
#include "PluginEditor.h"

namespace
{
constexpr int barHeight = 34, categoryWidth = 170, categoryRowHeight = 30, starWidth = 34;

juce::Path starPath (juce::Rectangle<float> area)
{
    juce::Path star;
    star.addStar (area.getCentre(), 5, area.getWidth() * 0.22f, area.getWidth() * 0.5f);
    return star;
}
} // namespace

PresetBrowser::PresetBrowser()
{
    using namespace amv;

    categories = { "All", "Favourites", "User" };
    for (const auto* name : getPresetCategories())
        categories.add (name);

    search.setTextToShowWhenEmpty ("Search presets", colour::dimText);
    search.setFont (juce::FontOptions (14.0f));
    search.setColour (juce::TextEditor::backgroundColourId, colour::background);
    search.setColour (juce::TextEditor::outlineColourId, colour::panelBorder);
    search.setColour (juce::TextEditor::focusedOutlineColourId, colour::teal);
    search.setColour (juce::TextEditor::textColourId, colour::text);
    search.onTextChange = [this] { update(); };
    search.onReturnKey = [this] { list.grabKeyboardFocus(); };
    search.onEscapeKey = [this] { if (onClose) onClose(); };
    addAndMakeVisible (search);

    list.setRowHeight (28);
    list.setColour (juce::ListBox::backgroundColourId, colour::background);
    list.setColour (juce::ListBox::outlineColourId, colour::panelBorder);
    list.setOutlineThickness (1);
    list.getVerticalScrollBar().setColour (juce::ScrollBar::thumbColourId, colour::teal.withAlpha (0.6f));
    addAndMakeVisible (list);

    for (auto* button : { &loadFile, &openFolder, &close })
    {
        button->setColour (juce::TextButton::buttonColourId, colour::knob);
        button->setColour (juce::TextButton::textColourOffId, colour::text);
        addAndMakeVisible (*button);
    }

    loadFile.setTooltip ("Load a preset file from anywhere on the disk.");
    openFolder.setTooltip ("Show the folder with your presets. Files and folders you put there appear under User.");
    loadFile.onClick = [this] { if (onLoadFile) onLoadFile(); };
    close.onClick = [this] { if (onClose) onClose(); };
    openFolder.onClick = []
    {
        userFolder().createDirectory();
        userFolder().revealToUser();
    };

    loadFavourites();
    rescan();
}

juce::File PresetBrowser::userFolder()
{
    return juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile ("AMVSynth Presets");
}

juce::File PresetBrowser::favouritesFile()
{
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory).getChildFile ("AMVSynth").getChildFile ("favourites.txt");
}

void PresetBrowser::loadFavourites()
{
    favourites.clear();
    favouritesFile().readLines (favourites);
    favourites.removeEmptyStrings();
}

void PresetBrowser::saveFavourites() const
{
    const auto file = favouritesFile();
    file.getParentDirectory().createDirectory();
    file.replaceWithText (favourites.joinIntoString ("\n"));
}

std::vector<PresetBrowser::Entry> PresetBrowser::filter (const std::vector<Entry>& all, const juce::String& category, const juce::String& search,
                                                         const juce::StringArray& favourites)
{
    const auto words = juce::StringArray::fromTokens (search.trim(), " ", "");
    std::vector<Entry> result;

    for (const auto& entry : all)
    {
        const bool user = entry.program < 0;
        if (category == "Favourites" ? ! favourites.contains (entry.key())
                                     : category == "User" ? ! user
                                                          : category != "All" && (user || entry.category != category))
            continue;

        // Every word has to be in the name or in the category.
        bool matches = true;
        for (const auto& word : words)
            matches = matches && (entry.name.containsIgnoreCase (word) || entry.category.containsIgnoreCase (word));

        if (matches)
            result.push_back (entry);
    }

    return result;
}

void PresetBrowser::rescan()
{
    all.clear();

    const auto& presets = amv::getFactoryPresets();
    for (size_t i = 0; i < presets.size(); ++i)
        all.push_back ({ presets[i].name, presets[i].category, (int) i, {} });

    const auto folder = userFolder();
    auto files = folder.findChildFiles (juce::File::findFiles, true, "*.amvpreset");
    files.sort();

    for (const auto& file : files)
    {
        // A preset in a subfolder is listed with the name of that folder.
        const auto parent = file.getParentDirectory();
        all.push_back ({ file.getFileNameWithoutExtension(), parent == folder ? juce::String ("User") : parent.getFileName(), -1, file });
    }

    update();
}

void PresetBrowser::setCurrent (int program, const juce::File& file)
{
    Entry entry;
    entry.program = program;
    entry.file = file;
    if (program >= 0 && program < (int) amv::getFactoryPresets().size())
        entry.name = amv::getFactoryPresets()[(size_t) program].name;

    const auto key = program < 0 && file == juce::File() ? juce::String() : entry.key();
    if (key != currentKey)
    {
        currentKey = key;
        update();
    }
}

void PresetBrowser::update()
{
    shown = filter (all, categories[category], search.getText(), favourites);

    // Refilling the list must not load a preset.
    const juce::ScopedValueSetter<bool> guard (updating, true);
    list.updateContent();
    list.deselectAllRows();

    for (size_t i = 0; i < shown.size(); ++i)
        if (shown[i].key() == currentKey)
        {
            list.selectRow ((int) i);
            break;
        }

    list.repaint();
    repaint();
}

juce::Rectangle<int> PresetBrowser::categoryArea() const
{
    return getLocalBounds().reduced (14).withTrimmedTop (barHeight + 10).removeFromLeft (categoryWidth);
}

juce::Rectangle<int> PresetBrowser::categoryRow (int index) const
{
    return categoryArea().removeFromTop (categoryRowHeight * (index + 1)).removeFromBottom (categoryRowHeight);
}

void PresetBrowser::resized()
{
    auto area = getLocalBounds().reduced (14);
    auto bar = area.removeFromTop (barHeight).reduced (0, 3);
    close.setBounds (bar.removeFromRight (80));
    bar.removeFromRight (6);
    openFolder.setBounds (bar.removeFromRight (120));
    bar.removeFromRight (6);
    loadFile.setBounds (bar.removeFromRight (100));
    bar.removeFromLeft (categoryWidth + 10);
    search.setBounds (bar.removeFromLeft (juce::jmin (320, bar.getWidth() - 10)));

    area.removeFromTop (10);
    area.removeFromLeft (categoryWidth + 10);
    list.setBounds (area);
}

void PresetBrowser::paint (juce::Graphics& g)
{
    using namespace amv;
    const auto r = getLocalBounds().toFloat();
    g.setColour (colour::panel);
    g.fillRoundedRectangle (r, 10.0f);
    g.setColour (colour::panelBorder);
    g.drawRoundedRectangle (r.reduced (0.5f), 10.0f, 1.0f);

    g.setColour (colour::teal);
    g.setFont (juce::FontOptions (12.5f, juce::Font::bold));
    g.drawText ("PRESETS", getLocalBounds().reduced (14).removeFromTop (barHeight).removeFromLeft (categoryWidth), juce::Justification::centredLeft);

    for (int i = 0; i < categories.size(); ++i)
    {
        const auto row = categoryRow (i).toFloat().reduced (0.0f, 2.0f);
        const bool selected = i == category;

        if (selected || i == hoverCategory)
        {
            g.setColour (selected ? colour::track : colour::track.withAlpha (0.4f));
            g.fillRoundedRectangle (row, 5.0f);
        }

        if (selected)
        {
            g.setColour (colour::teal);
            g.fillRoundedRectangle (row.withWidth (3.0f).reduced (0.0f, 5.0f), 1.5f);
        }

        const int count = (int) filter (all, categories[i], {}, favourites).size();
        g.setColour (selected ? colour::text : colour::dimText);
        g.setFont (juce::FontOptions (13.5f));
        g.drawText (categories[i], row.withTrimmedLeft (12.0f), juce::Justification::centredLeft);
        g.setColour (colour::dimText.withAlpha (0.7f));
        g.setFont (juce::FontOptions (11.5f));
        g.drawText (juce::String (count), row.withTrimmedRight (10.0f), juce::Justification::centredRight);

        // A line between the three lists at the top and the factory categories.
        if (i == 2)
        {
            g.setColour (colour::panelBorder);
            g.fillRect (row.getX(), row.getBottom() + 1.5f, row.getWidth(), 1.0f);
        }
    }

    if (shown.empty())
    {
        g.setColour (colour::dimText);
        g.setFont (juce::FontOptions (13.5f));
        const auto text = categories[category] == "Favourites" && search.isEmpty() ? "Click the star in front of a preset to keep it here."
                        : categories[category] == "User" && search.isEmpty()       ? "Presets you save with SAVE appear here."
                                                                                     : "No preset matches.";
        g.drawText (text, list.getBounds(), juce::Justification::centred);
    }
}

void PresetBrowser::paintListBoxItem (int row, juce::Graphics& g, int width, int height, bool selected)
{
    using namespace amv;
    if (row < 0 || row >= (int) shown.size())
        return;

    const auto& entry = shown[(size_t) row];
    const auto area = juce::Rectangle<int> (width, height).toFloat();

    if (selected)
    {
        g.setColour (colour::teal.withAlpha (0.16f));
        g.fillRect (area);
        g.setColour (colour::teal);
        g.fillRect (area.withWidth (3.0f));
    }
    else if (row % 2 == 1)
    {
        g.setColour (colour::panel.withAlpha (0.45f));
        g.fillRect (area);
    }

    const auto star = starPath (juce::Rectangle<float> (0.0f, 0.0f, (float) starWidth, (float) height).withSizeKeepingCentre (14.0f, 14.0f));
    if (favourites.contains (entry.key()))
    {
        g.setColour (juce::Colour (0xfff5b84a));
        g.fillPath (star);
    }
    else
    {
        g.setColour (colour::dimText.withAlpha (0.45f));
        g.strokePath (star, juce::PathStrokeType (1.0f));
    }

    g.setColour (selected ? colour::text : colour::text.withAlpha (0.85f));
    g.setFont (juce::FontOptions (14.0f));
    g.drawText (entry.name, area.withTrimmedLeft ((float) starWidth).withTrimmedRight (130.0f), juce::Justification::centredLeft);
    g.setColour (entry.program < 0 ? colour::red.withAlpha (0.85f) : colour::dimText);
    g.setFont (juce::FontOptions (12.0f));
    g.drawText (entry.category, area.withTrimmedRight (14.0f), juce::Justification::centredRight);
}

void PresetBrowser::listBoxItemClicked (int row, const juce::MouseEvent& e)
{
    if (row < 0 || row >= (int) shown.size() || e.x >= starWidth)
        return;

    const auto key = shown[(size_t) row].key();
    if (favourites.contains (key))
        favourites.removeString (key);
    else
        favourites.add (key);

    saveFavourites();
    update();
}

void PresetBrowser::listBoxItemDoubleClicked (int, const juce::MouseEvent& e)
{
    if (e.x >= starWidth && onClose)
        onClose();
}

void PresetBrowser::selectedRowsChanged (int row)
{
    if (updating || row < 0 || row >= (int) shown.size())
        return;

    const auto entry = shown[(size_t) row];
    if (entry.key() == currentKey)
        return;

    currentKey = entry.key();
    if (onChoose)
        onChoose (entry);
}

void PresetBrowser::returnKeyPressed (int)
{
    if (onClose)
        onClose();
}

bool PresetBrowser::keyPressed (const juce::KeyPress& key)
{
    if (key == juce::KeyPress::escapeKey && onClose)
    {
        onClose();
        return true;
    }

    return false;
}

void PresetBrowser::visibilityChanged()
{
    if (isVisible())
    {
        rescan();
        list.scrollToEnsureRowIsOnscreen (list.getSelectedRow());
    }
}

void PresetBrowser::mouseDown (const juce::MouseEvent& e)
{
    for (int i = 0; i < categories.size(); ++i)
        if (categoryRow (i).contains (e.getPosition()))
        {
            category = i;
            update();
            return;
        }
}

void PresetBrowser::mouseMove (const juce::MouseEvent& e)
{
    int hover = -1;
    for (int i = 0; i < categories.size(); ++i)
        if (categoryRow (i).contains (e.getPosition()))
            hover = i;

    if (hover != hoverCategory)
    {
        hoverCategory = hover;
        repaint();
    }
}

void PresetBrowser::mouseExit (const juce::MouseEvent&)
{
    hoverCategory = -1;
    repaint();
}
