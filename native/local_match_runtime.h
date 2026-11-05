// Player-controlled local 1v1, using the original hall, battle and result UI.
#pragma once
namespace local_match {
static uintptr_t dialog_marker_root;
static void* dialog_marker;
static bool installed, runtime_failed, error_exit_requested;
static int selected,selected_opponent,selected_mode,selection_owner;
static int map_page;
static local_match_store::State pending_commit;
static void* pending_contract_battle;
static int dialog_action; // 1=start/view, 3=maps, 4=settings, 5=opponent, 6=team size
static const char* faction_name(int id){return id==1?"联盟军":"反抗军";}
static const char* mode_name(int mode){return mode==7?"3人混战":mode==8?"4人混战":mode==9?"5人混战":mode==10?"6人混战":mode==5?"1v1 占领":mode==6?"1v1 演习":mode==4?"3v3":(mode==2?"2v2":"1v1");}
static void select_defaults() {
    if(selection_owner!=local_faction::active){selected=state.scenario;selected_opponent=state.opponent;selected_mode=state.mode;selection_owner=local_faction::active;}
    const auto* route=local_match_maps::find(selected);
    if(!route||route->faction!=local_faction::active)selected=state.scenario;
    if(selected_opponent!=1&&selected_opponent!=2)selected_opponent=state.opponent;
    if(!control_point_policy::valid_mode(selected_mode))selected_mode=state.mode;
}
static const char* outcome_name(int result) {
    switch(result){case local_match_store::Victory:return "胜利";case local_match_store::Defeat:return "失败";
    case local_match_store::Surrender:return "投降";case local_match_store::Draw:return "平局";
    case local_match_store::Interrupted:return "中断（未判胜负）";case local_match_store::Error:return "运行异常（未判胜负）";default:return "无";}
}
static bool restore() {
    local_match_store::State loaded{};int status=local_match_store::load(local_faction::root(),loaded);
    if(status<0){LOG("Local AI ledger corrupt; refusing to reset");return false;}
    if(!status) {
        const auto* first=local_match_maps::first(local_faction::active);if(!first)return false;
        loaded.scenario=first->chapter;loaded.map=first->map;loaded.opponent=local_faction::active;
    }
    const auto* route=local_match_maps::find(loaded.scenario);
    if(!route||route->faction!=local_faction::active||route->map!=loaded.map){LOG("Local AI ledger route mismatch; preserving record");return false;}
    state=loaded;
    if(state.next>state.settled && !playing) {
        auto next=state;
        if(!local_match_store::settle(next,next.next,local_match_store::Interrupted,0,0,-1,nullptr,nullptr)||
           !local_match_store::write(local_faction::root(),next))return false;
        state=next;
    }
    return offline_world_runtime::sync(state);
}
static void* main_screen() {
    return static_object("com.geargames.aow.scenes.main","ClientMainContext","<Screen>k__BackingField");
}
static bool dialog(const char* title,const char* text,int action) {
    void* screen=main_screen();void* dispatcher=screen?*(void**)((char*)screen+0x100):nullptr;
    if(!dispatcher||!installed)return false;
    local_research_runtime::Roots roots;
    if(!dialog_marker_root){dialog_marker=string_new("art3-local-ai-dialog");dialog_marker_root=gchandle_new(dialog_marker,false);}
    void* builder=object_new(find_class("com.geargames.aow.client.gui.managedwindow","DialogWindowBuilder"));
    if(!dialog_marker_root||!roots.keep(builder))return false;
    void* ex=nullptr;void* ctor[]={dispatcher};invoke_named(builder,".ctor",1,ctor,&ex);
    bool start_action=action==1||action==7;
    const auto& ledger=action==7?local_coop::state:state;
    int buttons=start_action?6:1;void* body=string_new(text);void* args[]={body,&buttons};
    if(!ex)invoke_named(builder,"SetTextWithButton",2,args,&ex);
    void* caption=string_new(title);void* caption_args[]={caption};
    if(!ex)invoke_named(builder,"WithTitle",1,caption_args,&ex);
    if(start_action) {
        // DialogWindowCustomButton is a 16-byte value type (bool + string).
        struct Custom { bool enabled; uint8_t pad[7]; void* text; } custom{true,{},string_new(action==7?"开始合作":"开始对战")};
        void* custom_args[]={&custom};if(!ex)invoke_named(builder,"WithCustomButton",1,custom_args,&ex);
    }
    if(start_action && ledger.settled && !ex) {
        void* presets=find_class("com.geargames.aow.ugui","WindowButtonPresets");
        void* button_class=find_class("com.geargames.aow.ugui","WindowButtonData");
        void* cancel=runtime_invoke(class_method(presets,"Cancel",0),nullptr,nullptr,&ex);
        if(!roots.keep(cancel)||ex)return false;
        void* recent=runtime_invoke(class_method(presets,"Ok",0),nullptr,nullptr,&ex);
        if(!roots.keep(recent)||ex)return false;
        void* id=invoke_named(recent,"get_Id",0,nullptr,&ex);void* create_args[]={id};
        void* configure=runtime_invoke(class_method(button_class,"Create",1),nullptr,create_args,&ex);
        if(!roots.keep(configure)||ex)return false;
        void* text_args[]={string_new("查看上局")};invoke_named(configure,"WithText",1,text_args,&ex);
        recent=invoke_named(configure,"Build",0,nullptr,&ex);
        if(!roots.keep(recent)||ex)return false;
        void* start_args[]={string_new(action==7?"开始合作":"开始对战")};
        void* start=runtime_invoke(class_method(presets,"Custom",1),nullptr,start_args,&ex);
        void* buttons=array_new(button_class,3);
        if(!roots.keep(start)||!roots.keep(buttons)||ex)return false;
        auto items=(void**)((char*)buttons+32);items[0]=cancel;items[1]=recent;items[2]=start;
        void* a[]={buttons};invoke_named(builder,"WithButtons",1,a,&ex);
    }
    if(action>=3&&action<=6&&!ex) {
        void* presets=find_class("com.geargames.aow.ugui","WindowButtonPresets");
        void* klass=find_class("com.geargames.aow.ugui","WindowButtonData");
        int count=0;char labels[4][80]={},ids[4][48]={};
        if(action==3){int total=local_match_maps::count(local_faction::active),pages=(total+2)/3,index=0;
            if(!pages)return false;map_page%=pages;
            for(const auto& route:local_match_maps::routes){
                if(route.faction!=local_faction::active)continue;int ordinal=index++;
                if(ordinal/3!=map_page)continue;
                snprintf(labels[count],80,"%s%s%s",route.label,route.max_seats<6?"（至2v2）":"",route.chapter==selected?"（当前）":"");
                snprintf(ids[count++],48,"local-map-%d",route.chapter);
            }
            if(pages>1){snprintf(labels[count],80,"下一页（%d/%d）",map_page+1,pages);snprintf(ids[count++],48,"local-map-next");}
        }
        if(action==4) {
            count=4;const char* names[]={"选择地图","更换对手","队伍规模","返回规则"};const char* keys[]={"map","opponent","mode","entry"};
            for(int i=0;i<4;++i){snprintf(labels[i],80,"%s",names[i]);snprintf(ids[i],48,"local-settings-%s",keys[i]);}
        }
        if(action==5) {
            count=2;for(int i=0;i<2;++i){snprintf(labels[i],80,"%s AI%s",faction_name(i+1),selected_opponent==i+1?"（当前）":"");snprintf(ids[i],48,"local-opponent-%d",i+1);}
        }
        if(action==6){const auto* route=local_match_maps::find(selected);if(!route)return false;const int modes[]={1,2,4};
            for(int mode:modes)if(ai_team_policy::seats(mode)<=route->max_seats){snprintf(labels[count],80,"%s%s",mode_name(mode),selected_mode==mode?"（当前）":"");snprintf(ids[count++],48,"local-mode-%d",mode);}}
        void* choices=array_new(klass,count);if(!roots.keep(choices))return false;
        for(int index=0;index<count;++index) {
            void* a[]={string_new(labels[index])};
            void* choice=runtime_invoke(class_method(presets,"Custom",1),nullptr,a,&ex);
            if(!roots.keep(choice)||ex)return false;
            void* id_args[]={string_new(ids[index])};invoke_named(choice,"set_Id",1,id_args,&ex);
            ((void**)((char*)choices+32))[index]=choice;
        }
        void* a[]={choices};if(!ex)invoke_named(builder,"WithButtons",1,a,&ex);
    }
    void* user_args[]={dialog_marker};if(!ex)invoke_named(builder,"WithUserData",1,user_args,&ex);
    dialog_action=action;
    if(!ex)invoke_named(builder,"Show",1,user_args,&ex);
    if(ex){dialog_action=0;LOG("Local AI dialog exception=%s",class_name(object_class(ex)));return false;}
    return true;
}
static bool tile_unlocked(void* presenter){
    if(!presenter)return false;void* ex=nullptr;void* opened=invoke_named(presenter,"IsTileAvailableByRank",0,nullptr,&ex);
    return opened&&!ex&&*((bool*)opened+16);
}
static void entry(void* self,void*) {
    if(!offline_network||playing)return;
    if(self&&!strcmp(class_name(object_class(self)),"PVPTilePresenter")&&!tile_unlocked(self)){
        void* ex=nullptr;invoke_named(self,"OnLockClick",0,nullptr,&ex);return;
    }
    if(!restore()){dialog("本地 AI 对战","对战记录无法读取或保存，请保留本地数据后重试。",0);return;}
    select_defaults();
    // The original PvP card starts its local 1v1 directly. Team sizes live on
    // the original 2v2/3v3 cards; no additional configuration page.
    if(!start_local_match(selected,selected_opponent,1,true))dialog("未能开始对战","初始化或记录保存失败。当前进度已保留。",0);
    return;
}
static void (*button_original)(void*,int,void*);
static void button(void* self,int result,void* method) {
    bool ours=false;
    void* model=self?*(void**)((char*)self+0x30):nullptr;
    void* ex=nullptr;void* data=model?invoke_named(model,"get_CurrentWindow",0,nullptr,&ex):nullptr;
    if(data&&!ex)ours=dialog_marker&&*(void**)((char*)data+16+0x68)==dialog_marker;
    int action=ours?dialog_action:0;
    if(ours)dialog_action=0;
    button_original(self,result,method);
    if(action==1&&result==0&&state.settled) {
        pending_result=true;save_failed=false;run=state.settled;
        LOG("Local AI saved result opened=%d run=%llu",publish(),(unsigned long long)run);
    }
    if(action==1&&result==2) {
        if(!start_local_match(selected,selected_opponent,selected_mode,local_match_store::formal(state)))dialog("未能开始对战","初始化或记录保存失败。当前战役进度已保留。",0);
    }
    if(action==7&&result==0&&local_coop::state.settled){local_coop::pending_result=true;local_coop::publish();}
    if(action==7&&result==2&&!local_coop::start())dialog("未能开始合作","原合作输入、初始化或独立记录不可用，当前进度已保留。",0);
}
static void map_settings(void*,void*) {
    if(playing||!offline_network)return;
    if(!restore()){dialog("地图选择","本地记录不可读，已保留原档。",0);return;}
    select_defaults();const auto* route=local_match_maps::find(selected);char body[256];
    snprintf(body,sizeof(body),"%s · %s · %s\n对手：%s AI\n可选择地图、对手阵营和队伍规模。返回键取消。",faction_name(local_faction::active),route->label,mode_name(selected_mode),faction_name(selected_opponent));
    dialog("本地对战设置",body,4);
}
static void (*choice_original)(void*,void*,void*);
static void choice(void* self,void* choice,void* method) {
    void* model=self?*(void**)((char*)self+0x30):nullptr;void* ex=nullptr;
    void* data=model?invoke_named(model,"get_CurrentWindow",0,nullptr,&ex):nullptr;
    bool ours=dialog_action>=3&&dialog_action<=6&&data&&!ex&&dialog_marker&&*(void**)((char*)data+16+0x68)==dialog_marker;
    char id[48]={};void* value=choice?invoke_named(choice,"get_Id",0,nullptr,&ex):nullptr;
    if(ours&&!ex&&local_research_runtime::ascii(value,id,sizeof(id))) {
        int action=dialog_action,chapter=-1,opponent=0,mode=0;
        bool map=action==3&&sscanf(id,"local-map-%d",&chapter)==1&&local_match_maps::find(chapter)&&local_match_maps::find(chapter)->faction==local_faction::active;
        bool next_map=action==3&&!strcmp(id,"local-map-next");
        bool ai=action==5&&sscanf(id,"local-opponent-%d",&opponent)==1&&(opponent==1||opponent==2);
        const auto* selected_route=local_match_maps::find(selected);
        bool team=action==6&&sscanf(id,"local-mode-%d",&mode)==1&&(mode==1||mode==2||mode==4)&&selected_route&&ai_team_policy::seats(mode)<=selected_route->max_seats;
        bool settings=action==4&&(!strcmp(id,"local-settings-map")||!strcmp(id,"local-settings-opponent")||!strcmp(id,"local-settings-mode")||!strcmp(id,"local-settings-entry"));
        if(!map&&!next_map&&!ai&&!team&&!settings){LOG("Local AI unknown settings choice");return;}
        dialog_action=0;button(self,1,nullptr);
        if(next_map){++map_page;dialog("选择对战地图","选择本局地图。返回键取消。",3);return;}
        if(map){selected=chapter;const auto* route=local_match_maps::find(selected);if(ai_team_policy::seats(selected_mode)>route->max_seats)selected_mode=route->max_seats>=4?2:1;}
        if(ai)selected_opponent=opponent;
        if(team)selected_mode=mode;
        if(settings&&!strcmp(id,"local-settings-map")){int index=0;map_page=0;for(const auto& r:local_match_maps::routes)if(r.faction==local_faction::active){if(r.chapter==selected){map_page=index/3;break;}++index;}
            dialog("选择对战地图","选择本局地图。返回键取消。",3);return;}
        if(settings&&!strcmp(id,"local-settings-opponent")){dialog("选择 AI 对手","选择对手阵营。双方从总部和1200资源开始。返回键取消。",5);return;}
        if(settings&&!strcmp(id,"local-settings-mode")){dialog("队伍规模","1v1：你对抗一名AI。\n2v2：你与一名同阵营AI对抗两名敌方AI。\n3v3：你与两名同阵营AI对抗三名敌方AI。\n战绩按当前阵营累计。返回键取消。",6);return;}
        entry(nullptr,nullptr);return;
    }
    choice_original(self,choice,method);
}
static void available(void* self,void*) {
    // Only the original PvP selector becomes a local entry. No rank/unlock edits.
    void* ex=nullptr;void* toggle=invoke_named(self,"get_Toggle",0,nullptr,&ex);
    bool yes=true;void* args[]={&yes};
    if(toggle&&!ex)invoke_named(toggle,"set_enabled",1,args,&ex);
    void* icon=self?*(void**)((char*)self+0x28):nullptr;float alpha=1.f;void* a[]={&alpha};
    if(icon)invoke_named(icon,"set_alpha",1,a,&ex);
}
static void finished(bool win,int tick,int last_tick) {
    if(!playing||state.settled==run)return;
    void* battle=static_object("com.geargames.aow.scenes.battle","ClientBattleContext","m_battle");
    if(!battle||!local_duel::seeded){save_failed=true;return;}
    int buildings[]={local_duel::team_alive(battle,1,0x148),local_duel::team_alive(battle,2,0x148)};
    int units[]={local_duel::team_alive(battle,1,0x140),local_duel::team_alive(battle,2,0x140)};
    bool free_for_all=local_battle_format::free_for_all(state.mode);int actual_winner=win?1:2;
    if(free_for_all){
        buildings[1]=units[1]=0;
        for(int id=2;id<=local_duel::seat_count();++id){buildings[1]+=local_duel::alive(local_duel::side(battle,id),0x148);units[1]+=local_duel::alive(local_duel::side(battle,id),0x140);}
        int index=*(int8_t*)((char*)battle+0x2a8);void* alliances=*(void**)((char*)battle+0x170);
        void* winner=index>=0&&alliances&&index<int(array_length(alliances))?((void**)((char*)alliances+32))[index]:nullptr;
        actual_winner=winner?*(int*)((char*)winner+0x10):-1;
    }
    int reason=*(int*)((char*)battle+0xd8);
    if(free_for_all&&reason!=1)actual_winner=-1;
    auto result=runtime_failed?local_match_store::Error:(reason==6?local_match_store::Surrender:
        (reason==7?local_match_store::Interrupted:(reason!=1?local_match_store::Error:
        (local_duel::control_points::session.draw?local_match_store::Draw:win?local_match_store::Victory:local_match_store::Defeat))));
    auto next=state;
    pending_contract_battle=battle;
    bool ok=local_match_store::settle(next,run,result,tick,reason,runtime_failed||result==local_match_store::Draw?-1:actual_winner,buildings,units)&&
        battle_contract_runtime::stamp(next,battle)&&local_warpoints::stamp(next)&&local_match_store::write(local_faction::root(),next);
    pending_commit=next;
    if(ok){state=next;local_history_journal::pvp(state,local_history_journal::now());
        if(!offline_world_runtime::sync(state))LOG("Local strength update deferred; original result retained for cold recovery run=%llu",(unsigned long long)run);}
    save_failed=!ok;pending_result=true;local_duel::stopped=true;
    LOG("Local AI result saved=%d run=%llu outcome=%d tick=%d last=%d reason=%d campaign_writes=0",ok,(unsigned long long)run,result,tick,last_tick,reason);
}
static void (*surrender_send_original)(void*);
static void surrender_send(void* method){
    surrender_send_original(method);
    if(playing&&!pending_result&&!local_duel::finish_requested){
        local_duel::requested_surrender.store(true);
        local_duel::log("HUMAN_SURRENDER_CONFIRMED run=%llu original_ui_and_command=1 deferred_to_logic_thread=1",(unsigned long long)run);
    }
}
static bool publish() {
    if(!pending_result)return true;
    if(save_failed) {
        if(battle_contract_runtime::stamp(pending_commit,pending_contract_battle)&&local_warpoints::stamp(pending_commit)&&
           local_match_store::write(local_faction::root(),pending_commit)){state=pending_commit;save_failed=false;local_history_journal::pvp(state,local_history_journal::now());offline_world_runtime::sync(state);}
        else {playing=false;pending_result=false;local_duel::active=ai_lab::active=false;local_bot_runtime::reset(false);
            return dialog("对战记录未保存","本局结果未能写入存储。请保留日志；重启将按中断记录。",0);}
    }
    // Reconcile the real settled run before another match can start. The
    // contract journal deduplicates by run; no counter is inferred from totals.
    if(!local_contracts::sync_state()||!local_warpoints::sync()||!apply_local_progress())return false;
    local_history_view::clear();
    // Reuse the original result page with zero career XP/rewards and no objectives.
    auto previous=local_campaign_results::latest;
    auto& view=local_campaign_results::latest;view={};view.pending=true;
    view.win=state.outcome==local_match_store::Victory;view.run=state.settled;view.chapter=-1001;
    char title[160];snprintf(title,sizeof(title),"本地 AI %s · map%d · %s AI · %s（已保存）",mode_name(state.mode),state.map,faction_name(state.opponent),outcome_name(state.outcome));
    bool ok=local_campaign_results::publish(true,title);
    local_campaign_results::latest=previous;
    if(!ok)return false;
    local_coop::presenting=false;
    pending_result=false;playing=false;presenting=true;local_duel::active=ai_lab::active=false;
    local_bot_runtime::reset(false);
    LOG("Local AI result page shown run=%llu campaign_writes=0",(unsigned long long)run);
    return true;
}
static void frame() {
    if(!playing)return;
    if(local_duel::stopped&&!local_duel::finish_requested&&!pending_result&&!error_exit_requested) {
        runtime_failed=error_exit_requested=true;
        void* provider=find_class("com.geargames.aow.scenes.battle.gui.globalDialogWindow","DialogBattleFinishProvider");
        const void* exit=provider?class_method(provider,"SendSideSurrender",0):nullptr;void* ex=nullptr;
        if(exit)runtime_invoke(exit,nullptr,nullptr,&ex);
        LOG("Local AI runtime failed; original exit command requested=%d",exit&&!ex);
    }
    // Release the tutorial input masks in this non-tutorial session.
    void* screen=static_object("com.geargames.aow.scenes.battle","ClientBattleContext","m_screen");
    static uint64_t camera_run;
    if(camera_run!=run&&local_duel::seeded) {
        void* own=static_object("com.geargames.aow.scenes.battle","ClientBattleContext","m_ownSide");
        void* buildings=own?*(void**)((char*)own+0xe0):nullptr;
        void* hq=buildings?list_item(buildings,0):nullptr;
        void* camera=screen?*(void**)((char*)screen+0xe0):nullptr;void* ex=nullptr;
        void* position=hq?invoke_named(hq,"get_TargetPosition",0,nullptr,&ex):nullptr;
        void* rotation=camera?invoke_named(camera,"get_CurrentRotationY",0,nullptr,&ex):nullptr;
        void* distance=camera?invoke_named(camera,"get_DistanceMax",0,nullptr,&ex):nullptr;
        if(position&&rotation&&distance&&!ex&&*(float*)((char*)distance+16)>0) {
            bool overscroll=false;void* a[]={(char*)position+16,(char*)rotation+16,(char*)distance+16,&overscroll};
            invoke_named(camera,"TranslateTo",4,a,&ex);
            if(!ex){camera_run=run;LOG("Local AI opening camera centered original_distance_max=1 run=%llu",(unsigned long long)run);}
        }
    }
    void* dispatcher=screen?*(void**)((char*)screen+0x1d0):nullptr;
    void* gui=dispatcher?*(void**)((char*)dispatcher+0x60):nullptr;
    void* blocker=gui?*(void**)((char*)gui+0x398):nullptr;
    void* ex=nullptr;
    void* is_active=blocker?invoke_named(blocker,"get_IsActive",0,nullptr,&ex):nullptr;
    if(is_active&&!ex&&*((uint8_t*)is_active+16))invoke_named(blocker,"Deactivate",0,nullptr,&ex);
    void* controller=static_object("com.geargames.aow.scenes.battle","ClientBattleContext","m_tutorialController");
    void* darken=controller?*(void**)((char*)controller+0x48):nullptr;
    void* colliders=darken?*(void**)((char*)darken+0x38):nullptr;
    bool no=false;void* args[]={&no};if(colliders)invoke_named(colliders,"SetActive",1,args,&ex);
}
static void (*tile_state_original)(void*,int,int,void*);
static void tile_state(void* self,int state,int buttons,void* method) {
    void* presenter=self?*(void**)((char*)self+0x88):nullptr;
    if(offline_network&&tile_unlocked(presenter))tile_state_original(self,2,0,method);
    else tile_state_original(self,state,buttons,method);
    if(offline_network&&self){void* paid=*(void**)((char*)self+0x140),*ex=nullptr;bool no=false;void* a[]={&no};if(paid)invoke_named(paid,"SetActive",1,a,&ex);}
}
static void (*settings_original)(void*,void*);
static void original_settings(void* self,void* method){
    settings_original(self,method);
    if(!offline_network)return;
    void* ex=nullptr;bool no=false;void* args[]={&no};
    // Keep the original settings panel and disabled controls in place. These
    // optional online rules are not implemented by the elimination adapter.
    for(int offset:{0x118,0x120,0x128,0x130}){
        void* checkbox=*(void**)((char*)self+offset);if(!checkbox)continue;
        invoke_named(checkbox,"ChangeValue",1,args,&ex);
        invoke_named(checkbox,"MakeEnabled",1,args,&ex);
    }
    void* hint=*(void**)((char*)self+0x138);
    void* text[]={string_new("当前本地对战使用歼灭规则。其他规则暂不可用。")};
    if(hint)invoke_named(hint,"set_text",1,text,&ex);
}
static void disabled_starter(void*,void*){} // Original paid-only action stays inert offline.
static bool original_team_tile(void* self){
    void* info=self?*(void**)((char*)self+0x98):nullptr;
    return info&&(*(int16_t*)((char*)info+0x18)==3||*(int16_t*)((char*)info+0x18)==4);
}
static void (*team_click_original)(void*,void*),(*team_available_original)(void*,void*);
static int (*team_state_original)(void*,int,void*);
static int team_state(void* self,int matching,void* method){
    int state=team_state_original(self,matching,method);
    // Every original state callback must see local availability, including
    // asynchronous schedule updates. Do not race them with a later UI override.
    if(offline_network&&original_team_tile(self)){
        void* presenter=*(void**)((char*)self+0x88),*ex=nullptr;
        void* opened=presenter?invoke_named(presenter,"IsTileAvailableByRank",0,nullptr,&ex):nullptr;
        if(opened&&!ex&&*((bool*)opened+16))return 1;
    }return state;
}
static void team_available(void* self,void* method){
    team_available_original(self,method);
    if(!offline_network||!original_team_tile(self))return;
    void* ex=nullptr;bool no=false;void* args[]={&no};
    void* presenter=*(void**)((char*)self+0x88);void* opened=presenter?invoke_named(presenter,"IsTileAvailableByRank",0,nullptr,&ex):nullptr;
    if(!opened||ex||!*((bool*)opened+16)){ex=nullptr;invoke_named(self,"OnSetLockTileState",0,nullptr,&ex);return;}
    // Native local play has no scheduled matching wait. Keep the original
    // card/buttons and suppress only their inapplicable countdown widgets.
    for(int offset:{0xb8,0xd0}){
        void* component=*(void**)((char*)self+offset);
        void* go=component?invoke_named(component,"get_gameObject",0,nullptr,&ex):nullptr;
        if(go&&!ex)invoke_named(go,"SetActive",1,args,&ex);
    }
    static void* observed[2]={};int index=!strcmp(class_name(object_class(self)),"UGUIBattle3X3Tile");
    if(observed[index]!=self){observed[index]=self;LOG("Local original team tile available class=%s error=%d",class_name(object_class(self)),ex!=nullptr);}
}
static void team_click(void* self,void* method){
    const char* name=self?class_name(object_class(self)):"";
    int mode=!strcmp(name,"Battle2X2TilePresenter")?2:(!strcmp(name,"Battle3X3TilePresenter")?4:0);
    LOG("Local original team click presenter=%s mode=%d",name,mode);
    if(!offline_network||!mode){team_click_original(self,method);return;}
    void* rank_ex=nullptr;void* rank=invoke_named(self,"IsTileAvailableByRank",0,nullptr,&rank_ex);
    if(!rank||rank_ex||!*((bool*)rank+16)){rank_ex=nullptr;invoke_named(self,"OnLockClick",0,nullptr,&rank_ex);return;}
    if(playing||local_coop::playing)return;
    if(!restore()){dialog("未能开始对战","对战记录不可用，已保留原存档。",0);return;}
    select_defaults();
    if(!start_local_match(selected,selected_opponent,mode,true))dialog("未能开始对战","初始化或记录保存失败。当前进度已保留。",0);
}
static void* (*prices_original)(void*,void*);
static void* prices_source;
static void* prices_copy;
static uintptr_t prices_root;
static void* local_prices(void* self,void* method){
    void* source=prices_original(self,method);if(!offline_network||!source)return source;
    if(source==prices_source&&prices_root)return prices_copy;
    local_research_runtime::Roots roots;local_faction::Roots clone_roots;void* ex=nullptr;
    void* copy=local_faction::clone(source,clone_roots);if(!copy)return source;
    for(int offset:{0x30,0x38,0x40}){
        void* old=*(void**)((char*)source+offset);if(!old)return source;
        void* dict=object_new(object_class(old));if(!roots.keep(dict))return source;
        invoke_named(dict,".ctor",0,nullptr,&ex);if(ex)return source;
        void* keys=invoke_named(old,"get_Keys",0,nullptr,&ex);if(!roots.keep(keys)||ex)return source;
        void* n=invoke_named(old,"get_Count",0,nullptr,&ex);int count=n&&!ex?*(int*)((char*)n+16):-1;
        if(count<0||count>64)return source;
        void* ids=array_new(find_class("System","Int32"),count);if(!roots.keep(ids))return source;
        int zero=0;void* args[]={ids,&zero};invoke_named(keys,"CopyTo",2,args,&ex);if(ex)return source;
        for(int i=0;i<count;++i){
            local_research_runtime::Roots item_roots;
            int key=*((int*)ids+8+i);void* value=nullptr;
            if(offset==0x40){value=string_new("");if(!item_roots.keep(value))return source;}
            else{
                void* type=find_class("com.geargames.aow.entities.tournament.tournamentNew","RewardItemSet");
                value=object_new(type);if(!item_roots.keep(value))return source;
                invoke_named(value,".ctor",0,nullptr,&ex);if(ex)return source;
                void* list=local_campaign::field_list(type,"items",item_roots);if(!list)return source;
                // The original price/reward view indexes Items[0]. Preserve
                // its real item types/icons/order, with private local amounts;
                // an empty set aborts original hall initialization.
                void* old_set=local_dictionary_item(old,key);
                void* old_items=old_set?*(void**)((char*)old_set+0x10):nullptr;
                int size=old_items?*(int*)((char*)old_items+0x18):0;
                if(size<1||size>32)return source;
                for(int j=0;j<size;++j){
                    local_faction::Roots value_roots;
                    void* item=local_faction::clone(list_item(old_items,j),value_roots);if(!item)return source;
                    bool credits=offset==0x30&&*(int8_t*)((char*)item+0x10)==0&&*(int*)((char*)item+0x14)==0;
                    // Publish the BASE; the original card37 view can show its
                    // skill bonus. Never pre-boost and then boost the label twice.
                    *(int*)((char*)item+0x40)=credits?formal_credit_rewards::v7_base:0;*(int*)((char*)item+0x48)=0;*((bool*)item+0x44)=false;
                    if(!local_campaign::append(list,item))return source;
                }
                field_set(value,class_field_from_name(type,"items"),list);
            }
            void* add[]={&key,value};invoke_named(dict,"Add",2,add,&ex);if(ex)return source;
        }
        if(!local_bot_runtime::put(copy,offset,dict))return source;
    }
    int zero=0;void* a[]={&zero};invoke_named(copy,"set_FirstWinBonus",1,a,&ex);
    invoke_named(copy,"set_FirstWinBonusPossible",1,a,&ex);if(ex)return source;
    uintptr_t root=gchandle_new(copy,false);if(!root)return source;
    if(prices_root)gchandle_free(prices_root);prices_root=root;prices_source=source;prices_copy=copy;
    LOG("Local original PvP prices zero cost, approved fixed base credits=%d, first_win=0, private model; career writes=0",formal_credit_rewards::v7_base);return copy;
}
static void (*stats_original)(void*,void*,void*);
static void decorate_stats(void* self,void* points) {
    void* desc=nullptr;
    // The message fields are checked by name instead of relying on layout.
    void* f=points?class_field_from_name(object_class(points),"description"):nullptr;
    if(f)desc=*(void**)((char*)points+field_offset(f));
    if(!desc)return;
    int presentation_id=*(int*)((char*)desc+0x10);
    if(presentation_id!=-1001&&presentation_id!=-1002)return;
    bool cooperative=presentation_id==-1002;
    bool historical=local_history_view::active&&local_history_view::selected.faction==uint32_t(local_faction::active)&&
        local_history_view::selected.kind==uint32_t(cooperative);
    const auto& record=historical?local_history_view::state:(cooperative?local_coop::state:state);
    const auto& coop_record=local_history_view::coop;
    void* ex=nullptr;void* view=*(void**)((char*)self+0x20);
    char title[128];snprintf(title,sizeof(title),"%s · %s",cooperative?"本地合作 PVE":"本地对战",outcome_name(record.outcome));
    for(int offset:{0x48,0x50}) {
        void* label=view?*(void**)((char*)view+offset):nullptr;void* a[]={string_new(title)};
        if(label)invoke_named(label,"set_text",1,a,&ex);
    }
    void* stars=*(void**)((char*)self+0x28);
    void* transform=stars?invoke_named(stars,"get_transform",0,nullptr,&ex):nullptr;
    float zero[3]={};void* scale_args[]={zero};
    if(transform)invoke_named(transform,"set_localScale",1,scale_args,&ex);
    void* controller=*(void**)((char*)self+0x38);
    void* empty=controller?*(void**)((char*)controller+0x28):nullptr;
    bool yes=true;void* labels_args[]={type_object(class_type(find_class("","UILabel"))),&yes};
    void* labels=empty?invoke_named(empty,"GetComponentsInChildren",2,labels_args,&ex):nullptr;
    char detail[1024];
    // The cooperative runtime is defined later; the normal fields are its
    // existing public presentation values. Archived fields come from the wire.
    uint32_t pumpkins=historical?coop_record.pumpkins:local_coop::last_pumpkins;
    uint32_t award=historical?coop_record.award:local_coop::last_award;
    uint64_t points_total=historical?coop_record.points:local_coop::battle_points;
    char goal[64];if(pumpkins==UINT32_MAX)snprintf(goal,sizeof(goal),"旧记录未记南瓜");else snprintf(goal,sizeof(goal),"南瓜%u",pumpkins);
    if(cooperative)snprintf(detail,sizeof(detail),"合作第%llu局 · %s · 战斗点+%u · 累计%llu\n%u胜%u负；存活建筑 %d:%d，部队 %d:%d（本队:敌队）",
        (unsigned long long)record.settled,goal,award,(unsigned long long)points_total,
        record.wins,record.losses,record.buildings[0],record.buildings[1],record.units[0],record.units[1]);
    else {char reward[160],strength[192];offline_world_runtime::result_text(local_faction::active,record,strength,sizeof(strength));
        if(local_match_store::formal(record)&&historical)snprintf(reward,sizeof(reward),"本局信用+%d；战争点+%d；军衔星+%d · 历史记录",local_match_store::victory_credits(record),record.reserved,local_match_store::career_stars(record));
        else if(local_match_store::formal(record))snprintf(reward,sizeof(reward),"信用+%d；战争点+%d · 余额%d；军衔星+%d · 累计%d",local_match_store::victory_credits(record),record.reserved,progress.quest_warpoints,local_match_store::career_stars(record),progress.stars);
        else snprintf(reward,sizeof(reward),"训练/旧记录 · 无战争点");
        snprintf(detail,sizeof(detail),"第%llu局 · map%d · %s · %s AI · %s · 已保存\n%s\n本方/敌方存活建筑 %d : %d    部队 %d : %d\n本阵营累计 %u胜 / %u负；%s",
        (unsigned long long)record.settled,record.map,mode_name(record.mode),faction_name(record.opponent),outcome_name(record.outcome),strength,record.buildings[0],record.buildings[1],record.units[0],record.units[1],record.wins,record.losses,reward);}
    void* localization_args[]={type_object(class_type(find_class("","UILocalize"))),&yes};
    void* localizers=empty?invoke_named(empty,"GetComponentsInChildren",2,localization_args,&ex):nullptr;
    for(size_t i=0;localizers&&i<array_length(localizers);++i) {
        void* component=((void**)((char*)localizers+32))[i];bool no=false;void* a[]={&no};
        if(component)invoke_named(component,"set_enabled",1,a,&ex);
    }
    for(size_t i=0;labels&&i<array_length(labels);++i) {
        void* label=((void**)((char*)labels+32))[i];void* a[]={string_new(detail)};
        if(label)invoke_named(label,"set_text",1,a,&ex);
    }
    LOG("Local AI result details adapted=%d labels=%zu",!ex,labels?array_length(labels):0);
}
static void stats(void* self,void* points,void* method) {
    stats_original(self,points,method);decorate_stats(self,points);
}
static bool (*result_animation_original)(void*,void*);
static bool result_animation(void* self,void* method) {
    bool more=result_animation_original(self,method);
    // The original animation enables/localizes this row after Initiate().
    // Decorate when it finishes, keeping the original animation and callbacks.
    if(!more) {
        void* owner=self?*(void**)((char*)self+0x20):nullptr;
        void* points=owner?*(void**)((char*)owner+0x88):nullptr;
        if(owner&&points)decorate_stats(owner,points);
    }
    return more;
}
static void install() {
    const uint32_t surrender_sig[]={0xf81e0ffe,0xa9014ff4,0x9000d354,0xd0009d93};
    surrender_send_original=(decltype(surrender_send_original))install_hook(0x834d81c,surrender_sig,(void*)surrender_send);
    const uint32_t entry_sig[]={0xf81c0ffe,0xa9015ff8,0xa90257f6,0xa9034ff4};
    const uint32_t available_sig[]={0xf81d0ffe,0xa90157f6,0xa9024ff4,0xd000ecb4};
    const uint32_t button_sig[]={0xd10543ff,0xa9127bfd,0xa91357f6,0xa9144ff4};
    // Entry signature is verified independently; install_hook fails closed.
    bool a=install_hook(0x801f3c4,entry_sig,(void*)entry)!=nullptr;
    bool b=install_hook(0x801d78c,available_sig,(void*)available)!=nullptr;
    button_original=(decltype(button_original))install_hook(0x7dd2b34,button_sig,(void*)button);
    const uint32_t tile_sig[]={0xa9be57fe,0xa9014ff4,0xaa0003f3,0xaa1f03e0};
    tile_state_original=(decltype(tile_state_original))install_hook(0x7e07c00,tile_sig,(void*)tile_state);
    const uint32_t battle_click_sig[]={0xf81e0ffe,0xa9014ff4,0xf000fd93,0xaa0003f4};
    a=install_hook(0x7dfe810,battle_click_sig,(void*)entry)!=nullptr&&a;
    const uint32_t locked_click_sig[]={0xf81e0ffe,0xa9014ff4,0xf000fd93,0xaa0003f4};
    a=install_hook(0x7dfed7c,locked_click_sig,(void*)entry)!=nullptr&&a;
    const uint32_t not_available_click_sig[]={0xf81e0ffe,0xa9014ff4,0xf000fd94,0xaa0003f3};
    a=install_hook(0x7dfec54,not_available_click_sig,(void*)entry)!=nullptr&&a;
    const uint32_t stats_sig[]={0xa9be57fe,0xa9014ff4,0xaa0103f4,0xaa0003f3};
    stats_original=(decltype(stats_original))install_hook(0x7aa86f4,stats_sig,(void*)stats);
    const uint32_t animation_sig[]={0xf81d0ffe,0xa90157f6,0xa9024ff4,0xf0011834};
    result_animation_original=(decltype(result_animation_original))install_hook(0x7aa8fd0,animation_sig,(void*)result_animation);
    const uint32_t choice_sig[]={0xf81e0ffe,0xa9014ff4,0xb4000201,0xf9401428};
    choice_original=(decltype(choice_original))install_hook(0x7dd2a5c,choice_sig,(void*)choice);
    const uint32_t gear_sig[]={0xf81d0ffe,0xa90157f6,0xa9024ff4,0xf000fd54};
    settings_original=(decltype(settings_original))install_hook(0x7e06de0,gear_sig,(void*)original_settings);
    const uint32_t team_click_sig[]={0xf81d0ffe,0xa90157f6,0xa9024ff4,0xb000fdb4};
    team_click_original=(decltype(team_click_original))install_hook(0x7dfc134,team_click_sig,(void*)team_click);
    const uint32_t team_available_sig[]={0xf81d0ffe,0xa90157f6,0xa9024ff4,0xb000fd74};
    team_available_original=(decltype(team_available_original))install_hook(0x7e042c8,team_available_sig,(void*)team_available);
    const uint32_t team_state_sig[]={0xa9be57fe,0xa9014ff4,0xb000fd75,0x2a0103f4};
    team_state_original=(decltype(team_state_original))install_hook(0x7e04724,team_state_sig,(void*)team_state);
    // Saved results now live in the original profile battle list. Leave the
    // original PvP information button and its original purpose intact.
    const uint32_t starter_sig[]={0xa9be57fe,0xa9014ff4,0xd000fd94,0xaa0003f3};
    bool starter_ok=install_hook(0x7dff0f8,starter_sig,(void*)disabled_starter)!=nullptr;
    const uint32_t prices_sig[]={0xf9400c00,0xb40000a0,0xf9400008,0xf941e101};
    prices_original=(decltype(prices_original))install_hook(0x7d7fa4c,prices_sig,(void*)local_prices);
    installed=a&&b&&button_original&&tile_state_original&&stats_original&&result_animation_original&&choice_original&&settings_original&&team_click_original&&team_state_original&&team_available_original&&starter_ok&&prices_original&&surrender_send_original;
    LOG("Local AI player entry installed=%d entry=%d availability=%d dialog=%d",installed,a,b,button_original!=nullptr);
}
}
