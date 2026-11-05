// Independent exactly-once bridge; a legacy v1 result never creates credit.
#pragma once
#include "cooperative_store.h"
#include "local_progress.h"
namespace cooperative_credit_store {
inline bool reconcile(local_progress::State& s,const cooperative_store::State& game,bool& changed){
    changed=false;if(s.credits<0||!cooperative_store::valid(game)||game.match.settled<s.cooperative_credit_cursor)return false;
    if(game.match.next!=game.match.settled)return true;
    bool free_changed=false;auto shop=s.shop;
    if(!shop_box_policy::finish(shop,shop_box_policy::Cooperative,game.match.settled,game.match.reason==1&&(game.match.outcome==local_match_store::Victory||game.match.outcome==local_match_store::Defeat),free_changed))return false;
    if(!game.credit_run||game.credit_run==s.cooperative_credit_cursor){if(free_changed){s.shop=shop;changed=true;}return true;}
    if(game.credit_run!=game.match.settled||game.credit_run<s.cooperative_credit_cursor||int64_t(s.credits)+game.credits>INT_MAX)return false;
    local_progress::StateBuffer copy;if(!copy.data)return false;auto& next=*copy.data;next=s;
    next.shop=shop;next.credits+=game.credits;next.cooperative_credit_cursor=game.credit_run;s=next;changed=true;return true;
}
}
