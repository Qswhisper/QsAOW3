// Native original models/Boost post-state objects plus private receipt plans.
// No target card is granted to the player and no native use/win is fabricated.
#pragma once
namespace boost_contract_audit {
static void* expected_battle,*expected_side;
static void (*production)(void*,void*,int);
static int notifications;static uint32_t seen;
static void probe(void* battle,void* side,int id){
    if(battle==expected_battle&&side==expected_side){
        for(int i=0;i<2;++i)if(contract_content::boost_rules[i].boost==id){++notifications;seen|=1u<<i;}}
    if(production)production(battle,side,id); // Production rejects this private/non-playing context.
}
static bool model(const local_progress::State& s,int id,int expected,int phase){
    local_contracts::Roots roots;int i=local_contracts::index(id);if(i<0)return false;
    const auto& d=contract_content::definitions[i];void* daily=local_contracts::daily(s,i,roots),*type=local_contracts::types[i];
    void* original=local_career::lookup("BATTLE","boosts",d.param);
    return daily&&original&&*(int*)((char*)original+0x10)==d.param&&
        original_stat_equals(daily,"get_Count",4,expected)&&original_stat_equals(daily,"get_State",1,phase)&&original_stat_equals(daily,"get_WinOnly",1,1)&&
        local_contracts::get(daily,0x10)==type&&original_stat_equals(type,"get_Type",4,9)&&original_stat_equals(type,"get_Subtype",1,0)&&
        original_stat_equals(type,"get_Param",4,d.param)&&original_stat_equals(type,"get_Value",4,1)&&original_stat_equals(type,"get_Flags",1,1);
}
static void run(){
    if(!progress_ready||access(DATA_ROOT "boost-contract-audit.request",F_OK))return;unlink(DATA_ROOT "boost-contract-audit.request");
    FILE* f=fopen(DATA_ROOT "boost-contract-audit.jsonl","w");if(!f)return;chmod(DATA_ROOT "boost-contract-audit.jsonl",0644);
    local_progress::StateBuffer saved_state,storage,next,cold;if(!saved_state.data||!storage.data||!next.data||!cold.data){fclose(f);return;}
    *saved_state.data=progress;auto& s=*storage.data;
    bool ok=!local_match::playing&&!local_coop::playing&&!local_run_active&&local_contracts::initialize()&&local_skill::initialize()&&battle_contract_runtime::boost_available();
    void* rank=local_career::lookup("SHARED","profileRanks",4);ok=ok&&rank;
    s=local_progress::seed(1690,30,197,rank?*(int*)((char*)rank+0x14):0);
    for(const auto& rule:contract_content::battle_rules)ok=ok&&contract_policy::initialize(s,rule.id,0);
    for(const auto& rule:contract_content::result_rules)ok=ok&&contract_policy::initialize(s,rule.id,0);
    ok=ok&&contract_policy::initialize(s,21290,0);
    ok=ok&&contract_policy::initialize(s,20090,0);
    for(const auto& rule:contract_content::boost_rules)ok=ok&&contract_policy::initialize(s,rule.id,0)&&model(s,rule.id,0,0);
    uint32_t mask=0;bool frozen=ok&&battle_contract_runtime::mask_for(s,mask)&&(mask&battle_contract_events::boost_mask)==battle_contract_events::boost_mask;
    local_research_runtime::Roots roots;void* config=campaign_seeds::decode(DATA_ROOT "offline-seeds/fmc1m.ggs",roots);
    expected_battle=local_hero::get(config,0x10);expected_side=local_dictionary_item(local_hero::get(expected_battle,0x178),1);
    uint8_t capture_before[sizeof(battle_contract_runtime::Capture)];memcpy(capture_before,&battle_contract_runtime::capture,sizeof(capture_before));
    production=local_inventory::applied_observer;notifications=0;seen=0;
    bool observations=frozen&&expected_battle&&expected_side&&production==battle_contract_runtime::boost_applied;
    if(observations){
        local_inventory::applied_observer=probe;
        for(const auto& rule:contract_content::boost_rules){
            void* type=local_career::lookup("BATTLE","boosts",rule.boost),*paid=object_new(find_class("com.geargames.aow.entities.battle","Boost")),*granted=object_new(find_class("com.geargames.aow.entities.battle","Boost"));
            void* ex=nullptr;int one=1,zero=0;bool made=type&&roots.keep(paid)&&roots.keep(granted);
            void* a[]={type,&one};void* b[]={type,&zero};if(made)invoke_named(paid,".ctor",2,a,&ex);if(made&&!ex)invoke_named(granted,".ctor",2,b,&ex);
            made=made&&!ex&&local_inventory::amount(paid)==1&&local_inventory::amount(granted)==0&&local_hero::get(paid,0x10)==type;
            local_inventory::Command paid_case{expected_side,paid,nullptr,rule.boost,2,0,true,false};
            local_inventory::Command granted_case{expected_side,nullptr,granted,rule.boost,0,1,false,false};
            bool paid_ok=made&&local_inventory::notify_applied(expected_battle,paid_case,0);
            bool grant_ok=made&&local_inventory::notify_applied(expected_battle,granted_case,0);
            int prior=notifications;paid_case.refused=true;
            bool rejected=made&&!local_inventory::notify_applied(expected_battle,paid_case,0)&&!local_inventory::notify_applied(expected_battle,granted_case,-1)&&notifications==prior;
            observations=observations&&paid_ok&&grant_ok&&rejected;
            fprintf(f,"{\"phase\":\"original_boost_poststate_boundary\",\"id\":%d,\"boost\":%d,\"pass\":%s,\"paid\":%s,\"script\":%s,\"refused_ignored\":%s,\"native_command_invocations\":0}\n",rule.id,rule.boost,paid_ok&&grant_ok&&rejected?"true":"false",paid_ok?"true":"false",grant_ok?"true":"false",rejected?"true":"false");
        }
        local_inventory::applied_observer=production;
    }
    bool ignored=observations&&notifications==4&&seen==3&&!memcmp(capture_before,&battle_contract_runtime::capture,sizeof(capture_before));
    ok=ok&&frozen&&ignored;local_match_store::State match;int budget=0,first=0,last=0;
    bool terminal=ok&&local_career_stars::from_rank(rank,budget,first,last)&&local_match_store::begin_pvp(match,true)&&local_match_store::freeze_career_stars(match,true,4,budget)&&
        local_match_store::freeze_victory_credits(match,0)&&local_match_store::freeze_contracts(match,battle_contract_events::boost_mask)&&
        local_match_store::settle(match,1,local_match_store::Victory,10000,1,1,nullptr,nullptr)&&local_match_store::stamp_contracts(match,{},{},3);
    match.checksum=local_match_store::hash(match);terminal=terminal&&local_match_store::valid(match)&&match.contract_mask==0xf00&&battle_contract_policy::reconcile(s,match)&&s.credits==1690;
    int claims=0;for(const auto& rule:contract_content::boost_rules){
        int i=local_contracts::index(rule.id),credits=0,gold=0;bool pass=terminal&&i>=0&&model(s,rule.id,1,1)&&local_contracts::plan_claim(s,contract_content::definitions[i],*next.data,credits,gold)&&
            credits==contract_content::definitions[i].rewards[0].amount&&!gold;
        if(pass){s=*next.data;pass=model(s,rule.id,0,0)&&!local_contracts::plan_claim(s,contract_content::definitions[i],*next.data,credits,gold);}
        fprintf(f,"{\"phase\":\"original_model_claim_plan\",\"id\":%d,\"pass\":%s}\n",rule.id,pass?"true":"false");ok=ok&&pass;claims+=pass;
    }
    char path[640];snprintf(path,sizeof(path),DATA_ROOT "boost-contract-private-%d",getpid());bool path_ok=!mkdir(path,0700);
    bool stored=ok&&claims==2&&s.credits==2590&&s.contract_points==20&&path_ok&&local_match_store::write(path,match)&&local_progress::write(path,s);
    local_match_store::State restored;stored=stored&&local_match_store::load(path,restored)==1&&local_progress::load(path,*cold.data)==1&&restored.version==12&&restored.size==136&&
        battle_contract_policy::reconcile(*cold.data,restored)&&cold.data->credits==2590&&cold.data->contract_points==20;
    for(const auto& rule:contract_content::boost_rules)stored=stored&&!battle_contract_policy::count(*cold.data,rule.id)&&contract_policy::receipt(*cold.data,rule.id)->claims==1;
    bool unchanged=!memcmp(saved_state.data,&progress,sizeof(progress));
    fprintf(f,"{\"summary\":true,\"pass\":%s,\"original_canonical_boosts_and_models\":%s,\"postcommand_notifications\":%d,\"private_observations_ignored\":%s,\"private_terminal_bridge\":%s,\"manual_claim_plans\":%d,\"cold_receipts_duplicate\":%s,\"formal_version\":%u,\"formal_wire_size\":%u,\"growth_size\":%zu,\"player_state_unchanged\":%s,\"native_command_invocations\":0,\"native_boost_effects\":0,\"player_save_writes\":0,\"player_rewards\":0,\"battle_runs\":0}\n",ok&&ignored&&terminal&&claims==2&&stored&&unchanged?"true":"false",ok?"true":"false",notifications,ignored?"true":"false",terminal?"true":"false",claims,stored?"true":"false",match.version,match.size,sizeof(progress),unchanged?"true":"false");fclose(f);
}
}
