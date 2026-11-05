// Original public point rules and durable PvP-result reconciliation only.
// Supply cycles and randomized claims are separate, currently pending policy.
#pragma once
#include "warpoint_rewards.h"
#include "warpoint_store.h"
namespace local_warpoints {
static bool ready;
static warpoint_rewards::Rule solo,team;
static bool model_hooks;
static bool balance_view_written;
static int balance_view_amount=-1;
static int (*points_projection)();
static void (*claim_handler)(void*,void*,void*);
static int points(void*,void*){return points_projection?points_projection():progress_ready?progress.quest_warpoints:0;}
static int no_limit(void*,void*){return 0;}
static bool limit_reached(void*,void*){return false;}
static void claim_pending_policy(void* model,void* box,void* method){if(claim_handler)claim_handler(model,box,method);else LOG("Local supply claim unavailable: local reward service not ready");}
static bool install(){
    const uint32_t getter[]={0xf81f0ffe,0xf9402000,0xb4000160,0xf9400008};
    const uint32_t cap[]={0xf81f0ffe,0xf9402000,0xb4000160,0xf9400008};
    const uint32_t reached[]={0xf81e0ffe,0xa9014ff4,0xaa0003f3,0x97fffea2};
    bool a=install_hook(0x7c026e8,getter,(void*)points)!=nullptr;
    bool b=install_hook(0x7c02758,cap,(void*)no_limit)!=nullptr;
    bool c=install_hook(0x7c02c8c,reached,(void*)limit_reached)!=nullptr;
    const uint32_t claim[]={0xd10103ff,0xa9015ffe,0xa90257f6,0xa9034ff4};
    bool d=install_hook(0x7c03ba4,claim,(void*)claim_pending_policy)!=nullptr;
    model_hooks=a&&b&&c&&d;LOG("Local supply point getters installed=%d no_daily_cap=1 claims_enabled=0",model_hooks);return model_hooks;
}
static bool initialize(){
    if(ready)return true;void* rows=local_career::list("SHARED","warPointsInfo");
    int count=rows?*(int*)((char*)rows+0x18):0;if(count<1||count>128)return false;
    bool found[2]={};warpoint_rewards::Rule rules[2];
    for(int i=0;i<count;++i){void* row=list_item(rows,i);if(!row)return false;int id=*(int*)((char*)row+0x10),slot=id==1?0:id==4?1:-1;if(slot<0)continue;
        void* params=*(void**)((char*)row+0x38);if(found[slot]||!params||array_length(params)!=4||
            !warpoint_rewards::from_hint((int*)((char*)params+32),4,*(int*)((char*)row+0x14),*(int*)((char*)row+0x18),rules[slot]))return false;
        found[slot]=true;
    }
    if(!found[0]||!found[1])return false;solo=rules[0];team=rules[1];ready=true;
    LOG("Local public war-point rules ready=1 solo=%d team=%d no_daily_cap=1",solo.victory,team.victory);return true;
}
static bool stamp_for(local_match_store::State& match,const local_progress::State& earned){
    if(match.next!=match.settled||match.outcome<local_match_store::Victory||match.outcome>local_match_store::Error)return false;
    if(match.version<5||!match.reserved2)return !match.reserved;
    if(!initialize()||(match.mode!=1&&match.mode!=2&&match.mode!=4))return false;
    int amount=0;auto outcome=(warpoint_rewards::Outcome)(match.outcome-1);
    if(!warpoint_rewards::award(match.mode==1?solo:team,warpoint_rewards::FormalPvp,outcome,match.reason,match.tick,amount))return false;
    // Freeze the complete result before its durable ledger write. Replays use
    // this amount, even if skills are learned after returning to the hall.
    if(amount&&!local_reward_skills::apply(earned,24,amount,amount))return false;
    match.reserved=amount;return true;
}
static bool stamp(local_match_store::State& match){return stamp_for(match,progress);}
static bool sync(){
    if(!progress_ready)return false;local_match_store::State match;int status=local_match_store::load(local_faction::root(),match);
    if(status<0)return false;if(!status)return true;
    local_progress::StateBuffer storage;if(!storage.data)return false;auto& next=*storage.data;next=progress;bool changed=false;if(!warpoint_store::reconcile(next,match,changed))return false;
    if(!changed)return true;
    if(!local_progress::write(local_faction::root(),next))return false;
    progress=next;LOG("Local PvP result committed faction=%d run=%llu warpoints=%d balance=%d career_stars=%d stars=%d credits_award=%d credits=%d",local_faction::active,(unsigned long long)match.settled,match.reserved,progress.quest_warpoints,local_match_store::career_stars(match),progress.stars,local_match_store::victory_credits(match),progress.credits);return true;
}
}
