#pragma once
#include "pgr_screen.h"
#include "pgr_splash.h"
#include "pgr_soundtrack.h"
#include "pgr_prompts.h"
#include "runtime_s3e.h"
class PgrPresentation {
 PgrSoundtrack music;std::string lastScreen,requested;bool openingSeen=false;
 std::function<void(const std::string&)> cueCallback;
 void Cue(const std::string& name){if(name==requested)return;requested=name;if(cueCallback)cueCallback(name);else music.Cue(name);}

public:
 void SetCueCallback(std::function<void(const std::string&)> fn){cueCallback=std::move(fn);}
 unsigned prompts=0;bool paused=false;
 explicit PgrPresentation(const std::string& directory):music(directory){}
 void Update(Cpu* cpu,S3ERuntime& runtime,const uint16_t* frame){
  auto screen=ReadPgrScreen(cpu);paused=screen.paused;
  if(screen.name!=lastScreen){lastScreen=screen.name;fprintf(stdout,"[Presentation] Screen %s\n",screen.name.c_str());}
  prompts=PgrSoftkeyPrompts(frame)|(screen.racing?4:0);
  if(!openingSeen){bool logo=screen.name=="22CSplashScreenGameState"&&PgrOpeningLogo(frame);if(!logo&&screen.name!="14CMainMenuState")return;openingSeen=true;fprintf(stdout,"[Presentation] %s frame=%u\n",logo?"Opening logo":"Splash skipped: main menu",runtime.Frames());}
  if(screen.name.empty()||screen.paused||screen.loading)return;
  if(screen.results)Cue("win");
  else if(screen.racing){if(!runtime.RaceCity().empty())Cue(runtime.RaceCity());}
  else Cue("title");
 }
 void Mix(int16_t* data,size_t count,uint32_t volume,bool groupPaused){music.Mix(data,count,volume,paused||groupPaused);}
};
