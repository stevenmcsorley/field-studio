#pragma once
#include <algorithm>
#include <array>
#include <cmath>
namespace field {
// Flat destination IDs stay compatible with saved projects and MIDI mappings.
inline constexpr std::array<std::array<int,7>,4> lfoGroups{{{{1,2,3,4,5,6,7}},{{8,9,10,11,-1,-1,-1}},{{12,13,14,15,16,-1,-1}},{{0,17,18,19,-1,-1,-1}}}};
inline constexpr std::array<int,4> lfoGroupSizes{{7,4,5,4}};
inline int lfoGroup(int target){return target>=1&&target<=7?0:target>=8&&target<=11?1:target>=12&&target<=16?2:3;}
inline int lfoParameter(int target){const auto group=lfoGroup(target);for(int i=0;i<lfoGroupSizes[group];++i)if(lfoGroups[group][i]==target)return i;return 0;}
inline int lfoDestination(int group,int parameter){group=std::clamp(group,0,3);return lfoGroups[group][std::clamp(parameter,0,lfoGroupSizes[group]-1)];}
inline float lfoWave(int shape,double phase){const float p=(float)(phase-std::floor(phase));switch(shape){case 0:return std::sin(p*6.283185307179586f);case 1:return 1-4*std::abs(p-.5f);case 2:return p*2-1;case 3:return p<.5f?1:-1;case 4:return 2*std::expm1(4*p)/std::expm1(4.f)-1;case 5:return p<.1f?1:-1;case 6:return 1-p*2;default:return 0;}}
inline constexpr std::array<const char*,20> lfoBaseParameters{{"","macro1","macro2","macro3","macro4","patchGain","","","attack","decay","sustain","release","fx1","fx2","fx3","fx4","effectMix","patchGain","tapeSpeed","volume"}};
// Signed attack/decay shaping is an original software law, not a measured curve.
inline float lfoEnvelope(float shape,double seconds){if(std::abs(shape)<.0001f)return 1;const float t=(float)(seconds/(.005+std::abs(shape)*4));return shape>0?std::min(1.f,t):std::max(0.f,1-t);}
}
