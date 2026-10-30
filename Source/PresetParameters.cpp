#include "PresetParameters.h"
#include <array>
#include <algorithm>
#include <cmath>
#include <limits>

namespace field::preset {
namespace {
struct Travel {const char* group;int index;double minimum,maximum;};
// Independently encoded format facts from observed Field control travel; these
// are not a lookup of DSP equations or manufacturer sound calibration.
constexpr Travel travel[]{
    {"synth.cluster",0,3072,17408},{"synth.cluster",2,512,24064},{"synth.cluster",3,3,1638},
    {"synth.digital",1,2048,26624},{"synth.digital",2,-32768,32767},
    {"synth.dna",0,-29491,32767},{"synth.dna",1,4608,12800},
    {"synth.drwave",0,0,24575},{"synth.drwave",1,0,16379},{"synth.drwave",2,0,16377},
    {"synth.fm",2,1024,17408},{"synth.phase",1,0,29491},
    {"synth.pulse",0,0,23168},{"synth.pulse",1,0,16384},{"synth.pulse",2,0,16384},{"synth.pulse",3,-16384,16384},
    {"synth.string",0,64,8256},{"synth.string",1,512,24064},{"synth.string",2,0,16384},{"synth.string",3,8256,16448},
    {"fx.delay",0,1024,11264},{"fx.delay",1,3276,32767},{"fx.delay",2,0,16384},
    {"fx.grid",0,1344,16704},{"fx.grid",1,1344,16704},
    {"fx.nitro",0,64,16448},{"fx.nitro",1,-32768,32767},{"fx.nitro",2,0,20643},{"fx.nitro",3,64,16448},
    {"fx.phone",0,204,20480},{"fx.phone",1,3072,17408},{"fx.phone",2,1536,16896},
    {"fx.punch",0,1344,12480},{"fx.punch",2,1536,25088},
    {"fx.spring",0,1344,16448},{"fx.spring",1,7744,16448},{"fx.spring",2,0,16384}
};
constexpr const char* melodic[]{"voltage","fm","cluster","digital","drwave","dimension","string","pulse","phase","dna","dsynth","vocoder","amp"};
constexpr const char* effectNames[]{"off","delay","spring","grid","nitro","phone","punch","cwo","terminal","mother","fazer"};
constexpr const char* lfoNames[]{"random","element","midi","tremolo","value","velocity"};

bool numeric(const juce::var& value,double& result){
    if(!value.isInt()&&!value.isInt64()&&!value.isDouble())return false;
    result=(double)value;return std::isfinite(result)&&std::floor(result)==result&&result>=-32768&&result<=32767;
}
bool readArray(const juce::var& metadata,const char* name,juce::var& output,juce::String& error){
    output=metadata.getProperty(name,{});if(output.isVoid())return true;
    auto* values=output.getArray();if(!values||(values->size()!=4&&values->size()!=8)){error=juce::String("Invalid ")+name+" array: expected four or eight raw control integers.";return false;}
    for(auto& value:*values){double number;if(!numeric(value,number)){error=juce::String("Invalid ")+name+" raw control integer.";return false;}}
    return true;
}
bool flag(const juce::var& metadata,const char* key,bool& result,juce::String& error){
    auto value=metadata.getProperty(key,false);
    if(value.isBool()){result=(bool)value;return true;}
    if((value.isInt()||value.isInt64())&&((int)value==0||(int)value==1)){result=(int)value!=0;return true;}
    error=juce::String("Invalid ")+key+" flag.";return false;
}
template<size_t N> int lookup(const juce::String& name,const char*const (&names)[N]){for(size_t i=0;i<N;++i)if(name==names[i])return (int)i;return -1;}
Travel range(const juce::String& group,int index){for(auto& row:travel)if(group==row.group&&index==row.index)return row;return {nullptr,index,0,32767};}
void add(ParameterTranslation& result,const juce::String& parameter,float value){
    for(auto& entry:result.parameterValues)if(entry.id==parameter){entry.value=value;return;}
    result.parameterValues.push_back({parameter,value});
}
void position(ParameterTranslation& result,const juce::String& group,int index,double raw,const juce::String& parameter){
    const auto bounds=range(group,index);
    const auto unit=std::clamp((raw-bounds.minimum)/(bounds.maximum-bounds.minimum),0.,1.);
    result.controls.push_back({group,index,raw,bounds.minimum,bounds.maximum,(float)unit,parameter,"observed raw travel normalized to software control position"});
    add(result,parameter,(float)unit);
    result.mapped.add(group+"["+juce::String(index)+"] -> "+parameter);
    if(raw<bounds.minimum||raw>bounds.maximum)result.unsupported.addIfNotAlreadyThere(group+" contains values beyond observed knob travel; control positions were clamped and raw metadata retained.");
}
void selector(ParameterTranslation& result,const char* group,int index,double raw,const char* parameter,const std::vector<double>& captures,const std::vector<float>& values){
    size_t selected=0;for(size_t i=1;i<captures.size();++i)if(std::abs(raw-captures[i])<std::abs(raw-captures[selected]))selected=i;
    add(result,parameter,values[selected]);
    result.controls.push_back({group,index,raw,captures.front(),captures.back(),(float)selected/(float)(captures.size()-1),parameter,"nearest captured selector value"});
    result.mapped.add(juce::String(group)+"["+juce::String(index)+"] -> "+parameter);
    if(raw!=captures[selected])result.unsupported.addIfNotAlreadyThere(juce::String(parameter)+" used the nearest captured selector; the exact hardware decision boundary is unverified.");
}
}

bool translateParameters(const juce::var& metadata,ParameterTranslation& output,juce::String& error){
    error.clear();if(!metadata.getDynamicObject()){error="Preset metadata must be an object.";return false;}
    ParameterTranslation result;
    result.sourceProfile="OP-1 field observed control travel; community fixture revision 24ac03e2de2f37c43272417d732ffd1f4e69636d";
    auto type=metadata.getProperty("type",{}).toString().trim().toLowerCase();
    juce::var knobs,fx,lfo,adsr;
    if(!readArray(metadata,"knobs",knobs,error)||!readArray(metadata,"fx_params",fx,error)||!readArray(metadata,"lfo_params",lfo,error)||!readArray(metadata,"adsr",adsr,error))return false;
    if(!flag(metadata,"fx_active",result.effectActive,error)||!flag(metadata,"lfo_active",result.lfoActive,error))return false;
    if(type=="sampler")result.unsupported.add("Sampler marker/root conversion is handled by PresetMapping; remaining sampler controls are preserved, not mapped to generic synth macros.");
    else if(type=="dbox")result.unsupported.add("DBox per-pad oscillator/envelope arrays are retained; no global melodic-macro substitution is applied.");
    else if(lookup(type,melodic)>=0){
        if(auto* values=knobs.getArray())for(int i=0;i<4;++i)position(result,"synth."+type,i,(double)(*values)[i],"macro"+juce::String(i+1));
        else result.unsupported.add("No melodic knob array was supplied.");
        if(type=="dsynth"&&knobs.isArray()&&knobs.getArray()->size()>4)result.unsupported.add("DSynth secondary oscillator controls 4-7 are retained but do not yet have independent software parameters.");
    }else if(type!="drum")result.unsupported.add("Unknown source engine: "+type+"; engine controls were retained without substitution.");

    auto effect=metadata.getProperty("fx_type",{}).toString().trim().toLowerCase();
    result.sourceEffect=lookup(effect,effectNames);
    if(result.effectActive&&result.sourceEffect>0){
        add(result,"effect",(float)result.sourceEffect);result.mapped.add("fx_type -> effect selection");
        if(effect=="fazer")result.unsupported.add("Fazer raw control travel is not established by the current fixture profile; its parameter array remains unapplied.");
        else if(auto* values=fx.getArray()){
            for(int i=0;i<4;++i)position(result,"fx."+effect,i,(double)(*values)[i],"fx"+juce::String(i+1));
            if(effect=="grid"||effect=="mother"||effect=="spring"||effect=="terminal"){
                auto bounds=range("fx."+effect,3);const auto mix=std::clamp(((double)(*values)[3]-bounds.minimum)/(bounds.maximum-bounds.minimum),0.,1.);add(result,"effectMix",(float)mix);result.mapped.add("Documented FX MIX control -> software wet/dry position");
            }else result.unsupported.add("The source FX has no established software-equivalent wet/dry law; software blend remains at its current default.");
        }else result.unsupported.add("Active effect has no parameter array; software effect defaults remain in use.");
    }else {
        add(result,"effect",0);
        if(result.effectActive)result.unsupported.add("Active source effect is unknown or unsupported; software effect is bypassed and source data retained.");
        else result.mapped.add("fx_active=false -> effect bypass");
    }
    auto lfoType=metadata.getProperty("lfo_type",{}).toString().trim().toLowerCase();result.sourceLfo=lookup(lfoType,lfoNames);
    add(result,"lfoDepth",0);
    if(result.sourceLfo>=0){add(result,"lfo",(float)result.sourceLfo);result.mapped.add("lfo_type -> LFO type selection");}
    if(result.lfoActive)result.unsupported.add("Active LFO rate/depth/destinations are not calibrated; modulation remains neutral and all source LFO values are retained.");
    else result.mapped.add("lfo_active=false -> neutral modulation");

    if(auto* values=adsr.getArray()){
        // Sustain is a dimensionless control position. Time and gain laws are
        // intentionally not invented from percentages or generic raw scaling.
        const double sustain=(double)(*values)[2];
        if(sustain>=0)position(result,"adsr",2,sustain,"sustain");
        else result.unsupported.add("Negative sustain encoding is outside this profile and remains unapplied.");
        result.unsupported.add("Hardware attack/decay/release timing and curve laws are unresolved; those source values are retained without guessed seconds.");
        if(values->size()==8){
            selector(result,"adsr",4,(double)(*values)[4],"voiceMode",{2048,6140,9209,14336},{0,1,2,3});
            selector(result,"adsr",6,(double)(*values)[6],"bendRange",{2048,5117,9209,13301,18432},{1,2,4,7,12});
            result.unsupported.add("Hardware portamento time and patch-volume gain laws are unresolved; glide and patch gain stay at software defaults.");
        }
    }
    result.unsupported.addIfNotAlreadyThere("Normalized control positions feed original software synth/effect algorithms. Exact hardware sound and parameter-response equivalence are not established.");
    output=std::move(result);return true;
}
}
