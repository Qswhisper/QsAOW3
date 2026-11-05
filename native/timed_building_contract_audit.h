// Original clock/event/model ABI on private decoded objects; no player battle.
#pragma once
namespace timed_building_contract_audit {
static void* expected_battle,*expected_side;static bool observed;
static void observe(void* battle,void* side,int key,int amount){
    if(battle==expected_battle&&side==expected_side&&key==30&&amount==1&&battle_contract_runtime::logic_tick&&
       battle_contract_runtime::logic_tick(battle,nullptr)==3600)observed=true;
}
static bool model(const local_progress::State& s,int expected,int phase){
    local_contracts::Roots roots;int i=local_contracts::index(21290);if(i<0)return false;
    void* daily=local_contracts::daily(s,i,roots);void* type=local_contracts::types[i];void* ex=nullptr;
    void* params=invoke_named(type,"get_Params",0,nullptr,&ex);
    return !ex&&params&&array_length(params)==2&&((int*)((char*)params+32))[0]==6&&((int*)((char*)params+32))[1]==4&&
        daily&&original_stat_equals(daily,"get_Count",4,expected)&&original_stat_equals(daily,"get_State",1,phase)&&
        original_stat_equals(daily,"get_WinOnly",1,1)&&local_contracts::get(daily,0x10)==type&&
        original_stat_equals(type,"get_Type",4,15)&&original_stat_equals(type,"get_Subtype",1,21)&&
        original_stat_equals(type,"get_Value",4,1)&&original_stat_equals(type,"get_Flags",1,1)&&original_stat_equals(type,"get_Points",4,15);
}
static void run(){
    if(!progress_ready||access(DATA_ROOT "timed-building-contract-audit.request",F_OK))return;unlink(DATA_ROOT "timed-building-contract-audit.request");
    FILE* f=fopen(DATA_ROOT "timed-building-contract-audit.jsonl","w");if(!f)return;chmod(DATA_ROOT "timed-building-contract-audit.jsonl",0644);
    local_progress::StateBuffer saved_state,storage,next,cold;if(!saved_state.data||!storage.data||!next.data||!cold.data){fclose(f);return;}
    *saved_state.data=progress;auto& s=*storage.data;
    bool ok=!local_match::playing&&local_contracts::initialize()&&local_skill::initialize()&&battle_contract_runtime::timed_available();
    void* rank=local_career::lookup("SHARED","profileRanks",4);
    s=local_progress::seed(1690,30,197,rank?*(int*)((char*)rank+0x14):0);ok=ok&&rank;
    for(const auto& rule:contract_content::battle_rules)ok=ok&&contract_policy::initialize(s,rule.id,0);
    for(const auto& rule:contract_content::result_rules)ok=ok&&contract_policy::initialize(s,rule.id,0);
    for(const auto& rule:contract_content::boost_rules)ok=ok&&contract_policy::initialize(s,rule.id,0);
    ok=ok&&contract_policy::initialize(s,21290,0);uint32_t mask=0;
    ok=ok&&contract_policy::initialize(s,20090,0);
    bool frozen=ok&&battle_contract_runtime::mask_for(s,mask)&&(mask&battle_contract_events::timed_mask)&&model(s,0,0);
    local_research_runtime::Roots roots;void* config=campaign_seeds::decode(DATA_ROOT "offline-seeds/fmc1m.ggs",roots);
    expected_battle=local_hero::get(config,0x10);expected_side=local_dictionary_item(local_hero::get(expected_battle,0x178),1);observed=false;
    void* ex=nullptr;int tick=3600;void* tick_args[]={&tick};
    if(expected_battle)invoke_named(expected_battle,"$ro",1,tick_args,&ex);
    bool clock=ok&&expected_battle&&!ex&&original_stat_equals(expected_battle,"$Qo",4,3600)&&battle_contract_runtime::logic_tick(expected_battle,nullptr)==3600;
    uint8_t capture_before[sizeof(battle_contract_runtime::Capture)];memcpy(capture_before,&battle_contract_runtime::capture,sizeof(capture_before));
    int key=30,amount=1;void* args[]={expected_battle,expected_side,&key,&amount};
    const void* method=class_method(find_class("com.geargames.aow.entities.battle","SideAct"),"$gg",4);
    auto prior_observer=battle_contract_runtime::observer;
    if(clock&&expected_side&&method&&!prior_observer){battle_contract_runtime::observer=observe;runtime_invoke(method,nullptr,args,&ex);battle_contract_runtime::observer=prior_observer;}
    bool native_event=clock&&method&&observed&&!ex&&!memcmp(capture_before,&battle_contract_runtime::capture,sizeof(capture_before));
    ok=ok&&frozen&&clock&&native_event;
    fprintf(f,"{\"phase\":\"original_clock_and_event\",\"pass\":%s,\"getter_tick\":%d,\"event_key\":30,\"private_event_ignored\":%s,\"exception\":\"%s\"}\n",ok?"true":"false",clock?3600:-1,native_event?"true":"false",ex?class_name(object_class(ex)):"none");
    timed_contract_events::Counts events;bool counted=ok&&timed_contract_events::add(battle_contract_events::timed_mask,events,30,3,3599)&&
        timed_contract_events::add(battle_contract_events::timed_mask,events,30,1,battle_contract_runtime::logic_tick(expected_battle,nullptr))&&events.supplies==4;
    local_match_store::State match;int budget=0,first=0,last=0;
    bool terminal=counted&&local_career_stars::from_rank(rank,budget,first,last)&&local_match_store::begin_pvp(match,true)&&
        local_match_store::freeze_career_stars(match,true,4,budget)&&local_match_store::freeze_victory_credits(match,0)&&
        local_match_store::freeze_contracts(match,battle_contract_events::timed_mask)&&
        local_match_store::settle(match,1,local_match_store::Victory,18000,1,1,nullptr,nullptr)&&local_match_store::stamp_contracts(match,{},events);
    match.checksum=local_match_store::hash(match);terminal=terminal&&local_match_store::valid(match)&&
        match.contract_mask==(battle_contract_events::timed_mask|battle_contract_events::timed_completed)&&battle_contract_policy::reconcile(s,match)&&model(s,1,1)&&s.credits==1690;
    int index=local_contracts::index(21290),credits=0,gold=0;
    bool claim=terminal&&index>=0&&local_contracts::plan_claim(s,contract_content::definitions[index],*next.data,credits,gold)&&credits==1200&&!gold&&
        next.data->credits==2890&&next.data->contract_points==15&&model(*next.data,0,0);
    if(claim)s=*next.data;
    bool duplicate=claim&&!local_contracts::plan_claim(s,contract_content::definitions[index],*next.data,credits,gold)&&contract_policy::receipt(s,21290)->claims==1;
    char path[640];snprintf(path,sizeof(path),DATA_ROOT "timed-building-contract-private-%d",getpid());bool path_ok=!mkdir(path,0700);
    bool stored=duplicate&&path_ok&&local_match_store::write(path,match)&&local_progress::write(path,s);local_match_store::State restored;
    stored=stored&&local_match_store::load(path,restored)==1&&local_progress::load(path,*cold.data)==1&&restored.version==12&&restored.size==136&&
        restored.contract_mask==192&&battle_contract_policy::reconcile(*cold.data,restored)&&model(*cold.data,0,0)&&cold.data->credits==2890&&cold.data->contract_points==15;
    bool unchanged=!memcmp(saved_state.data,&progress,sizeof(progress));
    fprintf(f,"{\"summary\":true,\"pass\":%s,\"original_clock_getter\":%s,\"original_event30_abi\":%s,\"private_event_ignored\":%s,\"original_params_and_models\":%s,\"private_terminal_bridge\":%s,\"manual_claim_plan\":%s,\"cold_receipt_duplicate\":%s,\"formal_version\":%u,\"formal_wire_size\":%u,\"growth_size\":%zu,\"player_state_unchanged\":%s,\"player_save_writes\":0,\"player_rewards\":0,\"battle_runs\":0}\n",ok&&terminal&&claim&&duplicate&&stored&&unchanged?"true":"false",clock?"true":"false",native_event?"true":"false",native_event?"true":"false",frozen&&terminal&&claim?"true":"false",terminal?"true":"false",claim?"true":"false",stored?"true":"false",match.version,match.size,sizeof(progress),unchanged?"true":"false");fclose(f);
}
}
