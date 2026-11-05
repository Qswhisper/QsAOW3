// Original type9/WinOnly targets; only new original successful card uses.
#pragma once
#include "battle_contract_events.h"
namespace boost_contract_events {
static constexpr int count=2;
static_assert(sizeof(contract_content::boost_rules)/sizeof(contract_content::BoostRule)==count,"Boost-result slots need explicit migration");
inline uint32_t eligible(int i){return 1u<<(8+i);}
inline uint32_t completed(int i){return 1u<<(10+i);}
inline bool valid(uint32_t mask,uint32_t used){
    if(used&~3u)return false;
    for(int i=0;i<count;++i)if((used&(1u<<i))&&!(mask&eligible(i)))return false;
    return true;
}
inline bool add(uint32_t mask,uint32_t& used,int boost){
    if(!valid(mask,used))return false;
    for(int i=0;i<count;++i)if((mask&eligible(i))&&contract_content::boost_rules[i].boost==boost)used|=1u<<i;
    return true;
}
inline uint32_t qualified(uint32_t used){uint32_t flags=0;for(int i=0;i<count;++i)if(used&(1u<<i))flags|=completed(i);return flags;}
}
