#include <cstdint>
#include <cstring>
#include <cstdio>
using u8=uint8_t;using u16=uint16_t;
constexpr int GFX_BOTTOM=0,GFX_LEFT=0;static uint8_t fb[320*240*3];
u8* gfxGetFramebuffer(int,int,u16*w,u16*h){*w=240;*h=320;return fb;}
#include "pgr_bottom.h"
int main(int argc,char**argv){if(argc!=2)return 1;const unsigned masks[]={0,3,4,0};const bool loads[]={true,false,false,false};
 for(unsigned i=0;i<4;++i){DrawPgrBottom(masks[i],loads[i]);char path[1024];snprintf(path,sizeof(path),"%s/bottom_%u.ppm",argv[1],i);FILE*f=fopen(path,"wb");if(!f)return 1;fprintf(f,"P6\n320 240\n255\n");for(unsigned y=0;y<240;++y)for(unsigned x=0;x<320;++x){auto*p=fb+(x*240+239-y)*3;unsigned char rgb[]={p[2],p[1],p[0]};fwrite(rgb,1,3,f);}fclose(f);}
 puts("PASS: native bottom renderer generated startup, menu, race and empty-prompt frames.");
}
