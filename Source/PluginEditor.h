#pragma once
#include "PluginProcessor.h"
#include <map>

class FieldLookAndFeel final : public juce::LookAndFeel_V4 {
public:
    FieldLookAndFeel();
    void drawRotarySlider(juce::Graphics&, int,int,int,int,float,float,float,juce::Slider&) override;
    void drawButtonBackground(juce::Graphics&,juce::Button&,const juce::Colour&,bool,bool) override;
};

class FieldEditor final : public juce::AudioProcessorEditor, private juce::Timer {
public:
    explicit FieldEditor(FieldProcessor&);
    ~FieldEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;
    bool keyPressed(const juce::KeyPress&) override;
    bool keyStateChanged(bool) override;
    void focusLost(FocusChangeType) override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;
    void mouseDoubleClick(const juce::MouseEvent&) override;
    void showPageForTesting(int mode,int page);
    void refreshForTesting() { timerCallback(); }
    juce::String macroMidiTargetForTesting(int i) const {return i>=0&&i<4?macros[(size_t)i].getProperties()["midiTarget"].toString():juce::String{};}
    bool macroEnabledForTesting(int i) const { return i>=0&&i<4&&macros[(size_t)i].isEnabled(); }
    void setShiftForTesting(bool value) { shiftLatched=value;setShift(value); }
private:
    FieldProcessor& processor;
    FieldLookAndFeel look;
    juce::ComboBox engineBox,effectBox,sequenceBox;
    juce::TextButton newButton{"New"},openButton{"Open"},saveButton{"Save"},importButton{"Import audio"},exportButton{"Export WAV"},helpButton{"?"};
    juce::OwnedArray<juce::TextButton> functionButtons,transportButtons;
    juce::TextButton inputButton{"INPUT"},tempoButton{"CLICK"},comButton{"COM"},shiftButton{"shift"};
    std::array<juce::Slider,4> macros;
    juce::Slider volume,tempo;
    std::array<juce::TextButton,3> tapePerformance;
    juce::TextButton tapeEditButton{"Tape edits"};
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>,4> attachments;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> volumeAttachment,tempoAttachment;
    std::unique_ptr<juce::FileChooser> chooser;
    juce::TooltipWindow tooltips{this,600};
    juce::Rectangle<float> hardware;
    std::array<juce::Rectangle<float>,14> whiteKeys;
    std::array<juce::Rectangle<float>,10> blackKeys;
    juce::StringArray macroLabels;
    int mode=0,page=0,tick=0,mouseNote=-1,lastElementSource=-1,lastLfoSync=-1,lastLfo=-1,lastVoiceMode=-1,lastEngine=-1,lastEffect=-1,lastMasterEffect=-1,lastSynthEngine=0;
    bool shifted=false,shiftLatched=false,draggingSelection=false,drawingSketch=false;
    int draggingSampleMarker=-1;
    double sampleViewStart=0,sampleViewEnd=1;
    struct SampleBin { float low=0,high=0; };
    std::array<std::array<SampleBin,256>,2> samplePreview{};
    size_t sampleFrames=0;
    double sampleSeconds=0;
    bool samplePreviewReady=false;
    float lfoPhaseSnapshot=0,modulationSnapshot=0;
    std::array<float,96> modulationHistory{};
    size_t modulationWrite=0;
    std::array<double,2> albumDurations{};
    std::map<int,int> typingNotes;
    std::array<bool,128> held{};
    std::array<bool,128> sequencerHeld{};
    std::array<std::vector<float>,4> waves;
    std::array<std::vector<double>,4> trackCuts;
    std::array<float,4> levels{{1,1,1,1}},pans{};
    std::array<double,4> trackDurations{};
    std::array<bool,4> mutes{},solos{};
    std::array<int,16> steps{};
    std::array<std::pair<float,float>,256> sketchPoints{};
    std::array<field::BallState,24> balls{};
    int sketchCount=0,sketchWriteIndex=0,fingerBank=0,lastSequencer=-1,tombolaSides=6;
    float tombolaRotation=0;
    uint64_t tombolaTriggers=0;
    int selectedTrack=0,sequenceStep=-1,sequenceOffset=0,sequenceLength=16,selectedPad=0;
    field::DrumPad padState;
    field::DBoxPad dboxState;
    bool playing=false,recording=false;
    uint64_t observedTapeFailures=0;
    bool observedCaptureExhaustion=false;
    double countInRemaining=0;
    double recorded=0,visibleSeconds=16;
    float scale=1;
    juce::String status=juce::String::fromUTF8("Ready · play the keys or connect MIDI");
    juce::String observedLearnTarget;
    void showMidiMappingMenu(const juce::String&);
    void controllerFileAction(bool save);
    void timerCallback() override;
    void setMode(int);
    void setPage(int);
    void configureMacros();
    void updateControlAvailability();
    void setShift(bool);
    void functionAction(int);
    void transportAction(int);
    void fileAction(int);
    void showHelp();
    void showMenu();
    void panic();
    void keyboardNote(int,float,bool);
    int noteAt(juce::Point<float>) const;
    juce::Point<float> localPoint(juce::Point<float>) const;
    juce::Rectangle<int> boundsFor(float,float,float,float) const;
    void drawCap(juce::Graphics&,juce::Rectangle<float>,bool,bool=false);
    void drawScreen(juce::Graphics&);
    void drawEngine(juce::Graphics&,juce::Rectangle<float>);
    void drawSampler(juce::Graphics&,juce::Rectangle<float>);
    void drawEffect(juce::Graphics&,juce::Rectangle<float>,bool master);
    void drawModulation(juce::Graphics&,juce::Rectangle<float>);
    juce::Rectangle<float> samplePlotBounds() const;
    void moveSampleMarker(float x);
    bool padIsHeld(int pad) const;
    void drawTape(juce::Graphics&,juce::Rectangle<float>);
    void drawMixer(juce::Graphics&,juce::Rectangle<float>);
    void drawSequence(juce::Graphics&,juce::Rectangle<float>);
    void appendSketchPoint(juce::Point<float>);
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FieldEditor)
};
