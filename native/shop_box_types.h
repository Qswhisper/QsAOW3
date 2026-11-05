// One pending free box. Starts are registered before the independent run save.
#pragma once
#include "supply_types.h"
namespace shop_box_policy {
enum Mode {Campaign=0,Formal=1,Cooperative=2};
struct Ledger {
    uint64_t claims=0,allowed[3]={},cursor[3]={};
    uint32_t games=0,pending=0,draw_count=0,grant_count=0;
    supply_types::Selected draws[16]={};
    supply_types::Grant grants[128]={};
};
static_assert(sizeof(Ledger)==1736,"Free box ledger ABI changed");
inline bool ready(const Ledger& s){return !s.claims||s.games==3;}
inline bool valid(const Ledger& s){
    if(s.games>3||s.pending>1||(s.pending&&s.games)||s.draw_count>16||s.grant_count>128)return false;
    for(int i=0;i<3;++i)if(s.cursor[i]>s.allowed[i])return false;
    if(!s.claims)return !s.games&&!s.pending&&!s.draw_count&&!s.grant_count;
    if(s.draw_count<2||s.draw_count>3||!s.grant_count)return false;
    for(uint32_t i=0;i<s.draw_count;++i){if(s.draws[i].item<12000||s.draws[i].item>12031||s.draws[i].amount<=0)return false;
        for(uint32_t j=0;j<i;++j)if(s.draws[j].item==s.draws[i].item)return false;}
    for(uint32_t i=0;i<s.grant_count;++i){const auto& g=s.grants[i];if(g.id<0||g.amount<=0||
        (!inventory_types::reward(g.type)&&!(g.type==15&&g.id==1))||(g.type<=2&&g.id))return false;}
    return true;
}
inline bool begin(Ledger& s,Mode mode,uint64_t run){
    if(mode<Campaign||mode>Cooperative||!run||run<s.allowed[mode]||!valid(s))return false;
    if(run==s.allowed[mode])return run>s.cursor[mode];s.allowed[mode]=run;return true;
}
inline bool finish(Ledger& s,Mode mode,uint64_t run,bool normal,bool& changed){
    changed=false;if(mode<Campaign||mode>Cooperative||!valid(s))return false;
    if(!run||run!=s.allowed[mode]||run<=s.cursor[mode])return true;
    s.cursor[mode]=run;if(normal&&!ready(s)&&!s.pending)++s.games;changed=true;return true;
}
}
