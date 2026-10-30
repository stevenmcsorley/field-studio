#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include <cstdint>

namespace field::midi {
// Software clock/routing policies, not measured hardware timing behaviour.
class ClockFollower {
public:
    void prepare(double sampleRate);
    void reset();
    bool observe(const juce::MidiMessage&,int64_t absoluteSample);
    void advance(int64_t absoluteSample);
    bool ready() const { return intervalCount>=6; }
    double bpm() const { return estimatedBpm; }
    bool playing() const { return running; }
    int songPosition() const;
    double quarterNotes() const { return clockPosition/24.0; }
private:
    void pulse(int64_t absoluteSample);
    void accept(double interval);
    void clearEstimate();
    double sr=44100,estimatedBpm=120,candidate=0;
    std::array<double,24> intervals{};
    int intervalCount=0,writeIndex=0,candidateCount=0;
    int64_t lastPulse=0;
    bool havePulse=false,running=false,firstStartPulse=false;
    double clockPosition=0;
};

// process() never reserves memory. The caller preallocates the destination
// MidiBuffer before entering the audio callback. Position is in quarter notes.
class ClockGenerator {
public:
    void prepare(double sampleRate);
    void reset();
    void process(juce::MidiBuffer&,int frames,double bpm,bool enabled,bool playing,
                 double quarterNotePosition,bool suppressForExternalClock=false,int sampleOffset=0);
private:
    void position(juce::MidiBuffer&,double quarterNotes,int sampleOffset,bool sendStop);
    double sr=44100,tempo=120,expectedPosition=0,nextClock=0;
    int64_t cursor=0,nextPulse=0;
    bool running=false,havePosition=false;
};

struct Releases { std::array<int,128> notes{};int count=0; };
class NoteState {
public:
    bool noteOn(int channel,int note);
    bool noteOff(int channel,int note);
    Releases controllerSustain(int channel,bool down);
    Releases resetChannel(int channel);
    Releases resetAll();
    bool heldAnywhere(int note) const;
    bool held(int channel,int note) const;
    bool pedalDown(int channel) const;
private:
    static bool valid(int channel,int note);
    std::array<std::array<bool,128>,16> keys{},sustained{};
    std::array<bool,16> pedals{};
};
}
