// local victory growth; these are not the absent online server rules.
#pragma once
#include <stdint.h>
#include <limits.h>
namespace career_star_rewards {
inline bool budget(int floor,int next,bool highest,int& amount){
    if(floor<0||(!highest&&next<=floor))return false;
    if(highest){amount=0;return true;}int64_t delta=int64_t(next)-floor,value=(delta+2)/3;
    if(value<1||value>0x7fffff)return false;amount=int(value);return true;
}
}
