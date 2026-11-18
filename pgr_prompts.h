#pragma once
#include <cstdint>
inline unsigned PgrSoftkeyPrompts(const uint16_t* image){
unsigned mask=0;
{static constexpr unsigned pixels[]={73604,73605,73610,73611,73924,73925,73929,73930,73931,74244,74245,74248,74249,74250,74564,74565,74567,74568,74569,74884,74885,74886,74887,74888,75204,75205,75206,75207,75524,75525,75526,75844,75845};bool match=true;for(auto p:pixels)if(image[p]!=0xffff){match=false;break;}if(match)mask|=1;}
{static constexpr unsigned pixels[]={73909,74228,74229,74547,74548,74549,74550,74551,74552,74553,74554,74555,74867,74868,74869,74870,74871,74872,74873,74874,74875,74876,75188,75189,75195,75196,75509,75515,75516,75832,75833,75834,75835,75836,76152,76153,76154,76155};bool match=true;for(auto p:pixels)if(image[p]!=0xffff){match=false;break;}if(match)mask|=2;}
return mask;
}