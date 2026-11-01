#pragma once
#include "Engine.h"
#include "RealtimeSnapshot.h"
namespace field {
struct UiTelemetry {
 std::array<char,256> projectLabel{},sourceLabel{};int activeVoices=0,heldCount=0;double position=0;float tempo=110,tapeSpeed=1;
 std::array<float,8> lfoRandomHistory{};
 std::array<float,4> lfoMidiValues{};
 float lfoPhase=0,modulation=0;bool playing=false,recording=false,externalInput=false;int pad=0,track=0,sequenceLength=16,sequenceStep=-1,fingerBank=0,sketchCount=2,tombolaSides=4;
 double recorded=0,countIn=0,rate=44100,tombolaRotation=0;uint64_t tapeFailures=0,tombolaTriggers=0;uintptr_t sampleIdentity=0;
 DrumPad drumPad{};DBoxPad dboxPad{};
 std::array<float,4> levels{{1,1,1,1}},pans{},peaks{};std::array<double,4> durations{};std::array<double,2> albumDurations{};std::array<bool,4> mutes{},solos{};
 std::array<std::bitset<128>,16> sequenceChords;std::array<int,16> steps{};std::array<bool,32> finger{};std::array<bool,128> held{};
 std::array<std::pair<float,float>,256> sketch{};std::array<BallState,24> balls{};
};
}
