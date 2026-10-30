#include "PluginEditor.h"
#include <cmath>

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
        {{"LOW FREQUENCY","FILTER FOLLOW","FEEDBACK","HIGH FREQUENCY"}},{{"TONE","GSM","BAUD","TELEMATIC"}},
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
    const auto r=juce::Rectangle<float>((float)x,(float)y,(float)w,(float)h).reduced((float)juce::jmin(w,h)*.12f);const float d=juce::jmin(r.getWidth(),r.getHeight());auto circle=juce::Rectangle<float>(d,d).withCentre(r.getCentre());
    g.setColour(juce::Colour(0xff89918e));g.fillEllipse(circle.expanded(1));g.setGradientFill(juce::ColourGradient(juce::Colour(0xfff7f8f2),circle.getX(),circle.getY(),juce::Colour(0xff969f9a),circle.getRight(),circle.getBottom(),false));g.fillEllipse(circle);
    g.setColour(juce::Colours::black.withAlpha(.18f));g.drawEllipse(circle.reduced(d*.12f),1.5f);auto core=circle.reduced(d*.26f);
    const auto c=s.findColour(juce::Slider::thumbColourId);g.setGradientFill(juce::ColourGradient(c.darker(.48f),core.getX(),core.getY(),c,core.getRight(),core.getBottom(),false));g.fillEllipse(core);
    g.setColour(juce::Colours::white.withAlpha(.6f));g.drawEllipse(core,1);
    const float angle=start+pos*(end-start);juce::Path pointer;pointer.addRoundedRectangle(-1.4f,-d*.42f,2.8f,d*.15f,1);g.setColour(ink.withAlpha(.8f));g.fillPath(pointer,juce::AffineTransform::rotation(angle).translated(circle.getCentreX(),circle.getCentreY()));
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
        else if(mode==3&&page==3){ids={"volume","compressor","masterMix","tempo"};macroLabels={"MASTER LEVEL","COMPRESSOR","EFFECT MIX","TEMPO"};}
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
    for(int i=0;i<4;++i){bool external=processor.hostSync&&macroLabels[i].contains("TEMPO");bool unavailableManual=page==3&&macroLabels[i]=="MANUAL INPUT"&&processor.value("elementSource")!=3;bool unavailableRate=page==3&&macroLabels[i]=="LFO RATE"&&processor.value("lfoSync")>.5f;bool continuousAmp=mode==0&&page==1&&processor.value("engine")==13;bool inactive=unavailableManual||unavailableRate||(continuousAmp&&(!shifted||i<3))||(mode==4&&processor.value("sequencer")==0)||(macroLabels[i].contains("GLIDE")&&(processor.value("voiceMode")==0||processor.value("voiceMode")==3));macros[(size_t)i].setEnabled(!external&&!inactive);macros[(size_t)i].setTooltip(inactive?(unavailableManual?"Select the manual Element source first":unavailableRate?"Cycle rate follows tempo and the selected sync division":continuousAmp?"Amp processes continuous input; Shift exposes patch level":mode==4?"Choose a sequencer first":"Portamento applies to overlapping mono/legato notes"):external?"External DAW tempo":macroLabels[i]+(macroLabels[i]=="MASTER LEVEL"?juce::String::fromUTF8(" · 1 = unity, 2 = +6 dB boost"):juce::String::fromUTF8(" · drag vertically, double click to reset")));}
    lastVoiceMode=(int)processor.value("voiceMode");lastEngine=(int)processor.value("engine");lastEffect=(int)processor.value("effect");lastMasterEffect=(int)processor.value("masterEffect");lastLfo=(int)processor.value("lfo");lastElementSource=(int)processor.value("elementSource");lastLfoSync=(int)processor.value("lfoSync");
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
        for(int i=0;i<4;++i){bool externalTempo=processor.hostSync&&macroLabels[i].contains("TEMPO");drawCap(g,{577+i*135.f,38,134,134},false);text(g,externalTempo?"HOST TEMPO":macroLabels[i],{581+i*135.f,43,126,12},9,ink);text(g,juce::String(externalTempo?processor.effectiveTempo.load():macros[(size_t)i].getValue(),macroLabels[i].contains("TEMPO")?0:(macroLabels[i].contains("ATTACK")||macroLabels[i].contains("RELEASE")||macroLabels[i].contains("ENVELOPE"))?3:2),{581+i*135.f,154,126,12},10,ink);}
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
    juce::String displayStatus=status;int albumRec=processor.albumRecording.load(),albumPlay=processor.albumPlaying.load();if(albumRec>=0)displayStatus="ALBUM "+juce::String(albumRec==0?"A":"B")+" REC";else if(albumPlay>=0)displayStatus="ALBUM "+juce::String(albumPlay==0?"A":"B")+" LISTEN";
    auto bottom=(float)getHeight()-31;text(g,processor.sampling?juce::String::fromUTF8("● ")+displayStatus:displayStatus,{22,bottom,(float)getWidth()-540,21},11,processor.sampling?colours[3]:juce::Colour(0xffacbbc0),juce::Justification::centredLeft);
    text(g,"TAPE "+juce::String(processor.tapeNumber()+1)+" / 8     "+seconds(processor.playhead.load())+"     "+(processor.hostSync?"HOST "+juce::String(processor.effectiveTempo.load(),0):juce::String(processor.value("tempo"),0))+" BPM     OCT "+juce::String(processor.octave),{(float)getWidth()-515,bottom,490,21},11,juce::Colour(0xffa2b7b9),juce::Justification::centredRight);
}
void FieldEditor::drawScreen(juce::Graphics& g){
    juce::Rectangle<float> r(304,38,271,134);g.setColour(juce::Colour(0xff3d4949));g.fillRoundedRectangle(r.expanded(1),3);g.setColour(screenBlack);g.fillRoundedRectangle(r,2);g.saveState();g.reduceClipRegion(r.toNearestInt());
    auto content=r.reduced(12,9);if(mode==2){drawTape(g,content);g.restoreState();return;}if(mode==3){drawMixer(g,content);g.restoreState();return;}if(mode==4){drawSequence(g,content);g.restoreState();return;}
    juce::String title=mode==1?juce::String::fromUTF8("DRUM · PAD ")+juce::String(selectedPad+1):FieldProcessor::engineNames[juce::jlimit(0,14,(int)processor.value("engine"))].toUpperCase();
    if(page==1)title=mode==1&&(int)processor.value("engine")==14?"DYNAMIC ENVELOPE":"ENVELOPE";else if(page==2)title=FieldProcessor::effectNames[juce::jlimit(0,FieldProcessor::effectNames.size()-1,(int)processor.value("effect"))].toUpperCase();else if(page==3)title=FieldProcessor::lfoNames[juce::jlimit(0,5,(int)processor.value("lfo"))].toUpperCase()+" LFO";
    text(g,title,content.withHeight(16),11,colours[(size_t)page],juce::Justification::centredLeft);text(g,"T"+juce::String(page+1),content.withHeight(16),9,juce::Colour(0xff748c90),juce::Justification::centredRight);
    auto area=content.withTrimmedTop(22).withTrimmedBottom(16);
    if(page==1&&mode==0&&processor.value("engine")==13){text(g,"CONTINUOUS INPUT ENGINE",area.withTrimmedTop(10).withTrimmedBottom(20),13,colours[1]);text(g,"SHIFT: PATCH LEVEL",area.withY(area.getBottom()-12).withHeight(12),8,colours[2]);}
    else if(page==1&&shifted&&mode==0){text(g,"PLAY SETTINGS",area.withHeight(15),10,colours[0]);text(g,juce::StringArray{"POLY","MONO","LEGATO","UNISON"}[(int)processor.value("voiceMode")],area.withTrimmedTop(17).withTrimmedBottom(16),22,colours[1]);text(g,"GLIDE "+juce::String(processor.value("glideTime"),2)+"s    BEND +/-"+juce::String(processor.value("bendRange"),0)+"st",area.withY(area.getBottom()-12).withHeight(12),8,colours[2]);}
    else if(page==0)drawEngine(g,area);
    else if(page==1){
        float a=processor.value("attack"),d=processor.value("decay"),s=processor.value("sustain"),rr=processor.value("release");if(mode==1&&(int)processor.value("engine")==14){a=dboxState.attack;d=dboxState.release;s=.15f;rr=dboxState.release;}float x=area.getX(),y=area.getBottom(),w=area.getWidth(),h=area.getHeight();float ax=w*(.07f+.14f*a/4),dx=w*(.1f+.13f*d/4),rx=w*(.12f+.14f*rr/8);std::array<juce::Point<float>,5> pts{{{x,y},{x+ax,y-h},{x+ax+dx,y-h*s},{x+w-rx,y-h*s},{x+w,y}}};
        juce::Path path;path.startNewSubPath(pts[0]);for(size_t i=1;i<pts.size();++i){for(int step=1;step<=24;++step){float t=step/24.f;float shaped=processor.value("envelopeCurve")>.5f&&!(mode==1&&processor.value("engine")==14)?(1-std::exp(-5*t))/(1-std::exp(-5.f)):t;path.lineTo(pts[i-1].x+(pts[i].x-pts[i-1].x)*t,pts[i-1].y+(pts[i].y-pts[i-1].y)*shaped);}}g.setColour(colours[1]);g.strokePath(path,juce::PathStrokeType(1.5));for(int i=0;i<4;++i){g.setColour(colours[(size_t)i]);g.fillEllipse(pts[(size_t)i+1].x-3,pts[(size_t)i+1].y-3,6,6);}
    }else if(page==2){
        int effect=(int)processor.value("effect");
        if(effect==7){
            // An original geometric CWO cow illustration, drawn without source assets.
            float x=area.getX()+61,y=area.getY()+8;g.setColour(colours[2]);g.drawRoundedRectangle({x+19,y+10,99,37},13,1.2f);g.drawEllipse(x,y,36,29,1.2f);g.drawLine(x+6,y+2,x+1,y-6,1.2f);g.drawLine(x+27,y+1,x+32,y-6,1.2f);g.drawLine(x+117,y+16,x+131,y+2,1.2f);
            for(int i=0;i<4;++i){float lx=x+34+i*23;g.drawLine(lx,y+45,lx-3,y+62,1.2f);g.drawLine(lx-3,y+62,lx+2,y+62,1.2f);}g.setColour(colours[0]);g.fillEllipse(x+8,y+11,3,3);g.fillEllipse(x+25,y+11,3,3);g.setColour(colours[3]);g.drawEllipse(x+8,y+20,19,8,1);
            for(int i=0;i<3;++i){g.setColour(colours[(size_t)i].withAlpha(.7f));g.drawEllipse(x+40+i*23,y+17,14+std::sin(tick*.03f+i)*4,21,1.2f);}g.setColour(colours[1]);g.drawLine(area.getX(),area.getBottom(),area.getRight(),area.getBottom(),.5f);
        }else for(int i=0;i<4;++i){auto col=colours[(size_t)i];g.setColour(col.withAlpha(.8f));juce::Path path;float x0=area.getX()+i*18.f;for(int j=0;j<140;++j){float u=j/139.f;float y=area.getCentreY()+std::sin(u*(effect==2?60:effect==3?24:12)+i*.7f+tick*.035f)*area.getHeight()*.35f*(1-u*.65f);float x=x0+u*(area.getWidth()-60);if(j==0)path.startNewSubPath(x,y);else path.lineTo(x,y);}g.strokePath(path,juce::PathStrokeType(1.1f));}
        if(effect==0)text(g,"INSERT OFF",area,12,colours[2]);
    }else{
        int l=(int)processor.value("lfo");juce::Path path;for(int i=0;i<180;++i){float u=i/179.f,phase=u*juce::MathConstants<float>::twoPi*2+tick*.025f;float v=l==0?std::sin(std::floor(u*12)*13.71f):l==5?u*.85f:l==1?std::sin(phase)*std::sin(phase*.29f):std::sin(phase);float x=area.getX()+u*area.getWidth(),y=area.getCentreY()-v*area.getHeight()*.4f;if(i==0)path.startNewSubPath(x,y);else path.lineTo(x,y);}g.setColour(colours[3]);g.strokePath(path,juce::PathStrokeType(1.4f));
    }
    text(g,page==0?(mode==1?juce::String::fromUTF8("24 VOICES · CHROMATIC PADS"):juce::String::fromUTF8("STEREO · ")+juce::String(processor.value("macro1")*100,0)+" / "+juce::String(processor.value("macro2")*100,0)):page==1?(mode==1&&(int)processor.value("engine")==14?"ATTACK / GAIN / RELEASE / SMOOTH":"A   /   D   /   S   /   R"):page==2?"MIX "+juce::String(processor.value("effectMix")*100,0)+"%":"RATE "+juce::String(processor.value("lfoRate"),2)+" Hz     DEPTH "+juce::String(processor.value("lfoDepth")*100,0)+"%",content.withY(content.getBottom()-13).withHeight(13),8,juce::Colour(0xff829a9c),juce::Justification::centredLeft);
    g.restoreState();
}
void FieldEditor::drawEngine(juce::Graphics& g,juce::Rectangle<float> a){
    const int e=(int)processor.value("engine");const float phase=tick*.025f,m=processor.value("macro1"),n=processor.value("macro2");
    if(mode==1&&e==14){
        for(int osc=0;osc<2;++osc){auto r=juce::Rectangle<float>(a.getX()+osc*126,a.getY()+3,115,43);auto c=colours[(size_t)osc];g.setColour(c.withAlpha(.45f));g.drawRoundedRectangle(r,2,.7f);float shape=osc==0?dboxState.wave1:dboxState.wave2;float envelope=osc==0?dboxState.envelope1:dboxState.envelope2;
            juce::Path line;for(int i=0;i<100;++i){float u=i/99.f,ph=u*3.f+phase*.1f;float sine=std::sin(ph*juce::MathConstants<float>::twoPi),saw=2*std::fmod(ph,1.f)-1;float v=sine*(1-shape)+saw*shape;float x=r.getX()+u*r.getWidth(),y=r.getCentreY()-v*13*std::exp(-u/(.1f+envelope));if(i==0)line.startNewSubPath(x,y);else line.lineTo(x,y);}g.setColour(c);g.strokePath(line,juce::PathStrokeType(1));text(g,"OSC "+juce::String(osc+1),{r.getX()+4,r.getY()+2,40,8},6,c,juce::Justification::centredLeft);
        }
        for(int i=0;i<24;++i){auto r=juce::Rectangle<float>(a.getX()+i*10.25f,a.getBottom()-16,8.5f,13);auto c=colours[(size_t)((i/6)%4)];g.setColour(c.withAlpha(i==selectedPad?.8f:.15f));g.fillRoundedRectangle(r,1);g.setColour(c);g.drawRoundedRectangle(r,1,.5f);}return;
    }
    if(mode==1){for(int i=0;i<24;++i){float x=a.getX()+(i%12)*20.f,y=a.getY()+(i/12)*32.f;auto c=colours[(size_t)(i%4)];g.setColour(c.withAlpha(i==selectedPad?.4f:.05f));g.fillRoundedRectangle({x,y,16,26},2);g.setColour(c.withAlpha(.6f));g.drawRoundedRectangle({x,y,16,26},2,1);text(g,juce::String(i+1),{x,y,16,26},8,c);}return;}
    if(e==1){for(int i=0;i<4;++i){float x=a.getX()+25+i*61.f,y=a.getCentreY()+std::sin(i+phase)*9;g.setColour(colours[(size_t)i]);g.drawEllipse(x-17,y-17,34,34,1.3f);text(g,juce::String(i+1),{x-16,y-16,32,32},12,colours[(size_t)i]);if(i<3)g.drawLine(x+18,y,x+44,y+std::sin(i+1+phase)*9,1);}return;}
    if(e==9){for(int i=0;i<52;++i){float u=i/51.f,x=a.getX()+u*a.getWidth(),v=std::sin(u*12+phase)*a.getHeight()*.4f;g.setColour(colours[0]);g.fillEllipse(x-1,a.getCentreY()+v-1,2,2);g.setColour(colours[3]);g.fillEllipse(x-1,a.getCentreY()-v-1,2,2);if(i%4==0){g.setColour(colours[2].withAlpha(.4f));g.drawLine(x,a.getCentreY()-v,x,a.getCentreY()+v,.8f);}}return;}
    if(e==5){for(int i=0;i<5;++i){float rad=15+i*6.f;g.setColour(colours[(size_t)(i%4)].withAlpha(.7f));g.drawEllipse(a.getCentreX()-rad*2+std::sin(phase+i)*n*12,a.getCentreY()-rad*.7f,rad*4,rad*1.4f,1);}return;}
    if(e==6){for(int k=0;k<6;++k){juce::Path string;for(int i=0;i<180;++i){float u=i/179.f,x=a.getX()+u*a.getWidth(),y=a.getY()+k*11+std::sin(u*juce::MathConstants<float>::pi)*std::sin(u*(8+k*3)+phase*2)*5*(.2f+m);if(i==0)string.startNewSubPath(x,y);else string.lineTo(x,y);}g.setColour(colours[(size_t)(k%4)]);g.strokePath(string,juce::PathStrokeType(.8f));}return;}
    if(e==10){for(int k=0;k<2;++k){auto r=juce::Rectangle<float>(a.getX()+k*126,a.getY()+5,114,a.getHeight()-10);g.setColour(colours[(size_t)k].withAlpha(.5f));g.drawRoundedRectangle(r,3,.8f);juce::Path wave;for(int i=0;i<100;++i){float u=i/99.f,x=r.getX()+u*r.getWidth(),y=r.getCentreY()+std::sin(u*(10+k*6)+phase*(k+1))*r.getHeight()*.35f;if(i==0)wave.startNewSubPath(x,y);else wave.lineTo(x,y);}g.setColour(colours[(size_t)k]);g.strokePath(wave,juce::PathStrokeType(1.2f));}return;}
    if(e==12||e==13){for(int i=0;i<22;++i){float h=(.2f+.8f*std::abs(std::sin(i*.73f+phase)*std::cos(i*.21f-phase)))*a.getHeight();g.setColour(colours[(size_t)(i%4)]);g.fillRoundedRectangle(a.getX()+i*11.f,a.getBottom()-h,7,h,1);}return;}
    if(e==11){const juce::ScopedTryLock lock(processor.engineLock);if(lock.isLocked()){auto& sample=processor.engine.sampleLeft();if(!sample.empty()){juce::Path line;for(int i=0;i<230;++i){size_t s=juce::jmin(sample.size()-1,(size_t)(i*(sample.size()/230.)));float x=a.getX()+i/229.f*a.getWidth(),y=a.getCentreY()-sample[s]*a.getHeight()*.45f;if(i==0)line.startNewSubPath(x,y);else line.lineTo(x,y);}g.setColour(colours[0]);g.strokePath(line,juce::PathStrokeType(1));return;}}text(g,"IMPORT AUDIO TO BEGIN",a,11,colours[2]);return;}
    const int count=e==2?7:e==6?5:e==10?2:1;
    for(int k=0;k<count;++k){juce::Path path;for(int i=0;i<240;++i){float u=i/239.f,v=std::sin(u*(10+m*34)+phase+k*.35f);if(e==3)v=std::round(v*4)/4;else if(e==7)v=v>n?1.f:-1.f;else if(e==8)v=std::sin(u*20+std::sin(u*7+phase)*m*8);else if(e==4)v=std::round(std::sin(u*(12+m*45)+phase)*6)/6;else if(e==0)v=(2*std::fmod(u*(2+m*4)+phase*.1f,1.f)-1)*.7f;float x=a.getX()+u*a.getWidth(),y=a.getCentreY()-v*a.getHeight()*(count>1?.27f:.4f)+(k-(count-1)*.5f)*4;if(i==0)path.startNewSubPath(x,y);else path.lineTo(x,y);}g.setColour(colours[(size_t)(k%4)].withAlpha(count>1?.7f:1.f));g.strokePath(path,juce::PathStrokeType(1.3f));}
}
void FieldEditor::drawTape(juce::Graphics& g,juce::Rectangle<float> a){
    text(g,"TAPE "+juce::String(processor.tapeNumber()+1)+juce::String::fromUTF8(" · ")+juce::StringArray{"STUDIO","VINTAGE","PORTA","DISC MINI"}[(int)processor.value("tapeStyle")],a.withHeight(13),9,colours[1],juce::Justification::centredLeft);
    text(g,countInRemaining>0?"COUNT "+juce::String((int)std::ceil(countInRemaining)):recording?juce::String::fromUTF8("● REC"):playing?juce::String::fromUTF8("▶ PLAY"):"STOP",a.withHeight(13),8,recording?colours[3]:colours[2],juce::Justification::centredRight);
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
    if(page==2){text(g,"MASTER INSERT",a.withHeight(13),9,colours[3],juce::Justification::centredLeft);text(g,FieldProcessor::effectNames[(int)processor.value("masterEffect")].toUpperCase(),a.withTrimmedTop(25).withTrimmedBottom(30),23,colours[3]);text(g,"MIX "+juce::String(processor.value("masterMix")*100,0)+"%       COMP "+juce::String(processor.value("compressor")*100,0)+"%",a.withY(a.getBottom()-15).withHeight(15),8,colours[2]);return;}
    if(page==3){text(g,"MASTER DYNAMICS",a.withHeight(13),9,colours[0],juce::Justification::centredLeft);float peak=juce::jlimit(0.f,1.f,processor.outputPeak.load());g.setColour(colours[0].withAlpha(.15f));g.fillRect(a.getX(),a.getCentreY()-9,a.getWidth(),18.f);g.setColour(colours[0]);g.fillRect(a.getX(),a.getCentreY()-9,a.getWidth()*peak,18.f);text(g,"COMPRESSOR "+juce::String(processor.value("compressor")*100,0)+"%     "+(processor.hostSync?"HOST "+juce::String(processor.effectiveTempo.load(),0):juce::String(processor.value("tempo"),0))+" BPM",a.withY(a.getBottom()-16).withHeight(16),8,colours[2]);return;}
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
        if(id==500)safe->processor.tapeEditMask=0;
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
void FieldEditor::keyboardNote(int n,float velocity,bool on){if(n<0||n>127)return;held[(size_t)n]=on;if(on)processor.keyboard.noteOn(1,n,velocity);else processor.keyboard.noteOff(1,n,0);repaint();}
void FieldEditor::panic(){processor.releaseTapePerformance();drawingSketch=false;typingNotes.clear();held.fill(false);mouseNote=-1;processor.transport("panic");}
bool FieldEditor::keyPressed(const juce::KeyPress& k){
    auto code=k.getKeyCode();if(code==juce::KeyPress::escapeKey){panic();return true;}if(code==juce::KeyPress::spaceKey){processor.transport("play");return true;}
    if(code=='-'||code=='='||code=='+'){panic();processor.octave=juce::jlimit(-3,3,processor.octave+(code=='-'?-1:1));status="Keyboard octave "+juce::String(processor.octave);return true;}
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
    else if(mode==1&&page==0&&(int)processor.value("engine")==11){int col=(int)((p.x-screen.getX())/20),row=(int)((p.y-screen.getY()-22)/32);if(col>=0&&col<12&&row>=0&&row<2){const juce::ScopedLock lock(processor.engineLock);processor.engine.setSelectedPad(row*12+col);selectedPad=row*12+col;padState=processor.engine.drumPad(selectedPad);configureMacros();}}
    else if(mode==3&&page==0&&p.y>screen.getBottom()-23){int ch=juce::jlimit(0,3,(int)((p.x-screen.getX())/62));const juce::ScopedLock lock(processor.engineLock);if(std::fmod(p.x-screen.getX(),62.f)<31)processor.engine.setTrackMute(ch,!processor.engine.trackMute(ch));else processor.engine.setTrackSolo(ch,!processor.engine.trackSolo(ch));}
    else if(mode==4){
        int seq=(int)processor.value("sequencer");
        if(seq==6){if(juce::Rectangle<float>(316,67,247,80).contains(p)){drawingSketch=true;sketchWriteIndex=0;appendSketchPoint(p);}return;}
        if(seq==0||seq==1||seq==4||seq==7)return;
        int col=(int)((p.x-screen.getX())/31),row=(int)((p.y-screen.getY()-23)/34);if(col>=0&&col<8&&row>=0&&row<2){int i=sequenceOffset+row*8+col;if(i>=sequenceLength)return;const juce::ScopedLock lock(processor.engineLock);int old=seq==3?processor.engine.fingerStep(fingerBank,i):processor.engine.sequenceStep(i);int note=(shifted||e.mods.isShiftDown())?(old<0?60:juce::jmin(127,old+1)):(old<0?60:-1);if(seq==3)processor.engine.setFingerStep(fingerBank,i,note);else processor.engine.setSequenceStep(i,note);}}

    timerCallback();repaint();
}
void FieldEditor::mouseDrag(const juce::MouseEvent& e){auto p=localPoint(e.position);if(drawingSketch){appendSketchPoint(p);return;}if(draggingSelection){processor.selectionEnd=juce::jlimit(processor.selectionStart+.1,360.,processor.snapTapeTime((double)((p.x-331)/232*visibleSeconds)));repaint();return;}int n=noteAt(p);if(n!=mouseNote){if(mouseNote>=0)keyboardNote(mouseNote,0,false);mouseNote=n;if(n>=0)keyboardNote(n,.8f,true);}}
void FieldEditor::mouseUp(const juce::MouseEvent&){if(drawingSketch){drawingSketch=false;status="Sketch saved: "+juce::String(sketchCount)+" points";}if(mouseNote>=0)keyboardNote(mouseNote,0,false);mouseNote=-1;if(draggingSelection){const juce::ScopedLock lock(processor.engineLock);processor.engine.setLoop(processor.selectionStart,processor.selectionEnd);draggingSelection=false;}}
void FieldEditor::timerCallback(){
    bool padChanged=false;
    tempo.setEnabled(!processor.hostSync);tempo.setTooltip(processor.hostSync?"External DAW tempo":juce::String::fromUTF8("Tempo · beats per minute"));for(int i=0;i<4;++i)macros[(size_t)i].setEnabled(!(processor.hostSync&&macroLabels[i].contains("TEMPO"))&&!(mode==4&&processor.value("sequencer")==0));
    ++tick;keyStateChanged(false);for(int n=0;n<128;++n)held[(size_t)n]=processor.keyboard.isNoteOnForChannels(0xffff,n);
    engineBox.setSelectedId((int)processor.value("engine")+1,juce::dontSendNotification);effectBox.setSelectedId((int)processor.value(mode==3?"masterEffect":"effect")+1,juce::dontSendNotification);sequenceBox.setSelectedId((int)processor.value("sequencer")+1,juce::dontSendNotification);
    int currentEngine=(int)processor.value("engine");
    if(currentEngine==14&&!processor.drumMode){if(lastEngine>=0&&lastEngine<14)lastSynthEngine=lastEngine;processor.drumMode=true;if(mode==0)mode=1;}
    else if(currentEngine!=14&&lastEngine==14&&currentEngine!=11){processor.drumMode=false;if(mode==1)mode=0;}
    if(currentEngine<14&&!processor.drumMode)lastSynthEngine=currentEngine;
    if(lastSequencer!=(int)processor.value("sequencer")){lastSequencer=(int)processor.value("sequencer");sequenceOffset=0;configureMacros();}
    if(lastElementSource!=(int)processor.value("elementSource")||lastLfoSync!=(int)processor.value("lfoSync")||lastLfo!=(int)processor.value("lfo")||lastVoiceMode!=(int)processor.value("voiceMode")||lastEngine!=(int)processor.value("engine")||lastEffect!=(int)processor.value("effect")||lastMasterEffect!=(int)processor.value("masterEffect"))configureMacros();
    {const juce::ScopedTryLock lock(processor.engineLock);if(lock.isLocked()){sequencerHeld.fill(false);for(int i=0;i<processor.engine.heldNoteCount();++i){int note=processor.engine.heldNote(i);if(note>=0&&note<128)sequencerHeld[(size_t)note]=true;}if(selectedPad!=processor.engine.selectedPad()){selectedPad=processor.engine.selectedPad();padChanged=true;}padState=processor.engine.drumPad(selectedPad);dboxState=processor.engine.dboxPad(selectedPad);playing=processor.engine.playing();recording=processor.engine.recording();countInRemaining=processor.engine.countInRemainingBeats();selectedTrack=processor.engine.selectedTrack();recorded=processor.engine.recordedSeconds();if((int)processor.value("sequencer")==3){int selected=processor.engine.fingerPattern();if(fingerBank!=selected)sequenceOffset=0;fingerBank=selected;sequenceLength=processor.engine.fingerLength(fingerBank);}else sequenceLength=processor.engine.sequenceLength();sequenceOffset=juce::jmin(sequenceOffset,(sequenceLength-1)/16*16);visibleSeconds=juce::jmax(16.,std::ceil(juce::jmax(recorded,processor.playhead.load()+2)/8)*8);sequenceStep=processor.engine.currentSequenceStep();for(int i=0;i<4;++i){trackDurations[(size_t)i]=processor.engine.tracks()[(size_t)i].left.size()/processor.engine.sampleRate();levels[(size_t)i]=processor.engine.trackLevel(i);pans[(size_t)i]=processor.engine.trackPan(i);mutes[(size_t)i]=processor.engine.trackMute(i);solos[(size_t)i]=processor.engine.trackSolo(i);if(mode==3&&page==0&&!macros[(size_t)i].isMouseButtonDown())macros[(size_t)i].setValue(shifted?pans[(size_t)i]:levels[(size_t)i],juce::dontSendNotification);if(tick%15==0||mode==2&&recording){waves[(size_t)i]=processor.engine.waveform(i,400);if(mode==2){const auto& cuts=processor.engine.tracks()[(size_t)i].cuts;trackCuts[(size_t)i].assign(cuts.begin(),cuts.begin()+std::min<size_t>(cuts.size(),4096));}}}for(int i=0;i<16;++i)steps[(size_t)i]=(int)processor.value("sequencer")==3?processor.engine.fingerStep(fingerBank,sequenceOffset+i):processor.engine.sequenceStep(sequenceOffset+i);if(mode==4){sketchCount=processor.engine.sketchLength();for(int i=0;i<sketchCount;++i)sketchPoints[(size_t)i]=processor.engine.sketchPoint(i);balls=processor.engine.tombolaBalls();tombolaRotation=(float)processor.engine.tombolaRotation();tombolaSides=processor.engine.tombolaSides();tombolaTriggers=processor.engine.tombolaTriggerCount();}}}
    if(mode==2){if(!macros[0].isMouseButtonDown())macros[0].setValue(shifted?processor.selectionStart:processor.playhead.load(),juce::dontSendNotification);if(!macros[1].isMouseButtonDown())macros[1].setValue(shifted?processor.selectionStart:processor.selectionEnd,juce::dontSendNotification);if(!macros[2].isMouseButtonDown())macros[2].setValue(processor.value("tapeSpeed")*(shifted?100:1),juce::dontSendNotification);}
    if(padChanged&&mode==1&&(((int)processor.value("engine")==11&&page==0)||((int)processor.value("engine")==14&&page<2)))configureMacros();
    for(int i=0;i<4;++i)functionButtons[i]->setToggleState(mode==i,juce::dontSendNotification);for(int i=4;i<8;++i)functionButtons[i]->setToggleState(mode==2?selectedTrack==i-4:page==i-4,juce::dontSendNotification);
    functionButtons[10]->setToggleState(processor.value("loop")>.5f,juce::dontSendNotification);functionButtons[11]->setToggleState(mode==4,juce::dontSendNotification);functionButtons[13]->setToggleState(processor.metronome,juce::dontSendNotification);inputButton.setToggleState(processor.inputMonitor,juce::dontSendNotification);tempoButton.setToggleState(processor.metronome,juce::dontSendNotification);transportButtons[3]->setToggleState(recording,juce::dontSendNotification);transportButtons[4]->setToggleState(playing,juce::dontSendNotification);
    repaint();
}
