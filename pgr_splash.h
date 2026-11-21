#pragma once
#include <cstdint>
inline bool PgrOpeningLogo(const uint16_t* image,unsigned width=320){
 struct Pixel{unsigned position,r,g,b;};static constexpr Pixel samples[]={
{14465,1,2,1},{14515,11,23,12},{14565,4,6,3},{14615,7,15,7},{14665,8,17,8},{27265,1,2,1},{27315,12,25,12},{27365,28,43,9},{27415,24,47,19},{27465,23,41,13},{40065,0,2,0},{40115,11,13,1},{40165,21,23,1},{40215,30,47,2},{40265,14,29,15},{52865,0,2,0},{52915,6,6,1},{52965,17,37,17},{53015,30,43,3},{53065,30,45,0},{65665,0,2,0},{65715,0,2,0},{65765,0,0,0},{65815,25,27,1},{65865,27,41,0}};
 unsigned matches=0;for(auto s:samples){auto p=image[(s.position/320)*width+s.position%320+(width-320)/2];auto close=[](unsigned a,unsigned b,unsigned tolerance){return a>b?a-b<=tolerance:b-a<=tolerance;};if(close(p>>11,s.r,2)&&close((p>>5)&63,s.g,3)&&close(p&31,s.b,2))++matches;}return matches>=22;
}
