#pragma once
#include "local_progress.h"
#include "contract_box_policy.h"
#include "supply_policy.h"
namespace contract_box_store {
inline bool claim(local_progress::State& s,int track,int rank,int id,const supply_policy::Draw& draw,const supply_types::Grant* grants,int count){
    auto* b=contract_box_policy::next(s.contract_boxes,track,rank);
    if(!b||b->id!=id||!supply_policy::valid_draw_pool(*b,contract_box_content::items,draw,true)||!grants||count<1||count>128)return false;
    local_progress::StateBuffer storage;if(!storage.data)return false;auto& next=*storage.data;next=s;
    if(!contract_box_policy::advance(next.contract_boxes,next.contract_points,track,rank,id))return false;
    auto& l=next.contract_boxes;l.draw_count=draw.count;l.grant_count=count;
    for(int i=0;i<draw.count;++i)l.draws[i]=draw.rows[i];
    for(int i=0;i<count;++i){const auto& g=grants[i];l.grants[i]=g;if(g.amount<=0||!effective_box_policy::future_grant(g.type))return false;
        if(g.type<=2){int* target=g.type==0?&next.credits:g.type==1?&next.gold:&next.supply.tokens;
            if(g.type<0||g.id||int64_t(*target)+g.amount>INT_MAX)return false;*target+=g.amount;}
        else if(g.type==7){if(!obsolete_reward_policy::speedup(g.type,g.id))return false;}
        else if(!local_progress::grant_stock(next,g.type,g.id,g.amount))return false;
    }
    if(!contract_box_types::valid(l,next.contract_points))return false;s=next;return true;
}
}
