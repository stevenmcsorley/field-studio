#pragma once
#include <array>
#include <cstdint>
#include <deque>
#include <utility>
#include <vector>

// Original DSP implementations guided by publicly documented concepts. These
// algorithms are not recovered Teenage Engineering DSP and are not sonically
// verified replicas. The wrapper must serialize access to this class.
namespace field {
struct Parameters {
    int engine = 0;
    float macro[4] = {.35f, .5f, .45f, .3f};
    float effectMacro[4] = {.35f, .5f, .45f, .3f};
    float sequencerMacro[4] = {.35f, .5f, .45f, .3f};
    float attack = .01f, decay = .2f, sustain = .7f, release = .25f;
    int effect = 0;
    float effectMix = .2f, volume = .65f, tempo = 110;
    int lfo = 0;
    int lfoTarget = 0; // pitch, source macros, gain, pan, filter, ADSR, FX macros/mix, patch/tape/master
    int lfoWaveform = 0; // sine, triangle, saw, square, exponential, blip
    bool lfoRetrigger = false, lfoSync = false;
    int lfoDivision = 4; // beats/cycle: 16,8,4,2,1,.75,.5,.375,.25,.1875,.125,.0625
    int elementSource = 0; // input envelope, voice envelope, previous output, manual
    float elementManual = .5f;
    int envelopeCurve = 0; // linear or original normalized exponential (not hardware calibrated)
    std::array<int,4> midiLfoTargets{{0,1,2,3}};
    std::array<float,4> midiLfoDepth{{0,0,0,0}};
    float lfoRate = 1, lfoDepth = 0;
    int sequencer = 0;
    bool sequenceOn = false;
    float seqSwing=0,seqGate=.65f;
    bool seqHold=false;
    int tapeStyle = 0;
    float tapeSpeed = 1;
    bool loop = false;
    bool drumMode = false;
    bool monitorInput = false;
    bool metronome = false;
    float sampleStart = 0, sampleLoopIn = 0, sampleLoopOut = 1, sampleEnd = 1;
    bool sampleLoop = false, sampleReverse = false, sampleOneShot = false;
    float sampleRootNote = 60;
    int voiceMode=0; // Poly, Mono, Legato, Unison
    float glideTime=0,patchGain=1;
};
// MIDI53..76 (F3..E5) addresses pads0..23; other notes wrap every24 semitones.
// This is the software keyboard mapping, not a verified hardware MIDI octave.
struct DrumPad {
    float start = 0, end = 1; // Normalized positions in the loaded stereo sample.
    float tune = 0, level = 1, pan = 0;
    bool reverse = false;
    int mode = 0; // 0 = one-shot; 1 = key/gate; 2 = gated loop.
    int chokeGroup = 0; // 0 = independent; 1..4 = mutually exclusive group.
};
// Original dual-oscillator drum model. Parameters describe the documented
// DBox control concepts; this is not recovered hardware DSP.
struct DBoxPad {
    float mix=.5f, wave1=0, envelope1=.2f, crossMod=0;
    float frequency=.3f, wave2=.5f, envelope2=.1f, cutoff=.6f;
    float attack=.001f, gain=1, release=.1f, smooth=.1f;
};
struct Point { float x=0,y=.5f; };
struct BallState { float x=.5f,y=.5f,vx=0,vy=0;int note=60;bool active=false; };
struct Track {
    std::vector<float> left, right;
    float level = 1, pan = 0;
    bool mute = false, solo = false;
    std::vector<double> cuts; // Tape splice positions, in seconds.
};
class Engine {
public:
    Engine();
    void prepare(double sampleRate, int maxBlock);
    void setParameters(const Parameters&);
    const Parameters& parameters() const { return params; }
    void noteOn(int note, float velocity);
    void noteOff(int note);
    void allNotesOff();
    int activeVoiceCount() const;
    float currentVoicePitch() const;
    void process(float* left, float* right, const float* inputLeft,
                 const float* inputRight, int frames, float* const* stems = nullptr);
    void loadSample(const std::vector<float>& left, const std::vector<float>& right, double sampleRate);
    const std::vector<float>& sampleLeft() const { return sampleL; }
    const std::vector<float>& sampleRight() const { return sampleR; }
    double loadedSampleRate() const { return sampleSr; }
    const DrumPad& drumPad(int index) const;
    void setDrumPad(int index, const DrumPad&);
    void resetDrumPads();
    const DBoxPad& dboxPad(int index) const;
    void setDboxPad(int index, const DBoxPad&);
    void resetDboxPads();
    int selectedPad() const { return padSelected; }
    void setSelectedPad(int index);
    void selectTrack(int);
    void setTrackLevel(int, float);
    void setTrackPan(int, float);
    void setTrackMute(int, bool);
    void setTrackSolo(int, bool);
    float trackLevel(int i) const;
    float trackPan(int i) const;
    bool trackMute(int i) const;
    bool trackSolo(int i) const;
    // setRecording, edits, sample load, export, restore and prepare are control
    // thread operations. Recording reserves six minutes for its track here,
    // never in process(). Holding the wrapper's lock is required for all calls.
    void setPlaying(bool);
    void setRecording(bool);
    void rewind();
    void seek(double seconds);
    void setLoop(double startSeconds, double endSeconds);
    double loopStart() const { return loopBegin / sr; }
    double loopEnd() const { return loopFinish / sr; }
    void clearTrack();
    void lift(double startSeconds, double endSeconds);
    void drop();
    void split();
    void undo();
    void redo();
    bool canUndo() const { return !undos.empty(); }
    bool canRedo() const { return !redos.empty(); }
    bool playing() const { return isPlaying; }
    bool recording() const { return isRecording; }
    double positionSeconds() const { return position / sr; }
    double recordedSeconds() const;
    double sampleRate() const { return sr; }
    int selectedTrack() const { return selected; }
    float peak() const { return peakValue; }
    double maxTapeSeconds() const { return 360.0; }
    std::vector<float> waveform(int track, int bins) const;
    std::pair<std::vector<float>, std::vector<float>> exportMix() const;
    std::array<Track, 4>& tracks() { return tape; }
    const std::array<Track, 4>& tracks() const { return tape; }
    void restoredTape(); // Clamp/sanitize direct track restoration; reset history.
    // Explicit step editing for Pattern/Finger/Sketch. -1 is rest, notes 0..127.
    void setSequenceStep(int step, int note, float velocity = .8f);
    int sequenceStep(int step) const;
    float sequenceVelocity(int step) const;
    void setSequenceLength(int steps);
    int sequenceLength() const { return seqLength; }
    void clearHeldNotes();
    int heldNoteCount() const { return heldCount; }
    int heldNote(int index) const { return index>=0&&index<heldCount?heldNotes[static_cast<size_t>(index)]:-1; }
    void setSketchPoint(int index,float x,float y);
    std::pair<float,float> sketchPoint(int index) const;
    void setSketchLength(int length);
    int sketchLength() const { return sketchCount; }
    std::array<BallState,24> tombolaBalls() const { return balls; }
    void setTombolaBall(int index,const BallState&);
    uint64_t tombolaTriggerCount() const { return ballTriggers; }
    float tombolaRotation() const { return static_cast<float>(ballAngle); }
    void setTombolaRotation(float radians);
    int tombolaSides() const;
    void setFingerPattern(int pattern);
    int fingerPattern() const { return fingerSelected; }
    void setFingerStep(int pattern,int step,int note,float velocity=.8f);
    int fingerStep(int pattern,int step) const;
    float fingerVelocity(int pattern,int step) const;
    void setFingerLength(int pattern,int length);
    int fingerLength(int pattern) const;
    int currentSequenceStep() const { return seqStep; }
    void setPitchBend(float semitones);
    void setModWheel(float value);
    void setMidiCC(int cc, float value);
    float modulationAt(int target) const { return target >= 0 && target < 20 ? modulationRoutes[static_cast<size_t>(target)] : 0; }
    double lfoPhasePosition() const { return lfoPhase; }
    void setOutputGainImmediately(float value); // Utility FX pass-through; main instrument normally smooths gain.
private:
    struct Voice {
        bool active = false, gate = false;
        int note = 60, stage = 0, engine = 0;
        bool drum = false;
        float envelopeProgress = 0;
        float velocity = 0, env = 0, releaseStart = 0, lp = 0, hp = 0, forcedRelease = 0;
        double phase = 0, phase2 = 0, age = 0, samplePos = 0;
        float pitch=60,detune=0,stringPrevious=0;
        double sampleBegin=0, sampleFinish=0, sampleLoopBegin=0, sampleLoopFinish=0;
        bool sampleReverse=false, sampleLooped=false, sampleOneShot=false;
        float sampleTune=0, sampleLevel=1, samplePan=0;
        int pad=-1, chokeGroup=0;
        DBoxPad dbox;
        float dboxLowL=0,dboxBandL=0,dboxLowR=0,dboxBandR=0;
        float dboxPrevious=0;
        std::array<float,5> dboxFilter{};
        uint32_t noise = 1;
        std::array<float, 4096> string{};
        int stringLength = 2, stringPos = 0;
        std::array<float, 12> vocL1{}, vocL2{}, vocR1{}, vocR2{};
    };
    struct Snapshot { int track = 0; Track audio; double head = 0; };
    struct VocoderBand { float b0=0,b2=0,a1=0,a2=0,z1=0,z2=0,envelope=0; };
    std::array<VocoderBand,12> vocoder;
    Parameters params;
    std::array<Track, 4> tape;
    std::array<Voice, 24> voices;
    std::array<int,128> keyStack{};
    std::array<float,128> keyVelocity{};
    int keyCount=0;
    std::vector<float> sampleL, sampleR;
    std::array<DrumPad,24> pads;
    std::array<DBoxPad,24> dboxPads;
    int padSelected=0;
    double sampleSr = 44100, sr = 44100, position = 0, loopBegin = 0, loopFinish = 88200;
    int selected = 0;
    bool isPlaying = false, isRecording = false;
    float peakValue = 0, smoothedVolume = .65f, pitchBend = 0, modWheel = 0;
    std::vector<float> delayL, delayR;
    size_t delayWrite = 0;
    float fxLowL = 0, fxLowR = 0, fxBandL = 0, fxBandR = 0, wetLowL=0, wetLowR=0;
    std::array<float, 8> phaserL{}, phaserR{};
    std::array<float,20> modulationRoutes{};
    std::array<float,4> midiControllers{};
    float previousOutputEnvelope = 0;
    float voiceRoute(int target, float velocity) const;
    double fxPhase = 0, lfoPhase = 0, lfoClock = 0, seqPhase = 0;
    float randomLfo = 0, inputEnv = 0, heldLfo = 0;
    uint32_t randomState = 0x12345678u;
    std::array<int, 32> heldNotes{};
    std::array<float, 32> heldVelocities{};
    int heldCount = 0, seqStep = -1, seqLastNote = -1;
    int seqLength=16;
    std::array<int, 999> steps{};
    std::array<float,999> stepVelocity{};
    struct FingerBank { std::array<int,32> note{};std::array<float,32> velocity{};int length=16; };
    std::array<FingerBank,16> fingerBanks;
    int fingerSelected=0;
    std::array<Point,256> sketch;
    int sketchCount=64;
    std::array<BallState,24> balls;
    std::array<float,24> ballVelocity{},ballCooldown{};
    double ballClock=0,ballAngle=0;
    uint64_t ballTriggers=0;
    struct NoteGate { int note=-1;double remaining=0; };
    std::array<NoteGate,64> noteGates;
    bool sequenceGateOpen=true;
    std::vector<float> clipboardL, clipboardR;
    std::vector<double> clipboardCuts;
    std::deque<Snapshot> undos, redos;
    void trigger(int note, float velocity);
    void triggerVoice(int note,float velocity,float detune=0);
    void releaseNote(int note);
    void snapshot();
    void trimHistory();
    Snapshot capture() const;
    void restore(Snapshot&&);
    void tickSequence();
    void tickTombola();
    void addBall(int note,float velocity);
    void gatedTrigger(int note,float velocity,double duration);
    void tickGates();
    std::pair<float,float> renderVoice(Voice&, float modulation, float inL, float inR);
    std::pair<float,float> renderDbox(Voice&, float modulation);
    std::pair<float,float> effect(float left, float right);
    size_t maxFrames() const { return static_cast<size_t>(sr * 360.0); }
};
}
