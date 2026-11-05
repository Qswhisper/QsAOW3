// Present an already committed local settlement in the original PvE window.
// This adapter does not award anything or manufacture a battle outcome.
#pragma once
namespace local_campaign_results {
struct Result {
    bool pending=false,win=false;
    uint64_t run=0;
    int faction=0,chapter=0,stars=0,gross_xp=0,net_xp=0,old_best_xp=0,credits=0;
    uint32_t objective_count=0,objectives=0;
};
static Result latest;
static void diagnostic_frame() {
    // Reuse the original per-tick executor for bounded development regression.
    // No tick assignment, entity mutation, result shortcut, or production speed change.
    static uint64_t requested;
    if(!local_run_active || requested==local_run || access(DATA_ROOT "campaign-fast.enabled",F_OK))return;
    if(interaction_trace::tick()<30)return;
    void* executor=static_object("com.geargames.aow.scenes.battle","ClientBattleContext","m_logicExecutor");
    const void* method=executor?class_method(object_class(executor),"FastExecuteLogicToTick",1):nullptr;
    if(!method)return;
    int target=6000;void* args[]={&target};void* ex=nullptr;
    runtime_invoke(method,executor,args,&ex);requested=local_run;
    LOG("Local campaign diagnostic fast execution requested=%d run=%llu target=%d original_per_tick=1",!ex,(unsigned long long)local_run,target);
}
static bool integer(void* target,const char* setter,int value) {
    const void* method=class_method(object_class(target),setter,1);
    void* ex=nullptr;void* args[]={&value};
    if(method)runtime_invoke(method,target,args,&ex);
    return method&&!ex;
}
static void capture(bool win,int stars,int gross_xp,uint32_t count,uint32_t objectives,
                    const local_progress::State& before,const local_progress::State& after) {
    latest=Result{};latest.pending=true;latest.win=win;latest.run=after.settled_run;
    latest.faction=local_faction::active;latest.chapter=local_mission_id;latest.stars=stars;
    latest.gross_xp=gross_xp;latest.net_xp=after.experience-before.experience;
    latest.credits=after.credits-before.credits;
    latest.objective_count=count;latest.objectives=objectives;
    for(uint32_t i=0;i<before.count;++i)
        if(before.missions[i].id==latest.chapter)latest.old_best_xp=before.missions[i].xp;
}
static bool publish(bool match=false,const char* match_title=nullptr,int match_mission=0) {
    if(!latest.pending)return true;
    if(!match && (!progress_ready || latest.faction!=local_faction::active || latest.run!=progress.settled_run))
        return false;
    local_faction::Roots roots;
    const auto* scenario=local_scenario(latest.chapter);
    void* original=local_mission_description(match?(match_mission?match_mission:(local_scenario(local_match::state.scenario)?local_scenario(local_match::state.scenario)->recorded:388)):(scenario?scenario->recorded:latest.chapter));
    void* description=local_faction::clone(original,roots);
    void* stats=object_new(find_class("com.geargames.aow.entities.battle.descriptions","PvEClientStatistics"));
    void* points=object_new(find_class("com.geargames.aow.messages","PvEBattlePointsMessage"));
    if(!description || !roots.keep(stats) || !roots.keep(points))return false;
    // Keep the real variant's objectives/reward metadata, but preserve the
    // public career chapter ID used by the original "play again" button.
    *(int*)((char*)description+0x10)=latest.chapter;
    void* ex=nullptr;invoke_named(stats,".ctor",0,nullptr,&ex);if(ex)return false;
    invoke_named(points,".ctor",0,nullptr,&ex);if(ex)return false;
    if(match) {
        void* empty=array_new(find_class("System","String"),0);
        if(!roots.keep(empty))return false;
        field_set(description,class_field_from_name(object_class(description),"aim_str"),empty);
        // Isolated presentation identity, never an unlocked campaign mission.
        void* title=string_new(match_title?match_title:"本地 AI 1v1");
        field_set(description,class_field_from_name(object_class(description),"name"),title);
    }
    void* aim_text=*(void**)((char*)description+0x68);
    size_t n=aim_text?array_length(aim_text):0;
    if(n>32 || n>latest.objective_count)return false;
    void* achieved=array_new(find_class("System","Boolean"),n);
    void* aim_xp=array_new(find_class("System","Int32"),n);
    if(!roots.keep(achieved) || !roots.keep(aim_xp))return false;
    uint32_t start=latest.objective_count-(uint32_t)n;
    for(size_t i=0;i<n;++i)
        *((uint8_t*)achieved+32+i)=(latest.objectives>>(start+i))&1;
    // Local XP is awarded by the persisted star/best-score policy, rather than
    // the remote per-objective schedule. Keep all original objective text and
    // real completion flags, but do not animate additional unawarded XP.
    field_set(description,class_field_from_name(object_class(description),"aim_xp"),aim_xp);
    field_set(stats,class_field_from_name(object_class(stats),"$UN"),achieved);
    field_set(stats,class_field_from_name(object_class(stats),"$wN"),string_new(""));
    // Obfuscated property order differs from field order. These original
    // setters were checked against getter instructions AND the UI call slots.
    // Offered/awarded/base/hard reward equal the one committed local payout;
    // the two displayed resource columns do not perform any extra transaction.
    if(!integer(stats,"$jiA",latest.credits) || // final resources, field +18
       !integer(stats,"$kiA",latest.credits) || // hard subtotal, +20 (unused in normal)
       !integer(stats,"$liA",latest.credits) || // actually awarded resources, +1c
       !integer(stats,"$miA",latest.old_best_xp) || // previous best score, +5c
       !integer(stats,"$oiA",latest.stars) ||   // stars, +24
       !integer(stats,"$piA",latest.credits) || // base resources, +14
       !integer(stats,"$riA",latest.gross_xp) || // final score, +48
       !integer(stats,"$siA",latest.gross_xp))return false; // battle score, +28
    *((int8_t*)stats+0x10)=0; // normal difficulty
    *((uint8_t*)stats+0x3c)=latest.win;
    field_set(points,class_field_from_name(object_class(points),"pveClientStatistics"),stats);
    field_set(points,class_field_from_name(object_class(points),"description"),description);
    void* klass=find_class("com.geargames.aow.scenes.main.gui.battle","GUIMainStrategicMapWindow");
    const void* show=klass?class_method(klass,"OnMessagePVEBattlePointsReceived",1):nullptr;
    if(!show)return false;
    void* args[]={points};runtime_invoke(show,nullptr,args,&ex);
    LOG("Local campaign result presented=%d faction=%d run=%llu chapter=%d win=%d stars=%d grossXp=%d netXp=%d credits=%d",
        !ex,latest.faction,(unsigned long long)latest.run,latest.chapter,latest.win,latest.stars,
        latest.gross_xp,latest.net_xp,latest.credits);
    if(ex){LOG("Local campaign result exception=%s",class_name(object_class(ex)));return false;}
    latest.pending=false;
    return true;
}
}
