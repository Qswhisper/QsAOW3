// Original type15/sub21: complete supply depots within the original window.
// Counts are local to one battle; only the qualified terminal bit is stored.
#pragma once
#include "battle_contract_events.h"
namespace timed_contract_events {
static_assert(sizeof(contract_content::timed_rules)/sizeof(contract_content::TimedRule)==1,"Timed-result journal slots need explicit migration");
struct Counts {int32_t supplies=0;};
inline bool valid(uint32_t mask,const Counts& c){
    const auto& rule=contract_content::timed_rules[0];
    return c.supplies>=0&&c.supplies<=rule.buildings&&((mask&battle_contract_events::timed_mask)||!c.supplies);
}
inline bool add(uint32_t mask,Counts& c,int key,int amount,int tick){
    if(!valid(mask,c))return false;
    if(!(mask&battle_contract_events::timed_mask)||key!=30)return true;
    const auto& rule=contract_content::timed_rules[0];
    if(amount<0||tick<0||rule.minutes<=0||rule.minutes>INT_MAX/600)return false;
    if(tick>rule.minutes*600)return true;
    int64_t total=int64_t(c.supplies)+amount;c.supplies=total>rule.buildings?rule.buildings:(int32_t)total;return true;
}
inline bool complete(const Counts& c){return c.supplies==contract_content::timed_rules[0].buildings;}
}
