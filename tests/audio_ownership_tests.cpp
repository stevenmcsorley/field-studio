#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <new>
#include <thread>
#ifdef _MSC_VER
#include <malloc.h>
#endif
namespace {
thread_local bool inCallback=false;
std::atomic<size_t> allocations{0},frees{0};
void check(bool ok,const char* message){if(!ok){std::cerr<<"FAIL: "<<message<<'\n';std::exit(1);}}
template<class Predicate>void waitFor(Predicate predicate,const char* message){auto end=std::chrono::steady_clock::now()+std::chrono::seconds(15);while(!predicate()){check(std::chrono::steady_clock::now()<end,message);std::this_thread::sleep_for(std::chrono::milliseconds(1));}}
void fill(juce::AudioBuffer<float>& block){for(int i=0;i<block.getNumSamples();++i){block.setSample(0,i,.17f);block.setSample(1,i,-.11f);}}
void process(FieldProcessor& p,juce::AudioBuffer<float>& block,juce::MidiBuffer& events){inCallback=true;p.processBlock(block,events);inCallback=false;}
struct Driver {
 FieldProcessor& processor;juce::AudioBuffer<float> block;juce::MidiBuffer events;std::atomic<bool> stopping{false},valid{true};std::atomic<uint64_t> calls{0};std::thread thread;
 Driver(FieldProcessor& p,int frames):processor(p),block(2,frames){events.ensureSize(65536);}
 void start(){thread=std::thread([this]{while(!stopping.load()){fill(block);events.clear();process(processor,block,events);float power=0;for(int c=0;c<2;++c)for(int i=0;i<block.getNumSamples();++i){const auto value=block.getSample(c,i);if(!std::isfinite(value))valid=false;power+=std::abs(value);}if(power<.0001f)valid=false;++calls;std::this_thread::sleep_for(std::chrono::microseconds(80));}});}
 void stop(){stopping=true;if(thread.joinable())thread.join();}
 ~Driver(){stop();}
};
void wave(const juce::File& path,float sample,int frames=512,double rate=8000){juce::AudioBuffer<float> audio(2,frames);for(int c=0;c<2;++c)for(int i=0;i<frames;++i)audio.setSample(c,i,sample);juce::WavAudioFormat format;auto stream=path.createOutputStream();std::unique_ptr<juce::AudioFormatWriter> writer(format.createWriterFor(stream.release(),rate,2,24,{},0));check(writer&&writer->writeFromAudioSampleBuffer(audio,0,frames),"write sample fixture");}
}
void* operator new(size_t size){if(inCallback)++allocations;if(auto* pointer=std::malloc(size?size:1))return pointer;throw std::bad_alloc();}
void* operator new[](size_t size){return ::operator new(size);}
void operator delete(void* pointer)noexcept{if(pointer&&inCallback)++frees;std::free(pointer);}
void operator delete[](void* pointer)noexcept{::operator delete(pointer);}
void operator delete(void* pointer,size_t)noexcept{::operator delete(pointer);}
void operator delete[](void* pointer,size_t)noexcept{::operator delete(pointer);}
void* operator new(size_t size,std::align_val_t alignment){if(inCallback)++allocations;
#ifdef _MSC_VER
 if(auto* pointer=_aligned_malloc(size?size:1,(size_t)alignment))return pointer;
#else
 void* pointer=nullptr;if(posix_memalign(&pointer,std::max((size_t)alignment,sizeof(void*)),size?size:1)==0)return pointer;
#endif
 throw std::bad_alloc();}
void* operator new[](size_t size,std::align_val_t alignment){return ::operator new(size,alignment);}
void operator delete(void* pointer,std::align_val_t)noexcept{if(pointer&&inCallback)++frees;
#ifdef _MSC_VER
 _aligned_free(pointer);
#else
 std::free(pointer);
#endif
}
void operator delete[](void* pointer,std::align_val_t alignment)noexcept{::operator delete(pointer,alignment);}
void operator delete(void* pointer,size_t,std::align_val_t alignment)noexcept{::operator delete(pointer,alignment);}
void operator delete[](void* pointer,size_t,std::align_val_t alignment)noexcept{::operator delete(pointer,alignment);}
int main(){
 juce::ScopedJuceInitialiser_GUI gui;const auto directory=juce::File::getSpecialLocation(juce::File::tempDirectory).getNonexistentChildFile("field-owned-audio",{},false);check(directory.createDirectory().wasOk(),"create ownership fixtures");
 const auto sampleA=directory.getChildFile("a.wav"),sampleB=directory.getChildFile("b.wav");wave(sampleA,.30f);wave(sampleB,-.20f,1024,16000);
 // The released callback must ignore a control mutex held by another thread,
 // including sample-accurate MIDI release while it remains held.
 for(double rate:{44100.,48000.,96000.,192000.})for(int frames:{32,64,512}){
  auto owner=std::make_unique<FieldProcessor>();auto& p=*owner;p.prepareToPlay(rate,frames);p.inputMonitor=true;p.setValue("midiThru",1);juce::AudioBuffer<float> block(2,frames);juce::MidiBuffer events;events.ensureSize(65536);fill(block);p.processBlock(block,events);
  std::atomic<bool> held{false},release{false};std::thread control([&]{const juce::ScopedLock lock(p.engineLock);held=true;while(!release.load())std::this_thread::yield();});waitFor([&]{return held.load();},"control guard not held");allocations=0;frees=0;bool nonzero=true,forwarded=false;
  for(int iteration=0;iteration<120;++iteration){fill(block);events.clear();if(iteration==0)events.addEvent(juce::MidiMessage::noteOn(1,60,.5f),0);if(iteration==1)events.addEvent(juce::MidiMessage::noteOff(1,60),0);process(p,block,events);if(iteration==1)for(const auto event:events)forwarded|=event.getMessage().isNoteOff();float power=0;for(int c=0;c<2;++c)for(int i=0;i<frames;++i){const float value=block.getSample(c,i);nonzero&=std::isfinite(value);power+=std::abs(value);}nonzero&=power>.0001f;}
  release=true;control.join();check(nonzero&&forwarded&&p.skippedBlocks==0,"control contention interrupted audio or MIDI");check(allocations==0&&frees==0,"steady owned callback allocated or freed");
 }
 // Verify the real production callback -> telemetry -> editor path. Passing
 // an artificial phase directly to the renderer cannot catch a frozen preview.
 for(int type:{0,4}){auto owner=std::make_unique<FieldProcessor>();auto& p=*owner;p.prepareToPlay(8000,128);p.setValue("lfo",(float)type);p.setValue("lfoEnabled",1);p.setValue("lfoModel",1);p.setValue("lfoRate",2);p.setValue("lfoSync",0);p.setValue("lfoRetrigger",0);p.setValue("lfoDepth",.4f);std::unique_ptr<juce::AudioProcessorEditor> editor(p.createEditor());auto* view=dynamic_cast<FieldEditor*>(editor.get());check(view,"live LFO preview has no editor");editor->setSize(1320,640);view->showPageForTesting(0,3);juce::AudioBuffer<float> audio(2,128);juce::MidiBuffer events;events.ensureSize(65536);for(int i=0;i<320;++i){audio.clear();events.clear();process(p,audio,events);}view->refreshLiveForTesting();const auto first=editor->createComponentSnapshot(editor->getLocalBounds());for(int i=0;i<11;++i){audio.clear();events.clear();process(p,audio,events);}view->refreshLiveForTesting();const auto next=editor->createComponentSnapshot(editor->getLocalBounds());const juce::Rectangle<int> region=type==0?juce::Rectangle<int>(398,156,50,25):juce::Rectangle<int>(353,158,40,20);int changed=0;for(int y=region.getY();y<region.getBottom();++y)for(int x=region.getX();x<region.getRight();++x)changed+=first.getPixelAt(x,y)!=next.getPixelAt(x,y);check(changed>5,"production callback did not animate the top-left LFO waveform in the real editor");}
 // Compound parameter publication remains coherent while the control thread
 // is paused halfway through a batch. Audio keeps its prior complete values.
 {auto owner=std::make_unique<FieldProcessor>();auto& p=*owner;p.prepareToPlay(8000,128);p.inputMonitor=true;juce::AudioBuffer<float> block(2,128);juce::MidiBuffer events;events.ensureSize(65536);for(int i=0;i<40;++i){fill(block);events.clear();p.processBlock(block,events);}const float before=block.getSample(0,127);std::atomic<bool> halfway{false},finish{false};std::thread writer([&]{FieldProcessor::DocumentGuard document(p);p.setValue("masterLeft",.2f);halfway=true;while(!finish.load())std::this_thread::yield();p.setValue("masterRight",.2f);});waitFor([&]{return halfway.load();},"parameter batch did not pause");for(int i=0;i<16;++i){fill(block);events.clear();process(p,block,events);check(std::abs(block.getSample(0,127)-before)<.0001f,"audio observed half a parameter batch");}finish=true;writer.join();fill(block);events.clear();process(p,block,events);check(std::abs(block.getSample(0,127)-before*.2f)<.0001f,"parameter batch did not publish");}
 // Existing synth/effect algorithms must produce the same audio as the prior
 // offline engine path. This is a behavioral comparison, not a UI screenshot.
 for(int synth:{0,5,9,11,12})for(int effect:{0,1,3,7,9,10}){
  auto live=std::make_unique<FieldProcessor>();auto offline=std::make_unique<FieldProcessor>(juce::File{},true);for(auto* p:{live.get(),offline.get()}){p->prepareToPlay(48000,128);p->setValue("engine",(float)synth);p->setValue("effect",(float)effect);p->setValue("masterEffect",(float)effect);p->setValue("effectMix",.4f);p->setValue("masterMix",.3f);if(synth==11){juce::String error;check(p->loadSampleFile(sampleA,error),"golden sample import");p->setValue("sampleLoop",1);}}
  juce::AudioBuffer<float> a(2,128),b(2,128);juce::MidiBuffer ea,eb;ea.ensureSize(65536);eb.ensureSize(65536);
  for(int block=0;block<36;++block){fill(a);fill(b);ea.clear();eb.clear();if(block==0){ea.addEvent(juce::MidiMessage::noteOn(1,60,.8f),0);eb.addEvent(juce::MidiMessage::noteOn(1,60,.8f),0);}if(block==15){ea.addEvent(juce::MidiMessage::pitchWheel(1,9000),35);eb.addEvent(juce::MidiMessage::pitchWheel(1,9000),35);}process(*live,a,ea);offline->processBlock(b,eb);for(int c=0;c<2;++c)for(int i=0;i<128;++i)check(std::abs(a.getSample(c,i)-b.getSample(c,i))<1e-6f,"ownership migration changed synth/effect audio");live->serviceForTesting();}
 }
 // A control-side edit must preserve unrelated controller changes that arrive
 // while its document is being prepared, including outgoing tape-bank state.
 {auto owner=std::make_unique<FieldProcessor>();auto& p=*owner;p.prepareToPlay(8000,128);juce::AudioBuffer<float> block(2,128);juce::MidiBuffer events;events.ensureSize(65536);
  check(p.beginMidiLearn("track1Pan",field::EncoderMode::absolute,false,false),"arm pan mapping");events.addEvent(juce::MidiMessage::controllerEvent(1,19,64),0);fill(block);process(p,block,events);p.serviceForTesting();
  std::atomic<bool> halfway{false},finish{false};std::thread writer([&]{FieldProcessor::DocumentGuard guard(p);p.engine.setTrackLevel(0,.4f);halfway=true;while(!finish.load())std::this_thread::yield();});waitFor([&]{return halfway.load();},"mixer edit did not pause");
  events.clear();events.addEvent(juce::MidiMessage::controllerEvent(1,19,110),17);fill(block);process(p,block,events);finish=true;writer.join();events.clear();fill(block);process(p,block,events);p.serviceForTesting();
  {FieldProcessor::DocumentGuard guard(p,false);check(std::abs(p.engine.trackLevel(0)-.4f)<1e-6f,"prepared mixer level lost");check(p.engine.trackPan(0)>.7f,"prepared level erased concurrent MIDI pan");}
  p.selectTape(1);fill(block);process(p,block,events);p.serviceForTesting();p.selectTape(0);fill(block);process(p,block,events);p.serviceForTesting();
  {FieldProcessor::DocumentGuard guard(p,false);check(std::abs(p.engine.trackLevel(0)-.4f)<1e-6f&&p.engine.trackPan(0)>.7f,"bank switch lost live mixer state");}
 }
 // Capture finalization must include the last acknowledged audio block,
 // rather than flattening an older control-side snapshot of the recording.
 {auto owner=std::make_unique<FieldProcessor>();auto& p=*owner;p.prepareToPlay(8000,128);p.setValue("captureThreshold",0);p.startSampleCapture(false);Driver driver(p,128);driver.start();waitFor([&]{return driver.calls>=20;},"capture callback did not start");p.stopSampleCapture();driver.stop();p.serviceForTesting();const auto sample=p.samplePreviewSnapshot();check(sample&&sample->left.size()>=20*128,"capture finalization lost recorded tail");check(!p.sampling.load(),"capture did not stop");}
 // Exercise the production owner with real saves, exports, imports, edits,
 // history, bank changes and project replacement while callbacks continue.
 for(double rate:{44100.,48000.,96000.,192000.})for(int frames:{32,64,512}){
  auto owner=std::make_unique<FieldProcessor>();auto& p=*owner;p.prepareToPlay(rate,frames);p.inputMonitor=true;p.setValue("volume",1);
  {FieldProcessor::DocumentGuard document(p);p.engine.tracks()[0].audio.assign((size_t)(rate*.08),.12f,.09f);p.engine.setLoop(0,.08);p.engine.setPlaying(true);p.selectionStart=.01;p.selectionEnd=.03;}
  Driver driver(p,frames);allocations=0;frees=0;driver.start();waitFor([&]{return driver.calls>=8;},"owned callback did not start");juce::String error;const auto project=directory.getChildFile("live.field"),mix=directory.getChildFile("live.wav");check(p.saveProject(project,error),"save live project");
  for(int iteration=0;iteration<5;++iteration){check(p.loadSampleFile(iteration%2?sampleA:sampleB,error),"import while rendering");p.requestLiveControl(field::LiveControl::pan,iteration%4,0,.2);p.transport("copy");p.transport("lift");p.transport("undo");p.transport("redo");p.transport("undo");check(p.exportAudio(mix,error),"export while rendering");check(p.saveProject(project,error),"save while rendering");p.selectTape(1);p.selectTape(0);check(p.loadProject(project,error),"load while rendering");p.serviceForTesting();}
  waitFor([&]{return driver.calls>=100;},"live work starved callbacks");driver.stop();p.serviceForTesting();check(driver.valid&&p.skippedBlocks==0,"media/control operations interrupted the pilot");check(allocations==0&&frees==0,"media commit allocated or destroyed an owner on audio");
 }
 directory.deleteRecursively();std::cout<<"Audio ownership tests passed\n";return 0;
}
