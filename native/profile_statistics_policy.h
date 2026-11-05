// Read-only projection of committed local progress into the original profile.
#pragma once
#include "local_progress.h"
#include "local_history_store.h"
namespace profile_statistics_policy {
struct Snapshot {
    int campaign_xp=0,campaign_stars=0,campaign_battles=0,campaign_wins=0;
    int completed=0,three_star=0,three_star_hard=0,pvp_stars=0;
    int pvp_battles=0,pvp_wins=0,total_ratio=0,current_ratio=50;
    bool current_known=true;
};
inline uint64_t count(const local_match_store::State& s){return uint64_t(s.wins)+s.losses+s.draws;}
inline int bounded(uint64_t n){return n>INT_MAX?INT_MAX:int(n);}
inline int ratio(uint64_t wins,uint64_t games){return games?int(wins*100/games):0;}
inline bool recent(const local_match_store::State& current,const std::vector<local_history_store::Record>& records,int faction,int& value){
    uint64_t n=count(current);
    if(n<=50){value=n?ratio(current.wins,n):50;return true;}
    std::vector<local_history_store::Record> all;
    for(const auto& r:records)if(r.faction==uint32_t(faction)&&r.kind==local_history_store::Pvp&&local_history_store::valid(r))all.push_back(r);
    local_history_store::Record latest;
    if(local_history_store::scored(current)&&local_history_store::make(faction,current,0,latest)&&!local_history_store::merge(all,latest))return false;
    std::sort(all.begin(),all.end(),[](const local_history_store::Record& a,const local_history_store::Record& b){return local_history_store::match(a).settled>local_history_store::match(b).settled;});
    uint64_t wins=current.wins,losses=current.losses,draws=current.draws;int found=0,won=0;
    for(const auto& r:all){
        auto s=local_history_store::match(r);
        // Match cumulative counters as well as ordering. Missing interrupted
        // runs are harmless; a missing scored run makes this window unknown.
        if(s.wins!=wins||s.losses!=losses||s.draws!=draws)return false;
        if(s.outcome==local_match_store::Victory){if(!wins)return false;--wins;++won;}
        else if(s.outcome==local_match_store::Draw){if(!draws)return false;--draws;}
        else {if(!losses)return false;--losses;}
        if(++found==50){value=won*2;return true;}
    }
    return false;
}
inline bool project(const local_progress::State& progress,const local_match_store::State& pvp,
                    const std::vector<local_history_store::Record>& records,int faction,Snapshot& out){
    if(faction<1||faction>2||progress.count>local_progress::kCapacity||progress.stars<0||pvp.mode==3)return false;
    Snapshot s;uint64_t xp=0,stars=0,battles=0,wins=0;
    for(uint32_t i=0;i<progress.count;++i){const auto& m=progress.missions[i];
        if(m.id<0||m.wins<0||m.losses<0||m.stars<0||m.stars>3||m.xp<0)return false;
        for(uint32_t j=0;j<i;++j)if(progress.missions[j].id==m.id)return false;
        xp+=m.xp;stars+=m.stars;battles+=uint64_t(m.wins)+m.losses;wins+=m.wins;
        if(m.wins){++s.completed;if(m.stars==3)++s.three_star;}
    }
    if(xp>INT_MAX||stars>uint64_t(progress.stars))return false;
    s.campaign_xp=int(xp);s.campaign_stars=int(stars);s.campaign_battles=bounded(battles);s.campaign_wins=bounded(wins);
    // Campaign best stars and formal victory stars are the only
    // implemented local rank-star sources. No new award is made here.
    s.pvp_stars=progress.stars-int(stars);
    s.pvp_battles=bounded(count(pvp));s.pvp_wins=bounded(pvp.wins);s.total_ratio=ratio(pvp.wins,count(pvp));
    s.current_known=recent(pvp,records,faction,s.current_ratio);
    if(!s.current_known)s.current_ratio=-1; // Presentation sentinel, never saved.
    out=s;return true;
}
}
