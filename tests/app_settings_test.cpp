#include "runtime_s3e.h"
#include "loader.h"
#include <cassert>
#include <algorithm>
#include <cstdio>
#include <cstring>
int main(int argc,char**argv){
 assert(argc==2);std::string error;auto cpu=Cpu::CreateUnicorn(&error);assert(cpu);S3EBinary bin;
 assert(LoadS3E(std::string(argv[1])+"/pgr.xe3u",bin));assert(SetupSandbox(cpu.get(),bin));assert(cpu->MemMap(0x60000000,4096)==0);
 S3ERuntime runtime(cpu.get(),argv[1],"probe_test_saves");runtime.Init(bin.imports,bin.config);
 auto call=[&](const char* name,uint32_t a=0,uint32_t b=0,uint32_t c=0,uint32_t d=0){
  auto it=std::find(bin.imports.begin(),bin.imports.end(),name);assert(it!=bin.imports.end());uint32_t flags=0x10,sp=STACK_BASE+STACK_SIZE-256,lr=0x60000080;
  cpu->RegWrite(Cpu::REG_CPSR,&flags);cpu->RegWrite(Cpu::REG_SP,&sp);cpu->RegWrite(Cpu::REG_LR,&lr);uint32_t args[]={a,b,c,d};for(int n=0;n<4;++n)cpu->RegWrite(n,&args[n]);uint32_t zero=0;cpu->MemWrite(sp,&zero,4);
  assert(cpu->Start(TRAMPOLINE_BASE+8*(it-bin.imports.begin()),lr,100000)==0);uint32_t result=0;cpu->RegRead(0,&result);assert(!runtime.Failed());return result;
 };
 auto put=[&](uint32_t at,const char* text){assert(cpu->MemWrite(at,text,strlen(text)+1)==0);};
 auto setting=[&](const char* section,const char* name,uint32_t expected){put(0x60000100,section);put(0x60000200,name);assert(call("s3eConfigGetInt",0x60000100,0x60000200,0x60000300)==0);uint32_t value=0;assert(cpu->MemRead(0x60000300,&value,4)==0);assert(value==expected);};
 setting("PGRMemory","Gx",1920000);setting("Core","MemSize",3580000);setting("GX","DataCacheSize",60000);setting("GX","PolyBufferSize",80000);
 setting("Game","Left",9);setting("Game","Right",11);setting("Game","Accel",10);setting("Game","Brake",12);setting("Game","Handbrake",17);setting("Game","LookBehind",18);
 puts("PASS: application display and control settings match the supplied game profile.");
}
