// Private original Daily models and claim plans. No player match or reward.
#pragma once
namespace fast_contract_audit {
static bool model(const local_progress::State& s,int expected,int phase){
    local_contracts::Roots roots;int i=local_contracts::index(20080);if(i<0)return false;
    void* daily=local_contracts::daily(s,i,roots);void* type=local_contracts::types[i];
    return daily&&original_stat_equals(daily,"get_Count",4,expected)&&original_stat_equals(daily,"get_State",1,phase)&&
        original_stat_equals(daily,"get_WinOnly",1,0)&&local_contracts::get(daily,0x10)==type&&
        original_stat_equals(type,"get_Type",4,16)&&original_stat_equals(type,"get_Subtype",1,25)&&
        original_stat_equals(type,"get_Value",4,1)&&original_stat_equals(type,"get_Param",4,0)&&
        original_stat_equals(type,"get_Flags",1,2)&&original_stat_equals(type,"get_Points",4,20);
}
static void run(){
    if(!progress_ready||access(DATA_ROOT "fast-contract-audit.request",F_OK))return;unlink(DATA_ROOT "fast-contract-audit.request");
    FILE* f=fopen(DATA_ROOT "fast-contract-audit.jsonl","w");if(!f)return;chmod(DATA_ROOT "fast-contract-audit.jsonl",0644);
    local_progress::StateBuffer saved_state,storage,next,cold;if(!saved_state.data||!storage.data||!next.data||!cold.data){fclose(f);return;}
    *saved_state.data=progress;auto& s=*storage.data;
    bool ok=!local_match::playing&&local_contracts::initialize()&&local_skill::initialize()&&battle_contract_runtime::available();
    int index=local_contracts::index(20080);ok=ok&&index>=0;
    const auto& d=contract_content::definitions[index>=0?index:0];
    int gates=0;uint32_t mask=0;void* rank4=nullptr;
    for(int rank_id:{3,4,6,7}){
        void* rank=local_career::lookup("SHARED","profileRanks",rank_id);if(rank_id==4)rank4=rank;
        s=local_progress::seed(1690,30,197,rank?*(int*)((char*)rank+0x14):0);
        for(const auto& rule:contract_content::battle_rules)ok=ok&&contract_policy::initialize(s,rule.id,0);
        for(const auto& rule:contract_content::result_rules)ok=ok&&contract_policy::initialize(s,rule.id,0);
    for(const auto& rule:contract_content::boost_rules)ok=ok&&contract_policy::initialize(s,rule.id,0);
        ok=ok&&contract_policy::initialize(s,contract_content::timed_rules[0].id,0);
    ok=ok&&contract_policy::initialize(s,20090,0);
        bool gate=ok&&rank&&local_contracts::eligible(d,s)==(rank_id==4||rank_id==6)&&battle_contract_runtime::mask_for(s,mask)&&
            bool(mask&(1u<<battle_contract_events::count))==(rank_id==4||rank_id==6);
        fprintf(f,"{\"phase\":\"original_rank_gate\",\"rank\":%d,\"pass\":%s,\"frozen_mask\":%u}\n",rank_id,gate?"true":"false",mask);
        ok=ok&&gate;gates+=gate;
    }
    s=local_progress::seed(1690,30,197,rank4?*(int*)((char*)rank4+0x14):0);
    for(const auto& rule:contract_content::battle_rules)ok=ok&&contract_policy::initialize(s,rule.id,0);
    for(const auto& rule:contract_content::result_rules)ok=ok&&contract_policy::initialize(s,rule.id,0);
    for(const auto& rule:contract_content::boost_rules)ok=ok&&contract_policy::initialize(s,rule.id,0);
    ok=ok&&contract_policy::initialize(s,contract_content::timed_rules[0].id,0);
    bool initial=ok&&model(s,0,0);ok=ok&&initial;int budget=0,first=0,last=0;
    local_match_store::State match;
    bool terminal=ok&&local_career_stars::from_rank(rank4,budget,first,last)&&
        local_match_store::begin_pvp(match,true)&&local_match_store::freeze_career_stars(match,true,4,budget)&&
        local_match_store::freeze_victory_credits(match,0)&&local_match_store::freeze_contracts(match,1u<<battle_contract_events::count)&&
        local_match_store::settle(match,1,local_match_store::Victory,15000,1,1,nullptr,nullptr)&&local_match_store::stamp_contracts(match,{});
    match.checksum=local_match_store::hash(match);
    terminal=terminal&&local_match_store::valid(match)&&battle_contract_policy::reconcile(s,match)&&model(s,1,1)&&s.credits==1690&&s.contract_points==0;
    int credits=0,gold=0;bool claim=terminal&&local_contracts::plan_claim(s,d,*next.data,credits,gold)&&credits==1200&&!gold&&
        next.data->credits==2890&&next.data->contract_points==20&&model(*next.data,0,0);
    if(claim)s=*next.data;
    bool duplicate=claim&&!local_contracts::plan_claim(s,d,*next.data,credits,gold)&&contract_policy::receipt(s,20080)->claims==1;
    char path[640];snprintf(path,sizeof(path),DATA_ROOT "fast-contract-private-%d",getpid());bool path_ok=!mkdir(path,0700);
    bool stored=duplicate&&path_ok&&local_match_store::write(path,match)&&local_progress::write(path,s);
    local_match_store::State restored;
    stored=stored&&local_match_store::load(path,restored)==1&&local_progress::load(path,*cold.data)==1&&
        restored.version==12&&restored.size==136&&battle_contract_policy::reconcile(*cold.data,restored)&&
        model(*cold.data,0,0)&&cold.data->credits==2890&&cold.data->contract_points==20&&contract_policy::receipt(*cold.data,20080)->claims==1;
    bool unchanged=!memcmp(saved_state.data,&progress,sizeof(progress));
    fprintf(f,"{\"summary\":true,\"pass\":%s,\"original_rank_gates\":%d,\"original_models\":%s,\"private_terminal_bridge\":%s,\"manual_claim_plan\":%s,\"cold_receipt_duplicate\":%s,\"formal_version\":%u,\"formal_wire_size\":%u,\"growth_size\":%zu,\"player_state_unchanged\":%s,\"player_save_writes\":0,\"player_rewards\":0,\"battle_runs\":0}\n",ok&&gates==4&&initial&&terminal&&claim&&duplicate&&stored&&unchanged?"true":"false",gates,initial&&terminal&&claim?"true":"false",terminal?"true":"false",claim?"true":"false",stored?"true":"false",match.version,match.size,sizeof(progress),unchanged?"true":"false");fclose(f);
}
}
