#pragma once
#include "loader.h"
#include <cstring>
// Change only the key-name call used to populate the nine controls rows.
// Other uses of the original formatter (and all input processing) stay intact.
inline bool PreparePgrControls(S3EBinary& bin){
 constexpr uint32_t site=0x5eca0,formatter=0x28144;
 const auto branch=[](uint32_t from,uint32_t to,uint32_t opcode){return opcode|(((to-from-8)/4)&0xffffff);};
 uint32_t stock=0;if(site+4>bin.code_and_data.size())return false;
 std::memcpy(&stock,bin.code_and_data.data()+site,4);
 if(stock!=branch(site,formatter,0xeb000000))return false;
 const uint32_t stub=(bin.code_and_data.size()+3)&~3u;
 // Preserve the original translated direction names for Left and Right.
 uint32_t code[]={0xe3540002,branch(stub+4,formatter,0x3a000000),0xe28f0008,0xe7901104,0xe0800001,0xe12fff1e};
 const char* labels[]={"","","R","L","X","X","X","Y","SELECT"};
 std::vector<uint8_t> payload(sizeof(code)+9*4);std::memcpy(payload.data(),code,sizeof(code));
 for(unsigned i=0;i<9;++i){uint32_t relative=payload.size()-sizeof(code);std::memcpy(payload.data()+sizeof(code)+i*4,&relative,4);payload.insert(payload.end(),labels[i],labels[i]+std::strlen(labels[i])+1);}
 bin.code_and_data.resize(stub);bin.code_and_data.insert(bin.code_and_data.end(),payload.begin(),payload.end());
 uint32_t replacement=branch(site,stub,0xeb000000);std::memcpy(bin.code_and_data.data()+site,&replacement,4);
 return true;
}
