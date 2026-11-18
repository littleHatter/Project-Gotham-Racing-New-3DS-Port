#include <3ds.h>
#include <sys/iosupport.h>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <string>
#include <exception>
#include "loader.h"
#include "pgr_controls.h"
#include "runtime_s3e.h"
#include "pgr_presentation.h"
#include "pgr_music_3ds.h"
#include "pgr_bottom.h"
u32 __stacksize__=1024*1024;
u32 __ctru_heap_size=24*1024*1024;
u32 __ctru_linear_heap_size=4*1024*1024;
extern "C" {
extern char *fake_heap_start,*fake_heap_end;
extern u32 __ctru_heap,__ctru_linear_heap;
void __system_allocateHeaps(){
    if(R_FAILED(svcControlMemory(&__ctru_heap,OS_HEAP_AREA_BEGIN,0,__ctru_heap_size,MEMOP_ALLOC,(MemPerm)(MEMPERM_READ|MEMPERM_WRITE))))svcBreak(USERBREAK_PANIC);
    if(R_FAILED(svcControlMemory(&__ctru_linear_heap,0,0,__ctru_linear_heap_size,MEMOP_ALLOC_LINEAR,(MemPerm)(MEMPERM_READ|MEMPERM_WRITE))))svcBreak(USERBREAK_PANIC);
    mappableInit(OS_MAP_AREA_BEGIN,OS_MAP_AREA_END);fake_heap_start=(char*)__ctru_heap;fake_heap_end=fake_heap_start+__ctru_heap_size;
}
}
namespace {
constexpr const char* ROOT="sdmc:/3ds/pgr_mobile";
FILE* logFile=nullptr;const devoptab_t* originalOut=nullptr;const devoptab_t* originalErr=nullptr;devoptab_t quiet{};
ssize_t QuietWrite(struct _reent*,void*,const char* data,size_t length){if(logFile)fwrite(data,1,length,logFile);return length;}
void FaultConsole(){static bool shown=false;if(!shown){consoleInit(GFX_BOTTOM,nullptr);gfxScreenSwapBuffers(GFX_BOTTOM,false);shown=true;}if(originalOut)devoptab_list[STD_OUT]=originalOut;if(originalErr)devoptab_list[STD_ERR]=originalErr;}
void Present(const uint16_t* image,unsigned prompts){u16 width=0,height=0;u8* fb=gfxGetFramebuffer(GFX_TOP,GFX_LEFT,&width,&height);memset(fb,0,400*240*3);for(unsigned y=0;y<240;++y)for(unsigned x=0;x<320;++x){uint16_t p=image[y*320+x];if(y>=228&&((prompts&1&&x<20)||(prompts&2&&x>=300)))p=0;u8* pixel=fb+((x+40)*240+239-y)*3;pixel[0]=(p&31)*255/31;pixel[1]=((p>>5)&63)*255/63;pixel[2]=((p>>11)&31)*255/31;}gfxFlushBuffers();gfxScreenSwapBuffers(GFX_TOP,false);}
class Audio3DS {
    static constexpr unsigned COUNT=12,SAMPLES=512;
    ndspWaveBuf buffers[COUNT]{};s16* pcm=nullptr;unsigned next=0,queued=0,dropped=0;bool ready=false,playing=false;
public:
    bool Init(){
        next=queued=dropped=0;playing=false;memset(buffers,0,sizeof(buffers));
        Result rc=ndspInit();if(R_FAILED(rc)){printf("[Audio3DS] NDSP init failed: %08lx; continuing without output\n",(unsigned long)rc);return false;}
        pcm=(s16*)linearAlloc(COUNT*SAMPLES*2);
        if(!pcm){puts("[Audio3DS] PCM allocation failed");ndspExit();return false;}
        ready=true;ndspChnReset(0);ndspChnSetInterp(0,NDSP_INTERP_LINEAR);ndspChnSetFormat(0,NDSP_FORMAT_MONO_PCM16);ndspChnSetRate(0,22050.0f);
        float mix[12]{};mix[0]=mix[1]=1.0f;ndspChnSetMix(0,mix);ndspChnSetPaused(0,true);
        for(unsigned i=0;i<COUNT;++i)buffers[i].data_pcm16=pcm+i*SAMPLES;
        puts("[Audio3DS] NDSP ready: 22050 Hz mono");return true;
    }
    void Push(const int16_t* samples,size_t count,uint32_t rate){
        if(!ready)return;
        if(count>SAMPLES||rate!=22050){if(!dropped++)puts("[Audio3DS] Unexpected PCM format");return;}
        auto& b=buffers[next];
        if(b.status==NDSP_WBUF_QUEUED||b.status==NDSP_WBUF_PLAYING){if(++dropped==1||dropped%100==0)printf("[Audio3DS] Queue full; dropped blocks=%u\n",dropped);return;}
        for(size_t i=0;i<count;++i)b.data_pcm16[i]=samples[i];DSP_FlushDataCache(b.data_pcm16,count*2);b.nsamples=count;b.offset=0;b.looping=false;b.status=NDSP_WBUF_FREE;b.next=nullptr;
        ndspChnWaveBufAdd(0,&b);next=(next+1)%COUNT;++queued;
        if(!playing&&queued>=2){ndspChnSetPaused(0,false);playing=true;}
    }
    void Shutdown(){if(ready){ndspChnWaveBufClear(0);ndspChnReset(0);ndspExit();linearFree(pcm);pcm=nullptr;ready=false;}playing=false;}
    ~Audio3DS(){Shutdown();}

};
}
extern "C" void ShowLogOnScreen(){FaultConsole();}
int main(){
 gfxInitDefault();gfxSetDoubleBuffering(GFX_BOTTOM,false);consoleInit(GFX_BOTTOM,nullptr);consoleDebugInit(debugDevice_CONSOLE);
 aptSetHomeAllowed(true);osSetSpeedupEnable(true);
 gfxSetScreenFormat(GFX_BOTTOM,GSP_BGR8_OES);DrawPgrBottom(0,true);gfxFlushBuffers();gfxScreenSwapBuffers(GFX_BOTTOM,false);
 originalOut=devoptab_list[STD_OUT];originalErr=devoptab_list[STD_ERR];quiet=*originalOut;quiet.name="pgr_test_log";quiet.write_r=QuietWrite;
 std::error_code ec;std::filesystem::create_directories(ROOT,ec);logFile=fopen("sdmc:/3ds/pgr_mobile/test5.log","wb");if(logFile)setvbuf(logFile,nullptr,_IOFBF,32768);devoptab_list[STD_OUT]=&quiet;devoptab_list[STD_ERR]=&quiet;setvbuf(stdout,nullptr,_IONBF,0);setvbuf(stderr,nullptr,_IONBF,0);
 int result=1;bool isNew=false;APT_CheckNew3DS(&isNew);
 try{
  if(!isNew){FaultConsole();puts("Requires a New Nintendo 3DS or New 2DS XL.");}
  else{
   std::string error;auto cpu=Cpu::CreateNative(&error);
   if(!cpu){FaultConsole();printf("Unable to start: %s\n",error.c_str());}
   else{
    S3EBinary binary;std::string assets=std::string(ROOT)+"/game";
    if(LoadS3E(assets+"/pgr.xe3u",binary)&&PreparePgrControls(binary)&&SetupSandbox(cpu.get(),binary,STOCK_BASE,64u<<20)){
     std::array<uint8_t,128> lastFrameState{};uint32_t lastFrameStateAddress=0,lastRecordedFrame=0;
     auto captureState=[&](uint8_t* target,uint32_t& pointer){cpu->MemRead(G(0x4a0c4154),&pointer,4);if(pointer>=HEAP_BASE&&uint64_t(pointer)+128<=uint64_t(HEAP_BASE)+HEAP_SIZE){cpu->MemRead(pointer,target,128);return true;}return false;};
     S3ERuntime runtime(cpu.get(),assets,std::string(ROOT)+"/user");runtime.Init(binary.imports,binary.config);PgrPresentation presentation(std::string(ROOT)+"/music");
     Audio3DS audio;bool audioReady=audio.Init();PgrMusic3DS music(std::string(ROOT)+"/music");bool independent=audioReady&&music.Init();
     if(independent)presentation.SetCueCallback([&](const std::string& song){music.Cue(song);});
     else runtime.SetExternalMusicCallback([&](int16_t* data,size_t count,uint32_t volume,bool paused){presentation.Mix(data,count,volume,paused);});
     runtime.SetAudioCallback([&](const int16_t* p,size_t n,uint32_t rate){audio.Push(p,n,rate);});
     runtime.SetPollCallback([&](S3ERuntime::Keys& keys){keys.fill(false);if(!aptMainLoop()){runtime.RequestQuit();return;}hidScanInput();u32 held=hidKeysHeld();
      keys[9]=(held&(KEY_DLEFT|KEY_CPAD_LEFT))!=0;keys[10]=(held&(KEY_DUP|KEY_CPAD_UP|KEY_R))!=0;keys[11]=(held&(KEY_DRIGHT|KEY_CPAD_RIGHT))!=0;keys[12]=(held&(KEY_DDOWN|KEY_CPAD_DOWN|KEY_L))!=0;
      keys[78]=(held&KEY_A)!=0;keys[209]=(held&KEY_X)!=0;keys[210]=(held&(KEY_B|KEY_START))!=0;keys[17]=(held&KEY_X)!=0;keys[18]=(held&KEY_Y)!=0;keys[19]=(held&KEY_SELECT)!=0;
     });
     runtime.SetFrameCallback([&](uint32_t frame,const uint16_t* image,S3ERuntime::Keys&){presentation.Update(cpu.get(),runtime,image);if(independent)music.SetControls(runtime.MasterVolume(),presentation.paused);DrawPgrBottom(presentation.prompts,false);Present(image,presentation.prompts);if(captureState(lastFrameState.data(),lastFrameStateAddress))lastRecordedFrame=frame;if((frame==1||frame%120==0)&&logFile)fflush(logFile);});
     puts("PGR New 3DS Test 5");result=runtime.Run(G(binary.entry_point),0);
     if(result){uint32_t registers[17]{};for(unsigned n=0;n<17;++n)cpu->RegRead(n,&registers[n]);uint32_t state=0;cpu->MemRead(G(0x4a0c4154),&state,4);uint8_t snapshot[128]{};bool valid=state>=HEAP_BASE&&uint64_t(state)+sizeof(snapshot)<=uint64_t(HEAP_BASE)+HEAP_SIZE;if(valid)cpu->MemRead(state,snapshot,sizeof(snapshot));FILE* detail=fopen("sdmc:/3ds/pgr_mobile/test5_report.bin","wb");if(detail){fwrite(registers,1,sizeof(registers),detail);fwrite(&state,1,sizeof(state),detail);fwrite(snapshot,1,sizeof(snapshot),detail);fwrite(&lastRecordedFrame,1,sizeof(lastRecordedFrame),detail);fwrite(&lastFrameStateAddress,1,sizeof(lastFrameStateAddress),detail);fwrite(lastFrameState.data(),1,lastFrameState.size(),detail);fclose(detail);}if(logFile)fflush(logFile);}

    }else{FaultConsole();puts("Could not load game files. Check the game folder.");}
   }
  }
 }catch(const std::exception& e){FaultConsole();printf("Unable to continue: %s\n",e.what());result=1;}
 if(result){FaultConsole();printf("\nGame stopped (result %d).\nPress A to return to the launcher.\n",result);gfxFlushBuffers();while(aptMainLoop()){hidScanInput();if(hidKeysDown()&KEY_A)break;gspWaitForVBlank();}}
 fflush(nullptr);if(logFile)fclose(logFile);devoptab_list[STD_OUT]=originalOut;devoptab_list[STD_ERR]=originalErr;gfxExit();return result;
}
