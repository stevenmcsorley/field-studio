#include "PluginEditor.h"
#include <cmath>
#include <algorithm>

namespace {
const juce::Colour ink{0xff303638}, metal{0xffc9cbca}, paper{0xffe5e7e5}, screenBlack{0xff11191c};
const std::array<juce::Colour,4> colours{{juce::Colour(0xff59bce5),juce::Colour(0xffd6b361),juce::Colour(0xffc0cacd),juce::Colour(0xffef7452)}};
constexpr std::array<int,14> whiteOffsets{{0,2,4,6,7,9,11,12,14,16,18,19,21,23}};
constexpr std::array<int,10> blackAfter{{0,1,2,4,5,7,8,9,11,12}};
constexpr std::array<int,10> blackOffsets{{1,3,5,8,10,13,15,17,20,22}};
const juce::StringArray typingKeys{"a","w","s","e","d","r","f","g","y","h","u","j","k","o","l","p",";","[","'", "z","x","c","v","b"};
const std::array<std::array<const char*,4>,15> names{{
    {{"MODULATION","GROUND NOISE","PHASE FILTER","DETUNE"}},{{"FM AMOUNT","FREQUENCY","TOPOLOGY","DETUNE"}},
    {{"WAVE COUNT","WAVE ENVELOPE","SPREAD","UNITOR"}},{{"WAVESHAPER","OCTAVE","RING MOD","DIGITALNESS"}},
    {{"WAVE / LENGTH","FILTER","PHASE","CHORUS"}},{{"WAVEFORM","STEREO","CUTOFF","RESONANCE"}},
    {{"TENSION","DECAY","DETUNE","IMPULSE"}},{{"FILTER","AMPLITUDE","PULSE TWO","MODULATION"}},
    {{"PHASE SHIFT","DISTORTION","PHASE FILTER","PHASE TILT"}},{{"FILTER","WAVE NUMBER","WAVE MODIFIER","NOISE"}},
    {{"ENVELOPE MIX","WAVEFORM 1","ENVELOPE 1","CROSS MOD"}},{{"START","LOOP IN","LOOP OUT","END"}},
    {{"WAVEFORM","FORMANT","BANDS","MIX"}},{{"VOLUME","COMPRESSOR","TONE","DRIVE"}},{{"ENVELOPE MIX","WAVEFORM 1","ENVELOPE 1","CROSS MOD"}}
}};
juce::StringArray effectLabels(int index,bool shifted,bool master){
    static const std::array<std::array<const char*,4>,11> labels{{
        {{"COLOUR","TIME","FEEDBACK","CHARACTER"}},{{"RANGE","SPEED","FEEDBACK","LEVEL"}},
        {{"TONE","TURNS","DAMPING","MIX"}},{{"X SIZE","Y SIZE","Z FEEDBACK","MIX"}},
        {{"LOW FREQUENCY","FILTER FOLLOW","FEEDBACK","HIGH FREQUENCY"}},{{"TONE","GSM","BAUD","TELEMETRY"}},
        {{"FREQUENCY","PUNCH","ROUNDS","POWER"}},{{"FREQUENCY","DELAY","FEEDBACK","SIDEBAND"}},
        {{"RATE","BITS","MODEL","MIX"}},{{"DISTANCE","GATE","COLOUR","MIX"}},{{"DEPTH","RATE","FEEDBACK","MIX"}}
    }};
    juce::StringArray result;for(auto name:labels[(size_t)juce::jlimit(0,10,index)])result.add(name);if(shifted)result.set(3,master?"MASTER FX MIX":"INSERT MIX");return result;
}
void text(juce::Graphics& g,const juce::String& s,juce::Rectangle<float> r,float size,juce::Colour c=ink,int justification=juce::Justification::centred){
    g.setColour(c);g.setFont(juce::Font(juce::FontOptions(size)));g.drawText(s,r,justification,false);
}
juce::String seconds(double t){int s=(int)t;return juce::String(s/60).paddedLeft('0',2)+":"+juce::String(s%60).paddedLeft('0',2)+"."+juce::String((int)(std::fmod(t,1.)*100)).paddedLeft('0',2);}
}

FieldLookAndFeel::FieldLookAndFeel(){
    setColour(juce::ComboBox::backgroundColourId,juce::Colour(0xff242e32));setColour(juce::ComboBox::textColourId,juce::Colour(0xffeef1ec));
    setColour(juce::ComboBox::outlineColourId,juce::Colour(0xff4d595d));setColour(juce::ComboBox::arrowColourId,juce::Colour(0xffa9b7bc));
    setColour(juce::PopupMenu::backgroundColourId,juce::Colour(0xff202a2e));setColour(juce::PopupMenu::textColourId,juce::Colour(0xffedf1ed));
    setColour(juce::PopupMenu::highlightedBackgroundColourId,juce::Colour(0xff40545b));setColour(juce::TextButton::textColourOffId,juce::Colour(0xffe2e8e6));
}
void FieldLookAndFeel::drawButtonBackground(juce::Graphics& g,juce::Button& b,const juce::Colour&,bool over,bool down){
    if((bool)b.getProperties()["hardware"]){
        auto r=b.getLocalBounds().toFloat().reduced(1);g.setColour(juce::Colour(0xff737b7b));g.fillRoundedRectangle(r,3);
        g.setGradientFill(juce::ColourGradient(down?juce::Colour(0xffa5abaa):juce::Colour(0xffe2e5e2),0,0,juce::Colour(0xffbfc4c1),0,r.getBottom(),false));g.fillRoundedRectangle(r.reduced(1),3);
        auto circle=r.reduced(8);float d=juce::jmin(circle.getWidth(),circle.getHeight());circle=juce::Rectangle<float>(d,d).withCentre(r.getCentre());
        g.setColour(juce::Colours::white.withAlpha(down?.08f:.4f));g.drawEllipse(circle,1.2f);
        g.setGradientFill(juce::ColourGradient(juce::Colour(down?0xffa0a8a6:0xffcbd0cc),circle.getX(),circle.getY(),juce::Colour(0xffe4e7e2),circle.getRight(),circle.getBottom(),false));g.fillEllipse(circle.reduced(1));
        if(over){g.setColour(juce::Colours::white.withAlpha(.13f));g.fillRoundedRectangle(r,3);}
        if(b.getToggleState()){g.setColour(juce::Colour(0xff4da7be));g.fillRoundedRectangle(r.getX()+r.getWidth()*.3f,r.getBottom()-5,r.getWidth()*.4f,2,1);}
    }else{
        auto r=b.getLocalBounds().toFloat().reduced(.5f);g.setColour(juce::Colour(down?0xff46585d:over?0xff35464c:0xff27343a));g.fillRoundedRectangle(r,5);g.setColour(juce::Colour(0xff526065));g.drawRoundedRectangle(r,5,1);
    }
}
void FieldLookAndFeel::drawRotarySlider(juce::Graphics& g,int x,int y,int w,int h,float pos,float start,float end,juce::Slider& s){
    if(!s.isEnabled())g.beginTransparencyLayer(.28f);
    const auto r=juce::Rectangle<float>((float)x,(float)y,(float)w,(float)h).reduced((float)juce::jmin(w,h)*.12f);const float d=juce::jmin(r.getWidth(),r.getHeight());auto circle=juce::Rectangle<float>(d,d).withCentre(r.getCentre());
    g.setColour(juce::Colour(0xff89918e));g.fillEllipse(circle.expanded(1));g.setGradientFill(juce::ColourGradient(juce::Colour(0xfff7f8f2),circle.getX(),circle.getY(),juce::Colour(0xff969f9a),circle.getRight(),circle.getBottom(),false));g.fillEllipse(circle);
    g.setColour(juce::Colours::black.withAlpha(.18f));g.drawEllipse(circle.reduced(d*.12f),1.5f);auto core=circle.reduced(d*.26f);
    const auto c=s.findColour(juce::Slider::thumbColourId);g.setGradientFill(juce::ColourGradient(c.darker(.48f),core.getX(),core.getY(),c,core.getRight(),core.getBottom(),false));g.fillEllipse(core);
    g.setColour(juce::Colours::white.withAlpha(.6f));g.drawEllipse(core,1);
    const float angle=start+pos*(end-start);juce::Path pointer;pointer.addRoundedRectangle(-1.4f,-d*.42f,2.8f,d*.15f,1);g.setColour(ink.withAlpha(.8f));g.fillPath(pointer,juce::AffineTransform::rotation(angle).translated(circle.getCentreX(),circle.getCentreY()));
    if(!s.isEnabled())g.endTransparencyLayer();
}

FieldEditor::FieldEditor(FieldProcessor& p):AudioProcessorEditor(p),processor(p){
    setLookAndFeel(&look);setWantsKeyboardFocus(true);setMouseClickGrabsKeyboardFocus(true);setOpaque(true);
    engineBox.addItemList(FieldProcessor::engineNames,1);effectBox.addItemList(FieldProcessor::effectNames,1);sequenceBox.addItemList(FieldProcessor::sequencerNames,1);
    for(auto* c:{&engineBox,&effectBox,&sequenceBox}){addAndMakeVisible(c);c->setWantsKeyboardFocus(false);}
    engineBox.setTooltip(juce::String::fromUTF8("Synthesis engine · import your own audio for Sampler"));effectBox.setTooltip(juce::String::fromUTF8("Insert effect · coloured knobs edit effect on T3"));sequenceBox.setTooltip(juce::String::fromUTF8("Sequencer · select a mode, then play a note or edit the grid"));
    engineBox.onChange=[this]{int e=engineBox.getSelectedId()-1;processor.setValue("engine",(float)e);if(e==14)processor.drumMode=true;else if(e!=11)processor.drumMode=false;if(e!=14&&!processor.drumMode)lastSynthEngine=e;setMode(processor.drumMode?1:0);configureMacros();grabKeyboardFocus();};
    effectBox.onChange=[this]{if(mode==3){processor.setValue("masterEffect",(float)(effectBox.getSelectedId()-1));page=2;configureMacros();}else{processor.setValue("effect",(float)(effectBox.getSelectedId()-1));setPage(2);}grabKeyboardFocus();};
    sequenceBox.onChange=[this]{int i=sequenceBox.getSelectedId()-1;processor.setValue("sequencer",(float)i);processor.setValue("sequenceOn",i>0?1.f:0.f);setMode(4);grabKeyboardFocus();};
    juce::TextButton* files[]{&newButton,&openButton,&saveButton,&importButton,&exportButton,&helpButton};
    for(int i=0;i<6;++i){auto* b=files[i];addAndMakeVisible(b);b->setWantsKeyboardFocus(false);b->onClick=[this,i]{if(i==5)showHelp();else fileAction(i);};}
    newButton.setTooltip("New empty tape bank");openButton.setTooltip("Open a Field Studio project");saveButton.setTooltip("Save all eight tapes, sample, effects and settings");importButton.setTooltip("Import WAV, AIFF, FLAC or Ogg into Sampler");exportButton.setTooltip("Export current tape mix as 24-bit stereo WAV");
    const juce::StringArray fn{"~","drum","tape","mix","1","2","3","4","IN","OUT","loop","seq","rev","click","M1","M2",juce::String::fromUTF8("•••")};
    const juce::StringArray tips{"Synthesizer","Drum machine","Four-track tape recorder",juce::String::fromUTF8("Mixer · Shift changes coloured knobs to pan"),juce::String::fromUTF8("T1 · engine or track 1"),juce::String::fromUTF8("T2 · envelope / mixer EQ / tape track 2"),juce::String::fromUTF8("T3 · insert / master FX / tape track 3"),juce::String::fromUTF8("T4 · modulation / master dynamics / tape track 4"),"Set loop selection start at playhead","Set loop selection end at playhead","Toggle tape loop","Open sequencer grid","Reverse tape speed","Toggle metronome",juce::String::fromUTF8("Recall mixer memo 1 · Shift: store · Sequencer: previous page"),juce::String::fromUTF8("Recall mixer memo 2 · Shift: store · Sequencer: next page"),"Tape style, host sync and editing menu"};
    for(int i=0;i<17;++i){auto* b=functionButtons.add(new juce::TextButton(fn[i]));b->getProperties().set("hardware",true);b->setColour(juce::TextButton::textColourOffId,ink);b->setColour(juce::TextButton::textColourOnId,ink);b->setWantsKeyboardFocus(false);b->setTooltip(tips[i]);b->onClick=[this,i]{functionAction(i);grabKeyboardFocus();};addAndMakeVisible(b);}
    const juce::StringArray tr{juce::String::fromUTF8("↑"),juce::String::fromUTF8("↓"),juce::String::fromUTF8("✂"),juce::String::fromUTF8("●"),juce::String::fromUTF8("▶"),juce::String::fromUTF8("■"),juce::String::fromUTF8("←"),juce::String::fromUTF8("→")};
    const juce::StringArray tt{juce::String::fromUTF8("Lift selected tape segment · Shift: undo"),juce::String::fromUTF8("Drop lifted audio at playhead · Shift: redo"),juce::String::fromUTF8("Split at playhead · Shift: clear track (undoable)"),juce::String::fromUTF8("Start / stop tape recording · Shift: capture input sample"),juce::String::fromUTF8("Play / pause · Space"),"Stop recording and playback",juce::String::fromUTF8("Seek back one second · Shift: rewind"),juce::String::fromUTF8("Seek forward one second · Shift: selection end")};
    for(int i=0;i<8;++i){auto* b=transportButtons.add(new juce::TextButton(tr[i]));b->getProperties().set("hardware",true);b->setColour(juce::TextButton::textColourOffId,i==3?juce::Colour(0xffbf513b):ink);b->setWantsKeyboardFocus(false);b->setTooltip(tt[i]);b->onClick=[this,i]{transportAction(i);grabKeyboardFocus();};addAndMakeVisible(b);}
    for(auto* b:{&inputButton,&tempoButton,&comButton,&shiftButton}){b->getProperties().set("hardware",true);b->setColour(juce::TextButton::textColourOffId,ink);b->setWantsKeyboardFocus(false);addAndMakeVisible(b);}
    inputButton.setTooltip(juce::String::fromUTF8("Monitor live audio input · required for Amp / Vocoder"));inputButton.onClick=[this]{processor.inputMonitor=!processor.inputMonitor;status=processor.inputMonitor?"Input monitoring on":"Input monitoring off";grabKeyboardFocus();};
    tempoButton.setTooltip("Metronome on / off");tempoButton.onClick=[this]{processor.metronome=!processor.metronome;grabKeyboardFocus();};
    comButton.setTooltip("Connections, host sync, tape styles and panic");comButton.onClick=[this]{showMenu();};shiftButton.setTooltip(juce::String::fromUTF8("Latch Shift · alternate tape edits and mixer pan"));shiftButton.onClick=[this]{shiftLatched=!shiftLatched;setShift(shiftLatched);grabKeyboardFocus();};
    for(int i=0;i<4;++i){auto& s=macros[(size_t)i];s.setSliderStyle(juce::Slider::RotaryVerticalDrag);s.setTextBoxStyle(juce::Slider::NoTextBox,false,0,0);s.setRotaryParameters(juce::MathConstants<float>::pi*1.25f,juce::MathConstants<float>::pi*2.75f,true);s.setColour(juce::Slider::thumbColourId,colours[(size_t)i]);s.setWantsKeyboardFocus(false);s.setPopupDisplayEnabled(true,true,this);s.setDoubleClickReturnValue(true,.5);addAndMakeVisible(s);}
    for(auto* s:{&volume,&tempo}){s->setSliderStyle(juce::Slider::RotaryVerticalDrag);s->setTextBoxStyle(juce::Slider::NoTextBox,false,0,0);s->setColour(juce::Slider::thumbColourId,juce::Colour(0xffcbd0cc));s->setWantsKeyboardFocus(false);s->setPopupDisplayEnabled(true,true,this);addAndMakeVisible(s);}
    volumeAttachment=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.parameters,"volume",volume);tempoAttachment=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.parameters,"tempo",tempo);
    const juce::StringArray performanceNames{"Hold brake","Hold reverse","Hold chop"};
    for(int i=0;i<3;++i){auto& button=tapePerformance[(size_t)i];button.setButtonText(performanceNames[i]);button.setWantsKeyboardFocus(false);addChildComponent(button);button.setTooltip("Hold to perform on tape; release restores normal playback");button.onStateChange=[this,i]{processor.setTapePerformance(i,tapePerformance[(size_t)i].isDown());};}
    addChildComponent(tapeEditButton);tapeEditButton.setWantsKeyboardFocus(false);tapeEditButton.onClick=[this]{showMenu();};
    volume.setTooltip("Master volume: 1 = unity, 2 = +6 dB boost");tempo.setTooltip(juce::String::fromUTF8("Tempo · beats per minute"));steps.fill(-1);mode=processor.drumMode?1:0;configureMacros();setResizable(true,true);setResizeLimits(1000,520,1920,1000);setSize(1320,640);startTimerHz(30);
}
FieldEditor::~FieldEditor(){stopTimer();panic();for(auto& a:attachments)a.reset();volumeAttachment.reset();tempoAttachment.reset();setLookAndFeel(nullptr);}
void FieldEditor::showPageForTesting(int m,int p){setMode(juce::jlimit(0,4,m));page=juce::jlimit(0,3,p);configureMacros();timerCallback();repaint();}
void FieldEditor::setMode(int m){if(mode==2&&m!=2)processor.releaseTapePerformance();mode=m;page=0;for(auto& b:tapePerformance)b.setVisible(mode==2);tapeEditButton.setVisible(mode==2);configureMacros();resized();repaint();}
void FieldEditor::setPage(int p){page=p;if(mode>1&&mode!=3)mode=processor.drumMode?1:0;configureMacros();repaint();}
void FieldEditor::setShift(bool s){if(shifted==s)return;shifted=s;shiftButton.setToggleState(s,juce::dontSendNotification);configureMacros();repaint();}
void FieldEditor::configureMacros(){
    for(size_t i=0;i<4;++i){attachments[i].reset();macros[i].onValueChange=nullptr;macros[i].setTextValueSuffix("");macros[i].setEnabled(true);}
    macroLabels.clear();juce::StringArray ids;
    bool heldSource=mode==4&&(int)processor.value("sequencer")==4;
    if(((mode==1&&page<2)||heldSource)&&(int)processor.value("engine")==14){
        {const juce::ScopedLock lock(processor.engineLock);selectedPad=processor.engine.selectedPad();dboxState=processor.engine.dboxPad(selectedPad);}
        std::array<double,4> lo{},hi{},values{},defaults{};
        if(page==1){lo={{.0002,0,.005,0}};hi={{1,2,4,1}};values={{dboxState.attack,dboxState.gain,dboxState.release,dboxState.smooth}};defaults={{.001,1,.1,.1}};macroLabels={"ATTACK","GAIN","RELEASE","SMOOTH"};}
        else if(shifted){lo={{0,0,.005,0}};hi={{1,1,4,1}};values={{dboxState.frequency,dboxState.wave2,dboxState.envelope2,dboxState.cutoff}};defaults={{.3,.5,.1,.6}};macroLabels={"FREQUENCY","WAVEFORM 2","ENVELOPE 2","CUTOFF"};}
        else{lo={{0,0,.005,0}};hi={{1,1,4,1}};values={{dboxState.mix,dboxState.wave1,dboxState.envelope1,dboxState.crossMod}};defaults={{.5,0,.2,0}};macroLabels={"ENVELOPE MIX","WAVEFORM 1","ENVELOPE 1","CROSS MOD"};}
        for(int i=0;i<4;++i){auto& slider=macros[(size_t)i];slider.setRange(lo[(size_t)i],hi[(size_t)i],.0001);slider.setSkewFactor((page==1&&(i==0||i==2))||(page==0&&i==2)?.35:1);slider.setValue(values[(size_t)i],juce::dontSendNotification);slider.setDoubleClickReturnValue(true,defaults[(size_t)i]);slider.onValueChange=[this,i]{const juce::ScopedLock lock(processor.engineLock);auto pad=processor.engine.dboxPad(selectedPad);float v=(float)macros[(size_t)i].getValue();if(page==1){if(i==0)pad.attack=v;else if(i==1)pad.gain=v;else if(i==2)pad.release=v;else pad.smooth=v;}else if(shifted){if(i==0)pad.frequency=v;else if(i==1)pad.wave2=v;else if(i==2)pad.envelope2=v;else pad.cutoff=v;}else{if(i==0)pad.mix=v;else if(i==1)pad.wave1=v;else if(i==2)pad.envelope1=v;else pad.crossMod=v;}processor.engine.setDboxPad(selectedPad,pad);};}
    }else if(((mode==1&&page==0)||(heldSource&&processor.drumMode))&&(int)processor.value("engine")==11){
        {const juce::ScopedLock lock(processor.engineLock);selectedPad=processor.engine.selectedPad();padState=processor.engine.drumPad(selectedPad);}
        const std::array<double,4> mins=shifted?std::array<double,4>{{-1,0,0,0}}:std::array<double,4>{{0,0,-24,0}};
        const std::array<double,4> maxs=shifted?std::array<double,4>{{1,2,4,2}}:std::array<double,4>{{1,1,24,2}};
        const std::array<double,4> vals=shifted?std::array<double,4>{{padState.pan,(double)padState.mode,(double)padState.chokeGroup,padState.level}}:std::array<double,4>{{padState.start,padState.end,padState.tune,padState.level}};
        macroLabels=shifted?juce::StringArray{"PAD PAN","PLAY MODE","CHOKE GROUP","PAD LEVEL"}:juce::StringArray{"PAD START","PAD END","PAD TUNE","PAD LEVEL"};
        for(int i=0;i<4;++i){auto& slider=macros[(size_t)i];slider.setRange(mins[(size_t)i],maxs[(size_t)i],shifted&&(i==1||i==2)?1.:.001);slider.setValue(vals[(size_t)i],juce::dontSendNotification);slider.setDoubleClickReturnValue(true,i==1&&!shifted?1:i==3?1:0);slider.onValueChange=[this,i]{const juce::ScopedLock lock(processor.engineLock);auto p=processor.engine.drumPad(selectedPad);float v=(float)macros[(size_t)i].getValue();if(shifted){if(i==0)p.pan=v;else if(i==1)p.mode=(int)v;else if(i==2)p.chokeGroup=(int)v;else p.level=v;}else{if(i==0)p.start=v;else if(i==1)p.end=v;else if(i==2)p.tune=v;else p.level=v;}processor.engine.setDrumPad(selectedPad,p);};}
    }else if(mode==2){
        macroLabels=shifted?juce::StringArray{"SLIDE SELECTION","LOOP IN","SPEED %","RECORD PAN"}:juce::StringArray{"SCRUB TAPE","LOOP OUT","TAPE SPEED","RECORD LEVEL"};
        macros[0].setRange(0,shifted?juce::jmax(.001,360.-(processor.selectionEnd-processor.selectionStart)):359.9,.001);macros[0].setValue(shifted?processor.selectionStart:processor.playhead.load(),juce::dontSendNotification);macros[0].setDoubleClickReturnValue(true,0);
        macros[0].onValueChange=[this]{const juce::ScopedLock lock(processor.engineLock);double value=processor.snapTapeTime(macros[0].getValue());if(shifted){const double delta=juce::jlimit(-processor.selectionStart,360.-processor.selectionEnd,value-processor.selectionStart);processor.engine.slideTracks(processor.tapeEditMask.load(),processor.selectionStart,processor.selectionEnd,delta);processor.selectionStart+=delta;processor.selectionEnd+=delta;processor.engine.setLoop(processor.selectionStart,processor.selectionEnd);}else processor.engine.seek(value);};
        macros[1].setRange(0,360,.001);macros[1].setValue(shifted?processor.selectionStart:processor.selectionEnd,juce::dontSendNotification);macros[1].setDoubleClickReturnValue(true,shifted?0:4);macros[1].onValueChange=[this]{const juce::ScopedLock lock(processor.engineLock);double value=processor.snapTapeTime(macros[1].getValue());if(shifted)processor.selectionStart=juce::jlimit(0.,processor.selectionEnd-.1,value);else processor.selectionEnd=juce::jlimit(processor.selectionStart+.1,360.,value);processor.engine.setLoop(processor.selectionStart,processor.selectionEnd);};
        macros[2].setRange(shifted?-200.:-2.,shifted?200.:2.,shifted?1.:.01);macros[2].setValue(processor.value("tapeSpeed")*(shifted?100:1),juce::dontSendNotification);macros[2].setDoubleClickReturnValue(true,shifted?100:1);if(shifted)macros[2].setTextValueSuffix("%");macros[2].onValueChange=[this]{processor.setValue("tapeSpeed",(float)(macros[2].getValue()/(shifted?100:1)));};
        attachments[3]=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.parameters,shifted?"tapeRecordPan":"tapeRecordLevel",macros[3]);macros[3].setDoubleClickReturnValue(true,shifted?0:1);
    }else if(mode==3&&page==0){
        for(int i=0;i<4;++i){auto& s=macros[(size_t)i];s.setRange(shifted?-1.:0.,shifted?1.:2.,.001);s.setValue(shifted?pans[(size_t)i]:levels[(size_t)i],juce::dontSendNotification);s.setDoubleClickReturnValue(true,shifted?0.:1.);s.onValueChange=[this,i]{const juce::ScopedLock lock(processor.engineLock);if(shifted)processor.engine.setTrackPan(i,(float)macros[(size_t)i].getValue());else processor.engine.setTrackLevel(i,(float)macros[(size_t)i].getValue());};macroLabels.add("TRACK "+juce::String(i+1)+(shifted?juce::String::fromUTF8(" · PAN"):juce::String::fromUTF8(" · LEVEL")));}
    }else{
        if(mode==3&&page==1){ids={"masterLow","masterMid","masterHigh","masterDrive"};macroLabels={juce::String::fromUTF8("LOW · dB"),juce::String::fromUTF8("MID · dB"),juce::String::fromUTF8("HIGH · dB"),"DRIVE"};}
        else if(mode==3&&page==2){ids={"masterFx1","masterFx2","masterFx3",shifted?"masterMix":"masterFx4"};macroLabels=effectLabels((int)processor.value("masterEffect"),shifted,true);}
        else if(mode==3&&page==3){ids={"masterLeft","masterRight","compressor","compressorRelease"};macroLabels={"MASTER LEFT","MASTER RIGHT","MASTER DRIVE","DRIVE RELEASE"};}
        else if(mode==4){
            int seq=(int)processor.value("sequencer");
            if(seq==7){ids={"seq1","seq2","seq3","seq4"};macroLabels={"ROTATION","HEAVINESS","SHAPE","BOUNCE"};}
            else if(seq==1){ids={"seq1","tempo","seqSwing","seqGate"};macroLabels={"DIRECTION","TEMPO","SWING","GATE LENGTH"};}
            else if(seq==6){ids={"tempo","seqSwing","seqGate","seq4"};macroLabels={"TEMPO","SWING","GATE LENGTH","VELOCITY"};}
            else if(seq==4){ids={"macro1","macro2","macro3","macro4"};int e=juce::jlimit(0,14,(int)processor.value("engine"));for(auto name:names[(size_t)e])macroLabels.add(name);if(e==11){ids={"sampleStart","sampleLoopIn","sampleLoopOut","sampleEnd"};macroLabels={"START","LOOP IN","LOOP OUT","END"};}}
            else if(seq==0){ids={"macro1","macro2","macro3","macro4"};macroLabels={"SELECT A MODE","SELECT A MODE","SELECT A MODE","SELECT A MODE"};}
            else{ids={"tempo","seqSwing","seqGate","seqHold"};macroLabels={"TEMPO","SWING","GATE LENGTH","LATCH"};}
        }
        else if(page==1&&shifted&&mode==0){ids={"voiceMode","glideTime","bendRange","patchGain"};macroLabels={"VOICE MODE","GLIDE SECONDS","BEND SEMITONES","PATCH LEVEL"};}
        else if(page==1){ids={"attack","decay","sustain","release"};macroLabels={"ATTACK","DECAY","SUSTAIN","RELEASE"};}
        else if(page==2){ids={"fx1","fx2","fx3",shifted?"effectMix":"fx4"};macroLabels=effectLabels((int)processor.value("effect"),shifted,false);}
        else if(page==3){int type=(int)processor.value("lfo");if(type==2){ids=shifted?juce::StringArray{"midiTarget1","midiTarget2","midiTarget3","midiTarget4"}:juce::StringArray{"midiDepth1","midiDepth2","midiDepth3","midiDepth4"};macroLabels=shifted?juce::StringArray{"CC1 DESTINATION","CC2 DESTINATION","CC3 DESTINATION","CC4 DESTINATION"}:juce::StringArray{"CC1 AMOUNT","CC2 AMOUNT","CC3 AMOUNT","CC4 AMOUNT"};}else if(type==1){ids=shifted?juce::StringArray{"elementSource","elementManual","lfoTarget","lfoDepth"}:juce::StringArray{"elementSource","lfoDepth","lfoTarget","volume"};macroLabels=shifted?juce::StringArray{"ELEMENT SOURCE","MANUAL INPUT","DESTINATION","AMOUNT"}:juce::StringArray{"ELEMENT SOURCE","LFO AMOUNT","DESTINATION","MASTER LEVEL"};}else if(shifted&&(type==3||type==0)){ids=type==3?juce::StringArray{"lfoWaveform","lfoDivision","lfoRetrigger","lfoSync"}:juce::StringArray{"lfoDivision","lfoRetrigger","lfoSync","volume"};macroLabels=type==3?juce::StringArray{"WAVEFORM","SYNC CYCLE","KEY RETRIGGER","TEMPO SYNC"}:juce::StringArray{"SYNC CYCLE","KEY RETRIGGER","TEMPO SYNC","MASTER LEVEL"};}else{ids={type==4?"macro4":type==5?"patchGain":"lfoRate","lfoDepth","lfoTarget","volume"};macroLabels={type==4?"VALUE":type==5?"PATCH LEVEL":"LFO RATE","LFO AMOUNT","DESTINATION","MASTER LEVEL"};}}
        else{ids={"macro1","macro2","macro3","macro4"};int e=juce::jlimit(0,14,(int)processor.value("engine"));for(auto n:names[(size_t)e])macroLabels.add(n);if(mode==1)macroLabels={"TONE","DECAY","NOISE / BODY","DRIVE"};else if(e==11&&processor.parameters.getParameter("sampleStart"))ids={"sampleStart","sampleLoopIn","sampleLoopOut","sampleEnd"};}
        for(size_t i=0;i<4;++i){attachments[i]=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.parameters,ids[(int)i],macros[i]);auto* p=processor.parameters.getParameter(ids[(int)i]);macros[i].setDoubleClickReturnValue(true,p?p->convertFrom0to1(p->getDefaultValue()):.5f);}
    }
    updateControlAvailability();
    lastVoiceMode=(int)processor.value("voiceMode");lastEngine=(int)processor.value("engine");lastEffect=(int)processor.value("effect");lastMasterEffect=(int)processor.value("masterEffect");lastLfo=(int)processor.value("lfo");lastElementSource=(int)processor.value("elementSource");lastLfoSync=(int)processor.value("lfoSync");
}

void FieldEditor::updateControlAvailability(){
    const bool external=processor.hostSync||processor.value("midiClockMode")==1;
    tempo.setEnabled(!external);
    tempo.setTooltip(processor.hostSync?"External DAW tempo":external?"External MIDI clock tempo":"Tempo - beats per minute");
    for(int i=0;i<4;++i){
        const auto label=macroLabels.size()>i?macroLabels[i]:juce::String();
        juce::String reason;
        if(external&&label.contains("TEMPO"))reason="Tempo follows the selected external clock";
        else if(page==3&&label=="MANUAL INPUT"&&processor.value("elementSource")!=3)reason="Choose Manual as the Element source first";
        else if(page==3&&label=="LFO RATE"&&processor.value("lfoSync")>.5f)reason="Rate follows the tempo sync division";
        else if(mode==0&&page==1&&processor.value("engine")==13&&(!shifted||i<3))reason="Amp processes continuous input; Shift exposes patch level";
        else if(mode==4&&processor.value("sequencer")==0)reason="Choose a sequencer first";
        else if(label.contains("GLIDE")&&(processor.value("voiceMode")==0||processor.value("voiceMode")==3))reason="Glide applies to overlapping Mono or Legato notes";
        else if(page==2&&mode!=2&&mode!=4&&processor.value(mode==3?"masterEffect":"effect")==0)reason="Effect is bypassed; choose an effect to use these controls";
        macros[(size_t)i].setEnabled(reason.isEmpty());
        macros[(size_t)i].setTooltip(reason.isNotEmpty()?reason:label+" - drag vertically; double-click resets");
    }
}

juce::Rectangle<int> FieldEditor::boundsFor(float x,float y,float w,float h) const {return juce::Rectangle<float>(hardware.getX()+x*scale,hardware.getY()+y*scale,w*scale,h*scale).toNearestInt();}
void FieldEditor::resized(){
    float w=(float)getWidth();int bx=20;engineBox.setBounds(bx,31,178,28);effectBox.setBounds(208,31,146,28);sequenceBox.setBounds(364,31,148,28);
    int right=getWidth()-20;helpButton.setBounds(right-30,31,30,28);right-=38;exportButton.setBounds(right-100,31,100,28);right-=108;importButton.setBounds(right-108,31,108,28);right-=116;saveButton.setBounds(right-56,31,56,28);right-=64;openButton.setBounds(right-56,31,56,28);right-=64;newButton.setBounds(right-50,31,50,28);
    for(int i=0;i<3;++i)tapePerformance[(size_t)i].setBounds(22+i*106,getHeight()-65,100,26);tapeEditButton.setBounds(340,getHeight()-65,100,26);
    float avail=(float)getHeight()-(mode==2?154:126);scale=juce::jmin((w-32)/1280.f,avail/480.f);hardware={(w-1280*scale)*.5f,83+(avail-480*scale)*.5f,1280*scale,480*scale};
    for(int i=0;i<17;++i)functionButtons[i]->setBounds(boundsFor(32+i*68.f,176,67,66));
    for(int i=0;i<3;++i)transportButtons[i]->setBounds(boundsFor(32+i*68.f,244,67,66));
    for(int i=3;i<6;++i)transportButtons[i]->setBounds(boundsFor(32+(i-3)*68.f,312,67,66));
    for(int i=6;i<8;++i)transportButtons[i]->setBounds(boundsFor(32+(i-6)*68.f,380,67,66));
    shiftButton.setBounds(boundsFor(168,380,67,66));
    volume.setBounds(boundsFor(174,40,58,58));tempo.setBounds(boundsFor(237,40,58,58));inputButton.setBounds(boundsFor(169,107,65,65));tempoButton.setBounds(boundsFor(236,107,65,65));comButton.setBounds(boundsFor(1122,107,65,65));
    for(int i=0;i<4;++i)macros[(size_t)i].setBounds(boundsFor(582+i*135.f,52,126,104));
    for(int i=0;i<14;++i)whiteKeys[(size_t)i]={236+i*68.f,312,67,134};
    for(int i=0;i<10;++i)blackKeys[(size_t)i]={236+blackAfter[(size_t)i]*68.f,244,67,66};
}
void FieldEditor::drawCap(juce::Graphics& g,juce::Rectangle<float> r,bool down,bool black){
    g.setColour(juce::Colour(0xff747b79));g.fillRoundedRectangle(r,3);g.setGradientFill(juce::ColourGradient(down?juce::Colour(0xffa4bfbd):paper,r.getX(),r.getY(),juce::Colour(0xffbbc2bd),r.getRight(),r.getBottom(),false));g.fillRoundedRectangle(r.reduced(1.2f),3);
    auto inset=r.reduced(11,9);if(black)inset=juce::Rectangle<float>(43,43).withCentre(r.getCentre());
    g.setColour(juce::Colours::white.withAlpha(.58f));g.drawRoundedRectangle(inset.expanded(1),black?21.f:20.f,1.5f);
    if(black){g.setGradientFill(juce::ColourGradient(down?colours[0].darker():juce::Colour(0xff0c1111),inset.getX(),inset.getY(),juce::Colour(down?0xff3b7e86:0xff242a29),inset.getRight(),inset.getBottom(),false));g.fillEllipse(inset);}
    else{g.setGradientFill(juce::ColourGradient(juce::Colour(down?0xff9bb4b0:0xffc1c9c2),inset.getX(),inset.getY(),paper,inset.getRight(),inset.getBottom(),false));g.fillRoundedRectangle(inset,20);if(down){g.setColour(colours[0].withAlpha(.7f));g.fillRoundedRectangle(inset.getX()+8,inset.getBottom()-8,inset.getWidth()-16,3,1);}}
}
void FieldEditor::paint(juce::Graphics& g){
    g.fillAll(juce::Colour(0xff172126));text(g,"FIELD STUDIO",{20,7,175,20},14,juce::Colour(0xffe8eddf),juce::Justification::centredLeft);text(g,"ENGINE",{20,17,170,13},8,juce::Colour(0xff8da1a6),juce::Justification::bottomLeft);text(g,mode==3?"MASTER EFFECT":"INSERT EFFECT",{208,17,146,13},8,juce::Colour(0xff8da1a6),juce::Justification::bottomLeft);text(g,"SEQUENCER",{364,17,148,13},8,juce::Colour(0xff8da1a6),juce::Justification::bottomLeft);
    g.setColour(juce::Colour(0xff2c3a3f));g.drawLine(20,72,(float)getWidth()-20,72,1);
    text(g,processor.projectName,{530,7,(float)getWidth()-560,19},12,juce::Colour(0xffa5b6ba),juce::Justification::centredRight);
    {juce::Graphics::ScopedSaveState save(g);g.addTransform(juce::AffineTransform::scale(scale).translated(hardware.getX(),hardware.getY()));
        g.setColour(juce::Colours::black.withAlpha(.35f));g.fillRoundedRectangle({8,17,1264,457},20);
        g.setGradientFill(juce::ColourGradient(juce::Colour(0xffe6e7e2),0,0,juce::Colour(0xffa8b0aa),1260,470,false));g.fillRoundedRectangle({8,10,1264,454},17);
        g.setColour(juce::Colours::white.withAlpha(.7f));g.drawRoundedRectangle({10,12,1260,450},15,1);
        // Fine aluminium grain, all drawn locally rather than sampled assets.
        for(int y=18;y<459;y+=3){g.setColour(juce::Colours::white.withAlpha(y%6==0?.025f:.012f));g.drawHorizontalLine(y,18,1260);}
        g.setColour(juce::Colour(0xff818985));g.fillRoundedRectangle({30,36,1160,412},4);
        drawCap(g,{32,38,135,134},false);for(int row=0;row<13;++row)for(int col=0;col<13;++col){float dx=(float)(col-6),dy=(float)(row-6);if(dx*dx+dy*dy<43){g.setColour(juce::Colour(0xff48504b));g.fillEllipse(58+col*7.f,62+row*7.f,3.6f,3.6f);}}
        drawCap(g,{169,38,132,66},false);text(g,"VOL",{173,95,60,8},7);text(g,processor.hostSync?"HOST":"BPM",{236,95,60,8},7);
        for(int i=0;i<4;++i){bool externalTempo=(processor.hostSync||processor.value("midiClockMode")==1)&&macroLabels[i].contains("TEMPO");drawCap(g,{577+i*135.f,38,134,134},false);text(g,externalTempo?"HOST TEMPO":macroLabels[i],{581+i*135.f,43,126,12},9,ink.withAlpha(macros[(size_t)i].isEnabled()?1.f:.35f));text(g,juce::String(externalTempo?processor.effectiveTempo.load():macros[(size_t)i].getValue(),macroLabels[i].contains("TEMPO")?0:(macroLabels[i].contains("ATTACK")||macroLabels[i].contains("RELEASE")||macroLabels[i].contains("ENVELOPE"))?3:2),{581+i*135.f,154,126,12},10,ink.withAlpha(macros[(size_t)i].isEnabled()?1.f:.35f));}
        drawCap(g,{1122,38,67,66},false);g.setColour(juce::Colour(0xffdc6b49));g.fillEllipse(1151,59,9,9);g.drawLine(1154,67,1144,79,2);
        // Separate square chromatic keys match the field instrument's layout.
        for(int i=0;i<14;++i){drawCap(g,{236+i*68.f,244,67,66},false);int note=53+whiteOffsets[(size_t)i]+processor.octave*12;drawCap(g,whiteKeys[(size_t)i],note>=0&&note<128&&held[(size_t)note]);}
        for(int i=0;i<10;++i){int note=53+blackOffsets[(size_t)i]+processor.octave*12;drawCap(g,blackKeys[(size_t)i],note>=0&&note<128&&held[(size_t)note],true);}
        for(int i=0;i<14;++i){const auto& r=whiteKeys[(size_t)i];text(g,juce::StringArray{"F","G","A","B","C","D","E"}[i%7],{r.getX(),r.getBottom()-22,r.getWidth(),12},8,ink.withAlpha(.5f));}
        drawScreen(g);
        text(g,"FIELD",{1203,293,52,28},13,ink);text(g,"STUDIO",{1203,317,52,24},10,ink);text(g,"01",{1203,352,52,34},26,ink);text(g,"STEREO",{1199,419,60,16},8,ink);
        g.setColour(juce::Colour(0xff8c9790));g.fillRoundedRectangle({1227,80,3,100},1.5f);g.setColour(colours[3]);float peak=juce::jlimit(0.f,1.f,processor.outputPeak.load());g.fillRoundedRectangle({1227,180-peak*100,3,peak*100},1.5f);
        for(auto pt:{juce::Point<float>(21,23),juce::Point<float>(1258,23),juce::Point<float>(21,450),juce::Point<float>(1258,450)}){g.setColour(juce::Colour(0xff929b94));g.fillEllipse(pt.x-2,pt.y-2,4,4);g.setColour(juce::Colour(0xff6b756d));g.drawLine(pt.x-1,pt.y,pt.x+1,pt.y,.7f);}
    }
    juce::String displayStatus=status;int albumRec=processor.albumRecording.load(),albumPlay=processor.albumPlaying.load();
    if(countInRemaining>0)displayStatus="TAPE ARMED / COUNT-IN "+juce::String((int)std::ceil(countInRemaining))+" beats / TRACK "+juce::String(selectedTrack+1);
    else if(recording)displayStatus="TAPE RECORDING / TRACK "+juce::String(selectedTrack+1);
    if(processor.sampling)displayStatus="SAMPLE CAPTURE / "+processor.sampleName;
    if(albumRec>=0)displayStatus="ALBUM "+juce::String(albumRec==0?"A":"B")+" RECORDING / "+seconds(albumDurations[(size_t)albumRec])+" / 06:00"+(processor.sampling?" / SAMPLE CAPTURE":"");
    else if(albumPlay>=0)displayStatus="ALBUM "+juce::String(albumPlay==0?"A":"B")+" LISTEN / "+seconds(albumDurations[(size_t)albumPlay]);
    auto bottom=(float)getHeight()-31;text(g,processor.sampling?juce::String::fromUTF8("● ")+displayStatus:displayStatus,{22,bottom,(float)getWidth()-540,21},11,processor.sampling?colours[3]:juce::Colour(0xffacbbc0),juce::Justification::centredLeft);
    text(g,"TAPE "+juce::String(processor.tapeNumber()+1)+" / 8     "+seconds(processor.playhead.load())+"     "+(processor.hostSync?"HOST "+juce::String(processor.effectiveTempo.load(),0):processor.value("midiClockMode")==1?"MIDI "+juce::String(processor.effectiveTempo.load(),0):juce::String(processor.value("tempo"),0))+" BPM     OCT "+juce::String(processor.octave.load()),{(float)getWidth()-515,bottom,490,21},11,juce::Colour(0xffa2b7b9),juce::Justification::centredRight);
}
void FieldEditor::drawScreen(juce::Graphics& g){
    juce::Rectangle<float> r(304,38,271,134);g.setColour(juce::Colour(0xff3d4949));g.fillRoundedRectangle(r.expanded(1),3);g.setColour(screenBlack);g.fillRoundedRectangle(r,2);g.saveState();g.reduceClipRegion(r.toNearestInt());
    auto content=r.reduced(12,9);if(mode==2){drawTape(g,content);g.restoreState();return;}if(mode==3){drawMixer(g,content);g.restoreState();return;}if(mode==4){drawSequence(g,content);g.restoreState();return;}
    const int engine=(int)processor.value("engine");
    juce::String title=mode==1?"DRUM / PAD "+juce::String(selectedPad+1):FieldProcessor::engineNames[juce::jlimit(0,14,engine)].toUpperCase();
    if(page==1)title=mode==1&&engine==14?"DYNAMIC ENVELOPE":"ENVELOPE";
    else if(page==2)title=FieldProcessor::effectNames[juce::jlimit(0,FieldProcessor::effectNames.size()-1,(int)processor.value("effect"))].toUpperCase();
    else if(page==3)title=FieldProcessor::lfoNames[juce::jlimit(0,5,(int)processor.value("lfo"))].toUpperCase()+" LFO";
    text(g,title,content.withHeight(16),11,colours[(size_t)page],juce::Justification::centredLeft);
    text(g,"T"+juce::String(page+1),content.withHeight(16),9,juce::Colour(0xff748c90),juce::Justification::centredRight);
    auto area=content.withTrimmedTop(22).withTrimmedBottom(16);
    juce::String footer;
    if(page==0){
        drawEngine(g,area);
        if(engine==11)footer=samplePreviewReady?juce::String(sampleSeconds,2)+"s / "+(mode==1?"PAD "+juce::String(selectedPad+1):"ROOT "+juce::MidiMessage::getMidiNoteName((int)processor.value("sampleRootNote"),true,false,3))+" / DOUBLE-CLICK: ZOOM":"IMPORT OR RECORD A SAMPLE";
        else if(mode==1)footer="PAD "+juce::String(selectedPad+1)+(padIsHeld(selectedPad)?" / KEY HELD":" / SELECTED")+" / ORIGINAL SYNTHESIS";
        else footer=engine==13||engine==12?"PARAMETER PREVIEW / OUTPUT METER":"CONTROL PREVIEW / ORIGINAL SYNTHESIS";
    }else if(page==1&&mode==0&&engine==13){
        text(g,"CONTINUOUS INPUT ENGINE",area.withTrimmedTop(9).withTrimmedBottom(24),12,colours[1]);
        text(g,"ADSR DOES NOT GATE INPUT",area.withY(area.getBottom()-25).withHeight(12),8,colours[2]);
        footer="SHIFT: PATCH LEVEL "+juce::String(processor.value("patchGain"),2);
    }else if(page==1&&shifted&&mode==0){
        text(g,juce::StringArray{"POLY","MONO","LEGATO","UNISON"}[juce::jlimit(0,3,(int)processor.value("voiceMode"))],area.withHeight(30),23,colours[0]);
        text(g,"GLIDE "+juce::String(processor.value("glideTime"),2)+" s / BEND "+juce::String(processor.value("bendRange"),0)+" st",area.withTrimmedTop(35).withHeight(13),8,colours[1]);
        footer="PATCH LEVEL "+juce::String(processor.value("patchGain"),2);
    }else if(page==1){
        juce::Path path;
        if(mode==1&&engine==14){
            const float attackFraction=.04f+.22f*std::sqrt(dboxState.attack),releaseFraction=.15f+.65f*dboxState.release/4.f;
            for(int i=0;i<200;++i){float u=i/199.f;float v=u<attackFraction?u/attackFraction:std::exp(-(u-attackFraction)*5.f/releaseFraction);v=std::pow(juce::jlimit(0.f,1.f,v),1.f+dboxState.smooth*2.f)*juce::jlimit(0.f,1.f,dboxState.gain/2.f);float x=area.getX()+u*area.getWidth(),y=area.getBottom()-v*area.getHeight();if(i==0)path.startNewSubPath(x,y);else path.lineTo(x,y);}
            g.setColour(colours[1]);g.strokePath(path,juce::PathStrokeType(1.8f));footer="ATTACK / GAIN / RELEASE / SMOOTH";
        }else{
            const float attack=processor.value("attack"),decay=processor.value("decay"),sustain=processor.value("sustain"),release=processor.value("release");
            const float at=.05f+.18f*std::sqrt(attack/4.f),dt=.08f+.17f*std::sqrt(decay/4.f),rt=.12f+.2f*std::sqrt(release/8.f);
            const std::array<juce::Point<float>,5> points{{{area.getX(),area.getBottom()},{area.getX()+at*area.getWidth(),area.getY()},{area.getX()+(at+dt)*area.getWidth(),area.getBottom()-sustain*area.getHeight()},{area.getRight()-rt*area.getWidth(),area.getBottom()-sustain*area.getHeight()},{area.getRight(),area.getBottom()}}};
            for(int stage=0;stage<4;++stage){juce::Path segment;segment.startNewSubPath(points[(size_t)stage]);for(int j=1;j<=32;++j){float t=j/32.f;if(processor.value("envelopeCurve")>.5f)t=(1-std::exp(-5*t))/(1-std::exp(-5.f));segment.lineTo(points[(size_t)stage].x+(points[(size_t)stage+1].x-points[(size_t)stage].x)*t,points[(size_t)stage].y+(points[(size_t)stage+1].y-points[(size_t)stage].y)*t);}g.setColour(colours[(size_t)stage]);g.strokePath(segment,juce::PathStrokeType(1.6f));g.fillEllipse(points[(size_t)stage+1].x-2,points[(size_t)stage+1].y-2,4,4);}
            footer="A "+juce::String(attack,2)+"s  D "+juce::String(decay,2)+"s  S "+juce::String(sustain,2)+"  R "+juce::String(release,2)+"s";
        }
    }else if(page==2)drawEffect(g,content.withTrimmedTop(22),false);
    else drawModulation(g,content.withTrimmedTop(22));
    if(footer.isNotEmpty())text(g,footer,content.withY(content.getBottom()-13).withHeight(13),7.2f,juce::Colour(0xff829a9c),juce::Justification::centredLeft);
    g.restoreState();
}

bool FieldEditor::padIsHeld(int pad) const {
    for(int n=0;n<128;++n)if(held[(size_t)n]&&((n-53)%24+24)%24==pad)return true;
    return false;
}
juce::Rectangle<float> FieldEditor::samplePlotBounds() const {return {316,73,247,mode==1?48.f:57.f};}
void FieldEditor::drawSampler(juce::Graphics& g,juce::Rectangle<float> area){
    const auto plot=samplePlotBounds();
    if(!samplePreviewReady){text(g,"IMPORT OR RECORD AUDIO",plot,11,colours[2]);return;}
    const float start=mode==1?padState.start:processor.value("sampleStart"),end=mode==1?padState.end:processor.value("sampleEnd");
    const float loopIn=processor.value("sampleLoopIn"),loopOut=processor.value("sampleLoopOut");
    const auto xpos=[&](double value){return plot.getX()+(float)((value-sampleViewStart)/(sampleViewEnd-sampleViewStart))*plot.getWidth();};
    {juce::Graphics::ScopedSaveState save(g);g.reduceClipRegion(plot.toNearestInt());
        g.setColour(colours[0].withAlpha(.09f));g.fillRect(juce::Rectangle<float>(xpos(start),plot.getY(),juce::jmax(0.f,xpos(end)-xpos(start)),plot.getHeight()));
        if(mode==0&&processor.value("sampleLoop")>.5f){g.setColour(colours[1].withAlpha(.12f));g.fillRect(juce::Rectangle<float>(xpos(loopIn),plot.getY(),juce::jmax(0.f,xpos(loopOut)-xpos(loopIn)),plot.getHeight()));}
        for(size_t channel=0;channel<2;++channel){float centre=plot.getY()+plot.getHeight()*(channel==0?.25f:.75f),height=plot.getHeight()*.22f;g.setColour(colours[2].withAlpha(.2f));g.drawLine(plot.getX(),centre,plot.getRight(),centre,.5f);
            g.setColour(colours[channel==0?0:2]);for(size_t bin=0;bin<256;++bin){const auto value=samplePreview[channel][bin];float x=plot.getX()+bin/255.f*plot.getWidth();g.drawLine(x,centre-juce::jlimit(-1.f,1.f,value.high)*height,x,centre-juce::jlimit(-1.f,1.f,value.low)*height,.8f);}
            text(g,channel==0?"L":"R",{plot.getX()+2,centre-height,10,9},7,colours[2],juce::Justification::centredLeft);
        }
        const std::array<float,4> markers{{start,loopIn,loopOut,end}};
        for(int i=0;i<4;++i){if(mode==1&&(i==1||i==2))continue;float x=juce::jlimit(plot.getX()+.7f,plot.getRight()-.7f,xpos(markers[(size_t)i]));if(xpos(markers[(size_t)i])<plot.getX()||xpos(markers[(size_t)i])>plot.getRight())continue;g.setColour(colours[(size_t)i]);g.drawLine(x,plot.getY(),x,plot.getBottom(),draggingSampleMarker==i?1.9f:1.f);g.fillRect(x-2,(i==1||i==2)?plot.getBottom()-6:plot.getY(),4.f,6.f);}
    }
    const bool reverse=mode==1?padState.reverse:processor.value("sampleReverse")>.5f;
    const juce::String playMode=mode==1?juce::StringArray{"ONE SHOT","GATED","LOOP"}[juce::jlimit(0,2,padState.mode)]:processor.value("sampleOneShot")>.5f?"ONE SHOT":processor.value("sampleLoop")>.5f?"LOOP":"GATED";
    text(g,(reverse?"REV":"FWD")+juce::String(" / ")+playMode+" / "+juce::String(start*sampleSeconds,2)+"-"+juce::String(end*sampleSeconds,2)+"s",{area.getX(),plot.getBottom()+2,area.getWidth(),10},7,colours[2],juce::Justification::centredLeft);
    if(mode==1){for(int i=0;i<24;++i){auto box=juce::Rectangle<float>(area.getX()+i*area.getWidth()/24,area.getBottom()-11,area.getWidth()/24-1,10);g.setColour((padIsHeld(i)?colours[3]:colours[0]).withAlpha(padIsHeld(i)?.8f:i==selectedPad?.32f:.07f));g.fillRect(box);g.setColour(i==selectedPad?colours[2]:colours[0].withAlpha(.3f));g.drawRect(box,i==selectedPad?1.f:.5f);}}
    else{const std::array<float,4> markers{{start,loopIn,loopOut,end}};const char* labels[]{"S ","LI ","LO ","E "};for(int i=0;i<4;++i)text(g,juce::String(labels[i])+juce::String(markers[(size_t)i]*sampleSeconds,2),{area.getX()+i*61.5f,area.getBottom()-10,61.5f,10},7,colours[(size_t)i]);}
}
void FieldEditor::moveSampleMarker(float x){
    if(draggingSampleMarker<0)return;
    const auto plot=samplePlotBounds();const float value=(float)juce::jlimit(0.,1.,sampleViewStart+(x-plot.getX())/plot.getWidth()*(sampleViewEnd-sampleViewStart));
    if(mode==1){const juce::ScopedLock lock(processor.engineLock);auto pad=processor.engine.drumPad(selectedPad);if(draggingSampleMarker==0)pad.start=juce::jlimit(0.f,pad.end,value);else pad.end=juce::jlimit(pad.start,1.f,value);processor.engine.setDrumPad(selectedPad,pad);padState=pad;}
    else{
        float start=juce::jlimit(0.f,1.f,processor.value("sampleStart")),end=juce::jlimit(start,1.f,processor.value("sampleEnd"));
        float loopIn=juce::jlimit(start,end,processor.value("sampleLoopIn")),loopOut=juce::jlimit(loopIn,end,processor.value("sampleLoopOut"));
        if(draggingSampleMarker==0)start=juce::jmin(value,end);
        else if(draggingSampleMarker==3)end=juce::jmax(value,start);
        else if(draggingSampleMarker==1)loopIn=value;
        else loopOut=value;
        loopIn=juce::jlimit(start,end,loopIn);loopOut=juce::jlimit(loopIn,end,loopOut);
        const float values[]{start,loopIn,loopOut,end};const char* ids[]{"sampleStart","sampleLoopIn","sampleLoopOut","sampleEnd"};
        for(int i=0;i<4;++i)if(processor.value(ids[i])!=values[i])processor.setValue(ids[i],values[i]);
    }
    status="Sample boundary "+juce::String(value*sampleSeconds,3)+" s";repaint();
}
void FieldEditor::drawEffect(juce::Graphics& g,juce::Rectangle<float> area,bool master){
    const int effect=juce::jlimit(0,10,(int)processor.value(master?"masterEffect":"effect"));
    std::array<float,4> v{};for(int i=0;i<4;++i)v[(size_t)i]=juce::jlimit(0.f,1.f,processor.value((master?"masterFx":"fx")+juce::String(i+1)));
    const float mix=processor.value(master?"masterMix":"effectMix");
    const auto body=area.withTrimmedBottom(16).reduced(3,3);const float x=v[0],y=v[1],z=v[2],w=v[3];
    if(effect==0){g.setColour(colours[2].withAlpha(.25f));g.drawLine(body.getX()+8,body.getCentreY(),body.getRight()-8,body.getCentreY(),1);text(g,"BYPASS",body.withHeight(body.getHeight()-8),17,colours[2]);text(g,"CHOOSE AN EFFECT ABOVE",area.withY(area.getBottom()-12).withHeight(12),8,colours[2]);return;}
    auto curve=[&](auto fn,juce::Colour colour,float thickness=1.3f){juce::Path path;for(int i=0;i<180;++i){float u=i/179.f,value=juce::jlimit(-1.2f,1.2f,fn(u));float px=body.getX()+u*body.getWidth(),py=body.getCentreY()-value*body.getHeight()*.4f;if(i==0)path.startNewSubPath(px,py);else path.lineTo(px,py);}g.setColour(colour);g.strokePath(path,juce::PathStrokeType(thickness));};
    if(effect==1){int taps=3+(int)(z*8);for(int i=0;i<taps;++i){float u=(i+1.f)/(taps+1),height=body.getHeight()*.8f*std::pow(.15f+z*.8f,(float)i);float at=body.getX()+u*body.getWidth()*(.45f+x*.55f);g.setColour(colours[(size_t)(i%4)].withAlpha(.25f+.7f*w));g.drawLine(at,body.getCentreY()-height*.5f,at,body.getCentreY()+height*.5f,1.5f);g.drawLine(body.getX(),body.getCentreY(),at,body.getCentreY(),.5f);}text(g,"DELAY TAPS",body.withHeight(10),7,colours[0],juce::Justification::centredLeft);g.setColour(colours[1]);g.fillEllipse(body.getRight()-9,body.getY()+y*(body.getHeight()-8),6,6);}
    else if(effect==2){curve([&](float u){return std::sin(u*(4+y*16)*juce::MathConstants<float>::twoPi)*std::exp(-u*(.3f+z*3))*(.4f+x*.6f);},colours[1].withAlpha(.35f+w*.65f),1.5f);}
    else if(effect==3){float width=body.getWidth()*(.35f+x*.45f),height=body.getHeight()*(.3f+y*.55f),cx=body.getCentreX(),cy=body.getCentreY();for(int i=0;i<4;++i){float offset=i*(3+z*5);g.setColour(colours[(size_t)i].withAlpha(.2f+w*.7f));g.drawRect(cx-width*.5f+offset,cy-height*.5f-offset,width,height,1.f);if(i>0){g.drawLine(cx-width*.5f+offset-3-z*5,cy-height*.5f-offset+3+z*5,cx-width*.5f+offset,cy-height*.5f-offset,.8f);}}}
    else if(effect==4){float low=std::min(x,w),high=std::max(x,w);curve([&](float u){float lo=1/(1+std::exp(-(u-low)*20)),hi=1/(1+std::exp((u-high)*20));return lo*hi*(1+z*.8f)-.5f;},colours[0]);g.setColour(colours[1]);g.drawLine(body.getX()+y*body.getWidth(),body.getY(),body.getX()+y*body.getWidth(),body.getBottom(),.7f);}
    else if(effect==5){const float steps=4+std::round(z*22),levels=2+std::round(x*10);curve([&](float u){float t=std::floor(u*steps)/steps;return std::round(std::sin(t*(10+y*20))*(levels))/levels*(.4f+w*.6f);},colours[2]);}
    else if(effect==6){curve([&](float u){float input=u*2-1;return std::tanh(input*(1+x*12+z*8))*std::pow(.4f+w*.6f,1+y);},colours[3],1.7f);g.setColour(colours[2].withAlpha(.25f));g.drawLine(body.getX(),body.getBottom(),body.getRight(),body.getY(),.7f);}
    else if(effect==7){float cx=body.getCentreX()-45,cy=body.getCentreY()-15;g.setColour(colours[2]);g.drawRoundedRectangle({cx,cy,88,31},10,1.1f);g.drawEllipse(cx-24,cy-7,27,25,1.1f);g.drawLine(cx-20,cy-6,cx-24-x*8,cy-13,1.1f);g.drawLine(cx-5,cy-6,cx+x*8,cy-13,1.1f);for(int i=0;i<4;++i)g.drawLine(cx+9+i*20,cy+30,cx+9+i*20+y*5,cy+42,1.1f);g.drawLine(cx+87,cy+6,cx+98+w*9,cy-7,1.1f);for(int i=0;i<3;++i){g.setColour(colours[(size_t)i]);g.drawEllipse(cx+10+i*23,cy+6,11+z*7,15,1);}g.setColour(colours[0]);g.fillEllipse(cx-18,cy+1,3,3);}
    else if(effect==8){const float steps=5+std::round(x*38),levels=2+std::round(y*12);curve([&](float u){float phase=std::floor(u*steps)/steps;return std::round(std::sin(phase*(8+z*16))*levels)/levels*(.35f+w*.65f);},colours[3]);}
    else if(effect==9){for(int i=0;i<5;++i){float inset=i*(4+x*5);auto box=body.reduced(inset,juce::jmin(body.getHeight()*.45f,inset*.45f));if(box.getWidth()<3||box.getHeight()<3)continue;g.setColour(colours[(size_t)(i%4)].interpolatedWith(colours[2],z).withAlpha((.18f+w*.6f)*(1-i*.12f)));g.drawRoundedRectangle(box,2+y*7,1);}g.setColour(colours[3]);g.drawLine(body.getX()+body.getWidth()*y,body.getY(),body.getX()+body.getWidth()*y,body.getBottom(),1);}
    else{for(int i=0;i<4;++i){float px=body.getX()+22+i*60,py=body.getCentreY()+std::sin(i+y*6)*x*17;g.setColour(colours[(size_t)i].withAlpha(.35f+w*.65f));g.drawEllipse(px-13,py-13,26,26,1.2f);if(i<3)g.drawLine(px+13,py,px+47,body.getCentreY()+std::sin(i+1+y*6)*x*17,1);if(z>.1f)g.drawEllipse(px-17,py-17,34,34,z);}}
    // Small coloured rails expose all four stored macro positions without
    // pretending these conceptual illustrations are measured audio responses.
    for(int i=0;i<4;++i){float left=area.getX()+i*61.5f;g.setColour(colours[(size_t)i].withAlpha(.12f));g.fillRect(left,area.getBottom()-15,55.f,2.f);g.setColour(colours[(size_t)i]);g.fillRect(left,area.getBottom()-15,55.f*v[(size_t)i],2.f);}
    text(g,"CONTROL PREVIEW / "+juce::String(master?"MASTER":"INSERT")+" MIX "+juce::String(mix*100,0)+"%",area.withY(area.getBottom()-11).withHeight(11),7,colours[2],juce::Justification::centredLeft);
}
void FieldEditor::drawModulation(juce::Graphics& g,juce::Rectangle<float> area){
    const int type=juce::jlimit(0,5,(int)processor.value("lfo"));const int target=juce::jlimit(0,FieldProcessor::lfoTargetNames.size()-1,(int)processor.value("lfoTarget"));
    const auto destination=FieldProcessor::lfoTargetNames[target];const float depth=processor.value("lfoDepth");
    auto graph=area.withTrimmedBottom(23).reduced(1,2);
    if(type==2){
        for(int i=0;i<4;++i){const auto targetName=FieldProcessor::lfoTargetNames[juce::jlimit(0,FieldProcessor::lfoTargetNames.size()-1,(int)processor.value("midiTarget"+juce::String(i+1)))];float y=area.getY()+i*18;g.setColour(colours[(size_t)i].withAlpha(.09f));g.fillRoundedRectangle({area.getX(),y,area.getWidth(),16},2);text(g,"CC"+juce::String(i+1)+" > "+targetName,{area.getX()+4,y,area.getWidth()-55,16},8,colours[(size_t)i],juce::Justification::centredLeft);text(g,juce::String(processor.value("midiDepth"+juce::String(i+1)),2),{area.getRight()-48,y,44,16},8,colours[(size_t)i]);}
        text(g,shifted?"SHIFT: DESTINATIONS / RELEASE: AMOUNTS":"FOUR INDEPENDENT CONTROLLER ROUTES",area.withY(area.getBottom()-13).withHeight(13),7,colours[2],juce::Justification::centredLeft);return;
    }
    if(type==4){float value=processor.value("macro4");g.setColour(colours[2].withAlpha(.2f));g.drawRect(graph.reduced(1),.7f);g.setColour(colours[3]);float x=graph.getX()+value*graph.getWidth();g.drawLine(x,graph.getY(),x,graph.getBottom(),2);text(g,"VALUE "+juce::String(value,2),graph,17,colours[1]);}
    else if(type==5){g.setColour(colours[2].withAlpha(.2f));g.drawLine(graph.getX(),graph.getBottom(),graph.getRight(),graph.getBottom(),.7f);g.drawLine(graph.getX(),graph.getY(),graph.getX(),graph.getBottom(),.7f);g.setColour(colours[3]);g.drawLine(graph.getX(),graph.getCentreY()+graph.getHeight()*.4f*depth,graph.getRight(),graph.getCentreY()-graph.getHeight()*.4f*depth,1.7f);text(g,"BIPOLAR VELOCITY RESPONSE",graph.withHeight(12),8,colours[1],juce::Justification::centredLeft);text(g,"SOFT",graph.withY(graph.getBottom()-10).withHeight(10),7,colours[2],juce::Justification::centredLeft);text(g,"LOUD",graph.withY(graph.getBottom()-10).withHeight(10),7,colours[2],juce::Justification::centredRight);}
    else if(type==3){
        const int shape=juce::jlimit(0,5,(int)processor.value("lfoWaveform"));juce::Path full,scaled;
        for(int i=0;i<180;++i){float u=i/179.f,p=u*2+lfoPhaseSnapshot;p-=std::floor(p);float value=0;switch(shape){case 0:value=std::sin(p*juce::MathConstants<float>::twoPi);break;case 1:value=1-4*std::abs(p-.5f);break;case 2:value=p*2-1;break;case 3:value=p<.5f?1:-1;break;case 4:value=2*(std::exp(4*p)-1)/(std::exp(4.f)-1)-1;break;default:value=p<.1f?1:-1;break;}float x=graph.getX()+u*graph.getWidth();if(i==0){full.startNewSubPath(x,graph.getCentreY()-value*graph.getHeight()*.4f);scaled.startNewSubPath(x,graph.getCentreY()-value*depth*graph.getHeight()*.4f);}else{full.lineTo(x,graph.getCentreY()-value*graph.getHeight()*.4f);scaled.lineTo(x,graph.getCentreY()-value*depth*graph.getHeight()*.4f);}}
        g.setColour(colours[3].withAlpha(.2f));g.strokePath(full,juce::PathStrokeType(.8f));g.setColour(colours[3]);g.strokePath(scaled,juce::PathStrokeType(1.4f));
    }else{
        if(type==1){const juce::StringArray sources{"AUDIO INPUT","VOICE ENVELOPE","MAIN OUTPUT","MANUAL CONTROL"};text(g,sources[juce::jlimit(0,3,(int)processor.value("elementSource"))],graph.withHeight(13),8,colours[0],juce::Justification::centredLeft);graph=graph.withTrimmedTop(17);}
        g.setColour(colours[2].withAlpha(.2f));g.drawLine(graph.getX(),graph.getCentreY(),graph.getRight(),graph.getCentreY(),.6f);juce::Path history;
        for(size_t i=0;i<modulationHistory.size();++i){float value=juce::jlimit(-1.f,1.f,modulationHistory[(modulationWrite+i)%modulationHistory.size()]);float x=graph.getX()+i/(float)(modulationHistory.size()-1)*graph.getWidth(),y=graph.getCentreY()-value*graph.getHeight()*.42f;if(i==0)history.startNewSubPath(x,y);else history.lineTo(x,y);}g.setColour(colours[3]);g.strokePath(history,juce::PathStrokeType(1.4f));
    }
    juce::String timing;
    if(type==0||type==3){if(processor.value("lfoSync")>.5f){const juce::StringArray divisions{"16 BEATS","8 BEATS","4 BEATS","2 BEATS","1 BEAT","3/4 BEAT","1/2 BEAT","3/8 BEAT","1/4 BEAT","3/16 BEAT","1/8 BEAT","1/16 BEAT"};timing="SYNC "+divisions[juce::jlimit(0,11,(int)processor.value("lfoDivision"))];}else timing=juce::String(processor.value("lfoRate"),2)+" Hz";if(type==3)timing=juce::StringArray{"SINE","TRIANGLE","SAW","SQUARE","EXP","BLIP"}[juce::jlimit(0,5,(int)processor.value("lfoWaveform"))]+" / "+timing;}
    else if(type==1)timing="ROUTED MOD "+juce::String(modulationSnapshot,2);
    else timing="AMOUNT "+juce::String(depth,2);
    text(g,timing,area.withY(area.getBottom()-23).withHeight(11),7,colours[1],juce::Justification::centredLeft);
    text(g,"> "+destination+(processor.value("lfoRetrigger")>.5f?" / KEY RESET":""),area.withY(area.getBottom()-11).withHeight(11),7,colours[2],juce::Justification::centredLeft);
}

void FieldEditor::drawEngine(juce::Graphics& g,juce::Rectangle<float> a){
    const int e=(int)processor.value("engine");const float phase=0,m=processor.value("macro1"),n=processor.value("macro2"),z=processor.value("macro3"),w=processor.value("macro4");
    if(mode==1&&e==14){
        for(int osc=0;osc<2;++osc){auto r=juce::Rectangle<float>(a.getX()+osc*126,a.getY()+3,115,43);auto c=colours[(size_t)osc];g.setColour(c.withAlpha(.45f));g.drawRoundedRectangle(r,2,.7f);float shape=osc==0?dboxState.wave1:dboxState.wave2;float envelope=osc==0?dboxState.envelope1:dboxState.envelope2;
            juce::Path line;for(int i=0;i<100;++i){float u=i/99.f,ph=u*3.f+phase*.1f;float sine=std::sin(ph*juce::MathConstants<float>::twoPi),saw=2*std::fmod(ph,1.f)-1;float v=sine*(1-shape)+saw*shape;float x=r.getX()+u*r.getWidth(),y=r.getCentreY()-v*13*std::exp(-u/(.1f+envelope));if(i==0)line.startNewSubPath(x,y);else line.lineTo(x,y);}g.setColour(c);g.strokePath(line,juce::PathStrokeType(1));text(g,"OSC "+juce::String(osc+1),{r.getX()+4,r.getY()+2,40,8},6,c,juce::Justification::centredLeft);
        }
        for(int i=0;i<24;++i){auto r=juce::Rectangle<float>(a.getX()+i*10.25f,a.getBottom()-16,8.5f,13);auto c=colours[(size_t)((i/6)%4)];g.setColour((padIsHeld(i)?colours[3]:c).withAlpha(padIsHeld(i)?.85f:i==selectedPad?.35f:.09f));g.fillRoundedRectangle(r,1);g.setColour(i==selectedPad?colours[2]:c);g.drawRoundedRectangle(r,1,i==selectedPad?1.f:.5f);}return;
    }
    if(mode==1){for(int i=0;i<24;++i){float x=a.getX()+(i%12)*20.f,y=a.getY()+(i/12)*32.f;auto c=colours[(size_t)(i%4)];g.setColour(c.withAlpha(i==selectedPad?.4f:.05f));g.fillRoundedRectangle({x,y,16,26},2);g.setColour(c.withAlpha(.6f));g.drawRoundedRectangle({x,y,16,26},2,1);text(g,juce::String(i+1),{x,y,16,26},8,c);}return;}
    if(e==1){
        std::array<juce::Point<float>,4> nodes;for(int i=0;i<4;++i)nodes[(size_t)i]={a.getX()+23+i*65.f,a.getCentreY()+(i%2?1:-1)*w*12};
        int topology=juce::jlimit(0,3,(int)(z*3.999f));const int edges[4][6]={{0,1,1,2,2,3},{0,3,1,3,2,3},{0,1,2,3,1,3},{0,1,1,0,2,3}};
        g.setColour(colours[1].withAlpha(.3f+m*.7f));for(int i=0;i<6;i+=2)g.drawLine(juce::Line<float>(nodes[(size_t)edges[topology][i]],nodes[(size_t)edges[topology][i+1]]),.6f+m*1.8f);
        for(int i=0;i<4;++i){g.setColour(colours[(size_t)i].withAlpha(i==3?.2f:.07f));g.fillEllipse(nodes[(size_t)i].x-16,nodes[(size_t)i].y-16,32,32);g.setColour(colours[(size_t)i]);g.drawEllipse(nodes[(size_t)i].x-16,nodes[(size_t)i].y-16,32,32,1.2f);text(g,juce::String(i+1),{nodes[(size_t)i].x-15,nodes[(size_t)i].y-15,30,30},11,colours[(size_t)i]);}
        text(g,"ROUTING "+juce::String(topology+1)+" / RATIO CONTROL "+juce::String(n,2),a.withY(a.getBottom()-9).withHeight(9),7,colours[2]);return;
    }
    if(e==9){int count=16+(int)(n*40);for(int i=0;i<count;++i){float u=i/(float)juce::jmax(1,count-1),x=a.getX()+u*a.getWidth(),noise=std::sin(i*12.17f+z*7)*w*5,v=std::sin(u*(8+z*12))*a.getHeight()*(.15f+m*.25f);g.setColour(colours[0]);g.fillEllipse(x-1,a.getCentreY()+v+noise-1,2,2);g.setColour(colours[3]);g.fillEllipse(x-1,a.getCentreY()-v-noise-1,2,2);if(i%3==0){g.setColour(colours[2].withAlpha(.25f+z*.4f));g.drawLine(x,a.getCentreY()-v,x,a.getCentreY()+v,.8f);}}return;}
    if(e==5){for(int i=0;i<5;++i){float rad=12+i*(3+z*5.f);g.setColour(colours[(size_t)(i%4)].withAlpha(.7f));g.drawEllipse(a.getCentreX()-rad*2+std::sin(i+m*6)*n*17,a.getCentreY()-rad*.7f,rad*4,rad*(.8f+w*.8f),1);}return;}
    if(e==6){for(int k=0;k<6;++k){juce::Path string;for(int i=0;i<180;++i){float u=i/179.f,x=a.getX()+u*a.getWidth(),y=a.getY()+k*11+std::sin(u*juce::MathConstants<float>::pi)*std::sin(u*(7+m*20+k*(.2f+z*2))+w*3)*8*(.2f+m)*std::exp(-u*(.1f+n*3));if(i==0)string.startNewSubPath(x,y);else string.lineTo(x,y);}g.setColour(colours[(size_t)(k%4)]);g.strokePath(string,juce::PathStrokeType(.8f));}return;}
    if(e==10){for(int k=0;k<2;++k){auto r=juce::Rectangle<float>(a.getX()+k*126,a.getY()+5,114,a.getHeight()-10);g.setColour(colours[(size_t)k].withAlpha(.25f+(k==0?1-m:m)*.65f));g.drawRoundedRectangle(r,3,.8f);juce::Path wave;for(int i=0;i<100;++i){float u=i/99.f,ph=u*(2+n*5)+std::sin(u*15)*w,signal=std::sin(ph*juce::MathConstants<float>::twoPi)*std::exp(-u/(.1f+z*2));float x=r.getX()+u*r.getWidth(),y=r.getCentreY()+signal*r.getHeight()*.35f;if(i==0)wave.startNewSubPath(x,y);else wave.lineTo(x,y);}g.strokePath(wave,juce::PathStrokeType(1.3f));text(g,k==0?"SOURCE A":"SOURCE B",r.withHeight(11),7,colours[(size_t)k]);}return;}
    if(e==12||e==13){
        auto diagram=a.withTrimmedBottom(13);
        if(e==12){for(int i=0;i<12;++i){float u=i/11.f,shape=std::exp(-std::pow((u-n)*(.8f+z*7),2))*(.35f+m*.65f),height=diagram.getHeight()*.8f*shape;g.setColour(colours[(size_t)(i%4)].withAlpha(.25f+w*.7f));g.fillRoundedRectangle(diagram.getX()+i*20.f,diagram.getBottom()-height,12,height,1);}text(g,"BAND SHAPE PREVIEW",diagram.withHeight(10),7,colours[2],juce::Justification::centredLeft);}
        else{juce::Path transfer;for(int i=0;i<160;++i){float u=i/159.f,input=u*2-1,value=std::tanh(input*(1+m*3+w*10))/(1+n*1.5f);float x=diagram.getX()+u*diagram.getWidth(),y=diagram.getCentreY()-value*diagram.getHeight()*.38f*(.5f+z*.5f);if(i==0)transfer.startNewSubPath(x,y);else transfer.lineTo(x,y);}g.setColour(colours[3]);g.strokePath(transfer,juce::PathStrokeType(1.5f));text(g,"INPUT > TONE / DRIVE",diagram.withHeight(10),7,colours[2],juce::Justification::centredLeft);}
        const float peak=juce::jlimit(0.f,1.f,processor.outputPeak.load());text(g,"OUT",{a.getX(),a.getBottom()-9,24,9},6,colours[2],juce::Justification::centredLeft);g.setColour(colours[2].withAlpha(.12f));g.fillRect(a.getX()+27,a.getBottom()-7,a.getWidth()-28,4.f);g.setColour(colours[0]);g.fillRect(a.getX()+27,a.getBottom()-7,(a.getWidth()-28)*peak,4.f);return;
    }
    if(e==11){drawSampler(g,a);return;}
    const int count=e==2?1+(int)(m*6.99f):e==0?2:1;
    for(int k=0;k<count;++k){juce::Path path;for(int i=0;i<240;++i){float u=i/239.f,v=std::sin(u*(10+m*34)+z*2+k*(.1f+w)*.65f);if(e==3){float steps=2+w*12;v=std::round(std::sin(u*(8+std::round(n*3)*8))*std::cos(u*z*23)*steps)/steps;}else if(e==7)v=((std::fmod(u*(2+m*3),1.f)<(.1f+z*.7f)?1.f:-1.f)+std::sin(u*30)*w*.3f)*n;else if(e==8)v=std::sin(u*(12+z*12)+std::sin(u*7+m*6)*n*8)+(u-.5f)*w*.5f;else if(e==4){float steps=2+n*10;v=std::round(std::sin(u*(12+m*45)+z*6+k*w)*steps)/steps;}else if(e==0)v=((2*std::fmod(u*(2+m*4)+k*w*.04f,1.f)-1)*(.3f+z*.5f)+std::sin(std::floor(u*90)*13.71f)*n*.16f);else if(e==2)v*=std::exp(-u*n*2)*(.4f+z*.6f);float x=a.getX()+u*a.getWidth(),y=a.getCentreY()-v*a.getHeight()*(count>1?.27f:.4f)+(k-(count-1)*.5f)*4;if(i==0)path.startNewSubPath(x,y);else path.lineTo(x,y);}g.setColour(colours[(size_t)(k%4)].withAlpha(count>1?.7f:1.f));g.strokePath(path,juce::PathStrokeType(1.3f));}
}
void FieldEditor::drawTape(juce::Graphics& g,juce::Rectangle<float> a){
    text(g,"TAPE "+juce::String(processor.tapeNumber()+1)+juce::String::fromUTF8(" · ")+juce::StringArray{"STUDIO","VINTAGE","PORTA","DISC MINI"}[(int)processor.value("tapeStyle")],a.withHeight(13),9,colours[1],juce::Justification::centredLeft);
    text(g,countInRemaining>0?"ARM / COUNT "+juce::String((int)std::ceil(countInRemaining)):recording?"REC T"+juce::String(selectedTrack+1):playing?juce::String::fromUTF8("▶ PLAY"):"STOP",a.withHeight(13),8,recording?colours[3]:colours[2],juce::Justification::centredRight);
    auto waveArea=a.withTrimmedTop(19).withTrimmedBottom(13);float top=waveArea.getY(),row=waveArea.getHeight()/4;
    for(int i=0;i<4;++i){auto r=juce::Rectangle<float>(waveArea.getX()+15,top+i*row,waveArea.getWidth()-15,row-2);g.setColour(colours[(size_t)i].withAlpha(((processor.tapeEditMask.load()?processor.tapeEditMask.load():(1<<selectedTrack))&(1<<i))?.14f:.04f));g.fillRect(r);text(g,juce::String(i+1),{waveArea.getX(),r.getY(),12,row},8,colours[(size_t)i]);g.setColour(colours[(size_t)i].withAlpha(.7f));g.drawLine(r.getX(),r.getCentreY(),r.getRight(),r.getCentreY(),.4f);
        for(double cut:trackCuts[(size_t)i])if(cut>=0&&cut<=visibleSeconds){float cx=r.getX()+(float)(cut/visibleSeconds)*r.getWidth();g.setColour(colours[(size_t)i]);g.drawLine(cx,r.getY(),cx,r.getBottom(),.7f);}
        auto& w=waves[(size_t)i];for(size_t j=0;j<w.size();++j){float secondsAt=(float)(trackDurations[(size_t)i]*j)/juce::jmax(1.,(double)w.size());if(secondsAt>visibleSeconds)break;float x=r.getX()+(float)(secondsAt/visibleSeconds)*r.getWidth(),h=juce::jlimit(.4f,row*.45f,std::abs(w[j])*row*.48f);g.drawLine(x,r.getCentreY()-h,x,r.getCentreY()+h,1);}
    }
    const float selectionX=waveArea.getX()+15+(float)(processor.selectionStart/visibleSeconds)*(waveArea.getWidth()-15),selectionRight=waveArea.getX()+15+(float)(processor.selectionEnd/visibleSeconds)*(waveArea.getWidth()-15);
    g.setColour(colours[0].withAlpha(.15f));g.fillRect(juce::Rectangle<float>(selectionX,waveArea.getY(),juce::jmax(0.f,selectionRight-selectionX),waveArea.getHeight()).getIntersection(waveArea));
    float x=waveArea.getX()+15+(float)(processor.playhead.load()/visibleSeconds)*(waveArea.getWidth()-15);g.setColour(colours[3]);g.drawLine(x,waveArea.getY(),x,waveArea.getBottom(),1.2f);
    text(g,seconds(processor.playhead.load())+"      IN "+juce::String(processor.selectionStart,1)+"   OUT "+juce::String(processor.selectionEnd,1),a.withY(a.getBottom()-11).withHeight(11),7.7f,colours[2],juce::Justification::centredLeft);
}
void FieldEditor::drawMixer(juce::Graphics& g,juce::Rectangle<float> a){
    if(page==1){
        text(g,"MASTER EQ / DRIVE",a.withHeight(13),9,colours[1],juce::Justification::centredLeft);
        juce::Path line;for(int i=0;i<200;++i){float u=i/199.f;float gain=processor.value("masterLow")*std::exp(-u*u*18)+processor.value("masterMid")*std::exp(-std::pow((u-.5f)*5,2))+processor.value("masterHigh")*std::exp(-(1-u)*(1-u)*18);float x=a.getX()+u*a.getWidth(),y=a.getCentreY()-gain*1.2f;if(i==0)line.startNewSubPath(x,y);else line.lineTo(x,y);}g.setColour(colours[1]);g.strokePath(line,juce::PathStrokeType(1.6f));
        text(g,"LOW             MID             HIGH             DRIVE",a.withY(a.getBottom()-12).withHeight(12),7,colours[2]);return;
    }
    if(page==2){text(g,"MASTER / "+FieldProcessor::effectNames[(int)processor.value("masterEffect")].toUpperCase(),a.withHeight(13),9,colours[3],juce::Justification::centredLeft);drawEffect(g,a.withTrimmedTop(18),true);return;}
    if(page==3){text(g,"MASTER DYNAMICS",a.withHeight(13),9,colours[0],juce::Justification::centredLeft);float peak=juce::jlimit(0.f,1.f,processor.outputPeak.load());g.setColour(colours[0].withAlpha(.15f));g.fillRect(a.getX(),a.getCentreY()-9,a.getWidth(),18.f);g.setColour(colours[0]);g.fillRect(a.getX(),a.getCentreY()-9,a.getWidth()*peak,18.f);text(g,"COMPRESSOR "+juce::String(processor.value("compressor")*100,0)+"%     "+(processor.hostSync?"HOST "+juce::String(processor.effectiveTempo.load(),0):processor.value("midiClockMode")==1?"MIDI "+juce::String(processor.effectiveTempo.load(),0):juce::String(processor.value("tempo"),0))+" BPM",a.withY(a.getBottom()-16).withHeight(16),8,colours[2]);return;}
    text(g,shifted?juce::String::fromUTF8("MIXER · PAN"):juce::String::fromUTF8("MIXER · LEVEL"),a.withHeight(13),9,colours[2],juce::Justification::centredLeft);text(g,"SHIFT: PAN",a.withHeight(13),7,juce::Colour(0xff72888c),juce::Justification::centredRight);
    for(int i=0;i<4;++i){float x=a.getX()+i*62.f;auto c=colours[(size_t)i];text(g,juce::String(i+1),{x,a.getY()+18,52,12},9,c);auto r=juce::Rectangle<float>(x+16,a.getY()+34,20,51);g.setColour(c.withAlpha(.09f));g.fillRect(r);g.setColour(c.withAlpha(mutes[(size_t)i]?.12f:.8f));float h=juce::jlimit(0.f,51.f,levels[(size_t)i]*31);g.fillRect(r.withY(r.getBottom()-h).withHeight(h));text(g,mutes[(size_t)i]?"MUTE":solos[(size_t)i]?"SOLO":juce::String((int)std::round(pans[(size_t)i]*100)),{x,a.getBottom()-22,52,12},8,c);text(g,"M     S",{x,a.getBottom()-10,52,10},7.5f,c);}
}
void FieldEditor::drawSequence(juce::Graphics& g,juce::Rectangle<float> a){
    int seq=(int)processor.value("sequencer");
    auto title=FieldProcessor::sequencerNames[seq].toUpperCase();if(seq==3)title+=" / BANK "+juce::String(fingerBank+1);
    text(g,title,a.withHeight(13),9,colours[0],juce::Justification::centredLeft);
    if(seq==0){text(g,"CHOOSE A SEQUENCER",a.withTrimmedTop(25).withTrimmedBottom(25),14,colours[2]);text(g,"USE THE SEQUENCER MENU ABOVE",a.withY(a.getBottom()-15).withHeight(15),7,colours[2].withAlpha(.6f));return;}
    if(seq==1||seq==4){
        int count=0;for(bool n:sequencerHeld)if(n)++count;
        text(g,juce::String(count)+(seq==4?" NOTES LATCHED":" NOTES HELD"),a.withHeight(13),7,colours[2],juce::Justification::centredRight);
        auto area=a.withTrimmedTop(24).withTrimmedBottom(19);int index=0;
        if(count==0)text(g,seq==1?"HOLD NOTES TO ARPEGGIATE":"PLAY NOTES TO LATCH",area,12,colours[1]);
        for(int n=0;n<128;++n)if(sequencerHeld[(size_t)n]){float slot=juce::jmin(37.f,area.getWidth()/juce::jmax(1,count));float x=area.getX()+index*slot,w=juce::jmax(5.f,slot-4);float h=juce::jlimit(10.f,area.getHeight(),(n-24)/84.f*area.getHeight());auto c=colours[(size_t)(index%4)];g.setColour(c.withAlpha(.18f));g.fillRoundedRectangle({x,area.getBottom()-h,w,h},2);g.setColour(c);g.drawLine(x,area.getBottom()-h,x+w,area.getBottom()-h,1.3f);text(g,juce::MidiMessage::getMidiNoteName(n,true,false,3),{x,area.getBottom()-h-11,w,10},7,c);++index;}
        text(g,seq==1?"PLAY CHORDS / DIRECTION ON BLUE":juce::String::fromUTF8("NOTES STAY HELD · ESC RELEASES"),a.withY(a.getBottom()-11).withHeight(11),7,colours[2],juce::Justification::centredLeft);return;
    }
    if(seq==6){
        text(g,juce::String(sketchCount)+" POINTS",a.withHeight(13),7,colours[2],juce::Justification::centredRight);
        auto canvas=a.withTrimmedTop(20).withTrimmedBottom(16);
        g.setColour(colours[2].withAlpha(.09f));for(int i=0;i<=8;++i){float x=canvas.getX()+i/8.f*canvas.getWidth();g.drawLine(x,canvas.getY(),x,canvas.getBottom(),.5f);}for(int i=0;i<=4;++i){float y=canvas.getY()+i/4.f*canvas.getHeight();g.drawLine(canvas.getX(),y,canvas.getRight(),y,.5f);}
        juce::Path path;for(int i=0;i<sketchCount;++i){auto xy=sketchPoints[(size_t)i];auto point=juce::Point<float>(canvas.getX()+xy.first*canvas.getWidth(),canvas.getY()+xy.second*canvas.getHeight());if(i==0)path.startNewSubPath(point);else path.lineTo(point);}
        g.setColour(colours[0]);g.strokePath(path,juce::PathStrokeType(1.7f,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));
        if(sketchCount>0){auto p=sketchPoints[(size_t)(sketchCount-1)];g.setColour(colours[3]);g.fillEllipse(canvas.getX()+p.first*canvas.getWidth()-2.5f,canvas.getY()+p.second*canvas.getHeight()-2.5f,5,5);}
        text(g,juce::String::fromUTF8("DRAW TO REPLACE · HOLD A KEY TO PLAY"),a.withY(a.getBottom()-11).withHeight(11),7,colours[2],juce::Justification::centredLeft);return;
    }
    if(seq==7){
        int active=0;for(auto& b:balls)if(b.active)++active;
        text(g,processor.value("seqHold")>.5f?"LATCHED":"KEY / RELEASE",a.withHeight(13),7,colours[2],juce::Justification::centredRight);
        auto canvas=a.withTrimmedTop(18).withTrimmedBottom(15);float unit=canvas.getHeight()/(.88f*1.12f),cx=canvas.getCentreX(),cy=canvas.getCentreY();
        juce::Path container;for(int i=0;i<tombolaSides;++i){float angle=tombolaRotation+juce::MathConstants<float>::pi/tombolaSides+juce::MathConstants<float>::twoPi*i/tombolaSides;float x=cx+std::cos(angle)*.44f*unit,y=cy+std::sin(angle)*.44f*unit;if(i==0)container.startNewSubPath(x,y);else container.lineTo(x,y);}container.closeSubPath();g.setColour(colours[2].withAlpha(.08f));g.fillPath(container);g.setColour(colours[2].withAlpha(.7f));g.strokePath(container,juce::PathStrokeType(1));
        for(size_t i=0;i<balls.size();++i){auto b=balls[i];if(!b.active)continue;float x=cx+(b.x-.5f)*unit,y=cy+(b.y-.5f)*unit;g.setColour(colours[i%4]);g.fillEllipse(x-2.6f,y-2.6f,5.2f,5.2f);if(active<=8)text(g,juce::MidiMessage::getMidiNoteName(b.note,true,false,3),{x-10,y-11,20,8},6,colours[i%4]);}
        text(g,"BALLS",{a.getX(),canvas.getY()+7,60,11},7,colours[0],juce::Justification::centredLeft);text(g,juce::String(active),{a.getX(),canvas.getY()+19,60,20},18,colours[0],juce::Justification::centredLeft);
        text(g,"IMPACTS",{a.getRight()-60,canvas.getY()+7,60,11},7,colours[3],juce::Justification::centredRight);text(g,juce::String((juce::int64)tombolaTriggers),{a.getRight()-60,canvas.getY()+19,60,20},16,colours[3],juce::Justification::centredRight);
        text(g,juce::String::fromUTF8("PLAY KEYS TO DROP NOTES · ESC CLEARS"),a.withY(a.getBottom()-11).withHeight(11),7,colours[2],juce::Justification::centredLeft);return;
    }
    text(g,juce::String(sequenceOffset+1)+juce::String::fromUTF8("–")+juce::String(juce::jmin(sequenceLength,sequenceOffset+16))+" / "+juce::String(sequenceLength),a.withHeight(13),7,colours[2],juce::Justification::centredRight);
    for(int i=0;i<16;++i){auto r=juce::Rectangle<float>(a.getX()+(i%8)*31.f,a.getY()+23+(i/8)*34.f,27,29);if(sequenceOffset+i>=sequenceLength){g.setColour(colours[2].withAlpha(.08f));g.drawRoundedRectangle(r,2,.6f);continue;}auto c=colours[(size_t)((i/4)%4)];g.setColour(c.withAlpha(steps[(size_t)i]<0?.06f:sequenceOffset+i==sequenceStep?.65f:.21f));g.fillRoundedRectangle(r,2);g.setColour(c.withAlpha(.7f));g.drawRoundedRectangle(r,2,.7f);text(g,steps[(size_t)i]<0?juce::String::fromUTF8("—"):juce::MidiMessage::getMidiNoteName(steps[(size_t)i],true,true,3),r.withHeight(20),8,c);text(g,juce::String(sequenceOffset+i+1),r.withY(r.getBottom()-9).withHeight(8),6.5f,c.withAlpha(.5f));}
    text(g,juce::String::fromUTF8("CLICK: REST / NOTE · SHIFT CLICK: +1 SEMITONE"),a.withY(a.getBottom()-11).withHeight(11),7,colours[2],juce::Justification::centredLeft);
}
void FieldEditor::functionAction(int i){
    if(i<4){int e=(int)processor.value("engine");if(i==0){processor.drumMode=false;if(e==14)processor.setValue("engine",(float)lastSynthEngine);}else if(i==1){if(e!=11&&e!=14){lastSynthEngine=e;processor.setValue("engine",14);}processor.drumMode=true;}setMode(i);}
    else if(i<8){if(mode==2)processor.setTrack(i-4);else setPage(i-4);}
    else if(i==8)processor.transport("in");else if(i==9)processor.transport("out");else if(i==10)processor.setValue("loop",processor.value("loop")>.5f?0:1);
    else if(i==11){if(processor.value("sequencer")==0){processor.setValue("sequencer",5);processor.setValue("sequenceOn",1);}setMode(4);}
    else if(i==12)processor.setValue("tapeSpeed",-processor.value("tapeSpeed"));else if(i==13)processor.metronome=!processor.metronome;
    else if(i==14||i==15){if(mode==4&&((int)processor.value("sequencer")==2||(int)processor.value("sequencer")==3||(int)processor.value("sequencer")==5))sequenceOffset=i==14?juce::jmax(0,sequenceOffset-16):juce::jmin((sequenceLength-1)/16*16,sequenceOffset+16);else{bool store=shifted||juce::ModifierKeys::getCurrentModifiersRealtime().isShiftDown();if(store)processor.storeMemo(i-14);else processor.recallMemo(i-14);status=(store?"Stored mixer memo ":"Recalled mixer memo ")+juce::String(i-13);}}else showMenu();
    timerCallback();repaint();
}
void FieldEditor::transportAction(int i){
    const bool shift=shifted||juce::ModifierKeys::getCurrentModifiersRealtime().isShiftDown();
    const juce::StringArray actions=shift?juce::StringArray{"undo","redo","clear","record","play","stop","rewind","forward"}:juce::StringArray{"lift","drop","split","record","play","stop","back","forward"};
    if(i==3&&shift){if(processor.sampling){processor.stopSampleCapture();setMode(processor.drumMode?1:0);status="Input sample captured";}else{processor.startSampleCapture();status=juce::String::fromUTF8("Sampling input · Shift + record to finish");}}
    else if(i==7&&shift){const juce::ScopedLock lock(processor.engineLock);processor.engine.seek(processor.selectionEnd);}else processor.transport(actions[i]);
    if(i==3&&!shift)setMode(2);if(i!=3||!shift)status=i==0?(shift?"Undo":"Selection lifted to clipboard"):i==1?(shift?"Redo":"Clipboard dropped at playhead"):i==2?(shift?juce::String::fromUTF8("Track cleared · Shift + lift to undo"):"Tape split at playhead"):"Tape transport";timerCallback();
}
void FieldEditor::fileAction(int action){
    if(action==0){processor.newProject();panic();status="New tape bank";timerCallback();return;}
    if(chooser)return;
    bool saving=action==2||action>=4;juce::String title=action==1?"Open Field Studio project":action==2?"Save project":action==3?"Import sample":action>=5?"Export album side "+juce::String(action==5?"A":"B"):"Export current tape mix";
    juce::String filter=action==1||action==2?"*.field":action==3?"*.wav;*.aif;*.aiff;*.aifc;*.flac;*.ogg":"*.wav";
    auto base=juce::File::getSpecialLocation(juce::File::userDocumentsDirectory);if(saving)base=base.getChildFile(processor.projectName+(action==2?".field":action==5?"-album-A.wav":action==6?"-album-B.wav":".wav"));
    chooser=std::make_unique<juce::FileChooser>(title,base,filter,true);auto flags=saving?juce::FileBrowserComponent::saveMode|juce::FileBrowserComponent::canSelectFiles|juce::FileBrowserComponent::warnAboutOverwriting:juce::FileBrowserComponent::openMode|juce::FileBrowserComponent::canSelectFiles;
    chooser->launchAsync(flags,[safe=juce::Component::SafePointer<FieldEditor>(this),action](const juce::FileChooser& c){if(!safe)return;auto file=c.getResult();if(file!=juce::File{}){juce::String error;bool ok=false;if(action==1)ok=safe->processor.loadProject(file,error);else if(action==2)ok=safe->processor.saveProject(file.withFileExtension("field"),error);else if(action==3)ok=safe->processor.loadSampleFile(file,error);else if(action>=5)ok=safe->processor.exportAlbum(action-5,file.withFileExtension("wav"),error);else ok=safe->processor.exportAudio(file.withFileExtension("wav"),error);safe->status=ok?(action==1?"Opened ":action==2?"Saved ":action==3?"Sample loaded: ":"Exported ")+file.getFileName():error;if(ok&&action==3){safe->setMode(safe->processor.drumMode?1:0);safe->status=safe->processor.lastImportSummary;}safe->configureMacros();safe->timerCallback();}juce::MessageManager::callAsync([safe]{if(safe){safe->chooser.reset();safe->grabKeyboardFocus();}});});
}
void FieldEditor::showHelp(){
    juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::InfoIcon,juce::String::fromUTF8("Field Studio · quick guide"),juce::String::fromUTF8("Play: A W S E D R F G Y H U J K O L P ; [ ' Z X C V B.\nOctave: - / +. Space: play / pause. Esc: all notes off.\n\nShift + T2 envelope knobs access Poly/Mono/Legato/Unison, portamento, bend range and patch volume.\n\nChoose SYNTH or DRUM, then T1–T4 for engine, envelope, effect and LFO. Drag the coloured encoders vertically; double-click resets. DRUM opens DBox unless a sampler kit is selected. In DBox, T1 edits two oscillators (Shift: second layer), T2 edits each pad dynamic envelope. Play a key or click a pad strip to select it.\n\nTape: choose a track with 1–4. Record starts tape capture after any configured count-in; play notes or enable INPUT to record external audio. Shift + record captures a sample (six seconds melodic / twenty seconds drum); Shift + record again finishes it. COM > Sampler > Resample main output captures the synth, effects and tape internally. IN / OUT mark a loop. Click the tape screen to seek, Shift-drag to select. Lift / drop / split edit tape; Shift changes these to undo / redo / clear. Tape edits > Tape editing adds group tracks, copy, merge, join, slide, bounce, beat snap and count-in. Hold brake / reverse / chop below the tape panel for temporary playback tricks. Tape knobs scrub, loop-out, speed and record level; Shift gives slide, loop-in, speed percent and record pan. M1 / M2 recall two mixer memos; Shift + M1 / M2 stores the current mix. COM selects among eight tape banks.\n\nMixer: T1 gives track levels (Shift: pan); T2 is EQ / drive, T3 master FX, T4 dynamics. Four encoders edit the displayed page. Click M or S in the screen to mute / solo.\n\nSequencer: select a mode, then hold a note to run. Click steps to enable / rest; Shift-click raises their pitch. M1 / M2 page through step grids; COM sets length up to 999 steps. Finger has sixteen independent banks selected by the first sixteen keys or COM; its banks support up to 32 steps. Sketch lets you draw pitch over time. Tombola turns held notes into bouncing balls that trigger on impacts. COM can latch notes. Sequencer encoders show only controls used by the current mode; Hold edits the source sound, and Off disables the four encoders. In FX, Shift + orange adjusts insert mix.\n\nNew / Open / Save preserve projects locally, including audio. Import loads your own sample. Export WAV writes a 24-bit stereo tape mix. COM > Album records a live post-master mix onto sides A / B, listens to them, and exports each side separately. Standalone Options controls audio and MIDI devices.\n\nIndependent recreation built from public documentation. DSP and screen illustrations are original approximations; hardware-only radio, accelerometer and Bluetooth functions are not emulated. Exact sonic and visual parity are not verified."));
}
void FieldEditor::showMenu(){
    juce::PopupMenu m;m.addSectionHeader("TAPE & CONNECTIONS");m.addItem(260,"Import details",processor.lastImportDetails.isNotEmpty());juce::PopupMenu tapeBanks;for(int i=0;i<8;++i)tapeBanks.addItem(200+i,"Tape "+juce::String(i+1),true,processor.tapeNumber()==i);m.addSubMenu("Tape bank",tapeBanks);for(int i=0;i<4;++i)m.addItem(10+i,juce::StringArray{"Studio tape","Vintage tape","Porta tape","Disc Mini tape"}[i],true,(int)processor.value("tapeStyle")==i);
    juce::PopupMenu edits,editTracks,countIn,grid;
    editTracks.addItem(500,"Follow selected track",true,processor.tapeEditMask==0);for(int i=0;i<4;++i)editTracks.addItem(501+i,"Include track "+juce::String(i+1),true,(processor.tapeEditMask.load()&(1<<i))!=0);editTracks.addItem(505,"All four tracks",true,processor.tapeEditMask==15);edits.addSubMenu("Tracks to edit",editTracks);
    edits.addItem(510,"Copy selection");edits.addItem(511,"Lift selection");edits.addItem(512,"Drop at playhead");edits.addItem(513,"Merge clipboard onto selected track");edits.addItem(514,"Join cuts within selection");edits.addItem(515,"Slide selection back one beat");edits.addItem(516,"Slide selection forward one beat");edits.addItem(517,"Bounce selection onto selected track");edits.addItem(518,"Snap seek and selection to grid",true,processor.tapeSnap);const juce::StringArray gridNames{"Bar (4 beats)","Half bar (2 beats)","Beat","Eighth note"};for(int i=0;i<4;++i)grid.addItem(530+i,gridNames[i],true,(int)processor.value("tapeGrid")==i);edits.addSubMenu("Edit grid",grid);
    const int countBeats[]{0,1,2,4,8};for(int i=0;i<5;++i)countIn.addItem(520+i,countBeats[i]==0?"Off":juce::String(countBeats[i])+" beats",true,(int)processor.value("countInBeats")==countBeats[i]);edits.addSubMenu("Record count-in",countIn);m.addSubMenu("Tape editing",edits);
    juce::PopupMenu midiMenu,inputChannels,outputChannels,clockModes;
    for(int i=0;i<=16;++i)inputChannels.addItem(660+i,i==0?"Omni":"Channel "+juce::String(i),true,(int)processor.value("midiInputChannel")==i);midiMenu.addSubMenu("Input channel",inputChannels);
    for(int i=1;i<=16;++i)outputChannels.addItem(700+i,"Channel "+juce::String(i),true,(int)processor.value("midiOutputChannel")==i);midiMenu.addSubMenu("Output channel",outputChannels);
    const juce::StringArray clockNames{"Internal","Receive MIDI clock","Send MIDI clock"};for(int i=0;i<3;++i)clockModes.addItem(610+i,clockNames[i],true,(int)processor.value("midiClockMode")==i);midiMenu.addSubMenu("Clock source / output",clockModes);
    const char* midiIds[]{"midiThru","midiSendControls","midiRelativeControls","midiNotesIn","midiOtherIn","midiTransportIn","midiNotesOut","midiOtherOut"};const juce::StringArray midiLabels{"MIDI thru","Controller output (notes + encoders)","Relative encoders (two's complement)","Receive notes / sustain","Receive other channel messages","Receive transport / clock","Send notes / sustain","Send other messages"};for(int i=0;i<8;++i)midiMenu.addItem(620+i,midiLabels[i],true,processor.value(midiIds[i])>.5f);
    midiMenu.addItem(630,"Set outgoing SPP origin at playhead");midiMenu.addItem(631,"Reset outgoing SPP origin");midiMenu.addItem(640,"MIDI monitor / settings");m.addSubMenu("MIDI routing & clock",midiMenu);
    juce::PopupMenu lfos;for(int i=0;i<FieldProcessor::lfoNames.size();++i)lfos.addItem(60+i,FieldProcessor::lfoNames[i],true,(int)processor.value("lfo")==i);m.addSubMenu("Modulation type",lfos);
    juce::PopupMenu targets;auto targetsNames=FieldProcessor::lfoTargetNames;for(int i=0;i<targetsNames.size();++i)targets.addItem(100+i,targetsNames[i],true,(int)processor.value("lfoTarget")==i);m.addSubMenu("Modulation destination",targets);
    juce::PopupMenu curves;curves.addItem(400,"Linear",true,processor.value("envelopeCurve")==0);curves.addItem(401,"Exponential",true,processor.value("envelopeCurve")==1);m.addSubMenu("Envelope curve",curves);
    juce::PopupMenu shapes;juce::StringArray shapeNames{"Sine","Triangle","Saw","Square","Exponential","Blip"};for(int i=0;i<6;++i)shapes.addItem(410+i,shapeNames[i],true,(int)processor.value("lfoWaveform")==i);m.addSubMenu("LFO waveform",shapes);
    m.addItem(420,"LFO key retrigger",true,processor.value("lfoRetrigger")>.5f);m.addItem(421,"LFO tempo sync",true,processor.value("lfoSync")>.5f);
    juce::PopupMenu sources;juce::StringArray sourceNames{"Audio input","Voice envelope","Main output","Manual control"};for(int i=0;i<4;++i)sources.addItem(430+i,sourceNames[i],true,(int)processor.value("elementSource")==i);m.addSubMenu("Element source",sources);
    juce::PopupMenu stores,recalls;for(int i=0;i<8;++i){stores.addItem(120+i,"Slot "+juce::String(i+1));recalls.addItem(140+i,"Slot "+juce::String(i+1));}m.addSubMenu("Store current patch",stores);m.addSubMenu("Recall patch",recalls);
    juce::PopupMenu lengths;const int lens[]{16,32,64,128,256,512,999};for(int i=0;i<7;++i)lengths.addItem(160+i,juce::String(lens[i])+" steps",true,sequenceLength==lens[i]);m.addSubMenu("Sequence length",lengths,(int)processor.value("sequencer")!=3&&(int)processor.value("sequencer")!=6&&(int)processor.value("sequencer")!=7);
    juce::PopupMenu banks;for(int i=0;i<16;++i)banks.addItem(300+i,"Pattern "+juce::String(i+1),true,fingerBank==i);m.addSubMenu("Finger pattern",banks);
    juce::PopupMenu fingerLengths;const int fingerLens[]{8,16,24,32};for(int i=0;i<4;++i)fingerLengths.addItem(320+i,juce::String(fingerLens[i])+" steps",true,(int)processor.value("sequencer")==3&&sequenceLength==fingerLens[i]);m.addSubMenu("Finger pattern length",fingerLengths);
    m.addItem(173,"Latch sequencer notes",true,processor.value("seqHold")>.5f);
    juce::PopupMenu album;album.addItem(220,"Record side A");album.addItem(221,"Record side B");album.addItem(222,"Listen to side A");album.addItem(223,"Listen to side B");album.addItem(224,"Stop album");album.addSeparator();album.addItem(225,"Export side A as WAV");album.addItem(226,"Export side B as WAV");album.addSeparator();album.addItem(227,"Clear side A");album.addItem(228,"Clear side B");m.addSubMenu("Album A / B",album);
    m.addItem(170,"Sequencer enabled",true,processor.value("sequenceOn")>.5f);m.addItem(171,"Release held sequencer notes");
    if(mode==1&&(int)processor.value("engine")==11)m.addItem(172,"Reverse selected drum pad",true,padState.reverse);
    juce::PopupMenu presets;for(int i=0;i<processor.getNumPrograms();++i)presets.addItem(80+i,processor.getProgramName(i),true,processor.getCurrentProgram()==i);m.addSubMenu("Starting sounds",presets);
    if(processor.parameters.getParameter("sampleLoop")){juce::PopupMenu sampler;sampler.addItem(50,"Loop sample",true,processor.value("sampleLoop")>.5f);sampler.addItem(51,"Reverse sample",true,processor.value("sampleReverse")>.5f);sampler.addItem(52,"One-shot playback",true,processor.value("sampleOneShot")>.5f);sampler.addItem(53,processor.sampling?"Finish sampling":"Record input sample");sampler.addItem(54,processor.sampling?"Finish sampling":"Resample main output");m.addSubMenu("Sampler",sampler);}
    m.addSeparator();m.addItem(20,"Follow DAW transport and tempo",true,processor.hostSync);m.addItem(21,"Monitor audio input",true,processor.inputMonitor);m.addItem(22,"Metronome",true,processor.metronome);m.addItem(23,"Loop selection",true,processor.value("loop")>.5f);m.addSeparator();m.addItem(30,"Undo tape edit");m.addItem(31,"Redo tape edit");m.addItem(32,"Rewind to start");m.addItem(33,"All notes off");m.addSeparator();m.addItem(40,"Quick guide & implementation notes");
    m.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&comButton),[safe=juce::Component::SafePointer<FieldEditor>(this)](int id){if(!safe)return;
        if(id>=660&&id<=676)safe->processor.setValue("midiInputChannel",(float)(id-660));
        else if(id>=701&&id<=716)safe->processor.setValue("midiOutputChannel",(float)(id-700));
        else if(id>=610&&id<=612)safe->processor.setValue("midiClockMode",(float)(id-610));
        else if(id>=620&&id<=627){const char* ids[]{"midiThru","midiSendControls","midiRelativeControls","midiNotesIn","midiOtherIn","midiTransportIn","midiNotesOut","midiOtherOut"};safe->processor.setValue(ids[id-620],safe->processor.value(ids[id-620])>.5f?0:1);}
        else if(id==630||id==631)safe->processor.setValue("midiSppOrigin",id==630?(float)safe->processor.playhead.load():0);
        else if(id==640){auto& p=safe->processor;juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::InfoIcon,"MIDI monitor", "Received events: "+juce::String((juce::int64)p.midiReceived.load())+"\nLast status / data: "+juce::String::toHexString(p.midiLastStatus.load())+" / "+juce::String(p.midiLastData1.load())+" / "+juce::String(p.midiLastData2.load())+"\nEffective tempo: "+juce::String(p.effectiveTempo.load(),2)+" BPM\nOutput channel: "+juce::String((int)p.value("midiOutputChannel"))+"\nSPP origin: "+juce::String(p.value("midiSppOrigin"),2)+" seconds\n\nSelect standalone MIDI input/output ports in Options. DAW MIDI routing is configured in the host. Hardware BLE hosting and audio-pulse sync are not emulated.");}
        else if(id==500)safe->processor.tapeEditMask=0;
        else if(id>=501&&id<=504)safe->processor.tapeEditMask=safe->processor.tapeEditMask.load()^(1<<(id-501));
        else if(id==505)safe->processor.tapeEditMask=15;
        else if(id>=510&&id<=517){const char* commands[]{"copy","lift","drop","merge","join","slideBack","slideForward","bounce"};safe->processor.transport(commands[id-510]);safe->status="Tape edit: "+juce::String(commands[id-510]);}
        else if(id==518)safe->processor.tapeSnap=!safe->processor.tapeSnap;
        else if(id>=530&&id<=533)safe->processor.setValue("tapeGrid",(float)(id-530));
        else if(id>=520&&id<=524){const int beats[]{0,1,2,4,8};safe->processor.setValue("countInBeats",(float)beats[id-520]);}
        else if(id>=400&&id<402){safe->processor.setValue("envelopeCurve",(float)(id-400));safe->status="Envelope curve changed";}else if(id>=410&&id<416)safe->processor.setValue("lfoWaveform",(float)(id-410));else if(id==420)safe->processor.setValue("lfoRetrigger",safe->processor.value("lfoRetrigger")>.5f?0:1);else if(id==421)safe->processor.setValue("lfoSync",safe->processor.value("lfoSync")>.5f?0:1);else if(id>=430&&id<434)safe->processor.setValue("elementSource",(float)(id-430));else if(id>=300&&id<316){const juce::ScopedLock lock(safe->processor.engineLock);safe->processor.engine.setFingerPattern(id-300);safe->fingerBank=id-300;safe->sequenceOffset=0;safe->processor.setValue("sequencer",3);safe->processor.setValue("sequenceOn",1);safe->setMode(4);}
        else if(id>=320&&id<324){const int lens[]{8,16,24,32};const juce::ScopedLock lock(safe->processor.engineLock);safe->processor.engine.setFingerLength(safe->fingerBank,lens[id-320]);}
        else if(id==173)safe->processor.setValue("seqHold",safe->processor.value("seqHold")>.5f?0:1);
        else if(id==220||id==221){safe->processor.startAlbumRecording(id-220);safe->status="Recording album side "+juce::String(id==220?"A":"B");}
        else if(id==222||id==223){bool ok=safe->processor.playAlbum(id-222);safe->status=ok?"Listening to album side "+juce::String(id==222?"A":"B"):"That album side is empty";}
        else if(id==224){safe->processor.stopAlbum();safe->status="Album stopped";}
        else if(id==225||id==226)safe->fileAction(id-220);
        else if(id==227||id==228){safe->processor.clearAlbum(id-227);safe->status="Album side cleared";}
        else if(id==260){juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::InfoIcon,"Sound import details",safe->processor.lastImportDetails);}
        else if(id>=200&&id<208){safe->processor.selectTape(id-200);safe->status="Tape bank "+juce::String(id-199);}else if(id>=100&&id<120){safe->processor.setValue("lfoTarget",(float)(id-100));safe->setPage(3);}else if(id>=120&&id<128){safe->processor.savePatchSlot(id-120);safe->status="Stored patch in slot "+juce::String(id-119);}else if(id>=140&&id<148){safe->processor.recallPatchSlot(id-140);safe->configureMacros();safe->status="Recalled patch slot "+juce::String(id-139);}else if(id>=160&&id<167){const int lens[]{16,32,64,128,256,512,999};const juce::ScopedLock lock(safe->processor.engineLock);safe->processor.engine.setSequenceLength(lens[id-160]);}else if(id==170)safe->processor.setValue("sequenceOn",safe->processor.value("sequenceOn")>.5f?0:1);else if(id==171){const juce::ScopedLock lock(safe->processor.engineLock);safe->processor.engine.clearHeldNotes();}else if(id==172){const juce::ScopedLock lock(safe->processor.engineLock);auto pad=safe->processor.engine.drumPad(safe->selectedPad);pad.reverse=!pad.reverse;safe->processor.engine.setDrumPad(safe->selectedPad,pad);}else if(id>=10&&id<14)safe->processor.setValue("tapeStyle",(float)(id-10));else if(id>=60&&id<66){safe->processor.setValue("lfo",(float)(id-60));safe->setPage(3);}else if(id>=80&&id<96){safe->processor.setCurrentProgram(id-80);safe->setMode(safe->processor.drumMode?1:0);}else if(id>=50&&id<=52){const char* params[]{"sampleLoop","sampleReverse","sampleOneShot"};auto param=params[id-50];safe->processor.setValue(param,safe->processor.value(param)>.5f?0:1);}else if(id==53||id==54){if(safe->processor.sampling)safe->processor.stopSampleCapture();else safe->processor.startSampleCapture(id==54);safe->status=safe->processor.sampling?(id==54?juce::String::fromUTF8("Resampling output · Shift + record to finish"):juce::String::fromUTF8("Sampling input · Shift + record to finish")):"Sample captured";}else if(id==20)safe->processor.hostSync=!safe->processor.hostSync;else if(id==21)safe->processor.inputMonitor=!safe->processor.inputMonitor;else if(id==22)safe->processor.metronome=!safe->processor.metronome;else if(id==23)safe->processor.setValue("loop",safe->processor.value("loop")>.5f?0:1);else if(id==30)safe->processor.transport("undo");else if(id==31)safe->processor.transport("redo");else if(id==32)safe->processor.transport("rewind");else if(id==33)safe->panic();else if(id==40)safe->showHelp();safe->grabKeyboardFocus();});
}
void FieldEditor::keyboardNote(int n,float velocity,bool on){if(n<0||n>127)return;held[(size_t)n]=on;if(on)processor.keyboard.noteOn(processor.keyboardMidiChannel(),n,velocity);else for(int channel=1;channel<=16;++channel)if(processor.keyboard.isGuiNoteOn(channel,n))processor.keyboard.noteOff(channel,n,0);repaint();}
void FieldEditor::panic(){processor.releaseTapePerformance();drawingSketch=false;draggingSampleMarker=-1;typingNotes.clear();held.fill(false);mouseNote=-1;processor.transport("panic");}
bool FieldEditor::keyPressed(const juce::KeyPress& k){
    auto code=k.getKeyCode();if(code==juce::KeyPress::escapeKey){panic();return true;}if(code==juce::KeyPress::spaceKey){processor.transport("play");return true;}
    if(code=='-'||code=='='||code=='+'){panic();processor.octave=juce::jlimit(-3,3,processor.octave+(code=='-'?-1:1));status="Keyboard octave "+juce::String(processor.octave.load());return true;}
    if(k.getModifiers().isCommandDown()||k.getModifiers().isCtrlDown()){if(mode==2&&(code=='C'||code=='X'||code=='V')){processor.transport(code=='C'?"copy":code=='X'?"lift":"drop");return true;}if(code=='S'){fileAction(2);return true;}if(code=='O'){fileAction(1);return true;}if(code=='Z'){processor.transport(k.getModifiers().isShiftDown()?"redo":"undo");return true;}return false;}
    for(int i=0;i<typingKeys.size();++i)if(juce::String::charToString(k.getTextCharacter()).toLowerCase()==typingKeys[i]){if(typingNotes.count(code)==0){int n=53+i+processor.octave*12;typingNotes[code]=n;keyboardNote(n,.8f,true);}return true;}
    return false;
}
bool FieldEditor::keyStateChanged(bool){bool changed=false;setShift(shiftLatched||(hasKeyboardFocus(true)&&juce::ModifierKeys::getCurrentModifiersRealtime().isShiftDown()));for(auto it=typingNotes.begin();it!=typingNotes.end();){if(!juce::KeyPress::isKeyCurrentlyDown(it->first)){keyboardNote(it->second,0,false);it=typingNotes.erase(it);changed=true;}else ++it;}return changed;}
void FieldEditor::focusLost(FocusChangeType){panic();}
juce::Point<float> FieldEditor::localPoint(juce::Point<float> p) const{return {(p.x-hardware.getX())/scale,(p.y-hardware.getY())/scale};}
int FieldEditor::noteAt(juce::Point<float> p) const{for(int i=0;i<10;++i)if(blackKeys[(size_t)i].contains(p))return 53+blackOffsets[(size_t)i]+processor.octave*12;for(int i=0;i<14;++i)if(whiteKeys[(size_t)i].contains(p))return 53+whiteOffsets[(size_t)i]+processor.octave*12;return -1;}
void FieldEditor::appendSketchPoint(juce::Point<float> p){
    juce::Rectangle<float> canvas(316,67,247,80);
    float x=juce::jlimit(0.f,1.f,(p.x-canvas.getX())/canvas.getWidth()),y=juce::jlimit(0.f,1.f,(p.y-canvas.getY())/canvas.getHeight());
    if(sketchWriteIndex>=256){status="Sketch has 256 points; release to finish";return;}
    if(sketchWriteIndex>0){auto previous=sketchPoints[(size_t)(sketchWriteIndex-1)];if(std::hypot(x-previous.first,y-previous.second)<.005f)return;}
    const juce::ScopedLock lock(processor.engineLock);
    if(sketchWriteIndex==0){processor.engine.setSketchPoint(0,x,y);processor.engine.setSketchPoint(1,x,y);sketchPoints[0]={x,y};sketchPoints[1]={x,y};sketchWriteIndex=1;sketchCount=2;}
    else{processor.engine.setSketchPoint(sketchWriteIndex,x,y);sketchPoints[(size_t)sketchWriteIndex]={x,y};++sketchWriteIndex;sketchCount=juce::jmax(2,sketchWriteIndex);}
    processor.engine.setSketchLength(sketchCount);status="Drawing Sketch: "+juce::String(sketchCount)+" / 256 points";repaint();
}
void FieldEditor::mouseDown(const juce::MouseEvent& e){
    grabKeyboardFocus();auto p=localPoint(e.position);int n=noteAt(p);if(n>=0){mouseNote=n;keyboardNote(n,juce::jlimit(.25f,1.f,(p.y-240)/205),true);return;}
    juce::Rectangle<float> screen(316,47,247,116);if(!screen.contains(p))return;
    if(mode==2){const juce::ScopedLock lock(processor.engineLock);double pos=processor.snapTapeTime(juce::jlimit(0.,visibleSeconds,(double)((p.x-331)/232*visibleSeconds)));if(e.mods.isShiftDown()||shifted){draggingSelection=true;processor.selectionStart=juce::jmin(359.9,pos);processor.selectionEnd=processor.selectionStart+.1;}else processor.engine.seek(pos);}
    else if(mode==1&&page==0&&(int)processor.value("engine")==14&&p.y>screen.getBottom()-32){int pad=juce::jlimit(0,23,(int)((p.x-screen.getX())/10.25f));const juce::ScopedLock lock(processor.engineLock);processor.engine.setSelectedPad(pad);selectedPad=pad;configureMacros();}
    else if((mode==0||mode==1)&&page==0&&(int)processor.value("engine")==11){
        if(mode==1&&p.y>=135&&p.y<=147){selectedPad=juce::jlimit(0,23,(int)((p.x-316)/(247.f/24)));{const juce::ScopedLock lock(processor.engineLock);processor.engine.setSelectedPad(selectedPad);padState=processor.engine.drumPad(selectedPad);}configureMacros();}
        else if(samplePreviewReady&&samplePlotBounds().contains(p)){std::array<float,4> markers{{mode==1?padState.start:processor.value("sampleStart"),processor.value("sampleLoopIn"),processor.value("sampleLoopOut"),mode==1?padState.end:processor.value("sampleEnd")}};float distance=11;draggingSampleMarker=-1;for(int i=0;i<4;++i){if(mode==1&&(i==1||i==2))continue;float x=samplePlotBounds().getX()+(float)((markers[(size_t)i]-sampleViewStart)/(sampleViewEnd-sampleViewStart))*samplePlotBounds().getWidth();float delta=std::abs(x-p.x);if(delta<distance){distance=delta;draggingSampleMarker=i;}}if(draggingSampleMarker>=0)moveSampleMarker(p.x);else status="Drag a colour marker; double-click waveform to zoom";}
    }
    else if(mode==3&&page==0&&p.y>screen.getBottom()-23){int ch=juce::jlimit(0,3,(int)((p.x-screen.getX())/62));const juce::ScopedLock lock(processor.engineLock);if(std::fmod(p.x-screen.getX(),62.f)<31)processor.engine.setTrackMute(ch,!processor.engine.trackMute(ch));else processor.engine.setTrackSolo(ch,!processor.engine.trackSolo(ch));}
    else if(mode==4){
        int seq=(int)processor.value("sequencer");
        if(seq==6){if(juce::Rectangle<float>(316,67,247,80).contains(p)){drawingSketch=true;sketchWriteIndex=0;appendSketchPoint(p);}return;}
        if(seq==0||seq==1||seq==4||seq==7)return;
        int col=(int)((p.x-screen.getX())/31),row=(int)((p.y-screen.getY()-23)/34);if(col>=0&&col<8&&row>=0&&row<2){int i=sequenceOffset+row*8+col;if(i>=sequenceLength)return;const juce::ScopedLock lock(processor.engineLock);int old=seq==3?processor.engine.fingerStep(fingerBank,i):processor.engine.sequenceStep(i);int note=(shifted||e.mods.isShiftDown())?(old<0?60:juce::jmin(127,old+1)):(old<0?60:-1);if(seq==3)processor.engine.setFingerStep(fingerBank,i,note);else processor.engine.setSequenceStep(i,note);}}

    timerCallback();repaint();
}
void FieldEditor::mouseDrag(const juce::MouseEvent& e){auto p=localPoint(e.position);if(draggingSampleMarker>=0){moveSampleMarker(p.x);return;}if(drawingSketch){appendSketchPoint(p);return;}if(draggingSelection){processor.selectionEnd=juce::jlimit(processor.selectionStart+.1,360.,processor.snapTapeTime((double)((p.x-331)/232*visibleSeconds)));repaint();return;}int n=noteAt(p);if(n!=mouseNote){if(mouseNote>=0)keyboardNote(mouseNote,0,false);mouseNote=n;if(n>=0)keyboardNote(n,.8f,true);}}
void FieldEditor::mouseUp(const juce::MouseEvent&){if(draggingSampleMarker>=0){draggingSampleMarker=-1;configureMacros();}if(drawingSketch){drawingSketch=false;status="Sketch saved: "+juce::String(sketchCount)+" points";}if(mouseNote>=0)keyboardNote(mouseNote,0,false);mouseNote=-1;if(draggingSelection){const juce::ScopedLock lock(processor.engineLock);processor.engine.setLoop(processor.selectionStart,processor.selectionEnd);draggingSelection=false;}}
void FieldEditor::mouseDoubleClick(const juce::MouseEvent& event){
    const auto point=localPoint(event.position);
    if((mode==0||mode==1)&&page==0&&(int)processor.value("engine")==11&&samplePreviewReady&&samplePlotBounds().contains(point)){
        draggingSampleMarker=-1;
        if(sampleViewEnd-sampleViewStart<.5){sampleViewStart=0;sampleViewEnd=1;status="Sample overview";}
        else{double centre=juce::jlimit(0.,1.,(double)((point.x-samplePlotBounds().getX())/samplePlotBounds().getWidth()));sampleViewStart=juce::jlimit(0.,.75,centre-.125);sampleViewEnd=sampleViewStart+.25;status="Sample detail x4 / double-click for overview";}
        timerCallback();repaint();
    }
}
void FieldEditor::timerCallback(){
    bool padChanged=false;

    ++tick;keyStateChanged(false);for(int n=0;n<128;++n)held[(size_t)n]=processor.keyboard.isNoteOnForChannels(0xffff,n);
    engineBox.setSelectedId((int)processor.value("engine")+1,juce::dontSendNotification);effectBox.setSelectedId((int)processor.value(mode==3?"masterEffect":"effect")+1,juce::dontSendNotification);sequenceBox.setSelectedId((int)processor.value("sequencer")+1,juce::dontSendNotification);
    int currentEngine=(int)processor.value("engine");
    if(currentEngine==14&&!processor.drumMode){if(lastEngine>=0&&lastEngine<14)lastSynthEngine=lastEngine;processor.drumMode=true;if(mode==0)mode=1;}
    else if(currentEngine!=14&&lastEngine==14&&currentEngine!=11){processor.drumMode=false;if(mode==1)mode=0;}
    if(currentEngine<14&&!processor.drumMode)lastSynthEngine=currentEngine;
    if(lastSequencer!=(int)processor.value("sequencer")){lastSequencer=(int)processor.value("sequencer");sequenceOffset=0;configureMacros();}
    if(lastElementSource!=(int)processor.value("elementSource")||lastLfoSync!=(int)processor.value("lfoSync")||lastLfo!=(int)processor.value("lfo")||lastVoiceMode!=(int)processor.value("voiceMode")||lastEngine!=(int)processor.value("engine")||lastEffect!=(int)processor.value("effect")||lastMasterEffect!=(int)processor.value("masterEffect"))configureMacros();
    {const juce::ScopedTryLock lock(processor.engineLock);if(lock.isLocked()){
        lfoPhaseSnapshot=(float)processor.engine.lfoPhasePosition();modulationSnapshot=processor.engine.modulationAt((int)processor.value("lfoTarget"));
        modulationHistory[modulationWrite++%modulationHistory.size()]=modulationSnapshot;
        if((int)processor.value("engine")==11){
            const auto& left=processor.engine.sampleLeft();const auto& right=processor.engine.sampleRight();
            if(sampleFrames!=left.size()){sampleViewStart=0;sampleViewEnd=1;sampleFrames=left.size();}
            samplePreviewReady=!left.empty();sampleSeconds=left.size()/juce::jmax(1.,processor.engine.loadedSampleRate());
            if(samplePreviewReady)for(size_t channel=0;channel<2;++channel){const auto& source=channel==1&&!right.empty()?right:left;for(size_t bin=0;bin<256;++bin){float low=0,high=0;for(int tap=0;tap<8;++tap){double u=sampleViewStart+(sampleViewEnd-sampleViewStart)*(bin+(tap+.5)/8.)/256.;size_t index=std::min(source.size()-1,(size_t)(juce::jlimit(0.,1.,u)*(source.size()-1)));low=std::min(low,source[index]);high=std::max(high,source[index]);}samplePreview[channel][bin]={low,high};}}
        }
        if(tick%15==0){albumDurations[0]=processor.albumDuration(0);albumDurations[1]=processor.albumDuration(1);}
        sequencerHeld.fill(false);for(int i=0;i<processor.engine.heldNoteCount();++i){int note=processor.engine.heldNote(i);if(note>=0&&note<128)sequencerHeld[(size_t)note]=true;}if(selectedPad!=processor.engine.selectedPad()){selectedPad=processor.engine.selectedPad();padChanged=true;}padState=processor.engine.drumPad(selectedPad);dboxState=processor.engine.dboxPad(selectedPad);playing=processor.engine.playing();recording=processor.engine.recording();countInRemaining=processor.engine.countInRemainingBeats();selectedTrack=processor.engine.selectedTrack();recorded=processor.engine.recordedSeconds();if((int)processor.value("sequencer")==3){int selected=processor.engine.fingerPattern();if(fingerBank!=selected)sequenceOffset=0;fingerBank=selected;sequenceLength=processor.engine.fingerLength(fingerBank);}else sequenceLength=processor.engine.sequenceLength();sequenceOffset=juce::jmin(sequenceOffset,(sequenceLength-1)/16*16);visibleSeconds=juce::jmax(16.,std::ceil(juce::jmax(recorded,processor.playhead.load()+2)/8)*8);sequenceStep=processor.engine.currentSequenceStep();for(int i=0;i<4;++i){trackDurations[(size_t)i]=processor.engine.tracks()[(size_t)i].left.size()/processor.engine.sampleRate();levels[(size_t)i]=processor.engine.trackLevel(i);pans[(size_t)i]=processor.engine.trackPan(i);mutes[(size_t)i]=processor.engine.trackMute(i);solos[(size_t)i]=processor.engine.trackSolo(i);if(mode==3&&page==0&&!macros[(size_t)i].isMouseButtonDown())macros[(size_t)i].setValue(shifted?pans[(size_t)i]:levels[(size_t)i],juce::dontSendNotification);if(tick%15==0||mode==2&&recording){waves[(size_t)i]=processor.engine.waveform(i,400);if(mode==2){const auto& cuts=processor.engine.tracks()[(size_t)i].cuts;trackCuts[(size_t)i].assign(cuts.begin(),cuts.begin()+std::min<size_t>(cuts.size(),4096));}}}for(int i=0;i<16;++i)steps[(size_t)i]=(int)processor.value("sequencer")==3?processor.engine.fingerStep(fingerBank,sequenceOffset+i):processor.engine.sequenceStep(sequenceOffset+i);if(mode==4){sketchCount=processor.engine.sketchLength();for(int i=0;i<sketchCount;++i)sketchPoints[(size_t)i]=processor.engine.sketchPoint(i);balls=processor.engine.tombolaBalls();tombolaRotation=(float)processor.engine.tombolaRotation();tombolaSides=processor.engine.tombolaSides();tombolaTriggers=processor.engine.tombolaTriggerCount();}}}
    if(mode==2){if(!macros[0].isMouseButtonDown())macros[0].setValue(shifted?processor.selectionStart:processor.playhead.load(),juce::dontSendNotification);if(!macros[1].isMouseButtonDown())macros[1].setValue(shifted?processor.selectionStart:processor.selectionEnd,juce::dontSendNotification);if(!macros[2].isMouseButtonDown())macros[2].setValue(processor.value("tapeSpeed")*(shifted?100:1),juce::dontSendNotification);}
    if(padChanged&&mode==1&&(((int)processor.value("engine")==11&&page==0)||((int)processor.value("engine")==14&&page<2)))configureMacros();
    for(int i=0;i<4;++i)functionButtons[i]->setToggleState(mode==i,juce::dontSendNotification);for(int i=4;i<8;++i)functionButtons[i]->setToggleState(mode==2?selectedTrack==i-4:page==i-4,juce::dontSendNotification);
    functionButtons[10]->setToggleState(processor.value("loop")>.5f,juce::dontSendNotification);functionButtons[11]->setToggleState(mode==4,juce::dontSendNotification);functionButtons[13]->setToggleState(processor.metronome,juce::dontSendNotification);inputButton.setToggleState(processor.inputMonitor,juce::dontSendNotification);tempoButton.setToggleState(processor.metronome,juce::dontSendNotification);transportButtons[3]->setToggleState(recording,juce::dontSendNotification);transportButtons[4]->setToggleState(playing,juce::dontSendNotification);
    transportButtons[3]->setButtonText(countInRemaining>0?"ARM":processor.sampling?"SAMPLE":juce::String::fromUTF8("●"));
    updateControlAvailability();
    repaint();
}
