#pragma once
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
// Only the current and pending songs occupy memory. Mix never reads the SD card.
class PgrSoundtrack {
 struct Track{std::vector<int16_t> pcm;size_t pos=0;bool loop=true;};
 std::string directory,requested;Track current,pending;bool transition=false;int envelope=0;
 static constexpr int full=32768,fadeOut=5,fadeIn=9;
 static uint32_t le(const uint8_t*p){return uint32_t(p[0])|(uint32_t(p[1])<<8)|(uint32_t(p[2])<<16)|(uint32_t(p[3])<<24);}
 bool Load(const std::string& name,Track& out){
  if(name.empty()){out=Track{};return true;}FILE*f=fopen((directory+"/"+name+".wav").c_str(),"rb");if(!f){fprintf(stderr,"[Soundtrack] Missing %s.wav\n",name.c_str());return false;}
  uint8_t header[12];bool format=false,ok=fread(header,1,12,f)==12&&!memcmp(header,"RIFF",4)&&!memcmp(header+8,"WAVE",4);Track t;t.loop=name!="win";
  while(ok){uint8_t chunk[8];if(fread(chunk,1,8,f)!=8)break;uint32_t size=le(chunk+4);if(size>16u*1024*1024){ok=false;break;}
   if(!memcmp(chunk,"fmt ",4)){std::vector<uint8_t>b(size);ok=size>=16&&fread(b.data(),1,size,f)==size;if(ok)format=b[0]==1&&b[1]==0&&b[2]==1&&b[3]==0&&le(b.data()+4)==22050&&b[14]==16&&b[15]==0;}
   else if(!memcmp(chunk,"data",4)){if(!format||size%2){ok=false;break;}t.pcm.resize(size/2);ok=fread(t.pcm.data(),1,size,f)==size;break;}
   else if(fseek(f,size,SEEK_CUR)){ok=false;break;}if(size&1)if(fseek(f,1,SEEK_CUR)){ok=false;break;}
  }fclose(f);if(!ok||t.pcm.empty()){fprintf(stderr,"[Soundtrack] Invalid PCM WAV: %s\n",name.c_str());return false;}out=std::move(t);return true;
 }
public:
 explicit PgrSoundtrack(std::string path):directory(std::move(path)){}
 void Cue(const std::string& name){
  if(name==requested)return;requested=name;Track next;if(!Load(name,next))next=Track{};
  fprintf(stdout,"[Soundtrack] Cue %s\n",name.empty()?"silence":name.c_str());
  if(current.pcm.empty()||envelope==0){current=std::move(next);pending=Track{};transition=false;envelope=0;}
  else{pending=std::move(next);transition=true;}
 }
 void Mix(int16_t* effects,size_t count,uint32_t volume,bool paused){
  if(paused)return;
  for(size_t i=0;i<count;++i){
   if(transition){envelope=std::max(0,envelope-fadeOut);if(!envelope){current=std::move(pending);pending=Track{};transition=false;}}
   else envelope=std::min(full,envelope+fadeIn);
   if(current.pcm.empty())continue;
   if(current.pos==current.pcm.size()){if(current.loop)current.pos=0;else continue;}
   int64_t sample=int64_t(current.pcm[current.pos++])*std::min<uint32_t>(volume,256)*envelope*6/(256ll*full*5);
   effects[i]=int16_t(std::clamp<int64_t>(effects[i]+sample,-32768,32767));
  }
 }
};
