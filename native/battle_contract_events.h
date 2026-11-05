// Original type12 event IDs and original counts. No fabricated combat counters.
#pragma once
#include <stdint.h>
#include <climits>
#include "contract_content.generated.h"
namespace battle_contract_events {
static constexpr int count=5;
static_assert(sizeof(contract_content::battle_rules)/sizeof(contract_content::BattleRule)==count,"Frozen event schema changed");
static constexpr int result_count=1;
static_assert(sizeof(contract_content::result_rules)/sizeof(contract_content::ResultRule)==result_count,"Frozen result schema changed");
static constexpr uint32_t event_mask=(1u<<count)-1,result_mask=(1u<<(count+result_count))-1;
// v10 bit6 freezes original timed-building eligibility; bit7 records that
// the concrete within-battle condition was met. Neither reorders old slots.
static constexpr uint32_t timed_mask=64,timed_completed=128,v10_mask=255;
// v11 keeps the old layout: boost eligibility8..9, qualified victory10..11.
static constexpr uint32_t boost_mask=0x300,boost_completed=0xc00,v11_mask=0xfff;
// v12 bit12 freezes the activity goal; bit13 records a qualifying real win.
static constexpr uint32_t start_mask=0x137f,all_mask=0x3fff;
struct Counts {int32_t values[count]={};};
inline bool valid(uint32_t mask,const Counts& counters){
    if(mask&~all_mask)return false;
    for(int i=0;i<count;++i)if(counters.values[i]<0||counters.values[i]>contract_content::battle_rules[i].value||(!(mask&(1u<<i))&&counters.values[i]))return false;
    return true;
}
inline bool add(uint32_t mask,Counts& counters,int key,int amount){
    if(!valid(mask,counters))return false;
    bool matched=false;
    for(int i=0;i<count;++i)if(mask&(1u<<i)&&contract_content::battle_rules[i].subtype==key)matched=true;
    if(!matched)return true;
    if(amount<0)return false;
    for(int i=0;i<count;++i)if(mask&(1u<<i)&&contract_content::battle_rules[i].subtype==key){
        int64_t total=(int64_t)counters.values[i]+amount,goal=contract_content::battle_rules[i].value;
        counters.values[i]=(int32_t)(total>goal?goal:total);
    }
    return true;
}
}
