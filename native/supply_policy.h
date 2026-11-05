// Offline supply cycle and draw rules; independent of the server RNG.
#pragma once
#include <climits>
#include "supply_types.h"
#include "supply_content.generated.h"
#include "hero_card_policy.h"
#include "effective_box_policy.h"
namespace supply_policy {
using namespace supply_content;
enum Eligibility {Unsupported=-1,Ineligible=0,Eligible=1};
struct Draw {supply_types::Selected rows[16];int count=0;};
inline const Box* box(int id){for(const auto& b:boxes)if(b.id==id)return &b;return nullptr;}
inline const Group* group(int rank){const Group* found=nullptr;for(const auto& g:groups)if(g.rank<=rank&&(!found||g.rank>found->rank))found=&g;return found;}
inline const Group* active_group(const supply_types::Ledger& s,int rank){if(rank<1||s.group>rank)return nullptr;const auto* g=group(s.group?s.group:rank);return g&&(!s.group||g->rank==s.group)?g:nullptr;}
inline bool next(const supply_types::Ledger& s,int rank,const Box*& b,int& cost){
    b=nullptr;cost=0;if(!supply_types::valid(s)||s.pending)return false;const auto* g=active_group(s,rank);
    if(!g||s.step>=g->count)return false;b=box(g->boxes[s.step]);const auto* previous=s.step?box(g->boxes[s.step-1]):nullptr;
    if(!b||b->rank!=g->rank||b->points<=0||(s.step&&!previous))return false;
    cost=b->points-(previous?previous->points:0);return cost>0;
}
inline bool uniform(uint64_t& state,uint64_t bound,uint64_t& result){
    if(!bound)return false;uint64_t threshold=-bound%bound,value;
    do{value=hero_card_policy::draw(state);}while(value<threshold);result=value%bound;return true;
}
inline uint64_t seed(int faction,uint64_t claims,int box_id){
    uint64_t state=claims^(uint64_t(uint32_t(box_id))<<32)^(uint64_t(faction)<<56)^0x537570706c794c31ULL;
    return hero_card_policy::draw(state);
}
inline bool draw_pool(const Box& b,const Item* items,const Eligibility* eligibility,int count,uint64_t receipt,Draw& out){
    if(!eligibility||count!=b.count||count<1||count>128||b.minimum<1||b.maximum<b.minimum||b.maximum>16)return false;
    int mandatory=0,eligible=0;bool used[128]={};Draw selected;
    for(int i=0;i<count;++i){const auto& item=items[b.first+i];if(item.prob<=0||effective_box_policy::removed(item.type,item.type_id))continue;
        if(eligibility[i]==Unsupported)return false;
        if(item.minimum<=0||item.maximum<item.minimum)return false;
        if(eligibility[i]!=Eligible){if(item.prob_txt==100)return false;continue;}
        ++eligible;if(item.prob_txt==100){used[i]=true;++mandatory;}}
    if(mandatory>b.minimum||eligible<b.maximum)return false;
    uint64_t state=receipt,value=0;if(!uniform(state,uint64_t(b.maximum-b.minimum+1),value))return false;int target=b.minimum+int(value);
    for(int i=0;i<count;++i)if(used[i])selected.rows[selected.count++]={items[b.first+i].id,0};
    while(selected.count<target){uint64_t total=0;
        for(int i=0;i<count;++i)if(!used[i]&&eligibility[i]==Eligible&&items[b.first+i].prob>0&&!effective_box_policy::removed(items[b.first+i].type,items[b.first+i].type_id))total+=uint64_t(items[b.first+i].prob);
        if(!uniform(state,total,value))return false;int chosen=-1;
        for(int i=0;i<count;++i)if(!used[i]&&eligibility[i]==Eligible&&items[b.first+i].prob>0&&!effective_box_policy::removed(items[b.first+i].type,items[b.first+i].type_id)){uint64_t weight=items[b.first+i].prob;
            if(value<weight){chosen=i;break;}value-=weight;}
        if(chosen<0)return false;used[chosen]=true;selected.rows[selected.count++]={items[b.first+chosen].id,0};
    }
    for(int n=0;n<selected.count;++n){const Item* item=nullptr;for(int i=0;i<count;++i)if(items[b.first+i].id==selected.rows[n].item)item=&items[b.first+i];
        if(!item||!uniform(state,uint64_t(item->maximum-item->minimum)+1,value))return false;selected.rows[n].amount=item->minimum+int(value);}
    out=selected;return true;
}
inline bool draw(const Box& b,const Eligibility* e,int n,uint64_t seed,Draw& out){return draw_pool(b,items,e,n,seed,out);}
inline bool valid_draw_pool(const Box& b,const Item* items,const Draw& d,bool future=false){
    if(d.count<b.minimum||d.count>b.maximum||d.count>16)return false;
    for(int n=0;n<d.count;++n){const Item* item=nullptr;
        for(int i=0;i<b.count;++i)if(items[b.first+i].id==d.rows[n].item)item=&items[b.first+i];
        if(!item||item->prob<=0||d.rows[n].amount<item->minimum||d.rows[n].amount>item->maximum||
           (future&&effective_box_policy::removed(item->type,item->type_id)))return false;
        for(int j=0;j<n;++j)if(d.rows[j].item==d.rows[n].item)return false;}
    for(int i=0;i<b.count;++i){const auto& item=items[b.first+i];if(item.prob>0&&item.prob_txt==100&&(!future||!effective_box_policy::removed(item.type,item.type_id))){
        bool found=false;for(int n=0;n<d.count;++n)if(d.rows[n].item==item.id)found=true;if(!found)return false;}}
    return true;
}
inline bool valid_draw(const Box& b,const Draw& d){return valid_draw_pool(b,items,d);}
inline bool advance(supply_types::Ledger& s,int rank,int expected_box,int cost,const Draw& drawn,const supply_types::Grant* grants,int grant_count){
    const Box* b=nullptr;int required=0;if(!next(s,rank,b,required)||b->id!=expected_box||cost!=required||s.claims==UINT64_MAX||
       !valid_draw_pool(*b,items,drawn,true)||!grants||grant_count<1||grant_count>128)return false;
    const auto* g=active_group(s,rank);auto result=s;++result.claims;result.last_cycle=s.cycle;result.last_box=b->id;result.last_cost=cost;
    result.group=g->rank;++result.step;result.draw_count=drawn.count;result.grant_count=grant_count;result.pending=1;
    for(int i=0;i<drawn.count;++i)result.draws[i]=drawn.rows[i];for(int i=0;i<grant_count;++i)result.grants[i]=grants[i];
    if(result.step==g->count){if(result.cycle==UINT64_MAX)return false;++result.cycle;result.group=result.step=0;}
    if(!supply_types::valid(result))return false;s=result;return true;
}
}
