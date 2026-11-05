#pragma once
#include <stdint.h>
#include "inventory_types.h"
namespace supply_types {
struct Selected {int32_t item,amount;};
struct Grant {int32_t type,id,amount;}; // Original RewardItem codes.
struct Ledger {
    uint64_t cycle,claims;
    int32_t group,step,tokens,last_box,last_cost;
    uint32_t draw_count,grant_count,pending;
    uint64_t last_cycle;
    Selected draws[16];
    Grant grants[128];
};
static_assert(sizeof(Ledger)==1720,"Supply journal ABI changed");
inline bool valid(const Ledger& s){
    if(s.tokens<0||s.group<0||s.step<0||s.step>12||(s.group==0)!=(s.step==0)||s.draw_count>16||s.grant_count>128||s.pending>1||s.last_cycle>s.cycle||s.cycle-s.last_cycle>1)return false;
    if(!s.claims)return !s.cycle&&!s.group&&!s.step&&!s.last_box&&!s.last_cost&&!s.draw_count&&!s.grant_count&&!s.pending&&!s.last_cycle;
    if(s.last_box<=0||s.last_cost<=0||!s.draw_count||!s.grant_count||s.cycle>s.claims)return false;
    for(uint32_t i=0;i<s.draw_count;++i){if(s.draws[i].item<=0||s.draws[i].amount<=0)return false;
        for(uint32_t j=0;j<i;++j)if(s.draws[j].item==s.draws[i].item)return false;}
    for(uint32_t i=0;i<s.grant_count;++i){const auto& g=s.grants[i];if(g.id<0||g.amount<=0||!inventory_types::reward(g.type)||((g.type<=2)&&g.id))return false;}
    return true;
}
}
