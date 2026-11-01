#pragma once
#include "RtQueue.h"
#include <cstdint>
#include <type_traits>

namespace field {
// Fixed payloads only. Producers are serialized by the processor's control-side
// mutex; audio drains at most 32 commands per block and never takes that mutex.
enum class LiveControl {
 play,rewind,seek,seekRelative,track,level,pan,mute,solo,brake,reverse,roll,
 drumParameter,dboxParameter,pad,fingerBank,fingerLength,sequenceLength,
 sequenceNote,sequenceStep,fingerStep,clearHeld,manualAdvance,sketchPoint
};
struct LiveCommand {
 LiveControl kind{};int a=0,b=0;double x=0,y=0;uint64_t epoch=0;
};
static_assert(std::is_trivially_copyable_v<LiveCommand>);
}
