#include "PluginProcessor.h"
#include <cmath>

// Original presets for OPVYN's DSP. These are not OP-1 factory files.
void FieldProcessor::initializeFactoryPresets(){
    const std::array<int,8> initial{{0,1,5,6,9,4,7,10}};
    for(int kind=0;kind<15;++kind)for(int variation=0;variation<3;++variation){
        juce::ValueTree patch("PATCH"),sound("PARAMETERS");
        for(int key=0;key<P_count;++key)if(field::PresetLibrary::soundParameter(parameterIds[(size_t)key])){
            auto* target=rawTargets[(size_t)key];juce::ValueTree param("PARAM");param.setProperty("id",parameterIds[(size_t)key],nullptr);param.setProperty("value",target->convertFrom0to1(target->getDefaultValue()),nullptr);sound.addChild(param,-1,nullptr);
        }
        auto set=[&](const juce::String& id,float value){for(auto param:sound)if(param["id"].toString()==id){param.setProperty("value",value,nullptr);return;}};
        set("engine",(float)kind);
        for(int knob=0;knob<4;++knob)set("macro"+juce::String(knob+1),.15f+((kind*13+variation*19+knob*23)%70)*.01f);
        set("attack",variation==2?.35f:.005f);set("decay",variation==1?.22f:.7f);set("sustain",variation==1?.25f:.8f);set("release",variation==2?1.1f:.25f);
        set("effect",variation==2?2.f:variation==1?1.f:0.f);set("effectMix",variation==0?0.f:.18f);set("lfo",3);set("lfoTarget",1);set("lfoRate",.8f);set("lfoDepth",variation==2?.08f:0.f);
        if(variation==2){set("lfoModel",0);set("lfoRetrigger",0);} // Preserve established Slow orbit sound.
        const bool drum=kind==14;
        const auto name=engineNames[kind]+juce::StringArray{" / First light"," / Short circuit"," / Slow orbit"}[variation]+(kind==12||kind==13?" (input)":"");
        patch.setProperty("presetName",name,nullptr);patch.setProperty("drum",drum,nullptr);patch.setProperty("rate",44100.,nullptr);patch.setProperty("name",kind==11?"Original harmonic sample":"No sample loaded",nullptr);patch.setProperty("metadata","{}",nullptr);
        patch.addChild(sound,-1,nullptr);
        std::vector<float> samples;if(kind==11){samples.resize(44100);for(size_t i=0;i<samples.size();++i){double t=(double)i/44100.;samples[i]=(float)(.45*std::exp(-t*(variation==1?7.:2.))*(std::sin(t*261.625565*juce::MathConstants<double>::twoPi)+.22*std::sin(t*523.25113*juce::MathConstants<double>::twoPi)));}set("sampleOneShot",1);}
        patch.setProperty("left",juce::MemoryBlock(samples.data(),samples.size()*sizeof(float)),nullptr);patch.setProperty("right",juce::MemoryBlock(samples.data(),samples.size()*sizeof(float)),nullptr);
        juce::Array<juce::var> pads,dbox;
        for(int i=0;i<24;++i){auto p=engine.drumPad(i);pads.add(juce::Array<juce::var>{p.start,p.end,p.tune,p.level,p.pan,p.reverse,p.mode,p.chokeGroup});auto d=engine.dboxPad(i);d.crossMod=variation*.12f;d.wave2=variation*.25f;dbox.add(juce::Array<juce::var>{d.mix,d.wave1,d.envelope1,d.crossMod,d.frequency,d.wave2,d.envelope2,d.cutoff,d.attack,d.gain,d.release,d.smooth});}
        patch.setProperty("pads",pads,nullptr);patch.setProperty("dbox",dbox,nullptr);
        presetLibrary.addFactory({"factory-"+juce::String(kind)+"-"+juce::String(variation),name,"ENGINE: "+engineNames[kind].toUpperCase(),drum,true,patch});
        if(variation==0&&!drum)for(int slot=0;slot<8;++slot)if(initial[(size_t)slot]==kind)defaultPatchSlots[(size_t)slot]=patch;
        if(drum)for(int slot=0;slot<8;++slot)if(slot%3==variation)defaultPatchSlots[(size_t)(8+slot)]=patch;
    }
    patchSlots=defaultPatchSlots;
}
