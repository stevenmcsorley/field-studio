// A deliberately small Linux VST3 ABI smoke host. Uses the bundled Steinberg
// interfaces directly; it neither links JUCE nor opens the plug-in editor.
// This checks this module's basic host lifecycle/audio, not coverage of DAWs.
#include "pluginterfaces/base/ipluginbase.h"
#include "pluginterfaces/vst/ivstcomponent.h"
#include "pluginterfaces/vst/ivstaudioprocessor.h"
#include "pluginterfaces/vst/ivstevents.h"
#include "pluginterfaces/vst/ivsthostapplication.h"
#include "pluginterfaces/vst/vstspeaker.h"
#include <dlfcn.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
using namespace Steinberg;
using namespace Steinberg::Vst;
void require(bool good, const std::string& message) {
    if (!good) throw std::runtime_error(message);
}
void ok(tresult result, const char* operation) {
    require(result==kResultOk,std::string(operation)+" failed, VST3 result="+std::to_string(result));
}
bool same(const TUID a, const TUID b) { return std::memcmp(a,b,sizeof(TUID))==0; }

template <class T> class Owned {
public:
    explicit Owned(T* p=nullptr): ptr(p) {}
    ~Owned() { if(ptr) ptr->release(); }
    Owned(const Owned&)=delete;
    Owned& operator=(const Owned&)=delete;
    T* get() const { return ptr; }
    T* operator->() const { return ptr; }
private:
    T* ptr;
};

class Host final : public IHostApplication {
public:
    tresult PLUGIN_API queryInterface(const TUID iid,void** out) override {
        if(!out) return kInvalidArgument;
        *out=nullptr;
        if(same(iid,FUnknown_iid)||same(iid,IHostApplication_iid)) {
            *out=static_cast<IHostApplication*>(this); addRef(); return kResultOk;
        }
        return kNoInterface;
    }
    uint32 PLUGIN_API addRef() override { return ++references; }
    uint32 PLUGIN_API release() override {
        const auto remaining=--references;
        if(!remaining) delete this;
        return remaining;
    }
    tresult PLUGIN_API getName(String128 name) override {
        if(!name) return kInvalidArgument;
        std::fill(name,name+128,char16{});
        const char* value="Field Studio native smoke host";
        for(size_t i=0;value[i]&&i<127;++i) name[i]=static_cast<char16>(value[i]);
        return kResultOk;
    }
    tresult PLUGIN_API createInstance(TUID,TUID,void** out) override {
        if(!out) return kInvalidArgument;
        *out=nullptr; return kNoInterface;
    }
private:
    std::atomic<uint32> references{1};
};

class Events final : public IEventList {
public:
    std::vector<Event> events;
    tresult PLUGIN_API queryInterface(const TUID iid,void** out) override {
        if(!out) return kInvalidArgument;
        *out=nullptr;
        if(same(iid,FUnknown_iid)||same(iid,IEventList_iid)) {
            *out=static_cast<IEventList*>(this); addRef(); return kResultOk;
        }
        return kNoInterface;
    }
    uint32 PLUGIN_API addRef() override { return ++references; }
    uint32 PLUGIN_API release() override {
        const auto remaining=--references;
        if(!remaining) delete this;
        return remaining;
    }
    int32 PLUGIN_API getEventCount() override { return static_cast<int32>(events.size()); }
    tresult PLUGIN_API getEvent(int32 index,Event& event) override {
        if(index<0||static_cast<size_t>(index)>=events.size()) return kInvalidArgument;
        event=events[static_cast<size_t>(index)]; return kResultOk;
    }
    tresult PLUGIN_API addEvent(Event& event) override {
        events.push_back(event); return kResultOk;
    }
private:
    std::atomic<uint32> references{1};
};

class Module {
public:
    using Entry=bool (PLUGIN_API*)(void*);
    using Exit=bool (PLUGIN_API*)();
    using Factory=IPluginFactory* (PLUGIN_API*)();
    explicit Module(const char* path) {
        handle=dlopen(path,RTLD_NOW|RTLD_LOCAL);
        if(!handle) throw std::runtime_error(std::string("dlopen: ")+dlerror());
    }
    ~Module() {
        if(entered&&exit) exit();
        if(handle) dlclose(handle);
    }
    template <class T> T symbol(const char* name) {
        dlerror();
        void* value=dlsym(handle,name);
        const char* error=dlerror();
        require(error==nullptr&&value!=nullptr,std::string("Missing VST3 entry point: ")+name);
        return reinterpret_cast<T>(value);
    }
    void start() {
        exit=symbol<Exit>("ModuleExit");
        const auto entry=symbol<Entry>("ModuleEntry");
        require(entry(handle),"ModuleEntry rejected initialization"); entered=true;
    }
    void stop() {
        if(entered) {
            const bool result=exit(); entered=false;
            require(result,"ModuleExit failed");
        }
    }
private:
    void* handle=nullptr;
    Exit exit=nullptr;
    bool entered=false;
};

struct Session {
    Owned<IComponent> component;
    Owned<IAudioProcessor> processor;
    bool initialized=false,active=false,processing=false;
    Session(IComponent* c,IAudioProcessor* p):component(c),processor(p) {}
    ~Session() {
        if(processing) processor->setProcessing(false);
        if(active) component->setActive(false);
        if(initialized) component->terminate();
    }
    void finish() {
        ok(processor->setProcessing(false),"setProcessing(false)"); processing=false;
        ok(component->setActive(false),"setActive(false)"); active=false;
        ok(component->terminate(),"terminate"); initialized=false;
    }
};

void exercise(IPluginFactory* factory, const TUID classId, bool enableStems) {
    Owned<Host> host(new Host());
    void* raw=nullptr;
    ok(factory->createInstance(classId,IComponent_iid,&raw),"createInstance(IComponent)");
    require(raw!=nullptr,"Factory returned null component");
    auto* component=static_cast<IComponent*>(raw);
    raw=nullptr;
    const auto query=component->queryInterface(IAudioProcessor_iid,&raw);
    if(query!=kResultOk||!raw) {
        component->release(); throw std::runtime_error("Component lacks IAudioProcessor");
    }
    Session session(component,static_cast<IAudioProcessor*>(raw));
    ok(component->initialize(host.get()),"initialize"); session.initialized=true;
    ok(session.processor->canProcessSampleSize(kSample32),"canProcessSampleSize(float32)");
    require(component->getBusCount(kAudio,kInput)==1,"Expected one stereo input bus");
    const auto outputBusCount=component->getBusCount(kAudio,kOutput);
    require(outputBusCount>=1&&outputBusCount<=16,"Unexpected audio output bus count");
    require(component->getBusCount(kEvent,kInput)>=1,"Instrument lacks note-event input");
    SpeakerArrangement input=SpeakerArr::kStereo;
    std::vector<SpeakerArrangement> outputArrangements(static_cast<size_t>(outputBusCount),enableStems?SpeakerArr::kStereo:SpeakerArrangement{});
    outputArrangements[0]=SpeakerArr::kStereo; // Main active, optional stem outputs disabled.
    ok(session.processor->setBusArrangements(&input,1,outputArrangements.data(),outputBusCount),"setBusArrangements(stereo main/stem layout)");
    BusInfo bus{};
    ok(component->getBusInfo(kAudio,kOutput,0,bus),"getBusInfo(output)");
    require(bus.channelCount==2,"Output arrangement is not stereo");
    ok(component->getBusInfo(kAudio,kInput,0,bus),"getBusInfo(input)");
    require(bus.channelCount==2,"Input arrangement is not stereo");
    constexpr int block=256;
    constexpr double rate=44100;
    ProcessSetup setup{};
    setup.processMode=kRealtime;setup.symbolicSampleSize=kSample32;
    setup.maxSamplesPerBlock=block;setup.sampleRate=rate;
    ok(session.processor->setupProcessing(setup),"setupProcessing");
    ok(component->activateBus(kAudio,kInput,0,true),"activate audio input");
    ok(component->activateBus(kAudio,kOutput,0,true),"activate main audio output");
    for(int32 i=1;i<outputBusCount;++i)
        ok(component->activateBus(kAudio,kOutput,i,enableStems),"configure optional audio output");
    ok(component->activateBus(kEvent,kInput,0,true),"activate event input");
    ok(component->setActive(true),"setActive(true)"); session.active=true;
    ok(session.processor->setProcessing(true),"setProcessing(true)"); session.processing=true;

    std::array<float,block> inLeft{},inRight{},left{},right{};
    float* inChannels[]{inLeft.data(),inRight.data()};
    float* outChannels[]{left.data(),right.data()};
    AudioBusBuffers inputs;
    std::vector<AudioBusBuffers> outputs(static_cast<size_t>(outputBusCount));
    inputs.numChannels=2;inputs.channelBuffers32=inChannels;inputs.silenceFlags=3;
    outputs[0].numChannels=2;outputs[0].channelBuffers32=outChannels;
    // SDK bus indices remain present even when optional buses are disabled.
    // Disabled entries have zero channels and null channel buffers.
    std::vector<std::array<float,block>> stemLeft(static_cast<size_t>(outputBusCount-1));
    std::vector<std::array<float,block>> stemRight(static_cast<size_t>(outputBusCount-1));
    std::vector<std::array<float*,2>> stemChannels(static_cast<size_t>(outputBusCount-1));
    if(enableStems) for(int32 i=1;i<outputBusCount;++i) {
        const auto index=static_cast<size_t>(i-1);
        stemChannels[index]={stemLeft[index].data(),stemRight[index].data()};
        outputs[static_cast<size_t>(i)].numChannels=2;
        outputs[static_cast<size_t>(i)].channelBuffers32=stemChannels[index].data();
    }
    Owned<Events> events(new Events()),outEvents(new Events());
    ProcessContext context{};
    context.sampleRate=rate;context.tempo=110;context.timeSigNumerator=4;context.timeSigDenominator=4;
    context.state=ProcessContext::kTempoValid|ProcessContext::kTimeSigValid;
    ProcessData data;
    data.processMode=kRealtime;data.symbolicSampleSize=kSample32;data.numSamples=block;
    data.numInputs=1;data.numOutputs=outputBusCount;data.inputs=&inputs;data.outputs=outputs.data();
    data.inputEvents=events.get();data.outputEvents=outEvents.get();data.processContext=&context;
    const auto process=[&]() {
        // Sentinels detect unwritten samples, including the pre-note region.
        left.fill(NAN);right.fill(NAN);outputs[0].silenceFlags=0;
        if(enableStems) for(size_t j=0;j<stemLeft.size();++j) {
            stemLeft[j].fill(NAN);stemRight[j].fill(NAN);outputs[j+1].silenceFlags=0;
        }
        ok(session.processor->process(data),"process");
        for(int i=0;i<block;++i) {
            require(std::isfinite(left[static_cast<size_t>(i)])&&std::isfinite(right[static_cast<size_t>(i)]),"Nonfinite or unwritten VST3 output");
            require(std::abs(left[static_cast<size_t>(i)])<=8&&std::abs(right[static_cast<size_t>(i)])<=8,"Unbounded VST3 output");
        }
        if(enableStems) for(size_t j=0;j<stemLeft.size();++j) for(int i=0;i<block;++i) {
            const auto index=static_cast<size_t>(i);
            require(std::isfinite(stemLeft[j][index])&&std::isfinite(stemRight[j][index]),"Nonfinite/unwritten optional stem output");
            // A new instance has empty tape and recording is off: live synth
            // belongs only to main, not to these tape-track output buses.
            require(std::abs(stemLeft[j][index])<1e-7f&&std::abs(stemRight[j][index])<1e-7f,"Live synth leaked into empty tape stem");
        }
        context.projectTimeSamples+=block;
        context.projectTimeMusic+=block/rate*context.tempo/60;
    };
    process();
    for(int i=0;i<block;++i)
        require(std::abs(left[static_cast<size_t>(i)])<1e-7f&&std::abs(right[static_cast<size_t>(i)])<1e-7f,"Instrument sounds before a note");
    Event note{};
    note.busIndex=0;note.sampleOffset=128;note.type=Event::kNoteOnEvent;
    note.noteOn.channel=0;note.noteOn.pitch=60;note.noteOn.velocity=.8f;
    note.noteOn.tuning=0;note.noteOn.noteId=1;
    ok(events->addEvent(note),"add note on");
    process();
    float peak=0;
    for(int i=0;i<block;++i) {
        if(i<128) require(std::abs(left[static_cast<size_t>(i)])<1e-7f&&std::abs(right[static_cast<size_t>(i)])<1e-7f,"MIDI note played before sample offset128");
        else peak=std::max({peak,std::abs(left[static_cast<size_t>(i)]),std::abs(right[static_cast<size_t>(i)])});
    }
    require(peak>1e-5f,"VST3 note-on produced no audible samples");
    events->events.clear();
    for(int i=0;i<8;++i) process();
    Event off{};
    off.busIndex=0;off.sampleOffset=64;off.type=Event::kNoteOffEvent;
    off.noteOff.channel=0;off.noteOff.pitch=60;off.noteOff.noteId=1;
    ok(events->addEvent(off),"add note off");process();events->events.clear();
    for(int i=0;i<200;++i) process();
    float tail=0;
    for(int i=0;i<block;++i) tail=std::max({tail,std::abs(left[static_cast<size_t>(i)]),std::abs(right[static_cast<size_t>(i)])});
    require(tail<1e-4f,"Default note did not release after note-off");
    session.finish();
    std::cout<<"VST3 main stereo + "<<(outputBusCount-1)<<(enableStems?" enabled":" disabled")<<" optional buses; note-on/off, finite output and MIDI sample-offset128 passed; peak="<<peak<<" tail="<<tail<<"\n";
}
}

int main(int argc,char** argv) {
    try {
        require(argc==2,"Usage: field_vst3_host /absolute/path/to/Field Studio.so");
        Module module(argv[1]);module.start();
        {
            Owned<IPluginFactory> factory(module.symbol<Module::Factory>("GetPluginFactory")());
            require(factory.get()!=nullptr,"GetPluginFactory returned null");
            void* raw=nullptr;
            ok(factory->queryInterface(IPluginFactory2_iid,&raw),"query IPluginFactory2");
            Owned<IPluginFactory2> factory2(static_cast<IPluginFactory2*>(raw));
            require(factory2.get()!=nullptr,"Null IPluginFactory2");
            const auto count=factory->countClasses();
            require(count>0&&count<128,"Unexpected factory class count");
            bool found=false;
            for(int32 i=0;i<count;++i) {
                PClassInfo2 info{};
                ok(factory2->getClassInfo2(i,&info),"getClassInfo2");
                const std::string category(info.category,strnlen(info.category,sizeof(info.category)));
                const std::string subcategories(info.subCategories,strnlen(info.subCategories,sizeof(info.subCategories)));
                const std::string name(info.name,strnlen(info.name,sizeof(info.name)));
                std::cout<<"Factory class: "<<name<<" ["<<category<<"] "<<subcategories<<"\n";
                if(category==kVstAudioEffectClass&&subcategories.find("Instrument")!=std::string::npos) {
                    exercise(factory.get(),info.cid,false);
                    exercise(factory.get(),info.cid,true);found=true;break;
                }
            }
            require(found,"Factory has no instrument audio component");
        }
        module.stop();
        std::cout<<"Native Linux VST3 module lifecycle smoke passed (no editor/DAW coverage)\n";
        return 0;
    } catch(const std::exception& error) {
        std::cerr<<"FAIL: "<<error.what()<<"\n";return 1;
    }
}
