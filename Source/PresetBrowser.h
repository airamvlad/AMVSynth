#pragma once

#include "Presets.h"

#include <juce_gui_basics/juce_gui_basics.h>

/** The preset browser: the factory presets by category, the user's own preset files, a search
    box and favourites.

    A click or the arrow keys load a preset straight away, so the list can be played through;
    a double click or Return closes the browser. The star in front of a name marks a favourite.
    The user presets are the .amvpreset files in "AMVSynth Presets" in the documents folder,
    with its subfolders; the favourites are kept in a small text file next to the settings. */
class PresetBrowser final : public juce::Component,
                            private juce::ListBoxModel
{
public:
    struct Entry
    {
        juce::String name, category;
        int program = -1;   // factory presets: the program number
        juce::File file;    // user presets: the file

        /** What a favourite is remembered by. */
        juce::String key() const { return program >= 0 ? "factory:" + name : "user:" + file.getFullPathName(); }
    };

    PresetBrowser();

    /** Where the user's presets are kept. */
    static juce::File userFolder();

    /** Builds the list for a category and a search text. `category` is "All", "Favourites",
        "User" or one of the factory categories. */
    static std::vector<Entry> filter (const std::vector<Entry>& all, const juce::String& category, const juce::String& search,
                                      const juce::StringArray& favourites);

    /** Reads the user folder again, for example after a preset was saved. */
    void rescan();

    /** Marks the preset that is loaded now: a program number, or a file for a user preset. */
    void setCurrent (int program, const juce::File& file);

    std::function<void (const Entry&)> onChoose;
    std::function<void()> onLoadFile, onClose;

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;
    void visibilityChanged() override;
    bool keyPressed (const juce::KeyPress&) override;

private:
    int getNumRows() override { return (int) shown.size(); }
    void paintListBoxItem (int row, juce::Graphics&, int width, int height, bool selected) override;
    void listBoxItemClicked (int row, const juce::MouseEvent&) override;
    void listBoxItemDoubleClicked (int row, const juce::MouseEvent&) override;
    void selectedRowsChanged (int lastRowSelected) override;
    void returnKeyPressed (int) override;

    juce::Rectangle<int> categoryArea() const;
    juce::Rectangle<int> categoryRow (int index) const;
    void update();
    void loadFavourites();
    void saveFavourites() const;
    static juce::File favouritesFile();

    std::vector<Entry> all, shown;
    juce::StringArray categories, favourites;
    int category = 0, hoverCategory = -1;
    juce::String currentKey;
    bool updating = false;

    juce::TextEditor search;
    juce::ListBox list { "presets", this };
    juce::TextButton loadFile { "LOAD FILE" }, openFolder { "OPEN FOLDER" }, close { "CLOSE" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PresetBrowser)
};
