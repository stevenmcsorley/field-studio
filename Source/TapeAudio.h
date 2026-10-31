#pragma once
#include "TapeStorage.h"
#include <memory>
#include <vector>

namespace field {
// Stereo tape value. Copies retain immutable pages; control operations reserve
// resources before the audio owner writes. Never grow a pool in process().
class TapeAudio {
public:
    TapeAudio() = default;
    TapeAudio(const TapeAudio&) = default;
    TapeAudio(TapeAudio&&) noexcept = default;
    TapeAudio& operator=(const TapeAudio&);
    TapeAudio& operator=(TapeAudio&&) noexcept;
    size_t size() const noexcept { return (size_t)data.size(); }
    bool empty() const noexcept { return data.empty(); }
    float read(int channel,size_t frame) const noexcept { return data.read(channel,frame); }
    std::array<float,2> readStereo(size_t frame) const noexcept { return data.readStereo(frame); }
    float interpolate(int channel,double frame) const noexcept;
    // Control-only. Ensures enough spare resources for a complete COW pass.
    void prepareWrite(size_t frames);
    void resize(size_t frames);
    void clear() noexcept { data.clear(); }
    void assign(size_t frames,float left,float right);
    void import(const std::vector<float>& left,const std::vector<float>& right);
    std::vector<float> exportChannel(int channel) const;
    void resample(double ratio,size_t frameLimit);
    void silence(size_t begin,size_t end);
    void write(size_t frame,float left,float right);
    // Audio-only: bounded, no allocation/free/locks. False leaves stereo intact.
    bool tryWrite(size_t frame,float left,float right) noexcept { return data.writeStereo(frame,left,right); }
    size_t collect(size_t budget=256) noexcept { return pool?pool->collect(budget):0; }
    uint64_t rootIdentity() const noexcept { return data.rootIdentity(); }
    tape::TapePool::Stats stats() const noexcept { return pool?pool->stats():tape::TapePool::Stats{}; }
private:
    // Order matters: buffers must release their roots before the pool dies.
    std::shared_ptr<tape::TapePool> pool;
    tape::TapeBuffer data;
};
}
