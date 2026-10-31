#include "PresetLibrary.h"
#include <algorithm>
#include <stdexcept>

namespace field {
namespace {
void require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
void validatePresetName(const juce::String& name){require(name.isNotEmpty()&&name.length()<=64&&name==name.trim(),"Use a preset name of 1–64 characters without leading/trailing spaces.");for(auto c:name)require(c>=32,"Control characters are not allowed in names.");}
bool reserved(const juce::String& name){return name.equalsIgnoreCase("SNAPSHOT")||name.equalsIgnoreCase("USER")||name.startsWithIgnoreCase("ENGINE: ");}
}
PresetLibrary::PresetLibrary(juce::File path):root(path==juce::File{}?juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory).getChildFile("Field Studio/Presets"):path){}
bool PresetLibrary::soundParameter(const juce::String& id){
    return id=="engine"||id=="voiceMode"||id=="glideTime"||id=="bendRange"||id=="patchGain"||id.startsWith("macro")||id=="attack"||id=="decay"||id=="sustain"||id=="release"||id=="effect"||id=="effectMix"||id.startsWith("fx")||id.startsWith("lfo")||id.startsWith("midiTarget")||id.startsWith("midiDepth")||id.startsWith("element")||id=="envelopeCurve"||id.startsWith("sample");
}
void PresetLibrary::validateName(const juce::String& name){
    require(name.isNotEmpty()&&name.length()<=64&&name==name.trim(),"Use a name of 1–64 characters without leading/trailing spaces.");
    require(!name.containsAnyOf("/\\:*?\"<>|")&&name!="."&&name!=".."&&!name.endsWithChar('.')&&!name.startsWithChar('.'),"This name contains reserved path characters.");
    for(auto c:name)require(c>=32,"Control characters are not allowed in names.");
    auto stem=name.upToFirstOccurrenceOf(".",false,false).toUpperCase();
    require(stem!="CON"&&stem!="PRN"&&stem!="AUX"&&stem!="NUL"&&!(stem.length()==4&&(stem.startsWith("COM")||stem.startsWith("LPT"))&&stem[3]>='0'&&stem[3]<='9'),"This name is reserved on Windows.");
}
juce::File PresetLibrary::modeDirectory(bool drum) const {
    require(!root.isSymbolicLink(),"Preset library cannot be a symbolic link.");
    auto dir=root.getChildFile(drum?"Drum":"Synth");require(!dir.isSymbolicLink(),"Preset mode folder cannot be a symbolic link.");return dir;
}
juce::File PresetLibrary::folderDirectory(bool drum,const juce::String& folder) const {
    require(!folder.startsWithIgnoreCase("ENGINE: "),"Factory presets are read-only; copy to USER first.");
    auto parts=juce::StringArray::fromTokens(folder,"/","");require(!parts.isEmpty()&&parts.size()<=8&&parts.joinIntoString("/")==folder,"Invalid folder path or more than eight levels.");
    auto dir=modeDirectory(drum);for(auto part:parts){validateName(part);dir=dir.getChildFile(part);require(!dir.isSymbolicLink(),"Preset folder cannot be a symbolic link.");}return dir;
}
juce::File PresetLibrary::entryFile(const Entry& entry) const {
    require(!entry.factory&&juce::Uuid(entry.id).toString()==entry.id,"Invalid or read-only preset ID.");
    auto file=folderDirectory(entry.drum,entry.folder).getChildFile(entry.id+".fieldpreset");require(!file.isSymbolicLink(),"Preset cannot be a symbolic link.");return file;
}
void PresetLibrary::addFactory(Entry entry){entry.factory=true;factory.push_back(std::move(entry));}
juce::StringArray PresetLibrary::folders(bool drum) const {
    juce::StringArray result;for(auto& entry:factory)if(entry.drum==drum)result.addIfNotAlreadyThere(entry.folder);
    result.sort(true);result.add("SNAPSHOT");result.add("USER");
    juce::StringArray users;
    std::function<void(const juce::File&,int)> scan=[&](const juce::File& parent,int depth){
        if(depth>=8)return;
        for(auto dir:parent.findChildFiles(juce::File::findDirectories,false)){
            if(dir.isSymbolicLink()||dir.getFileName().startsWithChar('.'))continue;
            auto key=dir.getRelativePathFrom(modeDirectory(drum)).replaceCharacter('\\','/');
            if(depth==0&&key.startsWithIgnoreCase("ENGINE: "))continue;
            if(key!="USER"&&key!="SNAPSHOT")users.add(key);
            require(users.size()<=1024,"Preset library exceeds 1024 custom folders.");scan(dir,depth+1);
        }
    };scan(modeDirectory(drum),0);users.sort(true);result.addArray(users);return result;
}
std::vector<PresetLibrary::Entry> PresetLibrary::entries(bool drum,const juce::String& folder) const {
    std::vector<Entry> result;
    if(folder.startsWith("ENGINE: ")){for(auto& entry:factory)if(entry.drum==drum&&entry.folder==folder)result.push_back(entry);}
    else {auto files=folderDirectory(drum,folder).findChildFiles(juce::File::findFiles,false,"*.fieldpreset");
        for(auto file:files){if(file.isSymbolicLink()||file.getSize()>256ll*1024*1024)continue;
            juce::FileInputStream stream(file);if(!stream.openedOk())continue;auto patch=juce::ValueTree::readFromStream(stream);
            if(!patch.hasType("PATCH")||(int)patch["libraryVersion"]!=1||(bool)patch["drum"]!=drum)continue;
            auto id=patch["id"].toString();if(juce::Uuid(id).toString()!=id||file.getFileNameWithoutExtension()!=id)continue;
            result.push_back({id,patch["presetName"].toString(),folder,drum,false,patch});
        }
    }
    std::sort(result.begin(),result.end(),[](const Entry& a,const Entry& b){int order=a.name.compareIgnoreCase(b.name);return order==0?a.id<b.id:order<0;});return result;
}
void PresetLibrary::write(const juce::File& file,const Entry& entry){
    require(file.getParentDirectory().createDirectory().wasOk(),"Cannot create preset folder.");
    auto patch=entry.patch.createCopy();patch.setProperty("libraryVersion",1,nullptr);patch.setProperty("id",entry.id,nullptr);patch.setProperty("presetName",entry.name,nullptr);patch.setProperty("drum",entry.drum,nullptr);
    juce::TemporaryFile temp(file);{juce::FileOutputStream stream(temp.getFile());require(stream.openedOk(),"Cannot write preset.");patch.writeToStream(stream);stream.flush();require(stream.getStatus().wasOk()&&stream.getPosition()<=256ll*1024*1024,"Preset write failed or exceeds 256 MB.");}
    require(temp.overwriteTargetFileWithTemporary(),"Could not finish writing preset.");
}
PresetLibrary::Entry PresetLibrary::save(bool drum,const juce::String& folder,const juce::String& name,const juce::ValueTree& patch){
    validatePresetName(name);require(!patch.hasProperty("drum")||(bool)patch["drum"]==drum,"Synth and drum preset libraries are separate.");require(patch.hasType("PATCH")&&patch.getChildWithName("PARAMETERS").isValid(),"Invalid sound preset.");
    Entry entry{juce::Uuid().toString(),name,folder,drum,false,patch.createCopy()};entry.patch.setProperty("presetName",name,nullptr);entry.patch.setProperty("drum",drum,nullptr);write(entryFile(entry),entry);return entry;
}
void PresetLibrary::createFolder(bool drum,const juce::String& name,const juce::String& parent){
    validateName(name);require(!reserved(name),"Choose a new user folder name.");const auto key=parent.isEmpty()?name:parent+"/"+name;auto dir=folderDirectory(drum,key);
    for(auto existing:folders(drum))require(!existing.equalsIgnoreCase(key),"A folder with this name already exists.");require(dir.createDirectory().wasOk(),"Could not create folder.");
}
void PresetLibrary::renameFolder(bool drum,const juce::String& from,const juce::String& to){
    require(!reserved(from),"Only custom user folders can be renamed.");validateName(to);require(!reserved(to),"Choose a new user folder name.");
    const auto parent=from.containsChar('/')?from.upToLastOccurrenceOf("/",false,false):juce::String{};const auto key=parent.isEmpty()?to:parent+"/"+to;
    auto source=folderDirectory(drum,from),target=folderDirectory(drum,key);for(auto existing:folders(drum))require(!existing.equalsIgnoreCase(key),"A folder with this name already exists.");require(source.isDirectory()&&source.moveFileTo(target),"Could not rename folder.");
}
juce::String PresetLibrary::transferFolder(bool drum,const juce::String& from,const juce::String& parent,bool move){
    require(!move||!reserved(from),"Built-in folders cannot be moved.");
    require(parent!=from&&!parent.startsWith(from+"/"),"A folder cannot be copied or moved inside itself.");
    const bool isFactory=from.startsWith("ENGINE: ");require(!isFactory||!entries(drum,from).empty(),"Factory folder does not exist.");const auto name=isFactory?from.substring(8):from.fromLastOccurrenceOf("/",false,false);
    const auto key=parent+"/"+name;auto target=folderDirectory(drum,key);require(!target.exists(),"Destination folder already exists.");require(folderDirectory(drum,parent).isDirectory()||parent=="USER"||parent=="SNAPSHOT","Choose an existing destination folder.");
    // Inspect every directory before a move; never transport links or silently
    // discard an unrecognized file when copying a preset collection.
    size_t bytes=0;int count=0;const int destinationDepth=juce::StringArray::fromTokens(key,"/","").size();
    std::function<void(const juce::String&,int)> inspect=[&](const juce::String& folder,int depth){
        require(depth<=8&&++count<=1024,"Folder transfer exceeds library limits.");
        if(folder.startsWith("ENGINE: "))return;
        const auto dir=folderDirectory(drum,folder);require(dir.isDirectory(),"Source folder no longer exists.");
        int files=0;for(auto item:dir.findChildFiles(juce::File::findFilesAndDirectories,false)){
            require(!item.isSymbolicLink(),"Folder transfers do not follow symbolic links.");
            if(item.isDirectory())inspect(folder+"/"+item.getFileName(),depth+1);
            else{require(item.hasFileExtension("fieldpreset"),"Folder contains files other than presets.");bytes+=(size_t)item.getSize();require(bytes<=512u*1024u*1024u,"Folder transfer exceeds 512 MB.");++files;}
        }
        require((int)entries(drum,folder).size()==files,"Folder contains an invalid preset file.");
    };inspect(from,destinationDepth);
    if(move){require(target.getParentDirectory().createDirectory().wasOk()&&folderDirectory(drum,from).moveFileTo(target),"Could not move folder.");return key;}
    const auto stage=modeDirectory(drum).getChildFile(".preset-copy-"+juce::Uuid().toString());
    struct Cleanup {juce::File dir;~Cleanup(){dir.deleteRecursively();}} cleanup{stage};
    std::function<void(const juce::String&,const juce::File&,int)> copy=[&](const juce::String& folder,const juce::File& destination,int depth){
        require(depth<=8,"Folder changed during copy.");
        require(destination.createDirectory().wasOk(),"Cannot stage folder copy.");
        for(auto entry:entries(drum,folder)){entry.id=juce::Uuid().toString();entry.factory=false;write(destination.getChildFile(entry.id+".fieldpreset"),entry);}
        if(!folder.startsWith("ENGINE: "))for(auto child:folderDirectory(drum,folder).findChildFiles(juce::File::findDirectories,false)){require(!child.isSymbolicLink(),"Folder changed during copy.");copy(folder+"/"+child.getFileName(),destination.getChildFile(child.getFileName()),depth+1);}
    };copy(from,stage,destinationDepth);require(target.getParentDirectory().createDirectory().wasOk()&&stage.moveFileTo(target),"Could not finish folder copy.");return key;
}
void PresetLibrary::deleteFolder(bool drum,const juce::String& name){require(!reserved(name),"Only custom user folders can be deleted.");auto dir=folderDirectory(drum,name);require(dir.isDirectory()&&dir.findChildFiles(juce::File::findFilesAndDirectories,false).isEmpty(),"Empty the folder before deleting it.");require(dir.deleteFile(),"Could not delete folder.");}
void PresetLibrary::rename(const Entry& entry,const juce::String& name){validatePresetName(name);auto file=entryFile(entry);require(file.existsAsFile(),"Preset no longer exists.");auto next=entry;next.name=name;write(file,next);}
void PresetLibrary::remove(const Entry& entry){auto file=entryFile(entry);require(file.existsAsFile()&&file.deleteFile(),"Could not delete preset.");}
}
