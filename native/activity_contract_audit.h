// Original pure final-activity APIs on PRIVATE typed data fixtures. No battle,
// player commands, player progress changes or manufactured natural results.
#pragma once
namespace activity_contract_audit {
static bool model(const local_progress::State& s,int count,int phase){
    int i=local_contracts::index(20090);if(i<0)return false;local_contracts::Roots roots;
    void* daily=local_contracts::daily(s,i,roots),*type=local_contracts::types[i];
    return daily&&original_stat_equals(daily,"get_Count",4,count)&&original_stat_equals(daily,"get_State",1,phase)&&original_stat_equals(daily,"get_WinOnly",1,1)&&
        original_stat_equals(type,"get_Type",4,3)&&original_stat_equals(type,"get_Subtype",1,70)&&original_stat_equals(type,"get_Value",4,1)&&original_stat_equals(type,"get_Flags",1,3);
}
static void run(){
    if(!progress_ready||access(DATA_ROOT "activity-contract-audit.request",F_OK))return;unlink(DATA_ROOT "activity-contract-audit.request");
    FILE* f=fopen(DATA_ROOT "activity-contract-audit.jsonl","w");if(!f)return;chmod(DATA_ROOT "activity-contract-audit.jsonl",0644);
    local_progress::StateBuffer saved_state,scratch,next,cold;if(!saved_state.data||!scratch.data||!next.data||!cold.data){fclose(f);return;}
    *saved_state.data=progress;auto saved_match=local_match::state;auto saved_coop=local_coop::ledger;
    bool ok=!local_match::playing&&!local_coop::playing&&!local_run_active&&activity_contract_runtime::available()&&local_contracts::initialize()&&local_skill::initialize();
    auto& s=*scratch.data;int gates=0;void* rank4=nullptr;
    for(int id:{3,4,6,7}){
        void* rank=local_career::lookup("SHARED","profileRanks",id);if(id==4)rank4=rank;
        s=local_progress::seed(1690,30,197,rank?*(int*)((char*)rank+0x14):0);
        for(const auto& d:contract_content::definitions)if(d.kind>=5)ok=ok&&contract_policy::initialize(s,d.id,0);
        uint32_t mask=0;int index=local_contracts::index(20090);bool expected=id==4||id==6;
        bool pass=ok&&rank&&index>=0&&local_contracts::eligible(contract_content::definitions[index],s)==expected&&battle_contract_runtime::mask_for(s,mask)&&bool(mask&activity_contract_events::eligible)==expected&&model(s,0,0);
        ok=ok&&pass;gates+=pass;
    }
    local_research_runtime::Roots roots;
    void* battle=object_new(find_class("com.geargames.aow.entities.battle","Battle"));
    void* side=object_new(find_class("com.geargames.aow.entities.battle","BattleSide"));
    ok=ok&&roots.keep(battle)&&roots.keep(side);int samples=0;
    struct Case {int numerator,denominator,tick,extra,winner,flag,expected;};
    const Case cases[]={{0,1000,0,0,1,0,0},{699,1000,0,0,1,0,699},{700,1000,0,0,1,0,700},{701,1000,0,0,1,0,701},
        {709,1000,0,0,1,0,709},{710,1000,0,0,1,0,710},{1000,1000,0,0,1,0,1000},{2000,1000,0,0,1,0,1000},
        {700,1000,599,0,1,0,700},{700,1000,600,0,1,0,636},{701,1000,0,1000,2,0,350},
        {0,0,0,0,1,0,1000},{0,0,0,0,1,-1,0},{0,0,0,0,2,0,0}};
    for(const auto& c:cases){
        if(!ok)break;
        *(int*)((char*)battle+0x74)=1000;*(int*)((char*)battle+0x78)=0;*(int*)((char*)battle+0x7c)=0;*(int*)((char*)battle+0x80)=1000;
        *(int*)((char*)battle+0xb0)=c.tick;*(int8_t*)((char*)battle+0x2a8)=c.winner;
        *(int*)((char*)side+0x3c)=100;*(int8_t*)((char*)side+0x2f8)=1;*(int8_t*)((char*)side+0x3ba)=c.flag;
        *(int*)((char*)side+0x410)=c.numerator;*(int*)((char*)side+0x414)=c.denominator;*(int*)((char*)side+0x418)=c.extra;
        int raw=-1,shown=-1;bool pass=activity_contract_runtime::read(battle,side,raw,shown)&&raw==c.expected&&shown==(raw+9)/10;
        fprintf(f,"{\"private_original_api\":true,\"index\":%d,\"expected_raw\":%d,\"raw\":%d,\"percent\":%d,\"pass\":%s}\n",samples,c.expected,raw,shown,pass?"true":"false");ok=ok&&pass;samples+=pass;
    }
    s=local_progress::seed(1690,30,197,rank4?*(int*)((char*)rank4+0x14):0);
    for(const auto& d:contract_content::definitions)if(d.kind>=5)ok=ok&&contract_policy::initialize(s,d.id,0);
    local_match_store::State m;bool terminal=ok&&local_match_store::begin_pvp(m,true)&&local_match_store::freeze_career_stars(m,true,4,4)&&local_match_store::freeze_victory_credits(m,0)&&local_match_store::freeze_contracts(m,activity_contract_events::eligible)&&
        local_match_store::settle(m,1,local_match_store::Victory,9000,1,1,nullptr,nullptr)&&local_match_store::stamp_contracts(m,{},{},0,701);
    m.checksum=local_match_store::hash(m);terminal=terminal&&local_match_store::valid(m)&&m.version==12&&m.contract_mask==0x3000&&battle_contract_policy::reconcile(s,m)&&model(s,1,1)&&s.credits==1690;
    int index=local_contracts::index(20090),credits=0,gold=0;
    const auto& definition=contract_content::definitions[index>=0?index:0];
    bool claim=terminal&&index>=0&&local_contracts::plan_claim(s,definition,*next.data,credits,gold)&&credits==1000&&!gold&&
        next.data->credits==2690&&next.data->contract_points==15&&model(*next.data,0,0)&&!local_contracts::plan_claim(*next.data,definition,*cold.data,credits,gold);
    char path[640];snprintf(path,sizeof(path),"%sactivity-contract-audit-store",DATA_ROOT);mkdir(path,0777);
    bool stored=claim&&local_match_store::write(path,m)&&local_progress::write(path,*next.data);local_match_store::State restored;
    stored=stored&&local_match_store::load(path,restored)==1&&local_progress::load(path,*cold.data)==1&&restored.version==12&&battle_contract_policy::reconcile(*cold.data,restored)&&
        !battle_contract_policy::count(*cold.data,20090)&&contract_policy::receipt(*cold.data,20090)->claims==1&&cold.data->credits==2690&&cold.data->contract_points==15;
    bool isolated=!memcmp(saved_state.data,&progress,sizeof(progress))&&!memcmp(&saved_match,&local_match::state,sizeof(saved_match))&&!memcmp(&saved_coop,&local_coop::ledger,sizeof(saved_coop));
    fprintf(f,"{\"summary\":true,\"pass\":%s,\"original_final_api_fixtures\":%d,\"original_rank_gate_cases\":%d,\"original_daily_models\":%s,\"private_terminal_plan\":%s,\"manual_claim_plan\":%s,\"cold_duplicate_receipt\":%s,\"formal_version\":%u,\"formal_wire_size\":%u,\"growth_size\":%zu,\"history_size\":%zu,\"player_state_unchanged\":%s,\"player_save_writes\":0,\"player_rewards\":0,\"battle_runs\":0}\n",ok&&terminal&&claim&&stored&&isolated?"true":"false",samples,gates,terminal&&claim?"true":"false",terminal?"true":"false",claim?"true":"false",stored?"true":"false",m.version,m.size,sizeof(progress),sizeof(local_history_store::Record),isolated?"true":"false");fclose(f);
}
}
