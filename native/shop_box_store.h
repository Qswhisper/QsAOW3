#pragma once
#include "local_progress.h"
#include "supply_policy.h"
#include "shop_box_content.generated.h"
namespace shop_box_store {
inline bool claim(local_progress::State& s,const supply_policy::Draw& draw,const supply_types::Grant* grants,int count){
    if(!shop_box_policy::valid(s.shop)||!shop_box_policy::ready(s.shop)||s.shop.pending||s.shop.claims==UINT64_MAX||
       !supply_policy::valid_draw_pool(shop_box_content::box,shop_box_content::items,draw,true)||!grants||count<1||count>128)return false;
    local_progress::StateBuffer storage;if(!storage.data)return false;auto& next=*storage.data;next=s;
    ++next.shop.claims;next.shop.games=0;next.shop.pending=1;next.shop.draw_count=draw.count;next.shop.grant_count=count;
    for(int i=0;i<draw.count;++i)next.shop.draws[i]=draw.rows[i];
    for(int i=0;i<count;++i){const auto& g=grants[i];next.shop.grants[i]=g;if(g.amount<=0||!effective_box_policy::future_grant(g.type))return false;
        if(g.type<=2){int* target=g.type==0?&next.credits:g.type==1?&next.gold:&next.supply.tokens;
            if(g.type<0||g.id||int64_t(*target)+g.amount>INT_MAX)return false;*target+=g.amount;}
        else if(g.type==15){if(g.id!=1)return false;}
        else if(g.type==7){if(!obsolete_reward_policy::speedup(g.type,g.id))return false;}
        else if(!local_progress::grant_stock(next,g.type,g.id,g.amount))return false;
    }
    if(!shop_box_policy::valid(next.shop))return false;s=next;return true;
}
}
