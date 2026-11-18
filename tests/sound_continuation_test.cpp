#include "runtime_s3e.h"
#include "loader.h"
#include <cassert>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <thread>
#include <chrono>
int main(int argc,char**argv){
 assert(argc==2);std::string error;auto cpu=Cpu::CreateUnicorn(&error);assert(cpu);S3EBinary bin;
 assert(LoadS3E(std::string(argv[1])+"/pgr.xe3u",bin));assert(SetupSandbox(cpu.get(),bin));assert(cpu->MemMap(0x60000000,4096)==0);
 S3ERuntime runtime(cpu.get(),argv[1],"probe_test_saves");runtime.Init(bin.imports,bin.config);
 auto call=[&](const char* name,uint32_t a=0,uint32_t b=0,uint32_t c=0,uint32_t d=0){
  auto it=std::find(bin.imports.begin(),bin.imports.end(),name);assert(it!=bin.imports.end());uint32_t flags=0x10,sp=STACK_BASE+STACK_SIZE-256,lr=0x60000080;
  cpu->RegWrite(Cpu::REG_CPSR,&flags);cpu->RegWrite(Cpu::REG_SP,&sp);cpu->RegWrite(Cpu::REG_LR,&lr);uint32_t args[]={a,b,c,d};for(int n=0;n<4;++n)cpu->RegWrite(n,&args[n]);uint32_t zero=0;cpu->MemWrite(sp,&zero,4);
  assert(cpu->Start(TRAMPOLINE_BASE+8*(it-bin.imports.begin()),lr,100000)==0);uint32_t result=0;cpu->RegRead(0,&result);assert(!runtime.Failed());return result;
 };

 // Real ARM callbacks: generator completes a segment; end callback records
 // the complete info structure and returns the remaining repetition count.
 const uint32_t generator[]={0xe3a02001,0xe580201c,0xe3a00000,0xe12fff1e};
 const uint32_t end[]={0xe92d0030,0xe5902004,0xe5913010,0xe2833001,0xe5813010,0xe890003c,0xe881003c,0xe1a00003,0xe8bd0030,0xe12fff1e};
 assert(cpu->MemWrite(0x60000400,generator,sizeof(generator))==0);
 assert(cpu->MemWrite(0x60000500,end,sizeof(end))==0);
 auto read=[&](uint32_t at){uint32_t v;assert(cpu->MemRead(at,&v,4)==0);return v;};
 auto reset=[&](){uint32_t zeros[8]={};assert(cpu->MemWrite(0x60000600,zeros,sizeof(zeros))==0);};
 auto pump=[&](unsigned n){for(unsigned i=0;i<n;++i){std::this_thread::sleep_for(std::chrono::milliseconds(27));call("s3eDeviceYield",0);}};
 call("s3eSoundChannelRegister",0,1,0x60000400,0);
 call("s3eSoundChannelRegister",0,0,0x60000500,0x60000600);
 reset();call("s3eSoundChannelPlay",0,0x60000700,128,2);pump(4);
 assert(read(0x60000610)==2);assert(read(0x60000600)==0);assert(read(0x60000604)==0);assert(read(0x60000608)==0x60000700);assert(read(0x6000060c)==128);assert(call("s3eSoundChannelGetInt",0,0)==0);
 reset();call("s3eSoundChannelPlay",0,0x60000700,128,~0u);pump(4);
 assert(read(0x60000610)>=3);assert(read(0x60000604)==~0u);assert(call("s3eSoundChannelGetInt",0,0)==1);
 call("s3eSoundChannelPause",0);auto before=read(0x60000610);pump(2);assert(read(0x60000610)==before);
 call("s3eSoundChannelResume",0);pump(2);assert(read(0x60000610)>before);
 call("s3eSoundChannelStop",0);before=read(0x60000610);pump(2);assert(read(0x60000610)==before);assert(call("s3eSoundChannelGetInt",0,0)==0);
 // The callback can supply the next segment's buffer and sample count.
 const uint32_t replaceEnd[]={0xe92d0030,0xe5912010,0xe2822001,0xe5812010,0xe890003c,0xe881003c,0xe591c014,0xe580c008,0xe591c018,0xe580c00c,0xe1a00003,0xe8bd0030,0xe12fff1e};
 assert(cpu->MemWrite(0x60000800,replaceEnd,sizeof(replaceEnd))==0);
 reset();const uint32_t replacement[]={0x60000900,64};assert(cpu->MemWrite(0x60000614,replacement,sizeof(replacement))==0);
 call("s3eSoundChannelRegister",0,0,0x60000800,0x60000600);
 call("s3eSoundChannelPlay",0,0x60000700,128,2);pump(4);
 assert(read(0x60000610)==2);assert(read(0x60000608)==0x60000900);assert(read(0x6000060c)==64);assert(call("s3eSoundChannelGetInt",0,0)==0);
 // An explicit stop issued inside an end callback takes priority over its
 // nonzero return value, so a completed/stopped voice cannot be resurrected.
 auto stopImport=std::find(bin.imports.begin(),bin.imports.end(),"s3eSoundChannelStop");assert(stopImport!=bin.imports.end());
 const uint32_t stopEnd[]={0xe92d4000,0xe3a00000,0xe59fc008,0xe12fff3c,0xe3a00001,0xe8bd8000,TRAMPOLINE_BASE+8*uint32_t(stopImport-bin.imports.begin())};
 assert(cpu->MemWrite(0x60000580,stopEnd,sizeof(stopEnd))==0);
 call("s3eSoundChannelRegister",0,0,0x60000580,0);
 call("s3eSoundChannelPlay",0,0x60000700,128,~0u);pump(2);assert(call("s3eSoundChannelGetInt",0,0)==0);
 puts("PASS: finite and infinite generated sound continuations, full end info, replacement segments, pause/resume, explicit stop and callback stop priority.");
}
