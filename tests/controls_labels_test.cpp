#include "pgr_controls.h"
#include <cassert>
#include <cstdio>
int main(int argc,char**argv){
 assert(argc==2);
 for(uint32_t base:{STOCK_BASE,0x48000000u}){
  std::string error;auto cpu=Cpu::CreateUnicorn(&error);assert(cpu);S3EBinary bin;
  assert(LoadS3E(std::string(argv[1])+"/pgr.xe3u",bin));
  const uint32_t stub=(bin.code_and_data.size()+3)&~3u;
  assert(PreparePgrControls(bin));assert(!PreparePgrControls(bin));
  assert(SetupSandbox(cpu.get(),bin,base));assert(cpu->MemMap(0x60000000,4096)==0);
  const char* expected[]={"R","L","X","X","X","Y","SELECT"};
  for(uint32_t row=2;row<9;++row){
   uint32_t flags=0x10,lr=0x60000000,sp=STACK_BASE+STACK_SIZE-256,sentinel=0x12345678;
   cpu->RegWrite(Cpu::REG_CPSR,&flags);cpu->RegWrite(Cpu::REG_LR,&lr);cpu->RegWrite(Cpu::REG_SP,&sp);cpu->RegWrite(4,&row);cpu->RegWrite(5,&sentinel);
   assert(cpu->Start(base+stub,lr,100)==0);uint32_t result=0,r5=0;cpu->RegRead(0,&result);cpu->RegRead(5,&r5);
   char text[16]{};assert(cpu->MemRead(result,text,sizeof(text))==0);assert(std::string(text)==expected[row-2]);assert(r5==sentinel);
  }
 }
 puts("PASS: all seven console key labels execute correctly at both image locations; incompatible layouts are rejected.");
}
