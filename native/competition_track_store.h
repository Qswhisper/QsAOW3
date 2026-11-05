// Wallet-owned track cursor and credit reconciliation share one commit.
#pragma once
#include "competition_track_policy.h"
#include "competition_track_content.generated.h"
#include "cooperative_credit_store.h"
namespace competition_track_store {
inline bool reconcile(local_progress::State& s,const cooperative_store::State& game,bool& changed){
    changed=false;if(!cooperative_store::valid(game))return false;
    local_progress::StateBuffer copy;if(!copy.data)return false;*copy.data=s;bool credit=false,track=false;
    if(!cooperative_credit_store::reconcile(*copy.data,game,credit))return false;
    if(game.match.next==game.match.settled&&game.match.settled&&
       !competition_track_policy::settle(copy.data->competition_tracks,competition_track_content::competition,game.match.settled,
           game.match.outcome==local_match_store::Victory&&game.match.reason==1,competition_track_content::maximum,track))return false;
    changed=credit||track;if(changed)s=*copy.data;return true;
}
inline bool claim(local_progress::State& s,int row,bool premium,const supply_types::Grant* grants,int count){
    if(row<0||row>=20||count<0||count>128||(!grants&&count))return false;
    local_progress::StateBuffer copy;if(!copy.data)return false;*copy.data=s;
    local_progress::CareerReward rewards[128];for(int i=0;i<count;++i)rewards[i]={grants[i].type,grants[i].id,grants[i].amount};
    if(!local_progress::apply_rewards(*copy.data,rewards,count)||
       !competition_track_policy::stage(copy.data->competition_tracks,competition_track_content::competition,competition_track_content::definition,
           row,premium,competition_track_content::thresholds[row],grants,count))return false;
    s=*copy.data;return true;
}
}
