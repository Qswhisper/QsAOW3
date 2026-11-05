#pragma once
#include "local_progress.h"
namespace local_boost_conversion {static bool installed;}
namespace boost_conversion_policy {
inline bool price(int base,int effect,int level,int& out){
    if(base<1||effect<0||level<0||level>11||int64_t(effect)*level>=1000)return false;
    int64_t value=int64_t(base)*(1000-int64_t(effect)*level)/1000;
    if(value<1||value>INT_MAX)return false;out=(int)value;return true;
}
inline bool convert(local_progress::State& state,int source,int target,int input,int unit_cost){
    if(source<0||target<0||source==target||input<1||unit_cost<1||input%unit_cost)return false;
    auto next=state;int output=input/unit_cost;
    if(!local_progress::consume_stock(next,4,source,input)||!local_progress::grant_stock(next,4,target,output))return false;
    state=next;return true;
}
}
