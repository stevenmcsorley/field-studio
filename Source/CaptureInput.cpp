#include "CaptureInput.h"
#define MA_NO_DECODING
#define MA_NO_ENCODING
#define MA_NO_RESOURCE_MANAGER
#define MA_NO_NODE_GRAPH
#define MA_NO_ENGINE
#define MA_NO_GENERATION
#define MINIAUDIO_IMPLEMENTATION
#include "../vendor/miniaudio/miniaudio.h"
#include <vector>
#include <thread>
struct CaptureInput::Impl {
 ma_context context{};ma_device device{};bool contextReady=false,deviceReady=false;
 struct Device {ma_device_id id;juce::String name;bool loopback=false;};std::vector<Device> entries;
};
CaptureInput::CaptureInput():impl(std::make_unique<Impl>()){}
CaptureInput::~CaptureInput(){stop();if(impl->contextReady)ma_context_uninit(&impl->context);}
juce::StringArray CaptureInput::devices(){
 if(!impl->contextReady){
#if JUCE_WINDOWS
 ma_backend backend=ma_backend_wasapi;
#elif JUCE_MAC
 ma_backend backend=ma_backend_coreaudio;
#else
 ma_backend backend=ma_backend_pulseaudio;
#endif
 ma_context_config contextConfig=ma_context_config_init();contextConfig.pulse.pApplicationName="OPVYN capture";contextConfig.pulse.tryAutoSpawn=MA_FALSE;
 if(ma_context_init(&backend,1,&contextConfig,&impl->context)!=MA_SUCCESS)return {};impl->contextReady=true;
 }
 ma_device_info *outputs=nullptr,*inputs=nullptr;ma_uint32 outputCount=0,inputCount=0;
 if(ma_context_get_devices(&impl->context,&outputs,&outputCount,&inputs,&inputCount)!=MA_SUCCESS)return {};
 impl->entries.clear();juce::StringArray names;
#if JUCE_WINDOWS
 for(ma_uint32 i=0;i<outputCount&&names.size()<128;++i){auto name="System loopback: "+juce::String::fromUTF8(outputs[i].name);impl->entries.push_back({outputs[i].id,name,true});names.add(name);}
#endif
 for(ma_uint32 i=0;i<inputCount&&names.size()<256;++i){auto name=juce::String::fromUTF8(inputs[i].name);impl->entries.push_back({inputs[i].id,name,false});names.add(name);}return names;
}
bool CaptureInput::start(int index,double rate,juce::String& error){
 stop();if(index<0||index>=(int)impl->entries.size()){error="Refresh the audio source list and choose an available input";return false;}
 auto& selected=impl->entries[(size_t)index];auto config=ma_device_config_init(selected.loopback?ma_device_type_loopback:ma_device_type_capture);
 config.capture.pDeviceID=&selected.id;config.capture.format=ma_format_f32;config.capture.channels=2;config.sampleRate=(ma_uint32)rate;config.periodSizeInFrames=256;config.periods=2;config.performanceProfile=ma_performance_profile_low_latency;config.pUserData=this;config.pulse.pStreamNameCapture="OPVYN input";
 config.dataCallback=[](ma_device* device,void*,const void* input,ma_uint32 count){static_cast<CaptureInput*>(device->pUserData)->ring.push(static_cast<const float*>(input),count);};
 auto result=ma_device_init(&impl->context,&config,&impl->device);if(result!=MA_SUCCESS){error="Cannot open audio source: "+juce::String(ma_result_description(result));return false;}impl->deviceReady=true;ring.clear();ring.dropped=0;bridge.reset(256);
 result=ma_device_start(&impl->device);if(result!=MA_SUCCESS){error="Cannot start audio source: "+juce::String(ma_result_description(result));stop();return false;}running=true;return true;
}
void CaptureInput::stop(){running.store(false,std::memory_order_release);while(readers.load(std::memory_order_acquire)!=0)std::this_thread::yield();if(impl->deviceReady){ma_device_uninit(&impl->device);impl->deviceReady=false;}}
