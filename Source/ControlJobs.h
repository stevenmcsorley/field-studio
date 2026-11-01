#pragma once
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <exception>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>

namespace field {
// One admitted control job, no unbounded pending queue. Never submit, query
// text, cancel(), or join from audio. invalidate() is bounded/atomic-only.
class ControlJobs {
public:
    ControlJobs():worker([this]{run();}){}
    ~ControlJobs(){shutdown();}
    ControlJobs(const ControlJobs&)=delete;
    ControlJobs& operator=(const ControlJobs&)=delete;
    bool submit(std::string label,std::function<void()> work,bool mutating=true){
        std::lock_guard<std::mutex> lock(mutex);
        if(stopping.load()||busyFlag.load())return false;
        pending=std::move(work);pendingLabel=std::move(label);pendingGeneration=next();
        gate.store(pendingGeneration,std::memory_order_release);mutatingFlag.store(mutating);
        statusText=pendingLabel;hasWork=true;busyFlag.store(true,std::memory_order_release);ready.notify_one();return true;
    }
    bool busy() const noexcept {return busyFlag.load(std::memory_order_acquire);}
    bool mutating() const noexcept {return mutatingFlag.load(std::memory_order_relaxed);}
    bool onWorker() const noexcept {return context.owner==this;}
    bool current() const noexcept {
        return !onWorker()||context.committed||(!stopping.load(std::memory_order_acquire)&&gate.load(std::memory_order_acquire)==context.generation);
    }
    bool beginCommit() noexcept {
        if(!onWorker()||context.committed)return true;
        if(stopping.load(std::memory_order_acquire))return false;
        auto expected=context.generation;
        if(!gate.compare_exchange_strong(expected,expected|committing,std::memory_order_acq_rel))return false;
        context.committed=true;return true;
    }
    // Linearizes cancellation against beginCommit. Once publication starts,
    // cancellation cannot undo it. The caller remains responsive either way.
    bool cancel() noexcept {
        if(!busy())return false;
        auto expected=gate.load(std::memory_order_acquire);
        while(!(expected&committing)){
            if(gate.compare_exchange_weak(expected,next(),std::memory_order_acq_rel))return true;
        }return false;
    }
    void invalidate() noexcept {gate.store(next(),std::memory_order_release);}
    void failCurrent(std::string message){if(onWorker())context.failure=std::move(message);}
    // Worker prepares data, then waits for a control-thread publication. Captured
    // references stay alive until serviceCommit completes or shutdown rejects it.
    // Never call either method from the audio callback.
    bool applyOnControl(std::function<void()> apply){
        if(!onWorker()||context.servicing){apply();return true;}
        if(context.committed)throw std::logic_error("A media job may publish only once");
        std::unique_lock<std::mutex> lock(mutex);
        if(stopping.load()||!current())return false;
        commitWork=std::move(apply);commitGeneration=context.generation;
        commitFinished=false;commitAccepted=false;commitFailure=nullptr;commitWaiting=true;
        ready.notify_all();ready.wait(lock,[this]{return commitFinished;});
        context.committed=commitAccepted;
        if(commitFailure)std::rethrow_exception(commitFailure);
        return commitAccepted;
    }
    void serviceCommit(){
        std::function<void()> apply;uint64_t generation=0;
        {std::lock_guard<std::mutex> lock(mutex);if(!commitWaiting)return;
            commitWaiting=false;generation=commitGeneration;auto expected=generation;
            if(stopping.load()||!gate.compare_exchange_strong(expected,generation|committing,std::memory_order_acq_rel)){
                commitWork={};commitFinished=true;ready.notify_all();return;
            }
            commitStarted=true;apply=std::move(commitWork);
        }
        auto previous=std::move(context);context={this,generation,true,{}};context.servicing=true;std::exception_ptr failure;
        try{apply();}catch(...){failure=std::current_exception();}
        context=std::move(previous);apply={};
        {std::lock_guard<std::mutex> lock(mutex);commitFailure=failure;commitAccepted=true;commitStarted=false;commitFinished=true;}
        ready.notify_all();
    }
    std::string status(){std::lock_guard<std::mutex> lock(mutex);return statusText;}
    void shutdown(){
        if(onWorker())std::terminate();
        if(!stopping.exchange(true)){
            invalidate();
            {std::lock_guard<std::mutex> lock(mutex);if(!commitStarted){commitWork={};commitWaiting=false;commitFinished=true;}}
            ready.notify_all();
        }
        if(worker.joinable())worker.join();
    }
private:
    static constexpr uint64_t committing=uint64_t{1}<<63,mask=committing-1;
    static_assert(std::atomic<uint64_t>::is_always_lock_free);
    struct Context {ControlJobs* owner=nullptr;uint64_t generation=0;bool committed=false;std::string failure;bool servicing=false;};
    static thread_local Context context;
    std::atomic<uint64_t> serial{0},gate{0};
    std::atomic<bool> busyFlag{false},mutatingFlag{false},stopping{false};
    std::mutex mutex;
    std::condition_variable ready;
    std::function<void()> pending;
    std::string pendingLabel,statusText;
    uint64_t pendingGeneration=0;
    bool hasWork=false;
    std::function<void()> commitWork;uint64_t commitGeneration=0;
    bool commitWaiting=false,commitStarted=false,commitFinished=false,commitAccepted=false;
    std::exception_ptr commitFailure;
    std::thread worker;
    uint64_t next() noexcept {return (serial.fetch_add(1,std::memory_order_relaxed)+1)&mask;}
    void run(){
        for(;;){
            std::function<void()> work;std::string label;uint64_t generation;
            {std::unique_lock<std::mutex> lock(mutex);ready.wait(lock,[this]{return hasWork||stopping.load();});
                if(stopping.load()){work=std::move(pending);hasWork=false;busyFlag=false;return;}
                work=std::move(pending);label=std::move(pendingLabel);generation=pendingGeneration;hasWork=false;
            }
            context={this,generation,false,{}};
            try {if(current())work();}catch(const std::exception& error){context.failure=error.what();}catch(...){context.failure="Unexpected background operation failure";}
            std::string result=!context.failure.empty()?label+": "+context.failure:!current()?label+" cancelled":label+" finished";
            auto expected=generation|committing;gate.compare_exchange_strong(expected,generation,std::memory_order_acq_rel);
            context={};work={};
            {std::lock_guard<std::mutex> lock(mutex);statusText=std::move(result);busyFlag.store(false,std::memory_order_release);}
        }
    }
};
inline thread_local ControlJobs::Context ControlJobs::context{};
}
