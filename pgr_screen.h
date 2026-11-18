#pragma once
#include "loader.h"
#include <string>
struct PgrScreen {std::string name;bool racing=false,paused=false,results=false,loading=false;};
inline PgrScreen ReadPgrScreen(Cpu* cpu){
 auto read=[&](uint32_t p){uint32_t n=0;cpu->MemRead(p,&n,4);return n;};
 auto valid=[](uint32_t p){return !(p&3)&&p>=HEAP_BASE&&uint64_t(p)+40<=uint64_t(HEAP_BASE)+HEAP_SIZE;};
 PgrScreen result;uint32_t manager=read(G(0x4a0c3060));if(!valid(manager))return result;
 uint32_t array=read(manager+0x10),count=read(manager+0x14);if(!valid(array)||count>64)return result;
 struct Type{uint32_t offset;const char* name;};static constexpr Type types[]={
{779604,"10CLoadState"},
{781104,"12CInGameState"},
{770856,"12CIwGameState"},
{781284,"12CIwGameState"},
{782292,"12CUpsellState"},
{782664,"13CAttractState"},
{771084,"13CGuiGameState"},
{771096,"13CGuiGameState"},
{771516,"13CGuiGameState"},
{771692,"13CGuiGameState"},
{771704,"13CGuiGameState"},
{771716,"13CGuiGameState"},
{771728,"13CGuiGameState"},
{771800,"13CGuiGameState"},
{771872,"13CGuiGameState"},
{771896,"13CGuiGameState"},
{771920,"13CGuiGameState"},
{772028,"13CGuiGameState"},
{772040,"13CGuiGameState"},
{772100,"13CGuiGameState"},
{772196,"13CGuiGameState"},
{772208,"13CGuiGameState"},
{772604,"13CGuiGameState"},
{772888,"13CGuiGameState"},
{772900,"13CGuiGameState"},
{772912,"13CGuiGameState"},
{772924,"13CGuiGameState"},
{773092,"13CGuiGameState"},
{773164,"13CGuiGameState"},
{773412,"13CGuiGameState"},
{773512,"13CGuiGameState"},
{783032,"13CGuiGameState"},
{783824,"13CResultsState"},
{784064,"13CSuccessState"},
{784704,"14CCongratsState"},
{785100,"14CMainMenuState"},
{785304,"14CRaceInfoState"},
{785544,"15CCarColourState"},
{786304,"15CTimeTrialState"},
{786692,"16CGarageGameState"},
{787256,"16CMedalsGameState"},
{787372,"16CPausedGameState"},
{789672,"18CSettingsGameState"},
{791532,"20CLanguageSelectState"},
{791648,"20CStatisticsGameState"},
{791764,"21CChampSelectGameState"},
{791880,"21CEventSelectGameState"},
{792848,"22CAchievementsGameState"},
{792972,"22CDifficultySelectState"},
{793460,"22CSplashScreenGameState"},
 };
 for(uint32_t i=0;i<count;++i){uint32_t p=read(array+i*4);if(!valid(p)||!(read(p+12)&1))continue;uint32_t vt=read(p);
  for(const auto& type:types)if(vt==ImageBase()+type.offset){result.name=type.name;break;}
 }
 result.racing=result.name=="12CInGameState";result.paused=result.name=="16CPausedGameState";
 result.results=result.name=="13CResultsState"||result.name=="13CSuccessState"||result.name=="14CCongratsState";
 result.loading=result.name=="10CLoadState";return result;
}
