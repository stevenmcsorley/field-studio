#include "Engine.h"
#include <cstring>
#include <new>

namespace field {
struct Engine::EditingHistory {std::deque<Snapshot> undos,redos;std::array<Track,4> clipboard;unsigned mask=0;size_t frames=0;};
std::shared_ptr<const Engine::EditingHistory> Engine::editingHistory() const {auto result=std::make_shared<EditingHistory>();result->undos=undos;result->redos=redos;result->clipboard=clipboard;result->mask=clipboardMask;result->frames=clipboardFrames;return result;}
void Engine::restoreEditingHistory(const EditingHistory& value){undos=value.undos;redos=value.redos;clipboard=value.clipboard;clipboardMask=value.mask;clipboardFrames=value.frames;}

Engine::~Engine(){allNotesOff();if(currentSample){currentSample->references.fetch_sub(1,std::memory_order_release);currentSample=nullptr;}}
Engine& Engine::operator=(Engine&& other) {if(this!=&other){this->~Engine();new(this) Engine(std::move(other));}return *this;}
std::shared_ptr<Engine::PreparedSample> Engine::sharedCurrentSample() const {for(const auto& asset:sampleAssets)if(asset.get()==currentSample)return asset;return {};}
void Engine::retainSharedSample(const std::shared_ptr<PreparedSample>& sample){if(sample&&std::none_of(sampleAssets.begin(),sampleAssets.end(),[&](const auto& existing){return existing==sample;}))sampleAssets.push_back(sample);adoptSharedSample(sample.get());}
void Engine::adoptSharedSample(PreparedSample* sample) noexcept {if(sample==currentSample)return;if(sample)sample->references.fetch_add(1,std::memory_order_relaxed);if(currentSample)currentSample->references.fetch_sub(1,std::memory_order_release);currentSample=sample;}
Engine::Document Engine::document() const noexcept {
 Document d;d.pads=pads;d.dbox=dboxPads;d.steps=steps;d.velocities=stepVelocity;d.chords=stepChords;d.sketch=sketch;
 for(size_t i=0;i<16;++i){d.finger[i].chords=fingerBanks[i].chords;d.finger[i].notes=fingerBanks[i].note;d.finger[i].velocities=fingerBanks[i].velocity;d.finger[i].length=fingerBanks[i].length;}
 d.selected=selected;d.pad=padSelected;d.length=seqLength;d.bank=fingerSelected;d.sketchLength=sketchCount;d.position=position/sr;d.loopStart=loopBegin/sr;d.loopEnd=loopFinish/sr;d.playing=isPlaying;d.recording=isRecording;return d;
}
unsigned Engine::documentChanges(const Document& a,const Document& b) noexcept {
 unsigned parts=0;if(a.position!=b.position)parts|=seekPart;if(a.loopStart!=b.loopStart||a.loopEnd!=b.loopEnd)parts|=loopPart;if(a.selected!=b.selected)parts|=trackPart;if(a.playing!=b.playing)parts|=playPart;if(a.recording!=b.recording)parts|=recordPart;
 if(a.pad!=b.pad||std::memcmp(a.pads.data(),b.pads.data(),sizeof(a.pads))||std::memcmp(a.dbox.data(),b.dbox.data(),sizeof(a.dbox)))parts|=padsPart;
 bool sequence=a.length!=b.length||a.bank!=b.bank||a.sketchLength!=b.sketchLength||a.steps!=b.steps||a.velocities!=b.velocities||a.chords!=b.chords||std::memcmp(a.sketch.data(),b.sketch.data(),sizeof(a.sketch));
 for(size_t i=0;i<16&&!sequence;++i)sequence=a.finger[i].length!=b.finger[i].length||a.finger[i].chords!=b.finger[i].chords||a.finger[i].notes!=b.finger[i].notes||a.finger[i].velocities!=b.finger[i].velocities;
 if(sequence)parts|=sequencePart;return parts;
}
void Engine::applyDocument(const Document& d,unsigned parts) noexcept {
 if(parts&trackPart){isRecording=false;selected=std::clamp(d.selected,0,3);}if(parts&seekPart)seek(d.position);if(parts&loopPart)setLoop(d.loopStart,d.loopEnd);if(parts&playPart)setPlaying(d.playing);
 if(parts&recordPart){isRecording=d.recording;if(d.recording){isPlaying=true;countInRemaining=countInSetting;countInElapsed=0;}else countInRemaining=0;}
 if(parts&padsPart){pads=d.pads;dboxPads=d.dbox;padSelected=std::clamp(d.pad,0,23);}
 if(parts&sequencePart){steps=d.steps;stepVelocity=d.velocities;stepChords=d.chords;sketch=d.sketch;seqLength=d.length;fingerSelected=d.bank;sketchCount=d.sketchLength;for(size_t i=0;i<16;++i){fingerBanks[i].chords=d.finger[i].chords;fingerBanks[i].note=d.finger[i].notes;fingerBanks[i].velocity=d.finger[i].velocities;fingerBanks[i].length=d.finger[i].length;}if(seqStep>=seqLength){seqStep=-1;seqPositionPending=-1;seqPhase=0;}}
}
void Engine::observeRuntime(int step,const std::array<bool,128>& notes) noexcept {seqStep=step;heldCount=0;for(int note=0;note<128&&heldCount<32;++note)if(notes[(size_t)note]){heldNotes[(size_t)heldCount]=note;heldVelocities[(size_t)heldCount++]=.8f;}}
}

namespace field {
void Engine::applyDocumentChanges(const Document& before,const Document& after,unsigned parts) noexcept {
 applyDocument(after,parts&~(padsPart|sequencePart));
 if(parts&padsPart){
  for(size_t i=0;i<24;++i){const auto& a=before.pads[i];const auto& b=after.pads[i];auto& out=pads[i];
#define FIELD_PATCH(member) if(a.member!=b.member)out.member=b.member
   FIELD_PATCH(start);FIELD_PATCH(end);FIELD_PATCH(tune);FIELD_PATCH(level);FIELD_PATCH(pan);FIELD_PATCH(reverse);FIELD_PATCH(mode);FIELD_PATCH(chokeGroup);
#undef FIELD_PATCH
   const auto& x=before.dbox[i];const auto& y=after.dbox[i];auto& target=dboxPads[i];
#define FIELD_PATCH(member) if(x.member!=y.member)target.member=y.member
   FIELD_PATCH(mix);FIELD_PATCH(wave1);FIELD_PATCH(envelope1);FIELD_PATCH(crossMod);FIELD_PATCH(frequency);FIELD_PATCH(wave2);FIELD_PATCH(envelope2);FIELD_PATCH(cutoff);FIELD_PATCH(attack);FIELD_PATCH(gain);FIELD_PATCH(release);FIELD_PATCH(smooth);
#undef FIELD_PATCH
  }
  if(before.pad!=after.pad)padSelected=after.pad;
 }
 if(parts&sequencePart){
  for(size_t i=0;i<999;++i)if(before.steps[i]!=after.steps[i]||before.chords[i]!=after.chords[i]||before.velocities[i]!=after.velocities[i]){steps[i]=after.steps[i];stepChords[i]=after.chords[i];stepVelocity[i]=after.velocities[i];}
  for(size_t bank=0;bank<16;++bank){const auto& a=before.finger[bank];const auto& b=after.finger[bank];auto& out=fingerBanks[bank];if(a.length!=b.length)out.length=b.length;for(size_t step=0;step<32;++step)if(a.notes[step]!=b.notes[step]||a.chords[step]!=b.chords[step]||a.velocities[step]!=b.velocities[step]){out.note[step]=b.notes[step];out.chords[step]=b.chords[step];out.velocity[step]=b.velocities[step];}}
  for(size_t i=0;i<256;++i)if(before.sketch[i].x!=after.sketch[i].x||before.sketch[i].y!=after.sketch[i].y)sketch[i]=after.sketch[i];
  if(before.length!=after.length)setSequenceLength(after.length);if(before.bank!=after.bank)fingerSelected=after.bank;if(before.sketchLength!=after.sketchLength)sketchCount=after.sketchLength;
 }
}
}
