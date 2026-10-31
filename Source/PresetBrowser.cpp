#include "PluginEditor.h"
#include <algorithm>

namespace {
const juce::Colour blue{0xff59bce5},ochre{0xffd6b361},grey{0xffc0cacd},red{0xffef7452};
void label(juce::Graphics& g,const juce::String& value,juce::Rectangle<float> area,float size,juce::Colour colour){g.setColour(colour);g.setFont(juce::Font(juce::FontOptions(size)));g.drawText(value,area,juce::Justification::centredLeft,true);}
}
void FieldEditor::updatePresetControls(){
    presetsButton.setVisible(mode<2&&!presetBrowser);
    presetsButton.setBounds(22,getHeight()-65,100,26);
    std::array<juce::TextButton*,5> buttons{{&presetDone,&presetSave,&presetNewFolder,&presetManage,&presetClose}};
    const int widths[]{108,124,108,100,80};int x=22;
    for(int i=0;i<5;++i){buttons[(size_t)i]->setVisible(presetBrowser);buttons[(size_t)i]->setBounds(x,getHeight()-65,widths[i],26);x+=widths[i]+8;}
    if(presetBrowser)presetDone.setEnabled(!presetEntries.empty());
}
void FieldEditor::openPresetBrowser(int slot){
    presetSavedUntil=0;heldPresetSlot=-1;presetSlot=juce::jlimit(0,7,slot);presetDrum=processor.drumMode.load();presetBrowser=true;
    try {
        presetFolders=processor.presetLibrary.folders(presetDrum);
        const auto engine=FieldProcessor::engineNames[juce::jlimit(0,14,(int)processor.value("engine"))].toUpperCase();
        presetFolderIndex=juce::jmax(0,presetFolders.indexOf("ENGINE: "+engine));presetIndex=0;refreshPresetBrowser();
        const auto name=processor.soundSlotName(presetSlot);for(size_t i=0;i<presetEntries.size();++i)if(presetEntries[i].name==name)presetIndex=(int)i;
        status="Sound "+juce::String(presetSlot+1)+" / blue: engine or folder / ochre: preset / play keys to audition";
    }catch(const std::exception& error){status=error.what();}
    configureMacros();updatePresetControls();grabKeyboardFocus();repaint();
}
void FieldEditor::closePresetBrowser(){presetBrowser=false;heldPresetSlot=-1;configureMacros();updatePresetControls();grabKeyboardFocus();repaint();}
void FieldEditor::refreshPresetBrowser(const juce::String& selectedId){
    juce::String folder=presetFolders.isEmpty()?juce::String{}:presetFolders[juce::jlimit(0,presetFolders.size()-1,presetFolderIndex)];
    presetFolders=processor.presetLibrary.folders(presetDrum);presetFolderIndex=juce::jmax(0,presetFolders.indexOf(folder));
    presetEntries=processor.presetLibrary.entries(presetDrum,presetFolders[presetFolderIndex]);
    presetIndex=juce::jlimit(0,juce::jmax(0,(int)presetEntries.size()-1),presetIndex);
    for(size_t i=0;i<presetEntries.size();++i)if(presetEntries[i].id==selectedId)presetIndex=(int)i;
}
void FieldEditor::selectPresetFolder(int index){
    if(presetFolders.isEmpty())return;
    presetFolderIndex=juce::jlimit(0,presetFolders.size()-1,index);presetIndex=0;
    try{refreshPresetBrowser();}catch(const std::exception& error){status=error.what();presetEntries.clear();}
    configureMacros();repaint();
}
void FieldEditor::selectPreset(int index,bool load){
    if(presetEntries.empty())return;presetIndex=juce::jlimit(0,(int)presetEntries.size()-1,index);
    if(load)loadBrowserPreset(false);
    // Do not rebuild a slider during its drag callback.
    macros[1].setValue(presetIndex,juce::dontSendNotification);repaint();
}
void FieldEditor::loadBrowserPreset(bool close){
    if(presetEntries.empty()){status="This folder is empty / save a snapshot or paste a preset";return;}
    const auto entry=presetEntries[(size_t)presetIndex];juce::String error;
    if(processor.loadLibrarySound(entry,presetSlot,error)){presetLoadedId=entry.id;status="Sound "+juce::String(presetSlot+1)+": "+entry.name;if(close)closePresetBrowser();}
    else status=error;
    repaint();
}
void FieldEditor::savePresetSnapshot(int slot){
    try{presetSlot=slot;const auto entry=processor.snapshotSound(slot);presetSavedUntil=juce::Time::getMillisecondCounterHiRes()+1600;status="Saved snapshot: "+entry.name;
        if(presetBrowser){presetFolders=processor.presetLibrary.folders(presetDrum);presetFolderIndex=presetFolders.indexOf("SNAPSHOT");refreshPresetBrowser(entry.id);presetLoadedId=entry.id;configureMacros();}
    }catch(const std::exception& error){status=error.what();}
    repaint();
}
void FieldEditor::configurePresetMacros(){
    macroLabels={"ENGINE / FOLDER","PRESET","SOUND SLOT","PRESET ACTIONS"};
    for(auto& slider:macros){slider.setEnabled(true);slider.setSkewFactor(1);slider.setDoubleClickReturnValue(false,0);}
    macros[0].setRange(0,juce::jmax(1,presetFolders.size()-1),1);macros[0].setValue(presetFolderIndex,juce::dontSendNotification);macros[0].onValueChange=[this]{selectPresetFolder((int)macros[0].getValue());};
    macros[1].setRange(0,juce::jmax(1,(int)presetEntries.size()-1),1);macros[1].setValue(presetIndex,juce::dontSendNotification);macros[1].setEnabled(!presetEntries.empty());macros[1].onValueChange=[this]{selectPreset((int)macros[1].getValue(),true);};
    macros[2].setRange(1,8,1);macros[2].setValue(presetSlot+1,juce::dontSendNotification);macros[2].onValueChange=[this]{presetSlot=(int)macros[2].getValue()-1;repaint();};
    macros[3].setEnabled(false);
    macros[0].setTooltip("Choose an engine, SNAPSHOT, USER or a custom folder");macros[1].setTooltip("Load and audition a preset in the selected sound slot");macros[2].setTooltip("Destination sound key (1–8)");macros[3].setTooltip("T1: rename preset / Shift+T1: new folder / T2: rename folder / T3: delete preset / T4: delete empty folder");
}
void FieldEditor::drawPresetBrowser(juce::Graphics& g,juce::Rectangle<float> r){
    label(g,presetDrum?"DRUM":"SYNTH",{r.getX(),r.getY(),33,11},8,blue);
    label(g,juce::String(presetSlot+1),{r.getX(),r.getY()+15,25,33},28,grey);
    label(g,"SLOT",{r.getX(),r.getY()+49,26,10},7,grey.withAlpha(.6f));
    const float x=r.getX()+29,w=(r.getWidth()-29)*.49f,right=x+w+6;
    g.setColour(blue.withAlpha(.4f));g.drawVerticalLine((int)(right-4),r.getY()+15,r.getBottom()-14);
    label(g,"ENGINE / FOLDER",{x,r.getY(),w,11},7.5f,blue);label(g,"PRESET",{right,r.getY(),r.getRight()-right,11},7.5f,ochre);
    const int start=juce::jlimit(0,juce::jmax(0,presetFolders.size()-8),presetFolderIndex-3);
    const int firstPreset=juce::jlimit(0,juce::jmax(0,(int)presetEntries.size()-8),presetIndex-3);
    for(int row=0;row<8;++row){const float y=r.getY()+17+row*10.5f;
        const int folder=start+row;if(folder<presetFolders.size()){
            if(folder==presetFolderIndex){g.setColour(blue.withAlpha(.19f));g.fillRect(x-2,y,w+2,10.5f);}
            const auto name=presetFolders[folder].replace("ENGINE: ","").replace("/"," > ");label(g,name,{x,y,w,10.5f},9,folder==presetFolderIndex?grey:blue.withAlpha(.75f));
        }
        const int index=firstPreset+row;if(index<(int)presetEntries.size()){
            const auto& entry=presetEntries[(size_t)index];if(index==presetIndex){g.setColour(ochre.withAlpha(.18f));g.fillRect(right-2,y,r.getRight()-right+2,10.5f);}
            auto name=entry.name;if(entry.factory)name=name.fromFirstOccurrenceOf(" / ",false,false);else if(entry.folder=="SNAPSHOT"&&name.substring(4,5)=="-"&&name.contains(" slot "))name=name.substring(5,19)+" #"+name.fromLastOccurrenceOf(" slot ",false,false);
            label(g,name,{right,y,r.getRight()-right,10.5f},9,index==presetIndex?grey:ochre.withAlpha(.8f));
        }
    }
    if(presetEntries.empty())label(g,"EMPTY",{right,r.getY()+18,r.getRight()-right,12},9,ochre);
    label(g,"T1 NAME   T2 FOLDER   T3 DEL   T4 DEL FOLDER",{r.getX(),r.getBottom()-12,r.getWidth(),12},6.7f,grey.withAlpha(.7f));
}
void FieldEditor::promptPresetName(const juce::String& title,const juce::String& initial,std::function<void(const juce::String&)> apply){
    auto* dialog=new juce::AlertWindow(title,"Names can contain up to 64 characters.",juce::MessageBoxIconType::NoIcon);
    dialog->addTextEditor("name",initial,"Name");dialog->addButton("Save",1,juce::KeyPress(juce::KeyPress::returnKey));dialog->addButton("Cancel",0,juce::KeyPress(juce::KeyPress::escapeKey));
    dialog->enterModalState(true,juce::ModalCallbackFunction::create([safe=juce::Component::SafePointer<FieldEditor>(this),dialog,apply=std::move(apply)](int result){if(!safe||result!=1)return;try{apply(dialog->getTextEditorContents("name"));safe->refreshPresetBrowser();safe->configureMacros();safe->repaint();}catch(const std::exception& error){safe->status=error.what();}}),true);
}
void FieldEditor::presetManagement(int action){
    if(!presetBrowser||presetFolders.isEmpty())return;
    const auto folder=presetFolders[presetFolderIndex];const bool drum=presetDrum;
    const field::PresetLibrary::Entry entry=presetEntries.empty()?field::PresetLibrary::Entry{}:presetEntries[(size_t)presetIndex];
    try {
        if((action==2||action==5)&&(folder=="USER"||folder=="SNAPSHOT"||folder.startsWith("ENGINE: "))){status="Only custom user folders can be renamed or deleted";return;}
        if(action==1){if(entry.id.isEmpty())return;if(entry.factory){status="Copy factory sounds to USER before renaming";return;}promptPresetName("Rename preset",entry.name,[this,entry](const juce::String& name){processor.presetLibrary.rename(entry,name);status="Renamed preset: "+name;});}
        else if(action==2){promptPresetName("Rename user folder",folder.fromLastOccurrenceOf("/",false,false),[this,drum,folder](const juce::String& name){processor.presetLibrary.renameFolder(drum,folder,name);const auto path=folder.containsChar('/')?folder.upToLastOccurrenceOf("/",true,false)+name:name;presetFolders=processor.presetLibrary.folders(drum);presetFolderIndex=presetFolders.indexOf(path);status="Renamed folder: "+name;});}
        else if(action==3){const auto parent=folder.startsWith("ENGINE: ")?juce::String("USER"):folder;promptPresetName("New folder in "+parent,"",[this,drum,parent](const juce::String& name){processor.presetLibrary.createFolder(drum,name,parent);presetFolders=processor.presetLibrary.folders(drum);presetFolderIndex=presetFolders.indexOf(parent+"/"+name);presetIndex=0;status="Created folder: "+name;});}
        else if(action==4||action==5){
            if(action==4&&(entry.id.isEmpty()||entry.factory)){status="Factory presets are read-only";return;}
            const auto name=action==4?entry.name:folder;
            juce::AlertWindow::showOkCancelBox(juce::MessageBoxIconType::WarningIcon,"Delete "+juce::String(action==4?"preset":"empty folder"),"Delete “"+name+"”? Loaded sound slots are retained.","Delete","Cancel",nullptr,juce::ModalCallbackFunction::create([safe=juce::Component::SafePointer<FieldEditor>(this),action,entry,drum,folder](int result){if(!safe||result!=1)return;try{if(action==4)safe->processor.presetLibrary.remove(entry);else safe->processor.presetLibrary.deleteFolder(drum,folder);safe->status="Deleted "+(action==4?entry.name:folder);safe->refreshPresetBrowser();safe->configureMacros();safe->repaint();}catch(const std::exception& error){safe->status=error.what();}}));
        }
        else if(action==6){if(entry.id.isEmpty())return;presetFolderClipboard.clear();presetClipboard=entry;presetClipboard.patch=entry.patch.createCopy();status="Lifted preset: "+entry.name+" / choose a user folder and Drop";}
        else if(action==7){
            if(presetFolderClipboard.isNotEmpty()){
                if(presetFolderClipboardDrum!=presetDrum){status="Synth and drum preset libraries are separate";return;}
                const auto path=processor.presetLibrary.transferFolder(presetDrum,presetFolderClipboard,folder,false);presetFolders=processor.presetLibrary.folders(presetDrum);presetFolderIndex=presetFolders.indexOf(path);refreshPresetBrowser();configureMacros();status="Copied folder to "+path;
            }else{
                if(presetClipboard.id.isEmpty()){status="Lift a preset or folder first";return;}if(presetClipboard.drum!=presetDrum){status="Synth and drum preset libraries are separate";return;}auto copy=processor.presetLibrary.save(presetDrum,folder,presetClipboard.name,presetClipboard.patch);refreshPresetBrowser(copy.id);configureMacros();status="Pasted preset: "+copy.name;
            }
        }
        else if(action==9){presetFolderClipboard=folder;presetFolderClipboardDrum=presetDrum;presetClipboard={};status="Lifted folder: "+folder+" / choose a destination and Drop";}

    }catch(const std::exception& error){status=error.what();}
    repaint();
}
void FieldEditor::showPresetManagement(){
    if(presetFolders.isEmpty())return;
    const auto folder=presetFolders[presetFolderIndex];
    const bool custom=folder!="USER"&&folder!="SNAPSHOT"&&!folder.startsWith("ENGINE: ");
    juce::PopupMenu menu;const bool user=!presetEntries.empty()&&!presetEntries[(size_t)presetIndex].factory;
    menu.addItem(1,"Rename preset (T1)",user);menu.addItem(2,"Rename folder (T2)",custom);menu.addItem(3,"New user folder (Shift + T1)");menu.addItem(4,"Delete preset (T3)",user);menu.addItem(5,"Delete empty folder (T4)",custom);menu.addSeparator();menu.addItem(6,"Lift / copy preset",!presetEntries.empty());menu.addItem(9,"Lift / copy folder (Shift + Lift)");menu.addItem(7,"Drop / paste",presetClipboard.id.isNotEmpty()||presetFolderClipboard.isNotEmpty());
    juce::PopupMenu copy,move,folderCopy,folderMove;for(int i=0;i<presetFolders.size();++i)if(!presetFolders[i].startsWith("ENGINE: ")){copy.addItem(10000+i,presetFolders[i]);if(user)move.addItem(20000+i,presetFolders[i]);folderCopy.addItem(30000+i,presetFolders[i]);if(custom)folderMove.addItem(40000+i,presetFolders[i]);}menu.addSubMenu("Copy preset to folder",copy,!presetEntries.empty());menu.addSubMenu("Move preset to folder",move,user);menu.addSubMenu("Copy folder into",folderCopy);menu.addSubMenu("Move folder into",folderMove,custom);
    menu.addItem(8,"Show preset files");
    const auto folders=presetFolders;const auto entry=presetEntries.empty()?field::PresetLibrary::Entry{}:presetEntries[(size_t)presetIndex];const bool drum=presetDrum;
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&presetManage),[safe=juce::Component::SafePointer<FieldEditor>(this),folders,folder,entry,drum](int action){
        if(!safe||!action)return;if(action==8){safe->processor.presetLibrary.directory().revealToUser();return;}if(action<10000){safe->presetManagement(action);return;}
        try{
            const int category=action/10000,index=action%10000;if(index<0||index>=folders.size())return;
            if(category>=3){const auto path=safe->processor.presetLibrary.transferFolder(drum,folder,folders[index],category==4);safe->presetFolders=safe->processor.presetLibrary.folders(drum);safe->presetFolderIndex=safe->presetFolders.indexOf(path);safe->refreshPresetBrowser();safe->status=juce::String(category==4?"Moved folder to ":"Copied folder to ")+path;}
            else{const bool moving=category==2;auto copy=safe->processor.presetLibrary.save(entry.drum,folders[index],entry.name,entry.patch);if(moving)safe->processor.presetLibrary.remove(entry);safe->presetFolderIndex=safe->presetFolders.indexOf(folders[index]);safe->refreshPresetBrowser(copy.id);safe->status=juce::String(moving?"Moved ":"Copied ")+entry.name;}
            safe->configureMacros();safe->repaint();
        }catch(const std::exception& error){safe->status=error.what();}
    });
}
void FieldEditor::tickPresetHold(){
    if(heldPresetSlot<0||mode>=2||presetBrowser)return;
    const int slot=heldPresetSlot;const bool down=heldPresetKeyboard?juce::KeyPress::isKeyCurrentlyDown('1'+slot):functionButtons[8+slot]->isDown();
    if(!down){heldPresetSlot=-1;if(heldPresetKeyboard&&!suppressSlotClick){processor.recallPatchSlot(slot);status="Sound "+juce::String(slot+1)+": "+processor.soundSlotName(slot);configureMacros();}suppressSlotClick=false;return;}
    const auto elapsed=juce::Time::getMillisecondCounterHiRes()-presetHoldStarted;
    if(elapsed>=2000&&!suppressSlotClick){suppressSlotClick=true;savePresetSnapshot(slot);}
}
