#pragma once
#include <juce_data_structures/juce_data_structures.h>
#include <vector>

namespace field {
// Control-thread persistent library. IDs, not display names, identify presets.
// Factory entries are immutable; copying one always creates a new user entry.
class PresetLibrary {
public:
    struct Entry {juce::String id,name,folder;bool drum=false,factory=false;juce::ValueTree patch;};
    explicit PresetLibrary(juce::File root={});
    const juce::File& directory() const {return root;}
    void addFactory(Entry);
    juce::StringArray folders(bool drum) const;
    std::vector<Entry> entries(bool drum,const juce::String& folder) const;
    Entry save(bool drum,const juce::String& folder,const juce::String& name,const juce::ValueTree& patch);
    void createFolder(bool drum,const juce::String& name,const juce::String& parent={});
    juce::String transferFolder(bool drum,const juce::String& from,const juce::String& parent,bool move);
    void renameFolder(bool drum,const juce::String& from,const juce::String& to);
    void deleteFolder(bool drum,const juce::String& name);
    void rename(const Entry&,const juce::String& name);
    void remove(const Entry&);
    static bool soundParameter(const juce::String& id);
private:
    juce::File root;
    std::vector<Entry> factory;
    juce::File modeDirectory(bool drum) const;
    juce::File folderDirectory(bool drum,const juce::String& folder) const;
    juce::File entryFile(const Entry&) const;
    static void validateName(const juce::String&);
    static void write(const juce::File&,const Entry&);
};
}
