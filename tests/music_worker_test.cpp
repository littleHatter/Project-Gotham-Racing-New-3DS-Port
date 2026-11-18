#include "pgr_music_3ds.h"
#include <cassert>
int main(int argc,char**argv){
 assert(argc==2);std::atomic<bool> done=false;std::thread dsp([&]{
  using clock=std::chrono::steady_clock;auto last=clock::now();
  while(!done){std::this_thread::sleep_for(std::chrono::milliseconds(2));auto now=clock::now();std::lock_guard<std::mutex> guard(mockDsp.lock);
   if(mockDsp.paused||mockDsp.queue.empty()){last=now;continue;}
   if(now-last<std::chrono::microseconds(46440))continue;last=now;auto*b=mockDsp.queue.front();mockDsp.queue.pop_front();for(unsigned i=0;i<b->nsamples;++i)mockDsp.rendered.push_back(int16_t(b->data_pcm16[i]*mockDsp.gain));++mockDsp.consumed;b->status=NDSP_WBUF_DONE;
  }
 });
 {PgrMusic3DS worker(argv[1]);assert(worker.Init());worker.Cue("title");std::this_thread::sleep_for(std::chrono::milliseconds(200));
  unsigned before;{std::lock_guard<std::mutex>g(mockDsp.lock);before=mockDsp.consumed;}
  // Simulate a game thread blocked in resource loading. Only the DSP and the
  // real music worker run during this interval; no game/frame/audio callbacks.
  std::this_thread::sleep_for(std::chrono::milliseconds(800));
  {std::lock_guard<std::mutex>g(mockDsp.lock);assert(mockDsp.consumed>=before+14);bool audible=false;for(auto s:mockDsp.rendered)audible|=s!=0;assert(audible);}
  worker.SetControls(256,true);std::this_thread::sleep_for(std::chrono::milliseconds(30));{std::lock_guard<std::mutex>g(mockDsp.lock);before=mockDsp.consumed;}
  std::this_thread::sleep_for(std::chrono::milliseconds(150));{std::lock_guard<std::mutex>g(mockDsp.lock);assert(mockDsp.consumed==before);}
  worker.SetControls(128,false);std::this_thread::sleep_for(std::chrono::milliseconds(100));{std::lock_guard<std::mutex>g(mockDsp.lock);assert(mockDsp.gain==.5f);assert(mockDsp.consumed>before);}
  worker.SetControls(0,false);std::this_thread::sleep_for(std::chrono::milliseconds(100));{std::lock_guard<std::mutex>g(mockDsp.lock);assert(mockDsp.gain==0);for(size_t i=mockDsp.rendered.size()-1024;i<mockDsp.rendered.size();++i)assert(mockDsp.rendered[i]==0);}
  worker.Shutdown();
 }
 done=true;dsp.join();puts("PASS: actual music worker continues across blocked game callbacks; pause, resume, volume, mute and shutdown pass (modeled NDSP).");
}
