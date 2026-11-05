#pragma once
#include <string.h>
#include "supply_types.h"
namespace contract_box_types {
struct Track {int32_t spent,group;uint32_t step,reserved;uint64_t cycles;};
struct Ledger {
    uint32_t initialized;int32_t baseline;
    Track tracks[2];
    uint64_t claims;
    int32_t last_box;uint32_t pending,draw_count,grant_count;
    supply_types::Selected draws[16];supply_types::Grant grants[128];
};
static_assert(sizeof(Ledger)==1744,"Contract box journal ABI changed");
inline bool valid(const Ledger& l,int points){
    if(l.initialized>1||l.baseline<0||l.baseline>points||l.pending>1||l.draw_count>16||l.grant_count>128)return false;
    if(!l.initialized){const Ledger zero{};return !memcmp(&l,&zero,sizeof(l));}
    for(int i=0;i<2;++i){const auto& t=l.tracks[i];if(t.spent<0||t.spent>points-l.baseline||t.group<0||t.reserved||t.step>uint32_t(i?1:4)||(t.group==0)!=(t.step==0)||t.cycles>l.claims)return false;}
    if(!l.claims)return !l.last_box&&!l.pending&&!l.draw_count&&!l.grant_count;
    if(l.last_box<=0||l.draw_count<1||l.grant_count<1)return false;
    for(uint32_t i=0;i<l.draw_count;++i){if(l.draws[i].item<=0||l.draws[i].amount<=0)return false;for(uint32_t j=0;j<i;++j)if(l.draws[i].item==l.draws[j].item)return false;}
    for(uint32_t i=0;i<l.grant_count;++i){const auto& g=l.grants[i];if(g.amount<=0||g.id<0||!inventory_types::reward(g.type)||(g.type<=2&&g.id))return false;}
    return true;
}
}
