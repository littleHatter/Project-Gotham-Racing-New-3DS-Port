#include "pgr_soundtrack.h"
#include <cassert>
#include <filesystem>
#include <fstream>
static void wav(const std::string& path,int16_t sample,unsigned count){
 std::ofstream f(path,std::ios::binary);auto w=[&](uint32_t n,unsigned bytes){for(unsigned i=0;i<bytes;++i)f.put(char(n>>(i*8)));};
 f.write("RIFF",4);w(36+count*2,4);f.write("WAVEfmt ",8);w(16,4);w(1,2);w(1,2);w(22050,4);w(44100,4);w(2,2);w(16,2);f.write("data",4);w(count*2,4);for(unsigned i=0;i<count;++i)w(uint16_t(sample),2);
}
int main(int argc,char**argv){assert(argc==2);std::string dir=argv[1];std::filesystem::create_directories(dir);wav(dir+"/title.wav",1000,100);wav(dir+"/paris.wav",-1000,100);wav(dir+"/win.wav",1000,100);
 auto mix=[](PgrSoundtrack&p,unsigned n,uint32_t v=256,bool pause=false){std::vector<int16_t>b(n);p.Mix(b.data(),n,v,pause);return b;};
 PgrSoundtrack p(dir);p.Cue("title");auto b=mix(p,10000);assert(b.back()==1200);assert(mix(p,1000,128).back()==600);for(auto v:mix(p,1000,0))assert(v==0);for(auto v:mix(p,1000,256,true))assert(v==0);assert(mix(p,1000).back()==1200);
 p.Cue("paris");b=mix(p,12000);assert(b.front()>0&&b.back()==-1200);bool nearZero=false;for(auto v:b)if(v==0)nearZero=true;assert(nearZero);
 PgrSoundtrack win(dir);win.Cue("win");b=mix(win,12000);bool audible=false;for(unsigned i=0;i<100;++i)audible|=b[i]!=0;assert(audible);for(unsigned i=100;i<b.size();++i)assert(b[i]==0);win.Cue("win");for(auto v:mix(win,3000))assert(v==0);win.Cue("title");assert(mix(win,14000).back()==1200);
 puts("PASS: music looping, one-shot results silence, no retrigger, fade ordering, master volume, mute and pause.");
}
