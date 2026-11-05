// Representative private original models and concrete terminal-event receipts.
// Natural combat/claims are not synthesized into the player's progression.
#pragma once
namespace battle_contract_audit {
static void* expected_battle,*expected_side;static bool observed;
static void observe(void* battle,void* side,int key,int amount){if(battle==expected_battle&&side==expected_side&&key==27&&amount==1)observed=true;}
static bool daily(const local_progress::State& s,int id,int expected,int phase){
    local_contracts::Roots roots;int index=local_contracts::index(id);if(index<0)return false;
    void* model=local_contracts::daily(s,index,roots);if(!model)return false;
    const auto& d=contract_content::definitions[index];
    return original_stat_equals(model,"get_Count",4,expected)&&original_stat_equals(model,"get_State",1,phase)&&
        original_stat_equals(model,"get_WinOnly",1,(d.flags&1)!=0)&&
        local_hero::get(model,0x10)==local_contracts::types[index];
}
static void run(){
    if(!progress_ready||access(DATA_ROOT "battle-contract-audit.request",F_OK))return;unlink(DATA_ROOT "battle-contract-audit.request");
    FILE* f=fopen(DATA_ROOT "battle-contract-audit.jsonl","w");if(!f)return;chmod(DATA_ROOT "battle-contract-audit.jsonl",0644);
    local_progress::StateBuffer saved_state,storage,next,cold;if(!saved_state.data||!storage.data||!next.data||!cold.data){fclose(f);return;}
    *saved_state.data=progress;auto& s=*storage.data;
    bool ok=!local_match::playing&&battle_contract_runtime::available()&&local_contracts::initialize()&&local_skill::initialize();
    void* rank=local_career::lookup("SHARED","profileRanks",4);int floor=rank?*(int*)((char*)rank+0x14):0;
    s=local_progress::seed(1690,30,197,floor);ok=ok&&rank&&contract_policy::initialize(s,20010,0);
    for(const auto& rule:contract_content::battle_rules)ok=ok&&contract_policy::initialize(s,rule.id,0);
    for(const auto& rule:contract_content::result_rules)ok=ok&&contract_policy::initialize(s,rule.id,0);
    for(const auto& rule:contract_content::boost_rules)ok=ok&&contract_policy::initialize(s,rule.id,0);
    ok=ok&&contract_policy::initialize(s,contract_content::timed_rules[0].id,0);
    ok=ok&&contract_policy::initialize(s,20090,0);
    uint32_t mask=0,expected_mask=0;
    for(int i=0;i<battle_contract_events::count;++i)if(!contract_content::battle_rules[i].faction||contract_content::battle_rules[i].faction==local_faction::active)expected_mask|=1u<<i;
    for(int i=0;i<battle_contract_events::result_count;++i)if(!contract_content::result_rules[i].faction||contract_content::result_rules[i].faction==local_faction::active)expected_mask|=1u<<(battle_contract_events::count+i);
    expected_mask|=battle_contract_events::timed_mask|battle_contract_events::boost_mask|activity_contract_events::eligible;
    bool freeze=ok&&battle_contract_runtime::mask_for(s,mask)&&mask==expected_mask;
    ok=ok&&freeze;int models=0,claims=0;
    for(const auto& rule:contract_content::battle_rules){bool pass=ok&&daily(s,rule.id,0,0);ok=ok&&pass;models+=pass;}
    // Exercise the installed original ABI on private decoded objects. Its raw
    // event is observed, while the production playing/context guards reject it.
    local_research_runtime::Roots roots;void* config=campaign_seeds::decode(DATA_ROOT "offline-seeds/fmc1m.ggs",roots);
    expected_battle=local_hero::get(config,0x10);expected_side=local_dictionary_item(local_hero::get(expected_battle,0x178),1);observed=false;
    uint8_t capture_before[sizeof(battle_contract_runtime::Capture)];memcpy(capture_before,&battle_contract_runtime::capture,sizeof(capture_before));
    void* ex=nullptr;int key=27,amount=1;void* args[]={expected_battle,expected_side,&key,&amount};
    const void* method=class_method(find_class("com.geargames.aow.entities.battle","SideAct"),"$gg",4);
    auto prior_observer=battle_contract_runtime::observer;
    if(ok&&expected_battle&&expected_side&&method&&!prior_observer){battle_contract_runtime::observer=observe;runtime_invoke(method,nullptr,args,&ex);battle_contract_runtime::observer=prior_observer;}
    bool native_event=method&&observed&&!ex&&!memcmp(capture_before,&battle_contract_runtime::capture,sizeof(capture_before));ok=ok&&native_event;
    fprintf(f,"{\"phase\":\"original_event\",\"pass\":%s,\"installed_hook\":%s,\"private_native_arguments\":%s,\"private_event_ignored\":%s,\"exception\":\"%s\"}\n",native_event?"true":"false",battle_contract_runtime::available()?"true":"false",observed?"true":"false",native_event?"true":"false",ex?class_name(object_class(ex)):"none");
    local_match_store::State match;int budget=0,first=0,last=0;
    bool terminal=ok&&local_career_stars::from_rank(rank,budget,first,last)&&local_match_store::begin_pvp(match,true)&&
        local_match_store::freeze_career_stars(match,true,4,budget)&&local_match_store::freeze_victory_credits(match,0)&&local_match_store::freeze_contracts(match,31);
    battle_contract_events::Counts events;for(const auto& rule:contract_content::battle_rules)terminal=terminal&&battle_contract_events::add(31,events,rule.subtype,rule.value);
    terminal=terminal&&local_match_store::settle(match,1,local_match_store::Victory,10000,1,1,nullptr,nullptr)&&local_match_store::stamp_contracts(match,events);
    match.checksum=local_match_store::hash(match);terminal=terminal&&local_match_store::valid(match)&&battle_contract_policy::reconcile(s,match)&&s.credits==1690&&s.contract_points==0;
    ok=ok&&terminal;
    for(const auto& rule:contract_content::battle_rules){
        int index=local_contracts::index(rule.id),credits=0,gold=0;bool pass=ok&&index>=0&&daily(s,rule.id,rule.value,1);
        if(pass)pass=local_contracts::plan_claim(s,contract_content::definitions[index],*next.data,credits,gold)&&!gold&&
            next.data->credits==s.credits+contract_content::definitions[index].rewards[0].amount&&
            next.data->contract_points==s.contract_points+contract_content::definitions[index].points;
        if(pass){s=*next.data;pass=daily(s,rule.id,0,0)&&!local_contracts::plan_claim(s,contract_content::definitions[index],*next.data,credits,gold);}
        fprintf(f,"{\"phase\":\"model_claim_plan\",\"id\":%d,\"pass\":%s,\"original_win_only\":%s}\n",rule.id,pass?"true":"false",rule.flags&1?"true":"false");
        ok=ok&&pass;models+=pass;claims+=pass;
    }
    char path[640];snprintf(path,sizeof(path),DATA_ROOT "battle-contract-private-%d",getpid());bool path_ok=!mkdir(path,0700);
    bool stored=ok&&path_ok&&s.credits==5890&&s.contract_points==75&&local_match_store::write(path,match)&&local_progress::write(path,s);
    local_match_store::State restored;
    stored=stored&&local_match_store::load(path,restored)==1&&local_progress::load(path,*cold.data)==1&&battle_contract_policy::reconcile(*cold.data,restored);
    for(const auto& rule:contract_content::battle_rules)stored=stored&&battle_contract_policy::count(*cold.data,rule.id)==0&&contract_policy::receipt(*cold.data,rule.id)->claims==1;
    // ARM64 cooperative serialization keeps the original176-byte physical file.
    char coop_path[680];snprintf(coop_path,sizeof(coop_path),"%s/coop",path);bool coop_ok=stored&&!mkdir(coop_path,0700);cooperative_store::State coop;
    coop_ok=coop_ok&&cooperative_store::load(coop_path,coop)==0&&cooperative_store::begin(coop)&&cooperative_store::freeze_credits(coop,0)&&
        cooperative_store::settle(coop,1,local_match_store::Victory,10000,1,1,nullptr,nullptr,5)&&cooperative_store::write(coop_path,coop);
    cooperative_store::State coop_cold;coop_ok=coop_ok&&cooperative_store::load(coop_path,coop_cold)==1&&coop_cold.credits==2770&&coop_cold.points==100;
    char slot[720];snprintf(slot,sizeof(slot),"%s/cooperative.%u",coop_path,unsigned(coop.generation&1));struct stat st{};
    coop_ok=coop_ok&&!stat(slot,&st)&&st.st_size==176;
    bool unchanged=!memcmp(saved_state.data,&progress,sizeof(progress));
    fprintf(f,"{\"summary\":true,\"pass\":%s,\"original_models\":%d,\"private_claim_plans\":%d,\"frozen_current_faction_mask\":%u,\"original_native_event_abi\":%s,\"private_event_ignored\":%s,\"private_terminal_bridge\":%s,\"cold_receipts_and_duplicate\":%s,\"cooperative_wire_176\":%s,\"growth_size\":%zu,\"formal_wire_size\":%zu,\"player_state_unchanged\":%s,\"player_save_writes\":0,\"player_rewards\":0,\"battle_runs\":0}\n",ok&&models==10&&claims==5&&stored&&coop_ok&&unchanged?"true":"false",models,claims,mask,native_event?"true":"false",native_event?"true":"false",terminal?"true":"false",stored?"true":"false",coop_ok?"true":"false",sizeof(progress),sizeof(local_match_store::State),unchanged?"true":"false");fclose(f);
}
}
