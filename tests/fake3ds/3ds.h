#pragma once
// Host-only NDSP/libctru model used by music_worker_test.cpp.
#include <atomic>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <thread>
#include <deque>
#include <vector>
#include <chrono>
#include <exception>
using s32=int32_t;using LightLock=std::mutex;
inline void LightLock_Init(LightLock*){}inline void LightLock_Lock(LightLock*p){p->lock();}inline void LightLock_Unlock(LightLock*p){p->unlock();}
constexpr int NDSP_WBUF_FREE=0,NDSP_WBUF_QUEUED=1,NDSP_WBUF_PLAYING=2,NDSP_WBUF_DONE=3,NDSP_INTERP_LINEAR=0,NDSP_FORMAT_MONO_PCM16=0,CUR_THREAD_HANDLE=0;
struct ndspWaveBuf{int16_t* data_pcm16=nullptr;unsigned nsamples=0,offset=0;bool looping=false;ndspWaveBuf*next=nullptr;std::atomic<int> status{0};};
struct MockDsp{std::mutex lock;std::deque<ndspWaveBuf*> queue;bool paused=true;float gain=1;std::vector<int16_t> rendered;unsigned consumed=0;};inline MockDsp mockDsp;
inline void* linearAlloc(size_t n){return malloc(n);}inline void linearFree(void*p){free(p);}inline void DSP_FlushDataCache(void*,size_t){}
inline void ndspChnReset(int){std::lock_guard<std::mutex>g(mockDsp.lock);mockDsp.queue.clear();mockDsp.paused=true;}
inline void ndspChnSetInterp(int,int){}inline void ndspChnSetFormat(int,int){}inline void ndspChnSetRate(int,float){}
inline void ndspChnSetPaused(int,bool pause){std::lock_guard<std::mutex>g(mockDsp.lock);mockDsp.paused=pause;}
inline void ndspChnSetMix(int,float mix[12]){std::lock_guard<std::mutex>g(mockDsp.lock);mockDsp.gain=mix[0];}
inline bool ndspChnIsPlaying(int){std::lock_guard<std::mutex>g(mockDsp.lock);return !mockDsp.paused&&!mockDsp.queue.empty();}
inline void ndspChnWaveBufAdd(int,ndspWaveBuf*b){std::lock_guard<std::mutex>g(mockDsp.lock);b->status=NDSP_WBUF_QUEUED;mockDsp.queue.push_back(b);}
inline void ndspChnWaveBufClear(int){std::lock_guard<std::mutex>g(mockDsp.lock);mockDsp.queue.clear();}
struct Thread_tag{std::thread t;};using Thread=Thread_tag*;
inline Thread threadCreate(void(*f)(void*),void*p,size_t,int,int,bool){auto*t=new Thread_tag;t->t=std::thread(f,p);return t;}
inline void threadJoin(Thread t,uint64_t){t->t.join();}inline void threadFree(Thread t){delete t;}
inline void svcGetThreadPriority(s32*p,int){*p=0x30;}inline void svcSleepThread(uint64_t n){std::this_thread::sleep_for(std::chrono::nanoseconds(n));}
enum APT_HookType{APTHOOK_ONSUSPEND,APTHOOK_ONRESTORE,APTHOOK_ONSLEEP,APTHOOK_ONWAKEUP,APTHOOK_ONEXIT};
struct aptHookCookie{void(*callback)(APT_HookType,void*)=nullptr;void*param=nullptr;};
inline void aptHook(aptHookCookie*c,void(*f)(APT_HookType,void*),void*p){c->callback=f;c->param=p;}inline void aptUnhook(aptHookCookie*){}
