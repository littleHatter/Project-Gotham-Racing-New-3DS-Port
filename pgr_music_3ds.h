#pragma once
#include <3ds.h>
#include "pgr_soundtrack.h"
#include <exception>
// Owns only NDSP channel 1. The game/runtime and its effects remain on channel 0.
// No guest state, framebuffer, or game callbacks are touched by this worker.
class PgrMusic3DS {
 static constexpr unsigned COUNT=12,SAMPLES=1024;
 LightLock lock;Thread worker=nullptr;aptHookCookie hook{};bool hooked=false;
 ndspWaveBuf buffers[COUNT]{};int16_t* pcm=nullptr;std::string directory,cue;
 uint32_t volume=256;bool paused=false,systemPaused=false,stopping=false;
 uint64_t blocks=0;unsigned underruns=0;
 struct Controls{std::string cue;uint32_t volume;bool paused,stop;};
 Controls Snapshot(){LightLock_Lock(&lock);Controls c{cue,volume,paused||systemPaused,stopping};LightLock_Unlock(&lock);return c;}
 static void Entry(void* arg){auto*self=static_cast<PgrMusic3DS*>(arg);try{self->Run();}catch(const std::exception& e){fprintf(stderr,"[Music3DS] Playback stopped: %s\n",e.what());}}
 static void AppEvent(APT_HookType event,void* arg){auto*self=static_cast<PgrMusic3DS*>(arg);LightLock_Lock(&self->lock);
  if(event==APTHOOK_ONSUSPEND||event==APTHOOK_ONSLEEP)self->systemPaused=true;
  else if(event==APTHOOK_ONRESTORE||event==APTHOOK_ONWAKEUP)self->systemPaused=false;
  else if(event==APTHOOK_ONEXIT)self->stopping=true;LightLock_Unlock(&self->lock);
 }
 void Run(){
  PgrSoundtrack music(directory);std::string applied;unsigned next=0;bool started=false,empty=false,lastPaused=true;uint32_t lastVolume=~0u;
  for(;;){auto controls=Snapshot();if(controls.stop)break;
   if(controls.cue!=applied){music.Cue(controls.cue);applied=controls.cue;controls=Snapshot();if(controls.stop)break;}
   if(controls.volume!=lastVolume){float mix[12]{};mix[0]=mix[1]=float(controls.volume)/256.0f;ndspChnSetMix(1,mix);lastVolume=controls.volume;}
   if(controls.paused!=lastPaused){ndspChnSetPaused(1,controls.paused||!started);lastPaused=controls.paused;}
   if(!controls.paused&&!applied.empty()){
    bool playing=started&&ndspChnIsPlaying(1);if(started&&!playing&&!empty){++underruns;empty=true;}if(playing)empty=false;
    for(unsigned n=0;n<COUNT;++n){auto& b=buffers[next];if(b.status==NDSP_WBUF_QUEUED||b.status==NDSP_WBUF_PLAYING)break;
     memset(b.data_pcm16,0,SAMPLES*2);music.Mix(b.data_pcm16,SAMPLES,256,false);DSP_FlushDataCache(b.data_pcm16,SAMPLES*2);
     b.nsamples=SAMPLES;b.offset=0;b.looping=false;b.next=nullptr;b.status=NDSP_WBUF_FREE;ndspChnWaveBufAdd(1,&b);++blocks;next=(next+1)%COUNT;
    }
    if(!started){started=true;ndspChnSetPaused(1,false);}
   }
   svcSleepThread(2000000);
  }
 }
public:
 explicit PgrMusic3DS(std::string path):directory(std::move(path)){LightLock_Init(&lock);}
 bool Init(){
  pcm=static_cast<int16_t*>(linearAlloc(COUNT*SAMPLES*2));if(!pcm){puts("[Music3DS] Unable to allocate music buffers");return false;}
  ndspChnReset(1);ndspChnSetInterp(1,NDSP_INTERP_LINEAR);ndspChnSetFormat(1,NDSP_FORMAT_MONO_PCM16);ndspChnSetRate(1,22050.0f);ndspChnSetPaused(1,true);
  for(unsigned i=0;i<COUNT;++i)buffers[i].data_pcm16=pcm+i*SAMPLES;
  s32 priority=0x30;svcGetThreadPriority(&priority,CUR_THREAD_HANDLE);worker=threadCreate(Entry,this,32*1024,std::max(0x18,int(priority)-1),-2,false);
  if(!worker){ndspChnReset(1);linearFree(pcm);pcm=nullptr;puts("[Music3DS] Unable to start music worker");return false;}
  aptHook(&hook,AppEvent,this);hooked=true;puts("[Music3DS] Independent music playback ready");return true;
 }
 void Cue(const std::string& name){LightLock_Lock(&lock);cue=name;LightLock_Unlock(&lock);}
 void SetControls(uint32_t master,bool pause){LightLock_Lock(&lock);volume=std::min<uint32_t>(master,256);paused=pause;LightLock_Unlock(&lock);}
 void Shutdown(){
  if(hooked){aptUnhook(&hook);hooked=false;}if(worker){LightLock_Lock(&lock);stopping=true;LightLock_Unlock(&lock);threadJoin(worker,UINT64_MAX);threadFree(worker);worker=nullptr;}
  if(pcm){ndspChnWaveBufClear(1);ndspChnReset(1);linearFree(pcm);pcm=nullptr;printf("[Music3DS] Output blocks=%llu underrun episodes=%u\n",static_cast<unsigned long long>(blocks),underruns);}
 }
 ~PgrMusic3DS(){Shutdown();}
};
