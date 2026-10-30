#include "PresetMapping.h"
#include <cmath>
#include <algorithm>
namespace field::preset {
namespace {
bool number(const juce::var& v,double& out){if(!v.isInt()&&!v.isInt64()&&!v.isDouble())return false;out=(double)v;return std::isfinite(out);}
bool array(const juce::var& object,const char* key,int size,bool required,juce::var& result,juce::String& error){
    result=object.getProperty(key,{});if(result.isVoid()&&!required)return true;
    auto* a=result.getArray();if(!a||a->size()!=size){error=juce::String("Invalid OP-1 ")+key+" array (expected "+juce::String(size)+" numbers).";return false;}
    for(auto& v:*a){double n;if(!number(v,n)){error=juce::String("Invalid numeric value in OP-1 ")+key+".";return false;}}return true;
}
}
bool sampleMapping(const juce::var& metadata,double duration,SampleMapping& output,juce::String& error){
    error.clear();SampleMapping plan;
    if(!std::isfinite(duration)||duration<=0){error="Invalid sample duration.";return false;}
    auto type=metadata.getProperty("type",{}).toString().trim().toLowerCase();
    if(type!="sampler"&&type!="drum"){output=plan;return true;}
    plan.sampled=true;plan.drums=type=="drum";
    if(type=="sampler"){
        plan.profile="Melodic sampler (6-second Q15 markers)";
        auto knobs=metadata.getProperty("knobs",{});
        if(!knobs.isVoid()){
            auto* a=knobs.getArray();if(!a||a->size()<4||a->size()>8){error="Invalid sampler marker array.";return false;}
            std::array<double,4> seconds{};
            for(int i=0;i<4;++i){double n;if(!number((*a)[i],n)||n<0||n>32767){error="Invalid sampler trim/loop marker.";return false;}seconds[(size_t)i]=n*6./32768.;}
            if(!(seconds[0]<=seconds[1]&&seconds[1]<=seconds[2]&&seconds[2]<=seconds[3])||seconds[0]>=duration||seconds[3]<=seconds[0]){error="Sampler markers do not describe playable audio.";return false;}
            for(int i=0;i<4;++i)plan.markers[(size_t)i]=(float)std::clamp(seconds[(size_t)i]/duration,0.,1.);
            plan.hasMarkers=true;if(a->size()>4)plan.warnings.add("Hardware loop/reverse/fade options are retained; enable the desired software playback mode explicitly.");
            if(seconds[3]>duration+.002)plan.warnings.add("Markers exceed the embedded audio; missing audio is not reconstructed.");
        }
        auto frequency=metadata.getProperty("base_freq",{});if(!frequency.isVoid()){
            double hz;if(!number(frequency,hz)||hz<=0){error="Invalid sampler base frequency.";return false;}
            const double note=69+12*std::log2(hz/440.);
            if(note>=0&&note<=127){plan.hasRoot=true;plan.rootNote=(float)note;}
            else plan.warnings.add("Root frequency is outside the supported MIDI range; retained in source metadata.");
        }
    }else{
        auto version=metadata.getProperty("drum_version",2);double versionNumber;if(!number(version,versionNumber)||versionNumber!=2){error="Unsupported drum-kit metadata version.";return false;}
        bool stereo=(bool)metadata.getProperty("stereo",false);
        if(!stereo&&duration>12.01){error="Mono kit exceeds the supported legacy 12-second marker profile; its marker scale is ambiguous.";return false;}
        plan.profile=stereo?"Stereo drum kit (20-second Q31 markers)":"Legacy mono drum kit (4058-unit markers at 44.1 kHz)";
        juce::var starts,ends,pitch,volume,pan,reverse,modes;
        if(!array(metadata,"start",24,true,starts,error)||!array(metadata,"end",24,true,ends,error)||!array(metadata,"pitch",24,false,pitch,error)||!array(metadata,"volume",24,false,volume,error)||!array(metadata,"pan",24,false,pan,error)||!array(metadata,"reverse",24,false,reverse,error)||!array(metadata,"playmode",24,false,modes,error))return false;
        auto optional=[](const juce::var& a,int i,double def){return a.isVoid()?def:(double)(*a.getArray())[i];};
        bool gainLimited=false,approxMode=false,unsupportedShape=false;
        for(int i=0;i<24;++i){
            const double start=(double)(*starts.getArray())[i],end=(double)(*ends.getArray())[i];
            if(start<0||end<0||start>2147483647.||end>2147483647.||end<start){error="Invalid drum slice boundaries.";return false;}
            auto& pad=plan.pads[(size_t)i];
            const double toSeconds=stereo?20./2147483646.:1./(4058.*44100.);
            const double begin=start*toSeconds,finish=end*toSeconds;
            if(begin>=duration||finish<=begin){pad.level=0;pad.start=pad.end=0;continue;}
            pad.start=(float)std::clamp(begin/duration,0.,1.);pad.end=(float)std::clamp(finish/duration,0.,1.);
            double tune=optional(pitch,i,0)/512.;if(tune<-24||tune>24)plan.warnings.addIfNotAlreadyThere("Pad tuning outside +/-24 semitones is limited; raw values remain attached.");pad.tune=(float)std::clamp(tune,-24.,24.);
            double gain=optional(volume,i,8192);if(gain<0||gain>32767){error="Invalid drum pad volume.";return false;}gain=std::pow(gain/8192.,2.);if(gain>2)gainLimited=true;pad.level=(float)std::min(2.,gain);
            double position=optional(pan,i,16384);if(position<0||position>32767){error="Invalid drum pad pan.";return false;}pad.pan=(float)((position-16384)/16384.);
            double direction=optional(reverse,i,8192);if(direction<0||direction>32767){error="Invalid drum pad direction.";return false;}pad.reverse=direction>=16384;
            double play=optional(modes,i,8192);if(play<0||play>32767){error="Invalid drum pad play mode.";return false;}
            if(play>=24576)pad.mode=2;else if(play>=16384){pad.mode=1;approxMode=true;}else if(play>=8192&&play!=8192){pad.mode=1;approxMode=true;}else pad.mode=0;
            if(finish>duration+.002)plan.warnings.addIfNotAlreadyThere("Some slice ends exceed the audio and were clipped.");
        }
        if(gainLimited)plan.warnings.add("Pad gains above +6 dB are limited; source gain values remain attached.");
        if(approxMode)plan.warnings.add("Some hardware gate/choke play modes use the closest software playback mode.");
        for(const char* key:{"attack","fademode","pan_ab","dyna_env"})if(metadata.hasProperty(key))unsupportedShape=true;
        if(unsupportedShape)plan.warnings.add("Hardware pad fades/transient and channel-routing settings are retained but not translated.");
    }
    if((bool)metadata.getProperty("fx_active",false))plan.warnings.add("Original effect parameters are retained; the imported sample uses a neutral software effect chain.");
    if((bool)metadata.getProperty("lfo_active",false))plan.warnings.add("Original modulation parameters are retained but not applied.");
    output=std::move(plan);return true;
}
}
