#pragma once
#include "pgr_artwork.h"
#include "display_toggle_art.h"
constexpr unsigned kToggleTop=240-kToggleHeight;
inline void DrawPgrBottom(unsigned prompts,bool loading,bool wide=false){
 static unsigned last=~0u;static bool lastLoading=false,lastWide=false;if(last==prompts&&lastLoading==loading&&lastWide==wide)return;last=prompts;lastLoading=loading;lastWide=wide;
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
 else{
  const uint8_t* icon=wide?kSwitchToStandard:kSwitchToWide;
  for(unsigned y=0;y<kToggleHeight;++y)for(unsigned x=0;x<kToggleWidth;++x){
   const auto* src=icon+(y*kToggleWidth+x)*4;
   auto* dst=fb+(x*240+239-(kToggleTop+y))*3;
   const unsigned alpha=src[3];
   for(unsigned c=0;c<3;++c)dst[c]=(src[2-c]*alpha+dst[c]*(255-alpha)+127)/255;
  }
 }
}
