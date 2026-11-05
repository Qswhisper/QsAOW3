// no-calendar contracts: one manual claim consumes only new real
// events since that contract's last baseline. No task assignment or fake wins.
#pragma once
#include "local_progress.h"
namespace contract_policy {
inline bool continued(int id){static constexpr int ids[]={20010,20020,20080,20060,20040,20090,21290,20490,20670,20900,20460,20880,21020,20760};
    for(int value:ids)if(value==id)return true;return false;}
inline bool rank_allowed(int id,int rank,int minimum,int maximum){return rank>=4&&rank>=minimum&&(!maximum||rank<=maximum||(continued(id)&&minimum==4&&maximum==6));}
inline int published_rank_max(int id,int minimum,int maximum){return continued(id)&&minimum==4&&maximum==6?0:maximum;}
inline const local_progress::ContractReceipt* receipt(const local_progress::State& s,int id){
    for(uint32_t i=0;i<s.contract_count;++i)if(s.contracts[i].id==id)return s.contracts+i;return nullptr;
}
inline bool initialize(local_progress::State& s,int id,uint64_t counter){
    if(id<0||receipt(s,id)||s.contract_count==64)return false;
    s.contracts[s.contract_count++]={id,0,counter};return true;
}
inline uint64_t quest_claims(const local_progress::State& s){uint64_t n=0;for(uint32_t i=0;i<s.quest_count;++i)if(s.quests[i].state==32)++n;return n;}
inline uint64_t event_counter(const local_progress::State& s,int kind){
    return kind==1?s.upgrade_count:kind==2?quest_claims(s):kind==3?s.contract_pvp_wins:kind==4?s.shop.claims:0;
}
inline int count(const local_progress::State& s,int id,uint64_t counter,int target){
    auto* r=receipt(s,id);if(!r||target<1||counter<r->baseline)return -1;
    uint64_t delta=counter-r->baseline;return delta>(uint64_t)target?target:(int)delta;
}
inline bool award_claim(local_progress::State& s,int id,uint64_t next_baseline,int credits,int gold,int xp,int points){
    auto* r=receipt(s,id);if(!r||r->claims==UINT32_MAX||credits<0||gold<0||xp<0||points<0)return false;
    if(int64_t(s.credits)+credits>INT_MAX||int64_t(s.gold)+gold>INT_MAX||int64_t(s.experience)+xp>INT_MAX||int64_t(s.contract_points)+points>INT_MAX)return false;
    auto next=s;auto* item=next.contracts+(r-s.contracts);++item->claims;
    // Discard pre-claim overflow: every next contract needs NEW behavior.
    item->baseline=next_baseline;next.credits+=credits;next.gold+=gold;next.experience+=xp;next.contract_points+=points;s=next;return true;
}
inline bool claim(local_progress::State& s,int id,uint64_t counter,int target,int credits,int gold,int xp,int points){
    if(count(s,id,counter,target)!=target)return false;
    return award_claim(s,id,counter,credits,gold,xp,points);
}
inline bool record_pvp(local_progress::State& s,uint64_t run,int mode,bool real_win){
    if(!s.contract_count||!run||mode<1||mode>4)return false;
    if(run==s.contract_pvp_cursor)return true;
    if(s.contract_pvp_cursor==UINT64_MAX||run!=s.contract_pvp_cursor+1||(mode==1&&real_win&&s.contract_pvp_wins==UINT64_MAX))return false;
    auto next=s;next.contract_pvp_cursor=run;if(mode==1&&real_win)++next.contract_pvp_wins;s=next;return true;
}
inline bool credits_bonus(int base,int effect,int level,int& out){
    if(base<0||effect<0||level<0||level>32)return false;
    int64_t bonus=int64_t(effect)*level,total=int64_t(base)+int64_t(base)*bonus/1000;
    // Original SkillsController multiplies in int32; never emulate wrapping.
    if(bonus>INT_MAX||int64_t(base)*bonus>INT_MAX||total>INT_MAX)return false;out=(int)total;return true;
}
}
