#pragma once
#include "pgr_artwork.h"
inline void DrawPgrBottom(unsigned prompts,bool loading){
 static unsigned last=~0u;static bool lastLoading=false;if(last==prompts&&lastLoading==loading)return;last=prompts;lastLoading=loading;
 u16 w,h;u8*fb=gfxGetFramebuffer(GFX_BOTTOM,GFX_LEFT,&w,&h);memset(fb,0,320*240*3);
 auto draw=[&](const uint32_t* data,unsigned width,unsigned height,unsigned left,unsigned top){
  for(unsigned y=0;y<height&&top+y<240;++y)for(unsigned x=0;x<width&&left+x<320;++x){uint32_t p=data[y*width+x];unsigned alpha=p>>24;if(!alpha)continue;u8*dst=fb+((left+x)*240+239-top-y)*3;
   for(unsigned c=0;c<3;++c){unsigned value=(p>>(c*8))&255;dst[c]=(value*alpha+dst[c]*(255-alpha)+127)/255;}
  }
 };
 using namespace PgrArt;draw(splash,splashW,splashH,0,0);
 if(prompts&1)draw(promptA,promptAW,promptAH,0,0);
 if(prompts&4)draw(promptStart,promptStartW,promptStartH,83,0);
 if(prompts&2)draw(promptB,promptBW,promptBH,243,0);
 if(loading)draw(PgrArt::loading,loadingW,loadingH,320-loadingW,240-loadingH);
}
