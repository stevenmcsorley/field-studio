#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "Engine.h"
#include "MidiSupport.h"
#include "RtKeyboard.h"
#include "RealtimeValues.h"
#include "MidiLearn.h"
#include <mutex>

class FieldProcessor final : public juce::AudioProcessor, private juce::Timer, private juce::AudioProcessorParameter::Listener {
public:
    struct SampleData {std::vector<float> left,right;double rate=44100;};
    std::shared_ptr<const SampleData> samplePreviewSnapshot(){const juce::ScopedLock lock(engineLock);return sampleData;}
    FieldProcessor();
    ~FieldProcessor() override;
    void prepareToPlay(double, int) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Field Studio"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
    double getTailLengthSeconds() const override { return 4; }
    int getNumPrograms() override { return 16; }
    int getCurrentProgram() override { return currentProgram+(drumMode?8:0); }
    void setCurrentProgram(int) override;
    const juce::String getProgramName(int) override;
    void changeProgramName(int,const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*,int) override;
    juce::AudioProcessorValueTreeState parameters;
    field::RtKeyboard keyboard;
    field::Engine engine;
    void startSampleCapture(bool fromOutput=false);
    void stopSampleCapture();
    void startAlbumRecording(int);
    bool playAlbum(int);
    void stopAlbum();
    void clearAlbum(int);
    double albumDuration(int) const;
    bool exportAlbum(int,const juce::File&,juce::String&);
    std::atomic<int> albumRecording{-1},albumPlaying{-1};
    std::atomic<bool> sampling{false};
    juce::var importedPresetMetadata;
    juce::CriticalSection engineLock;
    static const juce::StringArray engineNames, effectNames, sequencerNames, lfoNames, lfoTargetNames;
    float value(const juce::String&) const;
    void setValue(const juce::String&,float);
    field::Parameters currentParameters() const;
    void updateEngine();
    juce::StringArray midiMappingTargets() const;
    juce::String midiMappingLabel(const juce::String&) const;
    juce::String midiMappingDescription(const juce::String&) const;
    juce::String midiLearnTarget() const;
    bool beginMidiLearn(const juce::String&,field::EncoderMode,bool pickup=true,bool invert=false);
    void cancelMidiLearn(){midiLearn.cancel();}
    void clearMidiMapping(const juce::String&);
    void clearMidiMappings(){midiLearn.clearAll();}
    juce::ValueTree midiMappingState() const;
    bool restoreMidiMappingState(const juce::ValueTree&,juce::String& error);
    void synchronizeParameterDisplay(){flushParameterMirrors();} // message thread only
    bool loadSampleFile(const juce::File&, juce::String& error);
    bool exportAudio(const juce::File&, juce::String& error);
    juce::String takeOperationError();
    bool captureStorageExhausted() const noexcept { return captureExhausted.load(); }
    bool saveProject(const juce::File&, juce::String& error);
    bool loadProject(const juce::File&, juce::String& error);
    void transport(const juce::String& action);
    void slideTapeSelection(double destination){prepareTapeOperation(field::TapeEditAction::slide,destination,true);}
    void setTrack(int);
    void setTapePerformance(int control,bool down);
    void releaseTapePerformance();
    double snapTapeTime(double seconds) const;
    std::atomic<int> tapeEditMask{0}; // 0 follows selected track
    std::atomic<bool> tapeSnap{false};
    void selectTape(int);
    int tapeNumber() const { return currentTape; }
    void newProject();
    void savePatchSlot(int);
    void recallPatchSlot(int,int explicitSlot=-1);
    void storeMemo(int);
    void recallMemo(int);
    juce::String lastImportSummary,lastImportDetails;
    juce::String sampleName = "No sample loaded";
    juce::String projectName = "Untitled";
    std::atomic<float> outputPeak{0},effectiveTempo{110};
    std::atomic<int> skippedBlocks{0};
    std::atomic<double> playhead{0};
    std::atomic<bool> hostSync{false};
    std::atomic<bool> metronome{false};
    std::atomic<bool> inputMonitor{false};
    std::atomic<bool> drumMode{false};
    double selectionStart=0, selectionEnd=4;
    std::atomic<int> octave{0};
    int keyboardMidiChannel() const {int channel=(int)value("midiInputChannel");return channel==0?1:channel;}
    std::atomic<uint64_t> midiReceived{0},rejectedMidiEvents{0};
    std::atomic<int> midiLastStatus{0},midiLastData1{0},midiLastData2{0};
private:
    enum ParamKey {P_engine,P_macro1,P_macro2,P_macro3,P_macro4,P_attack,P_decay,P_sustain,P_release,P_sampleStart,P_sampleLoopIn,P_sampleLoopOut,P_sampleEnd,P_sampleLoop,P_sampleReverse,P_sampleOneShot,P_fx1,P_fx2,P_fx3,P_fx4,P_seq1,P_seq2,P_seq3,P_seq4,P_lfoTarget,P_effect,P_effectMix,P_masterLow,P_masterMid,P_masterHigh,P_masterDrive,P_compressor,P_masterEffect,P_masterMix,P_volume,P_tempo,P_lfo,P_lfoRate,P_lfoDepth,P_sequencer,P_sequenceOn,P_tapeStyle,P_tapeSpeed,P_loop,P_masterFx1,P_masterFx2,P_masterFx3,P_masterFx4,P_seqSwing,P_seqGate,P_seqHold,P_sampleRootNote,P_voiceMode,P_glideTime,P_bendRange,P_patchGain,P_lfoWaveform,P_lfoRetrigger,P_lfoSync,P_lfoDivision,P_elementSource,P_elementManual,P_envelopeCurve,P_midiTarget1,P_midiTarget2,P_midiTarget3,P_midiTarget4,P_midiDepth1,P_midiDepth2,P_midiDepth3,P_midiDepth4,P_countInBeats,P_tapeRecordLevel,P_tapeRecordPan,P_metronomeLevel,P_midiInputChannel,P_midiOutputChannel,P_midiClockMode,P_midiThru,P_midiSendControls,P_midiRelativeControls,P_midiNotesIn,P_midiOtherIn,P_midiTransportIn,P_midiNotesOut,P_midiOtherOut,P_midiSppOrigin,P_masterLeft,P_masterRight,P_compressorRelease,P_count};
    std::array<std::atomic<float>*,P_count> rawValues{};
    std::array<juce::RangedAudioParameter*,P_count> rawTargets{};
    std::array<juce::String,P_count> parameterIds;
    field::RealtimeValues<P_count> realtimeValues;
    static constexpr size_t extraMidiTargets=41;
    field::MidiLearn<P_count+extraMidiTargets> midiLearn;
    bool parseMidiMappings(const juce::ValueTree&,std::array<field::MidiBinding,P_count+extraMidiTargets>&,juce::String&) const;
    int midiMappingIndex(const juce::String&) const;
    float mappedMidiValue(size_t) const noexcept;
    void applyMappedMidi(size_t,float) noexcept;
    float cached(int key) const {auto* p=rawTargets[(size_t)key];return p->convertFrom0to1(p->getValue());}
    // Owned JUCE Float/Choice/Int/Bool types have atomic setValue and no-op
    // valueChanged. Custom parameter overrides require a new realtime audit.
    void setNormalized(int key,float v){rawTargets[(size_t)key]->setValue(v);realtimeValues.fromAudio((size_t)key,cached(key));}
    void setPhysical(int key,float v){auto* p=rawTargets[(size_t)key];setNormalized(key,p->convertTo0to1(v));}
    void parameterValueChanged(int,float) override;
    void parameterGestureChanged(int,bool) override {}
    void flushParameterMirrors();
    juce::ValueTree copyParameterState();
    static juce::AudioProcessorValueTreeState::ParameterLayout makeLayout();
    void timerCallback() override;
    void resetSampleParameters();
    std::atomic<int> pendingProgram{-1},pendingPatchAction{0};
    void applyPatchAction(int);
    int currentProgram=0,currentTape=0;
    std::array<juce::ValueTree,16> patchSlots;
    std::array<uint64_t,16> patchStoreRequest{};
    std::array<juce::ValueTree,2> memos;
    juce::AudioFormatManager formats;
    std::shared_ptr<const SampleData> sampleData=std::make_shared<SampleData>();
    uint64_t sampleRequest=0,controlRevision=0,documentEpoch=0;
    void startTapeRecording();
    void prepareTapeOperation(field::TapeEditAction,double delta=0,bool absolute=false);
    void reportOperationError(const juce::String&);
    std::mutex mediaEncodeMutex;
    juce::CriticalSection operationErrorLock;
    juce::String operationError;
    double sampleRateFile=44100;
    struct AlbumSide { field::TapeAudio audio; double rate=44100; };
    field::TapeAudio captureAudio;
    std::atomic<bool> captureExhausted{false};
    std::array<AlbumSide,2> albums;
    double albumHead=0;
    size_t albumLimit=0;
    size_t captureLimit=0;
    bool captureFromOutput=false;
    struct TapeState { std::array<field::Track,4> tracks; double position=0; };
    std::array<TapeState,8> tapes;
    juce::AudioBuffer<float> inputCopy;
    juce::MidiBuffer midiInput;
    juce::CriticalSection sampleWorkerLock;
    struct MasterState { float low[2]{}, upper[2]{}, envelope=0; };
    MasterState masterState;
    field::Engine masterFx;
    void processMaster(float*,float*,int,double,MasterState&,field::Engine&);
    double clickPhase=0;
    bool previousHostPlaying=false,hostPositionValid=false;
    double previousHostSeconds=0;
    int previousHostFrames=0;
    field::midi::NoteState midiNotes,guiMidiNotes;
    field::RtQueue<field::KeyboardEvent,512> missedReleases;
    bool missedReleaseOverflow=false; // audio thread only
    field::midi::ClockFollower midiClockFollower;
    field::midi::ClockGenerator midiClockGenerator;
    juce::MidiBuffer midiOutput;
    int64_t midiSampleClock=0;
    double midiTransportPpq=0,midiExpectedTapeSeconds=0,lastSppOrigin=0;
    bool midiPositionValid=false;
    int lastInputChannel=0,lastOutputChannel=1,lastClockMode=0;
    bool lastNotesIn=true,lastNotesOut=true;
    std::array<int,4> controllerValues{{-1,-1,-1,-1}};
    void midiController(int channel,int cc,int amount);
    void retainSkippedReleases(const juce::MidiBuffer&) noexcept;
    void clearMidiChannel(int);
    juce::ValueTree makeState();
    void restoreState(const juce::ValueTree&);
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FieldProcessor)
};
