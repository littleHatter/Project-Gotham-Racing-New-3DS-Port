#include "loader.h"
#include "pgr_controls.h"
#include "pgr_presentation.h"
#include "runtime_s3e.h"
#include "desktop_audio_sink.h"
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm>
#include <cstring>
int main(int argc,char**argv){
 setvbuf(stdout,nullptr,_IONBF,0);
 std::string assets=argc>1?argv[1]:"game",error;
 auto cpu=Cpu::CreateUnicorn(&error);if(!cpu){fprintf(stderr,"%s\n",error.c_str());return 2;}
 S3EBinary bin;if(!LoadS3E(assets+"/pgr.xe3u",bin)||!PreparePgrControls(bin)||!SetupSandbox(cpu.get(),bin,std::getenv("PAN_IMAGE_BASE")?std::strtoul(std::getenv("PAN_IMAGE_BASE"),nullptr,0):STOCK_BASE,64u<<20))return 2;
 for(const auto& p:bin.pointer_fixups){uint32_t actual=0;cpu->MemRead(G(bin.base_addr+p.offset),&actual,4);if(actual!=TRAMPOLINE_BASE+p.func_idx*8){fprintf(stderr,"Type-4 pointer verification failed\n");return 2;}}
 printf("[Probe] Type-4 function pointers patched: %zu\n",bin.pointer_fixups.size());
 auto signedBits=[](uint32_t value,unsigned bits)->int32_t{return int32_t(value<<(32-bits))>>(32-bits);};
 for(const auto& f:bin.arm_fixups){uint32_t insn=0;uint32_t address=G(bin.base_addr+f.offset);cpu->MemRead(address,&insn,4);uint32_t target=address+8+signedBits(insn&0xffffff,24)*4;if(target!=TRAMPOLINE_BASE+f.func_idx*8){fprintf(stderr,"[Verify] ARM import target mismatch\n");return 2;}}
 for(const auto& f:bin.thumb_fixups){uint16_t insn[2]{};uint32_t address=G(bin.base_addr+f.offset);cpu->MemRead(address,insn,4);uint32_t target=((address+4)&~3u)+signedBits(((insn[0]&0x7ff)<<11)|(insn[1]&0x7ff),22)*2;if(target!=TRAMPOLINE_BASE+f.func_idx*8){fprintf(stderr,"[Verify] Thumb import target mismatch\n");return 2;}}
 for(uint32_t offset:bin.relocations){if(offset+4>bin.code_and_data.size()){fprintf(stderr,"[Verify] relocation outside image\n");return 2;}uint32_t original=0,actual=0;std::memcpy(&original,bin.code_and_data.data()+offset,4);cpu->MemRead(G(bin.base_addr+offset),&actual,4);if(actual!=G(original)){fprintf(stderr,"[Verify] absolute relocation mismatch\n");return 2;}}
 fprintf(stderr,"[Verify] ARM=%zu Thumb=%zu pointers=%zu absolute=%zu all targets verified\n",bin.arm_fixups.size(),bin.thumb_fixups.size(),bin.pointer_fixups.size(),bin.relocations.size());
 if(std::getenv("PAN_MUSIC_TRACE"))cpu->AddCodeHook([](Cpu*c,uint64_t a,uint32_t,void*){uint8_t state[64]{};c->MemRead(G(0x4a1d3840),state,sizeof(state));uint32_t flags=0,lr=0;c->RegRead(14,&lr);std::memcpy(&flags,state+0x3c,4);fprintf(stderr,"[MusicSelection] pc=%08x lr=%08x track=%u variant=%u format=%u flags=%08x\n",(uint32_t)a,lr,state[0x18],state[0x19],(flags>>18)&7,flags);},nullptr,G(0x4a011c78),G(0x4a011c78));
 struct StateStore {uint32_t address=0,pc=0,lr=0;int size=0;uint64_t value=0;};
 struct StateRing {std::array<StateStore,256> stores{};uint64_t count=0;uint32_t state=0;};StateRing stateRing;
 if(getenv("PGR_STATE_WRITE_RING"))cpu->AddWriteHook([](Cpu*c,uint64_t,int,int64_t value,void* user){
  auto* ring=static_cast<StateRing*>(user);uint32_t state=uint32_t(value);if(!state||state==ring->state)return;ring->state=state;
  c->AddWriteHook([](Cpu*c,uint64_t addr,int size,int64_t value,void* user){auto* ring=static_cast<StateRing*>(user);StateStore event;event.address=uint32_t(addr);event.size=size;event.value=uint64_t(value);c->RegRead(Cpu::REG_PC,&event.pc);c->RegRead(Cpu::REG_LR,&event.lr);ring->stores[ring->count++%ring->stores.size()]=event;},ring,state,state+127);
 },&stateRing,G(0x4a0c4154),G(0x4a0c4154)+3);
 if(getenv("PGR_RENDER_TRACE")){
  cpu->AddWriteHook([](Cpu*c,uint64_t,int,int64_t value,void*){
   uint32_t pointer=uint32_t(value),pc=0;c->RegRead(Cpu::REG_PC,&pc);fprintf(stderr,"[RenderState] global=%08x pc=%08x\n",pointer,pc);
   if(pointer>=HEAP_BASE&&pointer<HEAP_BASE+HEAP_SIZE)c->AddWriteHook([](Cpu*c,uint64_t addr,int size,int64_t value,void*){
    static unsigned count=0;uint32_t pc=0,lr=0;c->RegRead(Cpu::REG_PC,&pc);c->RegRead(Cpu::REG_LR,&lr);
    if(count++<40||(uint32_t(value)&&(uint32_t(value)<HEAP_BASE||uint32_t(value)>=HEAP_BASE+HEAP_SIZE)))fprintf(stderr,"[MaterialWrite] address=%08x size=%d value=%08x pc=%08x lr=%08x\n",uint32_t(addr),size,uint32_t(value),pc,lr);
   },nullptr,pointer+4,pointer+7);
  },nullptr,G(0x4a0c4154),G(0x4a0c4154)+3);
 }
 uint32_t frameLimit=argc>2?std::stoul(argv[2]):2;
 uint32_t suspendFrame=getenv("PAN_TEST_SUSPEND_FRAME")?std::stoul(getenv("PAN_TEST_SUSPEND_FRAME")):0;
 std::string output=argc>3?argv[3]:"frames",saveDir=argc>4?argv[4]:"probe_user";
 std::filesystem::create_directories(output);
 struct Input {uint32_t begin,end,key;};std::vector<Input> input;
 if(argc>5){std::ifstream f(argv[5]);Input e;while(f>>e.begin>>e.end>>e.key){if(e.key>=256||e.end<e.begin)return 2;input.push_back(e);}}
 S3ERuntime rt(cpu.get(),assets,saveDir);
 std::string config=bin.config;
 if(std::getenv("PAN_TEST_UNLOCK_LEVELS")){
  fprintf(stderr,"[Diagnostic] Enabling the original game's UnlockAllLevels configuration for asset testing\n");
  config+="\n{OS=NGI}\n[Pandy]\nUnlockAllLevels=1\n";
 }
 if(const char* lang=getenv("PAN_TEST_LANGUAGE"))config+="\n{OS=NGI}\n[Pandy]\nDefaultLanguage="+std::to_string(std::stoul(lang))+"\n";
 rt.Init(bin.imports,config);
 if(getenv("PAN_TEST_VIDEO"))rt.SetVideoPlayer([](const std::string& path,const S3ERuntime::VideoRequest& request){fprintf(stderr,"[DesktopVideo] %s rect=%u,%u,%u,%u repeat=%u volume=%u\n",path.c_str(),request.left,request.top,request.width,request.height,request.repeat,request.volume);return std::atoi(getenv("PAN_TEST_VIDEO"));});
 DesktopAudioSink audioSink;
 std::ofstream pcm(output+"/audio_s16le_22050.raw",std::ios::binary);
 rt.SetAudioCallback([&](const int16_t* samples,size_t count,uint32_t rate){audioSink.Push(samples,count,rate);pcm.write(reinterpret_cast<const char*>(samples),count*2);});
 std::ofstream musicStem,effectsStem;
 if(std::getenv("PAN_AUDIO_STEMS")){musicStem.open(output+"/music_stream_s16le_22050.raw",std::ios::binary);effectsStem.open(output+"/effects_mix_s16le_22050.raw",std::ios::binary);rt.SetChannelAudioCallback([&](uint32_t ch,const int16_t* data,size_t count,uint32_t){if(ch==16)musicStem.write(reinterpret_cast<const char*>(data),count*2);if(ch==17)effectsStem.write(reinterpret_cast<const char*>(data),count*2);});}
 PgrPresentation presentation(getenv("PGR_MUSIC_DIR")?getenv("PGR_MUSIC_DIR"):assets+"/../music");
 rt.SetExternalMusicCallback([&](int16_t* data,size_t count,uint32_t volume,bool paused){presentation.Mix(data,count,volume,paused);});
 rt.SetFrameCallback([&](uint32_t n,const uint16_t* data,S3ERuntime::Keys& keys){
 presentation.Update(cpu.get(),rt,data);if(getenv("PGR_PRESENTATION_TRACE")){static unsigned previous=~0u;if(previous!=presentation.prompts){previous=presentation.prompts;fprintf(stdout,"[Prompt] frame=%u mask=%u city=%s\n",n,previous,rt.RaceCity().c_str());}}

 if(const char* marker=getenv("PGR_STOP_FILE")){if(std::filesystem::exists(marker)){std::filesystem::remove(marker);rt.RequestQuit();}}
 if(suspendFrame&&n>=suspendFrame){suspendFrame=0;rt.RequestSuspendAndStop();}
 if(const char* marker=getenv("PAN_TEST_SUSPEND_FILE")){if(std::filesystem::exists(marker)){std::filesystem::remove(marker);rt.RequestSuspendAndStop();}}
 if(getenv("PAN_3DS_UI")){const auto buttons=rt.TakeConsolePrompts();static uint32_t previous=~0u;if(previous!=buttons){fprintf(stderr,"[ConsolePrompts] frame=%u mask=%u\n",n,buttons);previous=buttons;}}
 keys.fill(false);for(const auto& e:input)if(n>=e.begin&&n<e.end)keys[e.key]=true;
 if(const char* path=std::getenv("NFS_LIVE_KEYS")){std::ifstream f(path);unsigned key;while(f>>key)if(key<256)keys[key]=true;}
 static S3ERuntime::Keys previous{};for(unsigned key=0;key<256;++key)if(keys[key]!=previous[key]){fprintf(stdout,"[Input] frame=%u key=%u down=%u\n",n,key,keys[key]?1:0);previous[key]=keys[key];}
 auto write=[&](const std::string& path){FILE*f=fopen(path.c_str(),"wb");if(!f)return;unsigned width=rt.SurfaceWidth();fprintf(f,"P6\n%u 240\n255\n",width);for(unsigned i=0;i<width*240;++i){auto p=data[i];unsigned char c[3]={static_cast<unsigned char>((p>>11)*255/31),static_cast<unsigned char>(((p>>5)&63)*255/63),static_cast<unsigned char>((p&31)*255/31)};fwrite(c,1,3,f);}fclose(f);};
 if(getenv("PGR_TEST_LIVE")&&(n==1000||n==1080))rt.RequestDisplayToggle();
 write(output+"/latest.ppm");if(n<=3||n%(std::getenv("PAN_CAPTURE_EVERY")?std::max(1,std::atoi(std::getenv("PAN_CAPTURE_EVERY"))):30)==0||n==frameLimit){char name[64];snprintf(name,sizeof(name),"/frame_%06u.ppm",n);write(output+name);}
 });
 fprintf(stderr,"[Entry] pc=%08x\n",G(bin.entry_point));
 int status=rt.Run(G(bin.entry_point),frameLimit);
 if(status&&!rt.Failed()){uint32_t pc=0,lr=0,sp=0;cpu->RegRead(Cpu::REG_PC,&pc);cpu->RegRead(Cpu::REG_LR,&lr);cpu->RegRead(Cpu::REG_SP,&sp);fprintf(stderr,"[Probe] CPU error %s pc=%08x lr=%08x sp=%08x\n",cpu->ErrorText(status),pc,lr,sp);for(int i=0;i<13;++i){uint32_t v=0;cpu->RegRead(i,&v);fprintf(stderr,"r%d=%08x ",i,v);}fprintf(stderr,"\n");}
 if(getenv("PGR_FAULT_DUMP")&&status){
  auto dump=[&](uint32_t addr,size_t size,const char* name){std::vector<uint8_t>b(size);if(cpu->MemRead(addr,b.data(),size)==Cpu::OK){std::ofstream f(output+"/"+name,std::ios::binary);f.write((char*)b.data(),b.size());}};
  dump(ImageBase(),bin.code_and_data.size(),"image.bin");dump(HEAP_BASE,HEAP_SIZE,"heap.bin");dump(STACK_BASE,STACK_SIZE,"stack.bin");
 }
 if(getenv("PGR_STATE_WRITE_RING")){fprintf(stderr,"[StateRing] state=%08x writes=%llu\n",stateRing.state,(unsigned long long)stateRing.count);uint64_t begin=stateRing.count>stateRing.stores.size()?stateRing.count-stateRing.stores.size():0;for(uint64_t i=begin;i<stateRing.count;++i){const auto& e=stateRing.stores[i%stateRing.stores.size()];fprintf(stderr,"[StateStore] n=%llu address=%08x size=%d value=%016llx pc=%08x lr=%08x\n",(unsigned long long)i,e.address,e.size,(unsigned long long)e.value,e.pc,e.lr);}}
 uint32_t stopPC=0;cpu->RegRead(Cpu::REG_PC,&stopPC);fprintf(stderr,"[Stop] pc=%08x frames=%u failed=%u\n",stopPC,rt.Frames(),rt.Failed()?1:0);
 return status?status:(rt.Failed()?1:0);
}
