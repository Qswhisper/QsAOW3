// Detached native definitions/reward/configuration tests; no player claim/save.
#pragma once
namespace supply_audit {
static int model_state(void* live,void* cfg,bool enabled){
    void* rows=nullptr;void* ex=nullptr;void* args[]={cfg,saved("SHARED"),local_supply::get(live,0x38),&enabled,&rows};
    void* value=invoke_named(live,"GetModelState",5,args,&ex);return value&&!ex?*(int*)((char*)value+16):-1;
}
static bool private_unlock(local_progress::State& s,int faction){
    void* stones=local_career::list(local_career::key(faction),"careerStones");int n=stones?*(int*)((char*)stones+0x18):0;
    for(int i=0;i<n;++i){void* stone=list_item(stones,i),*rewards=local_supply::get(stone,0x18);int count=rewards?*(int*)((char*)rewards+0x18):0;
        for(int j=0;j<count;++j){void* r=list_item(rewards,j);if(*(int*)((char*)r+0x14)==18&&*(int*)((char*)r+0x18)==12){
            s.stars=*(int*)((char*)stone+0x20);s.career_count=1;s.career_ids[0]=*(int*)((char*)stone+0x10);return local_supply::unlocked(s,faction);}}}
    return false;
}
static void run(){
    if(!progress_ready||access(DATA_ROOT "supply-audit.request",F_OK))return;unlink(DATA_ROOT "supply-audit.request");auto before=progress;
    FILE* f=fopen(DATA_ROOT "supply-audit.jsonl","w");if(!f)return;chmod(DATA_ROOT "supply-audit.jsonl",0644);
    bool ok=local_supply::installed&&local_supply::initialize();int adapter=0,messages=0,filtered=0,blocked=0;
    for(int faction:{1,2})for(int rank:{1,6}){void* definition=local_career::lookup("SHARED","profileRanks",rank);
        auto state=local_progress::seed(0,0,0,definition?*(int*)((char*)definition+0x14):0);state.quest_warpoints=10000;
        for(int id:{401,402,403}){const auto* box=supply_policy::box(id);supply_policy::Draw a,b;supply_types::Grant grants[128],again[128];int count=0,repeated=0;
            bool pass=box&&local_supply_rewards::supported(*box,state,faction)&&local_supply_rewards::prepare(*box,state,faction,a,grants,128,count)&&
                local_supply_rewards::prepare(*box,state,faction,b,again,128,repeated)&&count==repeated&&a.count==b.count&&!memcmp(a.rows,b.rows,a.count*sizeof(a.rows[0]))&&!memcmp(grants,again,count*sizeof(grants[0]));
            for(int i=0;pass&&i<count;++i){local_progress::CareerReward reward{grants[i].type,grants[i].id,grants[i].amount};
                pass=grants[i].type==2||local_career::supported(saved("UserConfiguration"),reward,state.stars,faction);
                if(grants[i].type==9){void* card=local_career::lookup("SHARED","heroCards",grants[i].id),*from=local_supply::get(card,0x40);pass=pass&&rank>=6&&from&&*(int*)((char*)from+0x10)<=rank;}}
            pass=pass&&supply_store::claim(state,rank,id,a,grants,count);
            local_faction::Roots roots;void* message=pass?local_supply::reward_message(state,roots):nullptr;void* list=local_supply::get(message,0x28);
            pass=pass&&message&&list&&*(int*)((char*)list+0x18)==count&&*(bool*)((char*)message+0x45)&&!*(int*)((char*)message+0x5c);
            for(int i=0;pass&&i<count;++i){void* item=list_item(list,i);pass=item&&original_stat_equals(item,"get_Amount",4,grants[i].amount)&&original_stat_equals(item,"get_TypeId",4,grants[i].id);}
            if(message)++messages;ok=ok&&pass;++adapter;state.supply.pending=0;
            fprintf(f,"{\"phase\":\"private_original_reward\",\"faction\":%d,\"rank\":%d,\"box\":%d,\"grants\":%d,\"pass\":%s}\n",faction,rank,id,count,pass?"true":"false");
        }
        ok=ok&&state.quest_warpoints==9100&&state.supply.step==3&&state.supply.claims==3;
        for(int id:{404,405,406}){const auto* box=supply_policy::box(id);bool denied=box&&!local_supply_rewards::supported(*box,state,faction);ok=ok&&denied;if(denied)++blocked;}
        const auto* first=supply_policy::box(401);
        for(int i=0;first&&i<first->count;++i){const auto& item=supply_content::items[first->first+i];if(item.type!=14)continue;
            local_supply_rewards::Candidates pool;auto status=local_supply_rewards::candidates(item,state,faction,pool);
            bool gate=rank==1?status==supply_policy::Ineligible:status==supply_policy::Eligible&&pool.count>0;ok=ok&&gate;if(gate)++filtered;}
    }
    auto locked=local_progress::seed(0,0,0,0),unlocked=locked;
    bool gate=private_unlock(unlocked,local_faction::active)&&!local_supply::unlocked(locked,local_faction::active)&&unlocked.stars==18;
    local_faction::Roots roots;void* cfg=local_supply::configuration(saved("UserConfiguration"),before,roots);void* live=local_supply::model();
    int closed=cfg&&live?model_state(live,cfg,false):-1;
    void* opened=local_supply::configuration(saved("UserConfiguration"),unlocked,roots);int available=opened&&live?model_state(live,opened,true):-1;
    gate=gate&&closed==1&&available==0;ok=ok&&gate;
    local_supply::list_for(progress,local_faction::active);
    bool isolated=!memcmp(&before,&progress,sizeof(progress));
    fprintf(f,"{\"phase\":\"original_unlock_and_model\",\"pass\":%s,\"original_unlock_stars\":%d,\"locked_state\":%d,\"available_state\":%d}\n",gate?"true":"false",unlocked.stars,closed,available);
    fprintf(f,"{\"summary\":true,\"pass\":%s,\"original_boxes\":208,\"original_items\":2975,\"private_reward_cases\":%d,\"native_messages\":%d,\"unsupported_boxes\":%d,\"hero_gate_cases\":%d,\"runtime_version\":%u,\"runtime_size\":%zu,\"player_state_unchanged\":%s,\"player_save_writes\":0,\"player_claims\":0,\"battle_runs\":0}\n",ok&&isolated?"true":"false",adapter,messages,blocked,filtered,progress.version,sizeof(progress),isolated?"true":"false");fclose(f);
}
}
