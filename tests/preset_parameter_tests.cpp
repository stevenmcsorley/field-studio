#include "PresetParameters.h"
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <set>
using namespace field::preset;
namespace {
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
float value(const ParameterTranslation& t,const char* id){for(auto& p:t.parameterValues)if(p.id==id)return p.value;throw std::runtime_error(std::string("Missing translated parameter: ")+id);}
bool has(const ParameterTranslation& t,const char* id){for(auto& p:t.parameterValues)if(p.id==id)return true;return false;}
bool close(float a,float b){return std::abs(a-b)<.0001f;}
ParameterTranslation convert(const char* json){auto metadata=juce::JSON::parse(json);const auto before=juce::JSON::toString(metadata);ParameterTranslation t;juce::String error;check(translateParameters(metadata,t,error),error.toRawUTF8());check(juce::JSON::toString(metadata)==before,"source metadata mutated");check(!t.exactSound,"control mappings must not claim exact sound");return t;}
void synthetic(){
    auto t=convert(R"({"type":"string","knobs":[64,24064,8192,16448]})");check(close(value(t,"macro1"),0)&&close(value(t,"macro2"),1)&&close(value(t,"macro3"),.5)&&close(value(t,"macro4"),1),"restricted String travel was scaled as generic16bit");
    t=convert(R"({"type":"dna","knobs":[-29491,4608,0,32767]})");check(value(t,"macro1")==0&&value(t,"macro2")==0,"signed DNA travel wrong");
    t=convert(R"({"type":"digital","knobs":[0,14336,0,32767]})");check(close(value(t,"macro2"),.5)&&close(value(t,"macro3"),.5),"Digital octave/bipolar control travel wrong");
    t=convert(R"({"type":"pulse","knobs":[23168,16384,16384,-16384]})");check(value(t,"macro1")==1&&value(t,"macro4")==0,"Pulse centered travel wrong");
    t=convert(R"({"type":"voltage","fx_active":true,"fx_type":"nitro","fx_params":[64,-32768,20643,16448]})");check(value(t,"effect")==4&&value(t,"fx1")==0&&value(t,"fx2")==0&&value(t,"fx3")==1&&value(t,"fx4")==1,"Nitro signed/range mapping wrong");check(!has(t,"effectMix"),"unknown wet/dry law must not be guessed");
    t=convert(R"({"type":"fm","fx_active":true,"fx_type":"delay","fx_params":[1024,3276,0,32767]})");check(value(t,"effect")==1&&value(t,"fx1")==0&&value(t,"fx2")==0&&value(t,"fx3")==0,"restricted Delay control minima wrong");
    t=convert(R"({"type":"fm","fx_active":true,"fx_type":"grid","fx_params":[1344,16704,32767,16384]})");check(close(value(t,"effectMix"),.5),"known dimensionless MIX position not translated");
    t=convert(R"({"type":"sampler","knobs":[0,8192,16384,32767],"adsr":[64,128,8192,256,9209,64,13301,8192]})");check(!has(t,"macro1")&&close(value(t,"sustain"),.25)&&value(t,"voiceMode")==2&&value(t,"bendRange")==7,"sampler routing or known ADSR selectors wrong");check(!has(t,"attack")&&!has(t,"decay")&&!has(t,"release")&&!has(t,"glideTime")&&!has(t,"patchGain"),"unmeasured time/gain laws were guessed");
    t=convert(R"({"type":"dbox","knobs":[1,2,3,4],"dbox_data":[[1,2,3,4,5,6,7,8]]})");check(!has(t,"macro1"),"perpadDBox mapped to global melodic macro");
    t=convert(R"({"type":"voltage","lfo_active":true,"lfo_type":"tremolo","lfo_params":[8192,1000,2000,0],"fx_active":false,"fx_type":"spring","fx_params":[1344,7744,16384,32767]})");check(value(t,"effect")==0&&!has(t,"fx1")&&value(t,"lfo")==3&&value(t,"lfoDepth")==0,"bypass flags or neutral unsupported LFO failed");check(t.lfoActive&&!t.effectActive&&t.sourceEffect==2,"source enable/selection facts lost");
    t=convert(R"({"type":"fm","fx_active":true,"fx_type":"unrecognized"})");check(value(t,"effect")==0&&!t.unsupported.isEmpty(),"unknown active FX was silently substituted");
    t=convert(R"({"type":"string","knobs":[0,0,0,0]})");check(value(t,"macro1")==0&&t.unsupported.joinIntoString(" ").contains("clamped"),"beyond-observed values require explicit clamp report");
    t=convert(R"({"type":"fm","adsr":[64,64,32767,64,6144,64,5120,4000]})");check(value(t,"voiceMode")==1&&value(t,"bendRange")==2&&t.unsupported.joinIntoString(" ").contains("decision boundary"),"nearest captured selector needs stated uncertainty");
    for(const char* invalid:{R"({"type":"fm","knobs":[1,2,3]})",R"({"type":"fm","knobs":[1,2,3,0.5]})",R"({"type":"fm","knobs":[1,2,3,32768]})",R"({"type":"fm","fx_active":"yes"})",R"([])"}){
        ParameterTranslation original;original.sourceProfile="unchanged";juce::String error;check(!translateParameters(juce::JSON::parse(invalid),original,error)&&!error.isEmpty()&&original.sourceProfile=="unchanged","malformed input must fail atomically");
    }
}
void audit(const juce::File& inventory){
    auto root=juce::JSON::parse(inventory);auto records=root.getProperty("fixtures",{});check(records.isArray(),"fixture inventory missing");
    int count=0,controls=0;std::set<std::string> types,fxTypes,lfoTypes;
    for(auto& record:*records.getArray()){
        const auto metadata=record.getProperty("metadata",{});auto before=juce::JSON::toString(metadata);juce::String error;ParameterTranslation t;
        if(!translateParameters(metadata,t,error))throw std::runtime_error("Fixture rejected: "+record.getProperty("file",{}).toString().toStdString()+" "+error.toStdString());
        for(auto& c:t.controls)check(std::isfinite(c.position)&&c.position>=0&&c.position<=1,"fixture control normalized outside unit interval");
        check(juce::JSON::toString(metadata)==before,"fixture metadata changed");check(!t.exactSound,"fixture audit claimed DSP equivalence");++count;controls+=(int)t.controls.size();types.insert(metadata.getProperty("type",{}).toString().toStdString());fxTypes.insert(metadata.getProperty("fx_type",{}).toString().toStdString());lfoTypes.insert(metadata.getProperty("lfo_type",{}).toString().toStdString());
    }
    std::cout<<"Fixture audit: "<<count<<" metadata records, "<<types.size()<<" engines, "<<fxTypes.size()<<" source FX names, "<<lfoTypes.size()<<" LFO names, "<<controls<<" translated control positions\n";
}
}
int main(int argc,char** argv){try{synthetic();std::cout<<"PASS: typed ranges, signed/restricted controls, FX flags, sampler/DBox isolation, captured selectors, unsupported-law reporting, atomic malformed rejection\n";if(argc==2)audit(juce::File(argv[1]));return 0;}catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}}
