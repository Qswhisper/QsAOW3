// Normal formal results consume their frozen original event counts once.
// The five new receipts use baseline as capped progress since the last claim.
// Existing research/quest/win/free-box receipt baselines keep their old meaning.
#pragma once
#include "contract_policy.h"
#include "local_match_store.h"
namespace battle_contract_policy {
inline int slot(int id){
    for(int i=0;i<battle_contract_events::count;++i)if(contract_content::battle_rules[i].id==id)return i;
    for(int i=0;i<battle_contract_events::result_count;++i)if(contract_content::result_rules[i].id==id)return battle_contract_events::count+i;
    if(contract_content::timed_rules[0].id==id)return 6;
    for(int i=0;i<boost_contract_events::count;++i)if(contract_content::boost_rules[i].id==id)return 7+i;
    if(contract_content::activity_rules[0].id==id)return 9;
    return -1;
}
inline int target(int slot){
    if(slot==9)return contract_content::activity_rules[0].value;
    if(slot==6)return contract_content::timed_rules[0].value;
    if(slot>=7&&slot<7+boost_contract_events::count)return contract_content::boost_rules[slot-7].value;
    if(slot<0||slot>=battle_contract_events::count+battle_contract_events::result_count)return -1;
    return slot<battle_contract_events::count?contract_content::battle_rules[slot].value:contract_content::result_rules[slot-battle_contract_events::count].value;
}
inline bool timed_win(const local_match_store::State& match,const contract_content::ResultRule& rule){
    // Original TYPE_PVP_WINS_FAST stores minutes in Subtype, not Value.
    // The existing native result clock is ten logic ticks per second.
    return local_match_store::formal(match)&&local_match_store::normal(match)&&match.outcome==local_match_store::Victory&&
        rule.subtype>0&&rule.subtype<=INT_MAX/600&&match.tick>=0&&match.tick<=rule.subtype*600;
}
inline int count(const local_progress::State& s,int id){
    int i=slot(id);auto* r=contract_policy::receipt(s,id);
    if(i<0||!r||r->baseline>(uint64_t)target(i))return -1;
    return (int)r->baseline;
}
inline bool reconcile(local_progress::State& s,const local_match_store::State& match){
    if(!local_match_store::valid(match)||match.next!=match.settled)return false;
    if(match.settled==s.contract_pvp_cursor)return true;
    bool win=local_match_store::formal(match)&&match.reason==1&&match.outcome==local_match_store::Victory;
    if(!s.contract_count||s.contract_pvp_cursor==UINT64_MAX||match.settled!=s.contract_pvp_cursor+1||
       (win&&match.mode==1&&s.contract_pvp_wins==UINT64_MAX))return false;
    if(match.contract_mask&&!local_match_store::formal(match))return false;
    for(int i=0;i<battle_contract_events::count;++i)if(match.contract_mask&(1u<<i)){
        if(count(s,contract_content::battle_rules[i].id)<0)return false;
    }
    for(int i=0;i<battle_contract_events::result_count;++i)if(match.contract_mask&(1u<<(battle_contract_events::count+i))){
        if(count(s,contract_content::result_rules[i].id)<0)return false;
    }
    if((match.contract_mask&battle_contract_events::timed_mask)&&count(s,contract_content::timed_rules[0].id)<0)return false;
    for(int i=0;i<boost_contract_events::count;++i)if((match.contract_mask&boost_contract_events::eligible(i))&&count(s,contract_content::boost_rules[i].id)<0)return false;
    if((match.contract_mask&activity_contract_events::eligible)&&count(s,contract_content::activity_rules[0].id)<0)return false;
    // Validate everything above before changing a single receipt or cursor.
    for(int i=0;i<battle_contract_events::count;++i)if(local_match_store::normal(match)&&match.contract_mask&(1u<<i)){
        const auto& rule=contract_content::battle_rules[i];
        if((rule.flags&1)&&match.outcome!=local_match_store::Victory)continue;
        auto* r=const_cast<local_progress::ContractReceipt*>(contract_policy::receipt(s,rule.id));
        uint64_t total=r->baseline+match.contract_events.values[i];r->baseline=total>(uint64_t)rule.value?rule.value:total;
    }
    for(int i=0;i<battle_contract_events::result_count;++i)if(match.contract_mask&(1u<<(battle_contract_events::count+i))){
        const auto& rule=contract_content::result_rules[i];if(!timed_win(match,rule))continue;
        auto* r=const_cast<local_progress::ContractReceipt*>(contract_policy::receipt(s,rule.id));
        if(r->baseline<(uint64_t)rule.value)++r->baseline;
    }
    if(match.contract_mask&battle_contract_events::timed_completed){
        const auto& rule=contract_content::timed_rules[0];
        auto* r=const_cast<local_progress::ContractReceipt*>(contract_policy::receipt(s,rule.id));
        if(r->baseline<(uint64_t)rule.value)++r->baseline;
    }
    for(int i=0;i<boost_contract_events::count;++i)if(match.contract_mask&boost_contract_events::completed(i)){
        const auto& rule=contract_content::boost_rules[i];
        auto* r=const_cast<local_progress::ContractReceipt*>(contract_policy::receipt(s,rule.id));
        if(r->baseline<(uint64_t)rule.value)++r->baseline;
    }
    if(match.contract_mask&activity_contract_events::completed){
        const auto& rule=contract_content::activity_rules[0];
        auto* r=const_cast<local_progress::ContractReceipt*>(contract_policy::receipt(s,rule.id));
        if(r->baseline<(uint64_t)rule.value)++r->baseline;
    }
    s.contract_pvp_cursor=match.settled;if(win&&match.mode==1)++s.contract_pvp_wins;
    return true;
}
inline bool claim(local_progress::State& s,int id,int credits,int gold,int xp,int points){
    int i=slot(id);if(i<0||count(s,id)!=target(i))return false;
    return contract_policy::award_claim(s,id,0,credits,gold,xp,points);
}
}
