#include "TapeAudio.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace field {
TapeAudio& TapeAudio::operator=(const TapeAudio& other){
    if(this!=&other){auto owner=other.pool;data=other.data;pool=std::move(owner);}return *this;
}
TapeAudio& TapeAudio::operator=(TapeAudio&& other) noexcept {
    if(this!=&other){auto owner=std::move(other.pool);data=std::move(other.data);pool=std::move(owner);}return *this;
}
float TapeAudio::interpolate(int channel,double frame) const noexcept {
    if(!std::isfinite(frame)||frame<0||frame>=size())return 0;
    const size_t at=(size_t)frame,next=std::min(at+1,size()-1);
    const float a=read(channel,at);return a+(read(channel,next)-a)*(float)(frame-at);
}
void TapeAudio::prepareWrite(size_t frames){
    frames=std::max(frames,size());if(frames>tape::maxTapeFrames)throw std::length_error("Tape exceeds six-minute maximum at 192 kHz");
    const size_t pages=(frames+tape::pageFrames-1)/tape::pageFrames;
    if(!pages)return;
    if(pool){pool->collect();const auto available=pool->stats();if(available.freePages>=pages+1&&available.freeNodes>=pages*3+3)return;}
    // A fresh pool contains the current generation plus a full COW generation.
    auto replacement=std::make_shared<tape::TapePool>(pages*2+16,pages*6+64);
    tape::TapeBuffer next(*replacement);
    if(!next.resizeLogical(size()))throw std::bad_alloc();
    for(size_t f=0;f<size();++f){const auto value=readStereo(f);if((value[0]!=0||value[1]!=0)&&!next.writeStereo(f,value[0],value[1]))throw std::bad_alloc();}
    data=std::move(next);pool=std::move(replacement);
}
void TapeAudio::resize(size_t frames){prepareWrite(frames);if(!data.resizeLogical(frames))throw std::bad_alloc();}
void TapeAudio::write(size_t frame,float left,float right){if(!tryWrite(frame,left,right))throw std::bad_alloc();}
void TapeAudio::assign(size_t frames,float left,float right){TapeAudio next;next.prepareWrite(frames);next.resize(frames);if(left!=0||right!=0)for(size_t f=0;f<frames;++f)next.write(f,left,right);*this=std::move(next);}
void TapeAudio::import(const std::vector<float>& left,const std::vector<float>& right){
    if(left.size()!=right.size())throw std::invalid_argument("Tape channels do not match");
    TapeAudio next;next.prepareWrite(left.size());next.resize(left.size());for(size_t f=0;f<left.size();++f)if(left[f]!=0||right[f]!=0)next.write(f,left[f],right[f]);*this=std::move(next);
}
std::vector<float> TapeAudio::exportChannel(int channel) const {std::vector<float> result(size());for(size_t f=0;f<size();++f)result[f]=read(channel,f);return result;}
void TapeAudio::resample(double ratio,size_t frameLimit){if(empty())return;TapeAudio next;const size_t frames=(size_t)std::min((double)frameLimit,std::ceil(size()*ratio));next.prepareWrite(frames);next.resize(frames);for(size_t f=0;f<frames;++f)next.write(f,interpolate(0,f/ratio),interpolate(1,f/ratio));*this=std::move(next);}
void TapeAudio::silence(size_t begin,size_t end){end=std::min(end,size());if(begin>=end)return;prepareWrite(size());for(size_t f=begin;f<end;++f)write(f,0,0);}
}

namespace field {
TapeAudio TapeAudio::preparedCopy(size_t frames) const {TapeAudio next;next.prepareWrite(std::max(frames,size()));if(!next.data.resizeLogical(size()))throw std::bad_alloc();for(size_t i=0;i<size();++i){const auto value=readStereo(i);if(value[0]!=0||value[1]!=0)next.write(i,value[0],value[1]);}return next;}
}
