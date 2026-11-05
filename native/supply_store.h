// One growth-save transaction: debit the threshold delta, grant and journal.
#pragma once
#include "local_progress.h"
#include "supply_policy.h"
namespace supply_store {
inline bool claim(local_progress::State& s,int rank,int box_id,const supply_policy::Draw& draw,const supply_types::Grant* grants,int count){
    const supply_content::Box* box=nullptr;int cost=0;
    if(!supply_policy::next(s.supply,rank,box,cost)||box->id!=box_id||s.quest_warpoints<cost||!grants||count<1||count>128)return false;
    auto next=s;
    if(!supply_policy::advance(next.supply,rank,box_id,cost,draw,grants,count))return false;
    for(int i=0;i<count;++i){const auto& g=grants[i];if(!effective_box_policy::future_grant(g.type))return false;
        if(g.type<=2){int* target=g.type==0?&next.credits:g.type==1?&next.gold:&next.supply.tokens;
            if(g.id||g.amount<=0||int64_t(*target)+g.amount>INT_MAX)return false;*target+=g.amount;}
        else if(g.type==7){if(!obsolete_reward_policy::speedup(g.type,g.id))return false;}
        else if(!local_progress::grant_stock(next,g.type,g.id,g.amount))return false;
    }
    next.quest_warpoints-=cost;s=next;return true;
}
}
