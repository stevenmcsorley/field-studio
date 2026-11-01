#include "PluginProcessor.h"
#include <algorithm>
#include <cstring>
#include <chrono>
#include <thread>

thread_local const FieldProcessor* FieldProcessor::renderingOwner=nullptr;
FieldProcessor::DocumentGuard::DocumentGuard(FieldProcessor& p,bool writable):owner(p){owner.engineLock.enter();if(owner.documentGuardDepth++==0)try{owner.beginDocument(writable);}catch(...){if(owner.controlGateRaised){owner.parameterWriteGate.fetch_add(1,std::memory_order_release);owner.controlGateRaised=false;}--owner.documentGuardDepth;owner.engineLock.exit();throw;}}
FieldProcessor::DocumentGuard::~DocumentGuard(){if(--owner.documentGuardDepth==0)try{owner.endDocument();}catch(const std::exception& error){try{owner.abortDocument();owner.reportOperationError(error.what());}catch(...){}}catch(...){try{owner.abortDocument();owner.reportOperationError("Document preparation failed");}catch(...){}}owner.engineLock.exit();}
std::shared_ptr<const FieldProcessor::ControlRoot> FieldProcessor::makeControlRoot(){
 auto root=std::make_shared<ControlRoot>();root->tapes=tapes;for(size_t i=0;i<4;++i)root->cuts[i]=engine.tracks()[i].cuts;for(size_t bank=0;bank<8;++bank){BankCuts cuts;if((int)bank==currentTape)cuts=root->cuts;else for(size_t i=0;i<4;++i)cuts[i]=tapes[bank].tracks[i].cuts;if(!bankCutsCache[bank]||*bankCutsCache[bank]!=cuts)bankCutsCache[bank]=std::make_shared<const BankCuts>(std::move(cuts));root->bankCuts[bank]=bankCutsCache[bank];}root->slots=patchSlots;root->memos=memos;root->sample=sampleData;root->asset=engine.sharedCurrentSample();root->history=engine.editingHistory();root->metadata=importedPresetMetadata;root->project=projectName;root->sampleName=sampleName;root->summary=lastImportSummary;root->details=lastImportDetails;root->inputName=externalSourceName;root->epoch=documentEpoch;root->tape=currentTape;root->program=currentProgram;root->sampleRate=sampleRateFile;projectName.copyToUTF8(root->projectLabel.data(),root->projectLabel.size());externalSourceName.copyToUTF8(root->sourceLabel.data(),root->sourceLabel.size());return root;
}
void FieldProcessor::captureModel(DocumentSnapshot& snapshot){
 snapshot.document=engine.document();for(size_t i=0;i<4;++i){const auto& source=engine.tracks()[i];snapshot.tracks[i]={source.audio,source.level,source.pan,source.mute,source.solo};}snapshot.albums=albums;snapshot.capture=captureAudio;snapshot.root=makeControlRoot();snapshot.sampling=documentSampling;snapshot.captureTriggered=captureTriggered;snapshot.captureFromOutput=captureFromOutput;snapshot.albumRecording=documentAlbumRecording;snapshot.albumPlaying=documentAlbumPlaying;snapshot.albumLimit=albumLimit;snapshot.captureLimit=captureLimit;snapshot.albumHead=albumHead;snapshot.captureRate=captureRate;snapshot.loopStart=selectionStart;snapshot.loopEnd=selectionEnd;snapshot.generation=submittedGeneration;
}
void FieldProcessor::applyModel(const DocumentSnapshot& snapshot){
 if(!snapshot.root)return;const auto& root=*snapshot.root;engine.applyDocument(snapshot.document,field::Engine::allDocumentParts);engine.retainSharedSample(root.asset);
 for(size_t i=0;i<4;++i){auto& destination=engine.tracks()[i];const auto& source=snapshot.tracks[i];destination.audio=source.audio;destination.level=source.level;destination.pan=source.pan;destination.mute=source.mute;destination.solo=source.solo;destination.cuts=root.cuts[i];}
 tapes=root.tapes;for(size_t bank=0;bank<8;++bank)if((int)bank!=root.tape){const auto& incoming=snapshot.banks[bank];tapes[bank].position=incoming.position;for(size_t i=0;i<4;++i){auto& out=tapes[bank].tracks[i];const auto& source=incoming.tracks[i];out.audio=source.audio;out.level=source.level;out.pan=source.pan;out.mute=source.mute;out.solo=source.solo;if(incoming.cuts)out.cuts=(*incoming.cuts)[i];}}albums=snapshot.albums;captureAudio=snapshot.capture;documentSampling=snapshot.sampling;captureTriggered=snapshot.captureTriggered;captureFromOutput=snapshot.captureFromOutput;documentAlbumRecording=snapshot.albumRecording;documentAlbumPlaying=snapshot.albumPlaying;albumLimit=snapshot.albumLimit;captureLimit=snapshot.captureLimit;albumHead=snapshot.albumHead;captureRate=snapshot.captureRate;selectionStart=snapshot.loopStart;selectionEnd=snapshot.loopEnd;
 // Metadata/history belong to an acknowledged document generation. Ordinary
 // recording snapshots update media without resetting the editor's history.
 if(currentTape!=root.tape||projectName!=root.project||documentEpoch!=root.epoch||sampleData!=root.sample||patchSlots!=root.slots||memos!=root.memos||lastAppliedRoot!=snapshot.root.get()){
  patchSlots=root.slots;memos=root.memos;sampleData=root.sample;importedPresetMetadata=root.metadata;projectName=root.project;sampleName=root.sampleName;lastImportSummary=root.summary;lastImportDetails=root.details;externalSourceName=root.inputName;currentTape=root.tape;currentProgram=root.program;documentEpoch=root.epoch;sampleRateFile=root.sampleRate;if(root.history)engine.restoreEditingHistory(*root.history);lastAppliedRoot=snapshot.root.get();refreshSlotNameCache();
 }
}
void FieldProcessor::pollDocuments(){
 PreparedCommit* completed=nullptr;
 while(completedCommits.pop(completed)){
  processedGeneration=std::max(processedGeneration,completed->generation);
  if(completed->acknowledged.frames>=lastDocumentSnapshot.frames)lastDocumentSnapshot=std::move(completed->acknowledged);
  if(completed->accepted&&completed->captureChanged&&!completed->sampling&&!completed->capture.empty()){stoppedCapture=std::move(completed->capture);stoppedCaptureGeneration=completed->generation;stoppedCaptureEpoch=lastDocumentSnapshot.root?lastDocumentSnapshot.root->epoch:documentEpoch;}
  if(!completed->accepted)reportOperationError("Prepared change rejected: live media or its document changed. Existing audio was kept.");
  commitOwners.erase(std::remove_if(commitOwners.begin(),commitOwners.end(),[&](const auto& item){return item.get()==completed;}),commitOwners.end());
 }
 documentSnapshots.consume([&](const DocumentSnapshot& snapshot){if(snapshot.frames>=lastDocumentSnapshot.frames)lastDocumentSnapshot=snapshot;});
 if(processedGeneration>=submittedGeneration&&!pendingCommit.load(std::memory_order_acquire)&&!stagedCommit)applyModel(lastDocumentSnapshot);
}
void FieldProcessor::beginDocument(bool writable){
 if(offlineFixture){documentSampling=sampling.load();documentAlbumRecording=albumRecording.load();documentAlbumPlaying=albumPlaying.load();}
 documentReadOnly=!writable;controlGateRaised=false;if(offlineFixture||!ownershipReady)return;
 pollDocuments();collectOwnedDocuments();if(!writable)return;parameterWriteGate.fetch_add(1,std::memory_order_acq_rel);controlGateRaised=true;
 if(auto* pending=pendingCommit.exchange(nullptr,std::memory_order_acq_rel)){
  auto found=std::find_if(commitOwners.begin(),commitOwners.end(),[&](const auto& item){return item.get()==pending;});if(found!=commitOwners.end()){stagedCommit=std::move(*found);commitOwners.erase(found);}
 }
 controlBefore=std::make_unique<DocumentSnapshot>();captureModel(*controlBefore);
}
void FieldProcessor::endDocument(){
 if(offlineFixture){sampling=documentSampling;albumRecording=documentAlbumRecording;albumPlaying=documentAlbumPlaying;return;}
 if(!ownershipReady||documentReadOnly)return;
 struct EndGate{std::atomic<unsigned>& gate;bool& raised;~EndGate(){gate.fetch_add(1,std::memory_order_release);raised=false;}} endGate{parameterWriteGate,controlGateRaised};
 DocumentSnapshot after;captureModel(after);const auto& before=*controlBefore;unsigned parts=field::Engine::documentChanges(before.document,after.document),media=0,mix=0;
 for(unsigned i=0;i<4;++i){const auto& a=before.tracks[i];const auto& b=after.tracks[i];if(!a.audio.sameGeneration(b.audio)||before.root->cuts[i]!=after.root->cuts[i])media|=1u<<i;if(a.level!=b.level||a.pan!=b.pan||a.mute!=b.mute||a.solo!=b.solo)mix|=1u<<i;}
 const bool selection=before.loopStart!=after.loopStart||before.loopEnd!=after.loopEnd;
 const bool full=before.root->epoch!=after.root->epoch;
 const bool capture=before.sampling!=after.sampling||before.captureFromOutput!=after.captureFromOutput||!before.capture.sameGeneration(after.capture)||before.captureLimit!=after.captureLimit;
 bool album=before.albumRecording!=after.albumRecording||before.albumPlaying!=after.albumPlaying||before.albumHead!=after.albumHead;for(size_t i=0;i<2;++i)album=album||!before.albums[i].audio.sameGeneration(after.albums[i].audio);
 const auto& a=*before.root;const auto& b=*after.root;
 bool metadata=a.project!=b.project||a.sampleName!=b.sampleName||a.summary!=b.summary||a.details!=b.details||a.inputName!=b.inputName||a.sample!=b.sample||a.asset!=b.asset||a.slots!=b.slots||a.memos!=b.memos||a.metadata!=b.metadata||a.program!=b.program||a.tape!=b.tape;
 for(size_t bank=0;bank<8&&!metadata;++bank){if(a.tapes[bank].position!=b.tapes[bank].position)metadata=true;for(size_t i=0;i<4&&!metadata;++i){const auto& x=a.tapes[bank].tracks[i];const auto& y=b.tapes[bank].tracks[i];metadata=!x.audio.sameGeneration(y.audio)||x.cuts!=y.cuts||x.level!=y.level||x.pan!=y.pan||x.mute!=y.mute||x.solo!=y.solo;}}
 if(!parts&&!media&&!mix&&!capture&&!album&&!full&&!metadata&&!selection&&!controlSignals){
  if(stagedCommit){auto* pointer=stagedCommit.get();commitOwners.push_back(std::move(stagedCommit));pendingCommit.store(pointer,std::memory_order_release);}controlBefore.reset();return;
 }
 const bool bankChanged=before.root->tape!=after.root->tape;
 const bool existing=bool(stagedCommit);auto packet=existing?std::move(stagedCommit):std::make_unique<PreparedCommit>();
 if(!existing){packet->baseGeneration=processedGeneration>=submittedGeneration?lastDocumentSnapshot.generation:submittedGeneration;packet->generation=submittedGeneration+1;packet->epoch=before.root->epoch;packet->baseTracks=before.tracks;packet->baseDocument=before.document;}
 if(bankChanged){parts|=field::Engine::seekPart|field::Engine::playPart|field::Engine::recordPart;media=15;mix=15;packet->mixFields.fill(15);}
 packet->selectionChanged|=selection;
 for(size_t i=0;i<4;++i){const auto& a=before.tracks[i];const auto& b=after.tracks[i];if(a.level!=b.level)packet->mixFields[i]|=1;if(a.pan!=b.pan)packet->mixFields[i]|=2;if(a.mute!=b.mute)packet->mixFields[i]|=4;if(a.solo!=b.solo)packet->mixFields[i]|=8;}
 for(size_t i=0;i<2;++i)if(!before.albums[i].audio.sameGeneration(after.albums[i].audio))packet->albumMask|=1u<<i;
 packet->albumTransportChanged|=before.albumRecording!=after.albumRecording||before.albumPlaying!=after.albumPlaying||before.albumHead!=after.albumHead;
 packet->liveEpoch=liveEpoch.load(std::memory_order_acquire);packet->parts|=parts;packet->mediaMask|=media;packet->mixMask|=mix;packet->signals|=controlSignals;controlSignals=0;packet->fullReset|=full;packet->captureChanged|=capture;packet->albumsChanged|=album;packet->rate=engine.sampleRate();packet->document=after.document;packet->tracks=after.tracks;packet->albums=after.albums;packet->capture=after.capture;packet->root=std::move(after.root);packet->sampling=after.sampling;packet->captureFromOutput=after.captureFromOutput;packet->captureTriggered=after.captureTriggered;packet->albumRecording=after.albumRecording;packet->albumPlaying=after.albumPlaying;packet->albumHead=after.albumHead;packet->albumLimit=after.albumLimit;packet->captureLimit=after.captureLimit;packet->captureRate=after.captureRate;packet->loopStart=after.loopStart;packet->loopEnd=after.loopEnd;
 for(int key=0;key<P_count;++key)packet->parameters[(size_t)key]=rawPhysical(key);
 if(packet->root->asset&&std::find(retainedRenderSamples.begin(),retainedRenderSamples.end(),packet->root->asset)==retainedRenderSamples.end())retainedRenderSamples.push_back(packet->root->asset);
 if(packet->fullReset){packet->synth=std::make_unique<field::Engine>();packet->synth->prepare(packet->rate,std::max(512,getBlockSize()));packet->synth->applyDocument(packet->document,field::Engine::allDocumentParts);for(size_t i=0;i<4;++i){auto& target=packet->synth->tracks()[i];target.audio=packet->tracks[i].audio;target.level=packet->tracks[i].level;target.pan=packet->tracks[i].pan;target.mute=packet->tracks[i].mute;target.solo=packet->tracks[i].solo;}packet->synth->adoptSharedSample(packet->root->asset.get());packet->master=std::make_unique<field::Engine>();packet->master->prepare(packet->rate,512);packet->master->setOutputGainImmediately(1);}
 after.root=packet->root;retainSnapshotOwners(after);if(std::find(retainedControlRoots.begin(),retainedControlRoots.end(),packet->root)==retainedControlRoots.end())retainedControlRoots.push_back(packet->root);
 auto* pointer=packet.get();commitOwners.push_back(std::move(packet));submittedGeneration=pointer->generation;pendingCommit.store(pointer,std::memory_order_release);controlBefore.reset();
}
void FieldProcessor::fillDocumentSnapshot(DocumentSnapshot& snapshot) const noexcept {
 const auto& r=renderDocument;snapshot.document=r.synth->document();for(size_t i=0;i<4;++i){const auto& source=r.synth->tracks()[i];snapshot.tracks[i]={source.audio,source.level,source.pan,source.mute,source.solo};}snapshot.albums=r.albums;snapshot.banks=r.banks;snapshot.capture=r.capture;snapshot.root=r.root;snapshot.sampling=r.sampling;snapshot.captureTriggered=r.captureTriggered;snapshot.captureFromOutput=r.captureFromOutput;snapshot.albumRecording=r.albumRecording;snapshot.albumPlaying=r.albumPlaying;snapshot.albumLimit=r.albumLimit;snapshot.captureLimit=r.captureLimit;snapshot.albumHead=r.albumHead;snapshot.captureRate=r.captureRate;snapshot.loopStart=r.loopStart;snapshot.loopEnd=r.loopEnd;snapshot.generation=r.generation;snapshot.frames=renderedFrames;
}
void FieldProcessor::publishDocument() noexcept {documentSnapshots.publish([this](DocumentSnapshot& snapshot){fillDocumentSnapshot(snapshot);});}
void FieldProcessor::acknowledgeDocument() noexcept {if(!renderAcknowledgement)return;auto* packet=renderAcknowledgement;fillDocumentSnapshot(packet->acknowledged);const auto generation=packet->generation;renderAcknowledgement=nullptr;completedGeneration.store(generation,std::memory_order_release);completedCommits.push(packet);}
void FieldProcessor::installPreparedCommit() noexcept {
 auto* packet=pendingCommit.exchange(nullptr,std::memory_order_acq_rel);if(!packet)return;auto& r=renderDocument;
 bool valid=packet->liveEpoch==liveEpoch.load(std::memory_order_acquire)&&packet->baseGeneration==r.generation&&packet->rate==r.synth->sampleRate()&&r.root&&packet->epoch==r.root->epoch;
 if(valid&&!packet->fullReset)for(unsigned i=0;i<4;++i)if((packet->mediaMask&(1u<<i))&&!r.synth->tracks()[i].audio.sameGeneration(packet->baseTracks[i].audio))valid=false;
 packet->accepted=valid;
 if(valid){
  if(parameterWriteGate.load(std::memory_order_acquire)&1u)renderValues=packet->parameters;
  if(packet->fullReset){
   r.synth.swap(packet->synth);r.master.swap(packet->master);masterState={};midiNotes.resetAll();guiMidiNotes.resetAll();midiClockFollower.reset();midiClockGenerator.reset();midiPositionValid=false;hostPositionValid=false;controllerValues.fill(-1);
   for(size_t bank=0;bank<8;++bank){r.banks[bank].cuts=packet->root->bankCuts[bank];r.banks[bank].position=packet->root->tapes[bank].position;for(size_t i=0;i<4;++i){const auto& t=packet->root->tapes[bank].tracks[i];r.banks[bank].tracks[i]={t.audio,t.level,t.pan,t.mute,t.solo};}}
  }else{
   if(r.root->tape!=packet->root->tape){auto& old=r.banks[(size_t)r.root->tape];old.position=r.synth->positionSeconds();old.cuts=r.root->bankCuts[(size_t)r.root->tape];for(size_t i=0;i<4;++i){const auto& t=r.synth->tracks()[i];old.tracks[i]={t.audio,t.level,t.pan,t.mute,t.solo};}}
   for(unsigned i=0;i<4;++i){auto& target=r.synth->tracks()[i];auto& incoming=packet->tracks[i];if(packet->mediaMask&(1u<<i))std::swap(target.audio,incoming.audio);const auto fields=packet->mixFields[i];if(fields&1)target.level=incoming.level;if(fields&2)target.pan=incoming.pan;if(fields&4)target.mute=incoming.mute;if(fields&8)target.solo=incoming.solo;}
   r.synth->setCountInBeats((int)renderValues[P_countInBeats]);r.synth->applyDocumentChanges(packet->baseDocument,packet->document,packet->parts);r.synth->adoptSharedSample(packet->root->asset.get());
  }
  if(packet->fullReset){r.synth->setCountInBeats((int)renderValues[P_countInBeats]);if(packet->document.recording)r.synth->applyDocument(packet->document,field::Engine::recordPart);}
  if(packet->captureChanged||packet->fullReset){std::swap(r.capture,packet->capture);r.sampling=packet->sampling;r.captureFromOutput=packet->captureFromOutput;r.captureTriggered=packet->captureTriggered;r.captureLimit=packet->captureLimit;r.captureRate=packet->captureRate;captureAutoStop=false;captureArmed=r.sampling&&!r.captureTriggered;captureSeconds=r.capture.size()/r.captureRate;}
  for(size_t side=0;side<2;++side)if(packet->fullReset||(packet->albumMask&(1u<<side)))std::swap(r.albums[side],packet->albums[side]);
  if(packet->albumTransportChanged||packet->fullReset){r.albumRecording=packet->albumRecording;r.albumPlaying=packet->albumPlaying;r.albumHead=packet->albumHead;r.albumLimit=packet->albumLimit;}
  if(packet->signals&1u){r.synth->allNotesOff();midiNotes.resetAll();guiMidiNotes.resetAll();}
  if(packet->selectionChanged||packet->fullReset){r.loopStart=packet->loopStart;r.loopEnd=packet->loopEnd;}r.root.swap(packet->root);r.generation=packet->generation;
 }

 // Rejected successors must still be acknowledged. Generation remains the
 // last accepted document; a dependent change then fails rather than rebases.
 renderAcknowledgement=packet;
}
void FieldProcessor::initializeRenderDocument(){
 renderDocument={};renderDocument.synth=std::make_unique<field::Engine>();renderDocument.synth->prepare(engine.sampleRate(),std::max(512,getBlockSize()));renderDocument.synth->applyDocument(engine.document(),field::Engine::allDocumentParts);for(size_t i=0;i<4;++i){const auto& source=engine.tracks()[i];auto& target=renderDocument.synth->tracks()[i];target.audio=source.audio;target.level=source.level;target.pan=source.pan;target.mute=source.mute;target.solo=source.solo;}renderDocument.root=makeControlRoot();renderDocument.synth->adoptSharedSample(renderDocument.root->asset.get());if(renderDocument.root->asset)retainedRenderSamples.push_back(renderDocument.root->asset);renderDocument.master=std::make_unique<field::Engine>();renderDocument.master->prepare(engine.sampleRate(),512);renderDocument.master->setOutputGainImmediately(1);for(size_t bank=0;bank<8;++bank){renderDocument.banks[bank].cuts=renderDocument.root->bankCuts[bank];renderDocument.banks[bank].position=tapes[bank].position;for(size_t i=0;i<4;++i){const auto& t=tapes[bank].tracks[i];renderDocument.banks[bank].tracks[i]={t.audio,t.level,t.pan,t.mute,t.solo};}}renderDocument.albums=albums;renderDocument.capture=captureAudio;renderDocument.loopStart=selectionStart;renderDocument.loopEnd=selectionEnd;documentSnapshots.reset();lastDocumentSnapshot={};submittedGeneration=processedGeneration=renderedFrames=0;completedGeneration=0;renderAcknowledgement=nullptr;pendingCommit=nullptr;commitOwners.clear();stagedCommit.reset();ownershipReady=true;retainedControlRoots.push_back(renderDocument.root);DocumentSnapshot pinned;captureModel(pinned);retainSnapshotOwners(pinned);for(int key=0;key<P_count;++key)renderValues[(size_t)key]=rawPhysical(key);fillDocumentSnapshot(lastDocumentSnapshot);
}
void FieldProcessor::retainSnapshotOwners(const DocumentSnapshot& snapshot){
 auto retain=[&](const field::TapeAudio& audio){auto pool=audio.poolHandle();if(pool&&std::find(retainedRenderPools.begin(),retainedRenderPools.end(),pool)==retainedRenderPools.end())retainedRenderPools.push_back(std::move(pool));};
 for(const auto& track:snapshot.tracks)retain(track.audio);for(const auto& side:snapshot.albums)retain(side.audio);retain(snapshot.capture);if(snapshot.root){for(const auto& bank:snapshot.root->tapes)for(const auto& track:bank.tracks)retain(track.audio);for(const auto& cuts:snapshot.root->bankCuts)if(cuts&&std::find(retainedBankCuts.begin(),retainedBankCuts.end(),cuts)==retainedBankCuts.end())retainedBankCuts.push_back(cuts);}
 for(const auto& bank:snapshot.banks)for(const auto& track:bank.tracks)retain(track.audio);
}
void FieldProcessor::collectOwnedDocuments(){
 retainedControlRoots.erase(std::remove_if(retainedControlRoots.begin(),retainedControlRoots.end(),[](const auto& root){return root.use_count()==1;}),retainedControlRoots.end());
 retainedBankCuts.erase(std::remove_if(retainedBankCuts.begin(),retainedBankCuts.end(),[](const auto& cuts){return cuts.use_count()==1;}),retainedBankCuts.end());
 retainedRenderPools.erase(std::remove_if(retainedRenderPools.begin(),retainedRenderPools.end(),[](const auto& pool){return pool.use_count()==1;}),retainedRenderPools.end());
 retainedRenderSamples.erase(std::remove_if(retainedRenderSamples.begin(),retainedRenderSamples.end(),[](const auto& sample){return field::Engine::sampleReferences(*sample)==0&&sample.use_count()==1;}),retainedRenderSamples.end());for(auto& track:lastDocumentSnapshot.tracks)track.audio.collect(64);for(auto& album:lastDocumentSnapshot.albums)album.audio.collect(64);lastDocumentSnapshot.capture.collect(64);}
void FieldProcessor::releaseResources(){if(offlineFixture||!ownershipReady)return;const juce::ScopedLock lock(engineLock);fillDocumentSnapshot(lastDocumentSnapshot);applyModel(lastDocumentSnapshot);}
void FieldProcessor::flushLiveControls(){if(offlineFixture){updateEngine();for(int i=0;i<16;++i)applyLiveCommands();publishControlUiState();}}

field::UiTelemetry FieldProcessor::uiState(){return uiTelemetry.read();}
field::UiTelemetry FieldProcessor::makeRenderTelemetry() const {return makeUiTelemetry(true);}

void FieldProcessor::abortDocument(){
 if(offlineFixture)return;stagedCommit.reset();if(controlBefore&&controlBefore->root)applyModel(*controlBefore);controlBefore.reset();controlSignals=0;submittedGeneration=processedGeneration;for(const auto& owner:commitOwners)submittedGeneration=std::max(submittedGeneration,owner->generation);
}

bool FieldProcessor::awaitDocument(uint64_t generation){
 if(offlineFixture||!ownershipReady)return true;
 const auto started=std::chrono::steady_clock::now();const double blockMs=actualBlockSize.load()/std::max(8000.,preparedRate.load())*1000.;const auto idleLimit=std::chrono::milliseconds((int)std::max(12.,blockMs*2+2));
 uint64_t frame=0;{DocumentGuard guard(*this,false);frame=lastDocumentSnapshot.frames;}
 auto progress=started;
 while(std::chrono::steady_clock::now()-started<std::chrono::milliseconds(250)){
  {DocumentGuard guard(*this,false);if(processedGeneration>=generation)return true;if(lastDocumentSnapshot.frames!=frame){frame=lastDocumentSnapshot.frames;progress=std::chrono::steady_clock::now();}}
  if(std::chrono::steady_clock::now()-progress>=idleLimit)break;std::this_thread::sleep_for(std::chrono::milliseconds(1));
 }
 return false; // A paused host keeps the prepared state pending until resume.
}
void FieldProcessor::stopRecordingForEdit(){
 if(offlineFixture||documentGuardDepth>0)return;
 bool recording;{DocumentGuard guard(*this,false);recording=engine.recording();}if(!recording)return;
 {DocumentGuard guard(*this);engine.setRecording(false);}
 uint64_t generation;{const juce::ScopedLock guard(engineLock);generation=submittedGeneration;}awaitDocument(generation);
}
