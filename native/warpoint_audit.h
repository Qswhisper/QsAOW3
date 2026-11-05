// Opt-in original-rule/getter audit. Private states only; no player writes/run.
#pragma once
namespace warpoint_audit {
static void run(){
    if(!progress_ready||access(DATA_ROOT "warpoint-audit.request",F_OK))return;
    unlink(DATA_ROOT "warpoint-audit.request");auto before=progress;
    FILE* f=fopen(DATA_ROOT "warpoint-audit.jsonl","w");if(!f)return;chmod(DATA_ROOT "warpoint-audit.jsonl",0644);
    bool ok=local_warpoints::model_hooks&&local_warpoints::initialize();int checked=0;
    for(int mode:{1,2,4})for(int outcome=1;outcome<=6;++outcome)for(int tick:{0,6000,6001,12000,12001,18000,18001}){
        local_match_store::State match{};match.mode=mode;match.version=5;match.reserved2=1;match.next=match.settled=1;
        match.outcome=outcome;match.reason=outcome<=2?1:outcome==3?6:7;match.tick=tick;
        int win=mode==1?100:200,expected=outcome==1?win:outcome==2?(tick>18000?(mode==1?25:50):tick>12000?(mode==1?17:35):tick>6000?(mode==1?10:20):0):0;
        bool pass=local_warpoints::stamp(match)&&match.reserved==expected;match.checksum=local_match_store::hash(match);
        auto wallet=local_progress::seed(10,0,0,0);bool changed=false;
        pass=pass&&warpoint_store::reconcile(wallet,match,changed)&&changed&&wallet.quest_warpoints==expected&&wallet.warpoint_pvp_cursor==1;
        pass=pass&&warpoint_store::reconcile(wallet,match,changed)&&!changed;
        match.reserved2=0;match.reserved=0;pass=pass&&local_warpoints::stamp(match)&&!match.reserved;
        ok=ok&&pass;++checked;
        fprintf(f,"{\"phase\":\"original_rules_private_wallet\",\"mode\":%d,\"outcome\":%d,\"ticks\":%d,\"award\":%d,\"pass\":%s}\n",mode,outcome,tick,expected,pass?"true":"false");
    }
    local_research_runtime::Roots roots;void* model=object_new(find_class("com.geargames.aow.ugui.supply","PlayerSupplyModel"));
    void* ex=nullptr;void* points=roots.keep(model)?invoke_named(model,"get_Points",0,nullptr,&ex):nullptr;
    bool getter=points&&!ex&&*(int*)((char*)points+16)==before.quest_warpoints;
    void* cap=invoke_named(model,"get_PointsTodayMax",0,nullptr,&ex);getter=getter&&cap&&!ex&&!*(int*)((char*)cap+16);
    void* limited=invoke_named(model,"get_IsTodayLimitReached",0,nullptr,&ex);getter=getter&&limited&&!ex&&!*(bool*)((char*)limited+16);
    bool isolated=!memcmp(&before,&progress,sizeof(progress));
    bool label=local_warpoints::balance_view_written&&local_warpoints::balance_view_amount==before.quest_warpoints;ok=ok&&label;
    fprintf(f,"{\"phase\":\"original_balance_label\",\"pass\":%s,\"balance\":%d,\"claims_enabled\":false}\n",label?"true":"false",local_warpoints::balance_view_amount);
    fprintf(f,"{\"summary\":true,\"pass\":%s,\"boundaries\":%d,\"solo_victory\":%d,\"team_victory\":%d,\"original_model_getters\":%s,\"runtime_version\":%u,\"runtime_size\":%zu,\"player_state_unchanged\":%s,\"player_save_writes\":0,\"battle_runs\":0}\n",ok&&getter&&isolated?"true":"false",checked,local_warpoints::solo.victory,local_warpoints::team.victory,getter?"true":"false",progress.version,sizeof(progress),isolated?"true":"false");fclose(f);
}
}
