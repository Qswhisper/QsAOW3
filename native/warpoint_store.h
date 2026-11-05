// Exactly-once bridge from a durable real result to the existing growth wallet.
#pragma once
#include "local_progress.h"
#include "local_match_store.h"
namespace warpoint_store {
inline bool reconcile(local_progress::State& s,const local_match_store::State& match,bool& changed){
    changed=false;if(!local_match_store::valid(match)||match.settled<s.warpoint_pvp_cursor)return false;
    if(match.next!=match.settled)return true;
    bool free_changed=false;auto shop=s.shop;
    if(!shop_box_policy::finish(shop,shop_box_policy::Formal,match.settled,local_match_store::formal(match)&&match.reason==1&&(match.outcome==local_match_store::Victory||match.outcome==local_match_store::Defeat),free_changed))return false;
    if(match.version<5||!match.reserved2||match.settled==s.warpoint_pvp_cursor){if(free_changed){s.shop=shop;changed=true;}return true;}
    if(!local_progress::credit_pvp_warpoints(s,match.settled,match.reserved,local_match_store::career_stars(match),local_match_store::victory_credits(match)))return false;
    s.shop=shop;changed=true;return true;
}
}
