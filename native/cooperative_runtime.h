// Original cooperative Battle/GAI input, one manual player and an AI ally.
// Scripted enemies, neutral sides, objectives and outcome remain original.
#pragma once
#include "cooperative_store.h"
#include "cooperative_credit_store.h"
#include "cooperative_goal_policy.h"
namespace local_coop {
using Roots=local_research_runtime::Roots;
static campaign_catalog::Catalog routes;
static uint64_t run;
static int last_step=-1000;
static cooperative_goal_policy::Scout scouts[cooperative_goal_policy::workers];
static local_duel::Goal pumpkin_goal;
static bool has_pumpkin_goal;
static bool runtime_failed,exit_requested,save_failed;
static cooperative_store::State ledger,pending_commit;
static bool objective_count(void* battle,uint32_t& count);
static void adopt(const cooperative_store::State& saved){
    ledger=saved;state=saved.match;battle_points=saved.points;last_award=saved.award;last_pumpkins=saved.pumpkins;
}
static char ledger_root[640];
static bool sync_credit(){
    if(!progress_ready||local_faction::active!=2)return false;local_progress::StateBuffer copy;if(!copy.data)return false;*copy.data=progress;bool changed=false;
    if(!competition_track_store::reconcile(*copy.data,ledger,changed))return false;
    if(changed){if(!local_progress::write(local_faction::root(),*copy.data))return false;progress=*copy.data;
        LOG("Local cooperative credit committed run=%llu credits=%u frozen_level=%u cursor=%llu generation=%llu",(unsigned long long)ledger.credit_run,ledger.credits,ledger.credit_level,(unsigned long long)progress.cooperative_credit_cursor,(unsigned long long)progress.generation);}
    return true;
}
static void log(const char* format,...){
    FILE* f=fopen(DATA_ROOT "art3offline-cooperative.txt","a");if(!f)return;
    va_list a;va_start(a,format);vfprintf(f,format,a);va_end(a);fputc('\n',f);fclose(f);
    chmod(DATA_ROOT "art3offline-cooperative.txt",0644);
}
static bool restore(){
    if(local_faction::active!=2)return false;
    snprintf(ledger_root,sizeof(ledger_root),"%s",offline_accounts::cooperative_root());mkdir(ledger_root,0700);
    cooperative_store::State loaded;int status=cooperative_store::load(ledger_root,loaded);
    if(status<0)return false;
    adopt(loaded);
    if(!playing&&state.next>state.settled){
        auto next=ledger;
        if(!cooperative_store::settle(next,next.match.next,local_match_store::Interrupted,0,0,-1,nullptr,nullptr,0)||!cooperative_store::write(ledger_root,next))return false;
        adopt(next);
    }
    return sync_credit();
}
static const campaign_catalog::Entry* route(){
    if(!routes.load(DATA_ROOT,"offline-cooperative/catalog.bin"))return nullptr;
    for(const auto& r:routes)if(r.faction==local_faction::active&&r.chapter==973&&r.map==558)return &r;
    return nullptr;
}
static bool commit_error(){
    auto next=ledger;
    bool ok=cooperative_store::settle(next,run,local_match_store::Error,0,0,-1,nullptr,nullptr,0)&&cooperative_store::write(ledger_root,next);
    if(ok)adopt(next);
    playing=false;local_bot_runtime::reset(false);log("START_FAILED run=%llu saved=%d",(unsigned long long)run,ok);return false;
}
static bool start(){
    if(!offline_network||playing||local_match::playing||local_run_active||!progress_ready||local_faction::active!=2)return false;
    if(!local_resources::prepare()||!local_resources::ready()){LOG("Local cooperative resources not initialized yet");return false;}
    const auto* input=route();if(!input||!routes.verify(*input)||!restore())return false;
    void* message=read_scenario_message(input->file,5040,"@local-cooperative");
    void* battle=message?*(void**)((char*)message+0x20):nullptr;
    if(!battle||*(int*)((char*)battle+0x28)!=973||*(int*)((char*)message+0x30)>0)return false;
    void* own=local_duel::side(battle,1);void* ally=local_duel::side(battle,3);void* enemy=local_duel::side(battle,2);
    if(!own||!ally||!enemy||local_duel::base_id(own)!=1100||local_duel::base_id(ally)!=1100||
       local_duel::alliance_of(battle,own)!=local_duel::alliance_of(battle,ally)||
       local_duel::alliance_of(battle,own)==local_duel::alliance_of(battle,enemy))return false;
    void* receiver=find_class("com.geargames.aow.scenes.logon.network","CSMainMissionDictionaryReceiver");
    void* dictionary_args[]={saved("MissionDictionary")};void* ex=nullptr;
    runtime_invoke(class_method(receiver,"ProcessDictionary",1),nullptr,dictionary_args,&ex);
    local_faction::Roots mission_roots;
    void* mission=local_faction::clone(local_mission_description(973),mission_roots);
    if(ex||!mission||*(int*)((char*)mission+0x14)!=558||*(int*)((char*)mission+0x24)!=5)return false;
    // The retained release dictionary has an older localized name for this ID.
    // Keep its verified map/type contract and use the observed original title.
    field_set(mission,class_field_from_name(object_class(mission),"name"),string_new("南瓜狩猎"));
    // The captured online cooperative Battle has this flag false. The local
    // session uses the original serverless PvE lifecycle, as campaign inputs do.
    // Preserve GAI/rules/seats; do not leave its online liveness contract active.
    *(bool*)((char*)battle+0xdc)=true;
    void* localization=static_object("com.geargames.aow.config","ClientContext","m_localizationContext");
    for(int key:{3712,3713})if(localization){
        void* existing=nullptr;void* a[]={&key,&existing};
        void* found=invoke_named(localization,"TryGetString",2,a,&ex);
        if(ex)return false;
        if(found&&!*((bool*)found+16)){
            // Original Chinese objective text observed in the official client.
            void* text=string_new(key==3712?"收集10个南瓜或摧毁敌方基地":"建筑保护将在战斗10分钟后失效。");
            void* add[]={&key,text};invoke_named(localization,"AddString",2,add,&ex);if(ex)return false;
        }
    }
    // Resolve and validate before reserving an independent cooperative run.
    if(ledger.match.next==UINT64_MAX||!offline_world_runtime::freeze_cooperative(ledger.match.next+1)||!local_campaign::initialize(battle,mission,1,true)||!local_inventory::prepare(battle,1)||!apply_local_battle_research(battle))return false;
    Roots roots;void* info=object_new(find_class("com.geargames.aow.scenes.battle","InitializationBattleInfo"));
    if(!roots.keep(info))return false;
    invoke_named(info,".ctor",0,nullptr,&ex);if(ex)return false;
    field_set(info,class_field_from_name(object_class(info),"Battle"),battle);
    *(int*)((char*)info+0x20)=1;*(bool*)((char*)info+0x24)=false;*(int*)((char*)info+0x28)=*(int*)((char*)message+0x30);
    if(!local_hero_battle::prepare(info,battle,1))return false;
    int credit_level=0;auto next=ledger;if(!local_history_journal::coop(ledger)||!local_reward_skills::cooperative_level(progress,credit_level)||!cooperative_store::begin(next)||!cooperative_store::freeze_credits(next,credit_level)||!register_shop_start(shop_box_policy::Cooperative,next.match.next)||!cooperative_store::write(ledger_root,next))return false;
    local_history_view::clear();
    adopt(next);run=state.next;playing=true;pending_result=presenting=false;
    local_match::presenting=false;runtime_failed=exit_requested=save_failed=false;last_step=-1000;
    for(auto& scout:scouts)scout=cooperative_goal_policy::Scout{};
    has_pumpkin_goal=false;
    local_duel::active=ai_lab::active=false;ai_lab::stopped=local_duel::stopped=false;
    local_duel::configured_seats=5;local_duel::sequence=0;local_duel::base_test=false;
    local_duel::damaged=local_duel::combat_sample=false;
    for(auto& seat:local_duel::seats)seat=local_duel::Seat{};
    local_bot_runtime::reset(true,true);
    FILE* controller=fopen(DATA_ROOT "art3offline-duel.txt","w");
    if(controller){fputs("schema=original-cooperative-ally-v1 player_ai_commands=0 ally=3 prototype_family=per-seat\n",controller);fclose(controller);}
    void* context=find_class("com.geargames.aow.scenes.battle","ClientBattleContext");
    static_set(class_field_from_name(context,"m_battleInfo"),info);
    local_run_active=false; // Never reserves or settles a campaign transaction.
    FILE* f=fopen(DATA_ROOT "art3offline-cooperative.txt","w");if(f)fclose(f);
    log("START run=%llu mission=973 map=558 player=1 ally=3 original_enemies=2,5 neutral=4 original_script=1 script_frozen=0 serverless_pve=1 career_writes=0",(unsigned long long)run);
    void* manager=find_class("com.geargames.aow.scenes.logon.network","CSMainLogonManager");
    runtime_invoke(class_method(manager,"ToStartBattle",0),nullptr,nullptr,&ex);
    return ex?commit_error():true;
}
static bool scout(void* battle,void* ally,int now,int worker,int* claims,int& claim_count){
    auto& state=scouts[worker];
    int& scout_id=state.unit;int& scout_order_tick=state.order_tick;int& scout_goal_id=state.goal;
    int& explore_index=state.explore;int& scout_x=state.x;int& scout_y=state.y;
    void* units=*(void**)((char*)ally+0x140);void* unit=nullptr;void* replacement=nullptr;
    for(size_t i=0;units&&i<array_length(units);++i){
        void* u=((void**)((char*)units+32))[i];if(!u||*(int*)((char*)u+0x184)<=0)continue;
        int id=*(int*)((char*)u+0x10);if(id==scout_id)unit=u;
        void* type=*(void**)((char*)u+0x40);
        // Basic infantry are land units and can use the normal chest capture.
        bool assigned=false;for(int j=0;j<cooperative_goal_policy::workers;++j)if(j!=worker&&scouts[j].unit==id)assigned=true;
        if(type&&*(int*)((char*)type+0x10)==3100&&!replacement&&!assigned)replacement=u;
    }
    if(!unit){unit=replacement;scout_id=unit?*(int*)((char*)unit+0x10):0;scout_goal_id=-1;scout_order_tick=-1000;}
    if(!unit)return true;
    int x=*(int16_t*)((char*)unit+0x48),y=*(int16_t*)((char*)unit+0x4a);
    void* chests=*(void**)((char*)battle+0x1d8);int n=chests?*(int*)((char*)chests+0x18):0;
    cooperative_goal_policy::Goal chosen{};
    void* points=*(void**)((char*)battle+0x1e0);int point_count=points?*(int*)((char*)points+0x18):0;
    for(int i=0;i<point_count&&i<256;++i){
        void* point=list_item(points,i);
        // The original mission uses both type 4/model 5 and type 6/model 10
        // land checkpoints. Keep their native phase/capture rules intact.
        // Visible is the original local observer cache.
        int kind=point?*((int8_t*)point+0x26):-1;
        if(!point||(kind!=4&&kind!=6)||*((int8_t*)point+0x28)!=0||*((int8_t*)point+0x3a)<=0)continue;
        int px=*(int16_t*)((char*)point+0x22),py=*(int16_t*)((char*)point+0x24);
        cooperative_goal_policy::consider(chosen,{*(int*)((char*)point+0x10),px,py,2,kind},x,y,scout_goal_id,claims,claim_count);
    }
    for(int i=0;i<n&&i<4096;++i){
        if(chosen.kind==2)break; // Mission objectives precede ordinary resources.
        void* chest=list_item(chests,i);if(!chest||*((bool*)chest+0x40)||*((int8_t*)chest+0x28)!=0)continue;
        void* visible=*(void**)((char*)chest+0x90);
        // The original client updates chest discovery for the local observer
        // (seat 1); allied seat 3 shares that observer's fog. Its own UI cache
        // remains zero, even when this scout has revealed the chest.
        if(!visible||array_length(visible)<=3||
           (!*((uint8_t*)visible+32+1)&&!*((uint8_t*)visible+32+3)))continue;
        int cx=*(int16_t*)((char*)chest+0x22),cy=*(int16_t*)((char*)chest+0x24);
        cooperative_goal_policy::consider(chosen,{*(int*)((char*)chest+0x10),cx,cy,1,0},x,y,scout_goal_id,claims,claim_count);
    }
    int target_id=chosen.id,tx=chosen.x,ty=chosen.y,goal_kind=chosen.kind,goal_type=chosen.type;
    if(target_id&&claim_count<cooperative_goal_policy::workers)claims[claim_count++]=target_id;
    if(goal_kind==2&&!has_pumpkin_goal){pumpkin_goal={tx,ty,target_id,4};has_pumpkin_goal=true;}
    if(!target_id){
        void* hq=ai_lab::building(ally,local_duel::base_id(ally));if(!hq)return true;
        if(now-scout_order_tick<400&&(abs(x-scout_x)>2||abs(y-scout_y)>2))return true;
        const int offsets[8][2]={{12,0},{0,12},{-12,0},{0,-12},{18,10},{-18,-10},{0,24},{-24,0}};
        int k=(explore_index++ + worker*3)%8;tx=*(int16_t*)((char*)hq+0x40)+offsets[k][0];ty=*(int16_t*)((char*)hq+0x42)+offsets[k][1];
        tx=tx<1?1:(tx>98?98:tx);ty=ty<1?1:(ty>98?98:ty);
    }else if(target_id==scout_goal_id&&
        ((abs(x-tx)<=2&&abs(y-ty)<=2)||now-scout_order_tick<100))return true;
    Roots roots;void* command=ai_lab::command("AICommUnitsMove",now,roots,3);
    void* ids=array_new(find_class("System","Int32"),1);if(!command||!roots.keep(ids))return false;
    // Same UnitMoveStyle.Forced as the original bonus/checkpoint click path
    // (ClickOnBonusBoxOrCheckPoint -> SendUnitsMove -> $KmA, field +48).
    // The escort retains ordinary assault movement and native auto-acquisition.
    *((int8_t*)command+0x48)=1;
    *(int*)((char*)ids+32)=scout_id;*(int16_t*)((char*)command+0x2e)=tx;*(int16_t*)((char*)command+0x30)=ty;
    if(!local_bot_runtime::put(command,0x40,ids))return false;
    int code=ai_lab::perform(battle,command);if(ai_lab::stopped)return false;
    scout_goal_id=target_id;scout_order_tick=now;scout_x=tx;scout_y=ty;
    log("SCOUT tick=%d side=3 worker=%d unit=%d target=%d,%d object=%d goal_kind=%d checkpoint_type=%d move_style=forced discovered_only=1 code=%d",now,worker,scout_id,tx,ty,target_id,goal_kind,goal_type,code);
    return true;
}
static void step(void* battle){
    if(!playing||pending_result||runtime_failed)return;
    int now=ai_lab::tick(battle);if(now<30||now-last_step<10)return;last_step=now;
    void* ally=local_duel::side(battle,3),*enemy=local_duel::side(battle,2);
    if(!ally||!enemy){runtime_failed=true;log("FAIL missing-original-seat");return;}
    if(!local_bot_runtime::ready(ally)&&!local_bot_runtime::prepare(battle,ally,now)){
        runtime_failed=true;log("FAIL ally-workspace tick=%d",now);return;
    }
    if(!local_duel::develop(battle,ally,now)||!local_duel::advance(battle,ally,now)){runtime_failed=true;log("FAIL ally-development tick=%d",now);return;}
    has_pumpkin_goal=false;int claims[cooperative_goal_policy::workers]={},claim_count=0,excluded[cooperative_goal_policy::workers]={};
    for(int i=0;i<cooperative_goal_policy::workers;++i){
        if(!scout(battle,ally,now,i,claims,claim_count)){runtime_failed=true;log("FAIL ally-scout tick=%d worker=%d",now,i);return;}
        excluded[i]=scouts[i].unit;
    }
    local_duel::Goal guard{};const local_duel::Goal* goal=has_pumpkin_goal?&pumpkin_goal:nullptr;
    void* hq=ai_lab::building(ally,local_duel::base_id(ally));
    if(!goal&&hq&&(now<6000||local_duel::alive(ally,0x140)<8)){
        guard={*(int16_t*)((char*)hq+0x40),*(int16_t*)((char*)hq+0x42)+4,0,3};goal=&guard;
    }
    if(!local_duel::army(battle,ally,enemy,now,goal,0,excluded,cooperative_goal_policy::workers)||ai_lab::stopped){runtime_failed=true;log("FAIL ally-army tick=%d",now);return;}
    if(now%200==0){uint32_t pumpkins=0;bool counted=objective_count(battle,pumpkins);
        log("LIVE tick=%d own_units=%d ally_units=%d ally_buildings=%d player_ai_commands=0 original_gai=1 counted=%d pumpkins=%u",now,
        local_duel::alive(local_duel::side(battle,1),0x140),local_duel::alive(ally,0x140),local_duel::alive(ally,0x148),counted,pumpkins);
    }
}
static bool objective_count(void* battle,uint32_t& count){
    // Mission973 markup1031/1033 increments BOTH side1 counter21 and side3
    // counter23 once per team pumpkin. Do not sum the duplicated counters or
    // count ordinary BonusChests. Counter slot14 is its actual value (+24);
    // slot16/+10 identifies its owning side, not the number collected.
    int values[2]={};int index=0;
    for(int side_id:{1,3}){
        void* side=local_duel::side(battle,side_id);
        void* counter=side?local_dictionary_item(*(void**)((char*)side+0x3c8),side_id==1?21:23):nullptr;
        if(!counter||strcmp(class_name(object_class(counter)),"Counter")||
           *(int*)((char*)counter+0x14)!=(side_id==1?21:23)||
           *(int*)((char*)counter+0x10)!=side_id||*((int8_t*)counter+0x18)!=4)return false;
        values[index++]=*(int*)((char*)counter+0x24);
    }
    if(values[0]<0||values[0]>100000||values[0]!=values[1])return false;
    count=values[0];return true;
}
static void finished(bool win,int tick,int last_tick){
    if(!playing||state.settled==run)return;
    void* battle=static_object("com.geargames.aow.scenes.battle","ClientBattleContext","m_battle");
    int reason=battle?*(int*)((char*)battle+0xd8):0;
    int buildings[]={local_duel::team_alive(battle,1,0x148),local_duel::team_alive(battle,2,0x148)};
    int units[]={local_duel::team_alive(battle,1,0x140),local_duel::team_alive(battle,2,0x140)};
    uint32_t pumpkins=0;bool counted=objective_count(battle,pumpkins);
    auto outcome=runtime_failed||!counted?local_match_store::Error:(reason==6?local_match_store::Surrender:
        (reason==7?local_match_store::Interrupted:(reason==1?(win?local_match_store::Victory:local_match_store::Defeat):local_match_store::Error)));
    auto next=ledger;bool settled=cooperative_store::settle(next,run,outcome,tick,reason,runtime_failed||!counted?-1:(win?1:2),buildings,units,pumpkins);
    bool ok=settled&&cooperative_store::write(ledger_root,next);
    pending_commit=next;if(ok){adopt(next);local_history_journal::coop(ledger,local_history_journal::now());}save_failed=!ok;pending_result=true;
    log("FINISH run=%llu saved=%d result=%d tick=%d last=%d native_reason=%d original_outcome=1 counted=%d pumpkins=%u award=%u points=%llu policy=local-v1",(unsigned long long)run,ok,outcome,tick,last_tick,reason,counted,pumpkins,next.award,(unsigned long long)next.points);
}
static bool publish(){
    if(playing&&!pending_result){commit_error();return true;}
    if(!pending_result)return true;
    if(save_failed){if(pending_commit.match.settled!=run||!cooperative_store::write(ledger_root,pending_commit))return false;adopt(pending_commit);save_failed=false;local_history_journal::coop(ledger,local_history_journal::now());}
    if(!sync_credit()||!apply_local_progress())return false;
    local_history_view::clear();
    auto previous=local_campaign_results::latest;auto& view=local_campaign_results::latest;
    view={};view.pending=true;view.win=state.outcome==local_match_store::Victory;view.run=state.settled;view.chapter=-1002;
    view.credits=ledger.credits;
    char title[192];snprintf(title,sizeof(title),"本地合作 PVE · 南瓜狩猎 · %s（已保存）",local_match::outcome_name(state.outcome));
    bool ok=local_campaign_results::publish(true,title,973);local_campaign_results::latest=previous;
    if(!ok)return false;
    local_match::presenting=false;
    playing=pending_result=false;presenting=true;local_bot_runtime::reset(false);
    log("RESULT_PAGE run=%llu campaign_writes=0",(unsigned long long)state.settled);return true;
}
static void frame(){
    if(!playing)return;
    if(runtime_failed&&!exit_requested&&!pending_result){
        exit_requested=true;void* c=find_class("com.geargames.aow.scenes.battle.gui.globalDialogWindow","DialogBattleFinishProvider");
        void* ex=nullptr;const void* m=c?class_method(c,"SendSideSurrender",0):nullptr;
        if(m)runtime_invoke(m,nullptr,nullptr,&ex);log("ERROR_EXIT requested=%d",m&&!ex);
    }
    // Same ordinary-PvE input mask contract as local player duels.
    void* screen=static_object("com.geargames.aow.scenes.battle","ClientBattleContext","m_screen");
    void* dispatcher=screen?*(void**)((char*)screen+0x1d0):nullptr;
    void* gui=dispatcher?*(void**)((char*)dispatcher+0x60):nullptr;
    void* blocker=gui?*(void**)((char*)gui+0x398):nullptr;void* ex=nullptr;
    void* active=blocker?invoke_named(blocker,"get_IsActive",0,nullptr,&ex):nullptr;
    if(active&&!ex&&*((bool*)active+16))invoke_named(blocker,"Deactivate",0,nullptr,&ex);
    void* tutorial=static_object("com.geargames.aow.scenes.battle","ClientBattleContext","m_tutorialController");
    void* darken=tutorial?*(void**)((char*)tutorial+0x48):nullptr;
    void* colliders=darken?*(void**)((char*)darken+0x38):nullptr;
    bool no=false;void* args[]={&no};if(colliders)invoke_named(colliders,"SetActive",1,args,&ex);
}
static bool tile(void* view){
    void* ex=nullptr;void* info=view?invoke_named(view,"get_TileInfo",0,nullptr,&ex):nullptr;
    return info&&!ex&&*(int16_t*)((char*)info+0x18)==6;
}
static bool presenter(void* self){return self&&!strcmp(class_name(object_class(self)),"CompetitionTilePresenter")&&tile(*(void**)((char*)self+0x78));}
static bool original_task(void* profile){
    if(!offline_network||local_faction::active!=2||!profile||strcmp(class_name(object_class(profile)),"CompetitionProfileInfo"))return false;
    void* info=*(void**)((char*)profile+0x10);
    return info&&*(int*)((char*)info+0x10)==10140;
}
static void set_ref(void* object,const char* name,void* value){field_set(object,class_field_from_name(object_class(object),name),value);}
static void* original_task_model(void* source,local_faction::Roots& roots){
    if(!original_task(source)||!restore())return nullptr;
    const char* ns="com.geargames.aow.entities.tournament.tournamentNew";
    void* profile=local_faction::clone(source,roots);
    void* info=local_faction::clone(*(void**)((char*)source+0x10),roots);
    void* plates=array_new(find_class(ns,"PlateInfo"),4);
    if(!profile||!info||!roots.keep(plates))return nullptr;
    void* ex=nullptr;
    const char* names[]={"胜利战斗点","累计战斗点","上局战斗点","合作战绩"};
    const char* hints[]={"本地规则：首胜100，重复胜50；正常失败每南瓜5点，上限25。投降、中断、异常0点。",
        "本地合作独立累计，不增加金币、资源、经验或星数。","查看上局真实结算；浏览不重复发放。","本地合作实际胜负记录。"};
    char values[4][96];snprintf(values[0],96,"%u",ledger.first_win?50:100);
    snprintf(values[1],96,"%llu",(unsigned long long)battle_points);snprintf(values[2],96,"+%u",last_award);
    snprintf(values[3],96,"%u胜 / %u负",state.wins,state.losses);
    for(int i=0;i<4;++i){
        local_faction::Roots item_roots;void* p=object_new(find_class(ns,"PlateInfo"));if(!item_roots.keep(p))return nullptr;
        invoke_named(p,".ctor",0,nullptr,&ex);if(ex)return nullptr;
        set_ref(p,"name",string_new(names[i]));set_ref(p,"icon",string_new(""));
        set_ref(p,"value",string_new(values[i]));set_ref(p,"desc",string_new(hints[i]));
        *(void**)((char*)plates+32+i*8)=p;
    }
    void* price=object_new(find_class(ns,"RewardItemSet"));if(!roots.keep(price))return nullptr;
    invoke_named(price,".ctor",0,nullptr,&ex);if(ex)return nullptr;
    // This view accepts a genuinely free item set; unlike the PvP tile it does
    // not index a mandatory first item. Keep the original set/list types.
    local_research_runtime::Roots price_roots;
    void* items=local_campaign::field_list(object_class(price),"items",price_roots);if(!items)return nullptr;
    set_ref(price,"items",items);
    set_ref(profile,"competitionInfo",info);set_ref(profile,"plates",plates);set_ref(profile,"battlePrice",price);
    void* pass=local_competition_tracks::model_for_view(roots);if(!pass)return nullptr;
    set_ref(profile,"battlePass",pass);set_ref(profile,"tutorialUrl",string_new(""));
    *(int*)((char*)profile+0x30)=0;*(int8_t*)((char*)profile+0x48)=2;*(bool*)((char*)profile+0x68)=true;
    set_ref(profile,"stageStr",string_new("本地合作"));set_ref(profile,"timeStr",string_new("随时可玩"));
    // No online schedule, licence or paid pass is part of this local contract.
    *(int*)((char*)info+0x18)=0;*(int*)((char*)info+0x48)=0;set_ref(info,"ticket",nullptr);
    return profile;
}
static void (*section_initiate_original)(void*,void*,void*,void*);
static void component_active(void* component,bool active,void** ex){
    void* go=component?invoke_named(component,"get_gameObject",0,nullptr,ex):nullptr;
    void* a[]={&active};if(go&&!*ex)invoke_named(go,"SetActive",1,a,ex);
}
static void section_initiate(void* self,void* profile,void* memory,void* method){
    if(!original_task(profile)){section_initiate_original(self,profile,memory,method);return;}
    local_faction::Roots roots;void* adapted=original_task_model(profile,roots);
    if(!adapted){log("ORIGINAL_UI model_failed=1 battle_start=0 career_writes=0");return;}
    // Keep the original section initialization for its task, controllers, image,
    // navigation and lists. Initialize its original pass with local receipts;
    // the remaining original tail automatically requests an online tutorial.
    void* ex=nullptr;void* a[]={adapted};invoke_named(self,"InitiateCompetitionCommon",1,a,&ex);
    void* pass_view=*(void**)((char*)self+0xc8);
    if(!ex){component_active(pass_view,true,&ex);if(!ex&&!local_competition_tracks::initiate(pass_view,*(void**)((char*)adapted+0x20),memory)){
        log("ORIGINAL_UI track_initiate_failed=1");return;}}
    void* view=invoke_named(self,"get_SectionRootView",0,nullptr,&ex);
    if(view&&!strcmp(class_name(object_class(view)),"UGUISectionCompetitionView")){
        void* group=*(void**)((char*)view+0x68);void* toggles=group?*(void**)((char*)group+0x28):nullptr;
        int count=toggles?*(int*)((char*)toggles+0x18):0;void* items=toggles?*(void**)((char*)toggles+0x10):nullptr;
        for(int i=0;items&&i<count&&i<3;++i){
            void* toggle=*(void**)((char*)items+32+i*8);bool no=false;void* off[]={&no};
            if(toggle){
                // Disable input while retaining the original tab and its slot.
                invoke_named(toggle,"set_enabled",1,off,&ex);
                bool yes=true;void* a[]={type_object(class_type(find_class("TMPro","TMP_Text"))),&yes};
                void* labels=invoke_named(toggle,"GetComponentsInChildren",2,a,&ex);
                float alpha=0.4f;void* faded[]={&alpha};
                for(size_t j=0;labels&&j<array_length(labels);++j){void* label=*(void**)((char*)labels+32+j*8);if(label)invoke_named(label,"set_alpha",1,faded,&ex);}
            }
        }
    }
    log("ORIGINAL_UI section_initiated=%d plates=4 free=1 battle_start=0 career_writes=0",!ex);
}
static void (*ready_open_original)(void*,void*);
static void ready_open(void* self,void* method){
    void* profile=self?*(void**)((char*)self+0x50):nullptr;
    if(!original_task(profile)){ready_open_original(self,method);return;}
    void* ex=nullptr;void* view=*(void**)((char*)self+0x58);
    if(view){invoke_named(view,"Activate",0,nullptr,&ex);invoke_named(view,"ShowEmpty",0,nullptr,&ex);}
    log("ORIGINAL_UI ready_local=%d online_wait=0",!ex);
}
static void (*more_load_original)(void*,void*),(*more_desc_original)(void*,void*);
static bool more_info(void* self){
    void* profile=self?*(void**)((char*)self+0x80):nullptr;if(!original_task(profile))return false;
    void* ex=nullptr;
    void* title=*(void**)((char*)self+0x40);void* caption[]={string_new("关于比赛")};if(title)invoke_named(title,"set_text",1,caption,&ex);
    char record[256];snprintf(record,sizeof(record),"本地合作：%u胜 / %u负。累计战斗点：%llu。\n上局：%s，战斗点+%u。",
        state.wins,state.losses,(unsigned long long)battle_points,local_match::outcome_name(state.outcome),last_award);
    const char* text[]={"南瓜狩猎：收集10个南瓜或摧毁敌方基地。玩家手动控制，队友由AI控制。",
        "战斗点：首胜100，重复胜50；正常失败每南瓜5点，上限25。投降、中断或异常0点。",
        "前10分钟建筑受保护；只能在高地建造。南瓜可能触发增益或负面效果，摧毁仓库可获得额外南瓜。",record};
    for(int i=0;i<4;++i){void* label=*(void**)((char*)self+0x48+i*8);void* a[]={string_new(text[i])};if(label)invoke_named(label,"set_text",1,a,&ex);}
    for(int offset:{0x68,0x70}){void* block=*(void**)((char*)self+offset);if(block)invoke_named(block,"Hide",0,nullptr,&ex);}
    log("ORIGINAL_UI native_more_info=%d",!ex);return true;
}
static void more_load(void* self,void* method){if(!more_info(self))more_load_original(self,method);}
static void more_desc(void* self,void* method){if(!more_info(self))more_desc_original(self,method);}
static void (*matching_coop_original)(void*,void*);
static void matching_coop(void* self,void* method){
    void* profile=self?*(void**)((char*)self+0x38):nullptr;
    if(!original_task(profile)){matching_coop_original(self,method);return;}
    bool ok=start();log("ORIGINAL_UI native_battle_button=1 start=%d",ok);
}
static void (*click_original)(void*,void*),(*locked_original)(void*,void*),(*unavailable_original)(void*,void*);
static int (*competition_tile_state_original)(void*,void*);
static int competition_tile_state(void* self,void* method){
    if(offline_network&&local_faction::active==2&&tile(self)&&local_match::tile_unlocked(self?*(void**)((char*)self+0x88):nullptr))return 1;
    return competition_tile_state_original(self,method);
}
static bool original_section(void* self){
    if(!offline_network||local_faction::active!=2)return false;
    if(!local_match::tile_unlocked(self))return false;
    if(playing||local_match::playing)return true;
    void* manager=self?*(void**)((char*)self+0xa0):nullptr;
    void* profile=self?*(void**)((char*)self+0xb0):nullptr;
    if(!manager||!original_task(profile)){
        log("ORIGINAL_UI missing_task_model=1");return true;
    }
    if(!route()||!restore()){log("ORIGINAL_UI entry_unavailable=1 preserved_save=1");return true;}
    local_resources::prepare();
    void* info=*(void**)((char*)profile+0x10);int id=info?*(int*)((char*)info+0x10):0;
    // Preserve the original pooled command, dependency injection and navigation.
    // Its availability query is adapted above; do not synthesize a replacement page.
    click_original(self,nullptr);
    log("ORIGINAL_UI id=%d original_click=1 battle_start=0 career_writes=0",id);
    return true;
}
static void click(void* self,void* method){if(presenter(self)&&original_section(self))return;click_original(self,method);}
static void locked(void* self,void* method){if(presenter(self)&&original_section(self))return;locked_original(self,method);}
static void unavailable(void* self,void* method){if(presenter(self)&&original_section(self))return;unavailable_original(self,method);}
static void (*configure_original)(void*,int,void*);
static void configure(void* self,int status,void* method){
    bool ours=offline_network&&local_faction::active==2&&tile(self);
    bool team=offline_network&&local_match::original_team_tile(self);
    bool unlocked=(ours||team)&&local_match::tile_unlocked(self?*(void**)((char*)self+0x88):nullptr);
    configure_original(self,unlocked?0:status,method);
}
static void install(){
    const uint32_t c[]={0xf81d0ffe,0xa90157f6,0xa9024ff4,0x9000fdb4};
    const uint32_t l[]={0xd10103ff,0xa9015ffe,0xa90257f6,0xa9034ff4};
    const uint32_t u[]={0xa9be57fe,0xa9014ff4,0xf000fdb3,0xaa0003f4};
    const uint32_t v[]={0xf81e0ffe,0xa9014ff4,0xaa0003f3,0xf9402c00};
    const uint32_t s[]={0xa9be57fe,0xa9014ff4,0x9000fd74,0xaa0003f3};
    const uint32_t si[]={0xa9bc67fe,0xa9015ff8,0xa90257f6,0xa9034ff4};
    const uint32_t mc[]={0xa9be57fe,0xa9014ff4,0xd0010ab4,0xaa0003f3};
    const uint32_t ro[]={0xf81c0ffe,0xa9015ff8,0xa90257f6,0xa9034ff4};
    const uint32_t ml[]={0xf81d0ffe,0xa90157f6,0xa9024ff4,0xaa0003f3};
    ready_open_original=(decltype(ready_open_original))install_hook(0x7c5e028,ro,(void*)ready_open);
    more_load_original=(decltype(more_load_original))install_hook(0x7bb668c,ml,(void*)more_load);
    more_desc_original=(decltype(more_desc_original))install_hook(0x7bb67f4,si,(void*)more_desc);
    section_initiate_original=(decltype(section_initiate_original))install_hook(0x7c67610,si,(void*)section_initiate);
    matching_coop_original=(decltype(matching_coop_original))install_hook(0x7c5b8bc,mc,(void*)matching_coop);
    competition_tile_state_original=(decltype(competition_tile_state_original))install_hook(0x7e05768,s,(void*)competition_tile_state);
    click_original=(decltype(click_original))install_hook(0x7dfd85c,c,(void*)click);
    locked_original=(decltype(locked_original))install_hook(0x7dfa544,l,(void*)locked);
    unavailable_original=(decltype(unavailable_original))install_hook(0x7dfa394,u,(void*)unavailable);
    configure_original=(decltype(configure_original))install_hook(0x7e032bc,v,(void*)configure);
    LOG("Local cooperative original entry hooks=%d",click_original&&locked_original&&unavailable_original&&configure_original&&competition_tile_state_original&&section_initiate_original&&matching_coop_original&&ready_open_original&&more_load_original&&more_desc_original);
}
}
