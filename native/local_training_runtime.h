// Local service adapter for the original training lobby and its pooled controls.
#pragma once
namespace local_training {
using Roots=local_research_runtime::Roots;
static constexpr int lobby_id=-31001;
static void *model,*maps,*formats,*visible_formats;
static uintptr_t map_root,format_root,model_root,visible_format_root;
static bool installed;
static offline_world::Snapshot preview_identities;
static void ref(void* o,const char* f,void* v){field_set(o,class_field_from_name(object_class(o),f),v);}
static void* get(void* o,int offset){return o?*(void**)((char*)o+offset):nullptr;}
static bool exception(void* ex,const char* phase){return local_campaign::failed_exception(ex,phase);}
static void* create(const char* name,Roots& roots){
    void* o=object_new(find_class("com.geargames.aow.entities.lobby",name));void* ex=nullptr;
    if(!roots.keep(o))return nullptr;invoke_named(o,".ctor",0,nullptr,&ex);return exception(ex,name)?nullptr:o;
}
static void* current(void* m){void* ex=nullptr;return m?invoke_named(m,"get_CurrentLobby",0,nullptr,&ex):nullptr;}
static bool ours(void* m){void* l=current(m);return offline_network&&l&&*(int*)((char*)l+0x54)==lobby_id;}
static bool config(void* self){return ours(get(self,0x10));}
static void root(void* value,uintptr_t& handle){auto next=gchandle_new(value,true);if(handle)gchandle_free(handle);handle=next;}
static bool publish(void* lobby){
    Roots roots;void* ex=nullptr;
    void* message=object_new(find_class("com.geargames.aow.messages.lobby","LobbyCreateNotification"));
    if(!roots.keep(message))return false;void* a[]={lobby};invoke_named(message,".ctor",1,a,&ex);
    void* args[]={message};if(!ex)invoke_named(model,"OnMessageLobbyCreateNotificationReceived",1,args,&ex);
    return !exception(ex,"training-lobby-notification");
}
static void* mission(int chapter){
    const auto* route=local_scenario(chapter);if(!route||!maps)return nullptr;
    for(int i=0;i<*(int*)((char*)maps+0x18);++i){void* m=list_item(maps,i);if(m&&*(int*)((char*)m+0x10)==route->recorded)return m;}
    return nullptr;
}
static int mode(void* format){return format?control_point_policy::lobby_mode(*(int*)((char*)format+0x10),*(int*)((char*)format+0x48)):0;}
static bool filter_formats(){
    const auto* route=local_match_maps::find(local_match::selected);if(!route||!formats)return false;
    Roots roots;void* list=local_research_runtime::new_list(formats,roots);if(!list)return false;
    for(int i=0;i<*(int*)((char*)formats+0x18);++i){void* f=list_item(formats,i);int m=mode(f);
        if(m&&ai_team_policy::seats(m)<=route->max_seats&&!local_campaign::append(list,f))return false;}
    visible_formats=list;root(list,visible_format_root);return true;
}
static bool slots(void* lobby,Roots& roots){
    void* own=get(model,0x28),*format=get(lobby,0x30);if(!own||!mode(format))return false;
    void* ls=local_campaign::field_list(object_class(lobby),"lobbySlots",roots);
    void* ps=local_campaign::field_list(object_class(lobby),"lobbyProfiles",roots);
    if(!ls||!ps)return false;int match_mode=mode(format),count=ai_team_policy::seats(match_mode);
    local_faction::Roots clones;
    auto preview_match=local_match::state;preview_match.mode=match_mode;preview_match.opponent=local_match::selected_opponent;
    const auto* route=local_match_maps::find(local_match::selected);if(!route)return false;preview_match.map=route->map;
    offline_world::Snapshot identities;if(!offline_world_runtime::preview(preview_match,identities))return false;
    for(int i=0;i<count;++i){
        bool player=i==0;int team=local_battle_format::team(match_mode,i+1)+1;int faction=team==1?local_faction::active:local_match::selected_opponent;
        local_faction::Roots actor_roots;
        void* profile=player?own:offline_world_runtime::profile(own,identities.actors[i],actor_roots);
        void* p=create("LobbyProfile",roots),*s=create("LobbySlot",roots);if(!profile||!p||!s)return false;
        if(!player){
            ref(profile,"fraction",local_faction::fraction(faction));
        }
        ref(p,"profile",profile);*(int*)((char*)p+0x18)=3;*(int*)((char*)p+0x28)=1;ref(p,"hintUnavailable",string_new(""));
        *(int8_t*)((char*)s+0x10)=i+1;*(int8_t*)((char*)s+0x20)=team;ref(s,"profile",p);
        if(!local_campaign::append(ls,s)||!local_campaign::append(ps,p))return false;
    }
    ref(lobby,"lobbySlots",ls);ref(lobby,"lobbyProfiles",ps);preview_identities=identities;return true;
}
static void (*create_original)(void*,int,void*);
static bool prepare_lobby(void* self,int id,void* method){

    if(!installed||local_match::playing||local_coop::playing||!local_match::restore())return false;
    local_match::select_defaults();Roots roots;local_faction::Roots clones;
    void* configuration=get(self,0x38),*source=get(configuration,0x10),*ex=nullptr;
    if(!source)return false;
    void* fs=local_campaign::field_list(object_class(configuration),"lobbyFormats",roots);
    void* ms=local_campaign::field_list(find_class("com.geargames.aow.entities.lobby","LobbyFormat"),"missionDescriptions",roots);
    if(!fs||!ms)return false;
    for(const auto& r:local_match_maps::routes)if(r.faction==local_faction::active){
        const auto* scenario=local_scenario(r.chapter);void* m=scenario?local_faction::clone(local_mission_description(scenario->recorded),clones):nullptr;
        if(!m)return false;char label[96];snprintf(label,sizeof(label),"%s%s",r.label,r.max_seats<6?"（至2v2）":"");ref(m,"name",string_new(label));if(!local_campaign::append(ms,m))return false;
    }
    void* selected_format=nullptr;
    for(int i=0;i<*(int*)((char*)source+0x18);++i){
        void* f=list_item(source,i);int m=mode(f);
        LOG("Local training format id=%d type=%d teams=%d",f?*(int*)((char*)f+0x10):0,f?*(int*)((char*)f+0x50):0,f?*(int*)((char*)f+0x48):0);
        int format_id=f?*(int*)((char*)f+0x10):0;
        if(!m||*(int*)((char*)f+0x50)!=1)continue;
        f=local_faction::clone(f,clones);if(!f)return false;
        // Scoped display configuration only. The career's actual rank is untouched.
        *(int*)((char*)f+0x4c)=1;*(int*)((char*)f+0x54)=3;
        ref(f,"missionDescriptions",ms);
        const char* rule=local_battle_format::free_for_all(m)?"歼灭所有对手。":m==5?"占领300分或歼灭获胜。":m==6?"占点获增益，歼灭获胜。":"歼灭获胜。";
        char description[384];snprintf(description,sizeof(description),"%s点敌头像切阵营%s；训练不评分。",rule,m==2||m==4?"，友军头像看档案":"");
        ref(f,"mainDesc",string_new(description));ref(f,"additionalDesc",string_new(description));
        ref(f,"startBattleHint",string_new("开始本地 AI 对战"));
        if(!local_campaign::append(fs,f))return false;
        const auto* route=local_match_maps::find(local_match::selected);
        if(route&&ai_team_policy::seats(m)<=route->max_seats&&(!selected_format||m==local_match::selected_mode))selected_format=f;
    }
    if(!selected_format){LOG("Local training refused missing original training format");return false;}
    void* l=create("Lobby",roots),*state=create("BattleButtonState",roots);if(!l||!state)return false;
    *(int*)((char*)l+0x54)=lobby_id;*(bool*)((char*)l+0x50)=true;*(bool*)((char*)l+0x51)=false;
    ref(l,"creator",get(self,0x28));ref(l,"lobbyFormat",selected_format);*(int*)((char*)state+0x10)=1;
    ref(state,"hint",string_new("开始本地 AI 对战"));ref(l,"battleButtonState",state);
    void* posts=local_campaign::field_list(object_class(l),"lobbyChatPosts",roots);if(!posts)return false;ref(l,"lobbyChatPosts",posts);
    model=self;root(model,model_root);maps=ms;root(ms,map_root);formats=fs;root(fs,format_root);
    if(!filter_formats())return false;
    void* chosen=mission(local_match::selected);if(!chosen||!slots(l,roots))return false;ref(l,"missionDescription",chosen);
    local_match::selected_mode=mode(selected_format);
    bool ok=publish(l);
    LOG("Local training created=%d map=%d opponent=%d mode=%d career_writes=0",ok,local_match_maps::find(local_match::selected)->map,local_match::selected_opponent,local_match::selected_mode);
    return ok;
}
static void create_lobby(void* self,int id,void* method){
    if(!offline_network){create_original(self,id,method);return;}
    if(!prepare_lobby(self,id,method))local_match::dialog("训练暂不可用","本地配置或记录无法读取。原进度已保留，请保留日志后重试。",0);
}
static void* (*missions_original)(void*,void*),*(*formats_original)(void*,void*);
static void* available_missions(void* self,void* m){return config(self)?maps:missions_original(self,m);}
static void* available_formats(void* self,void* m){return config(self)?visible_formats:formats_original(self,m);}
static int (*mission_type_original)(void*,void*),(*format_type_original)(void*,void*);
static int mission_type(void* self,void* m){return config(self)?0:mission_type_original(self,m);}
static int format_type(void* self,void* m){return config(self)?0:format_type_original(self,m);}
static void (*mission_original)(void*,int,void*),(*format_original)(void*,int,void*);
static void change_mission(void* self,int id,void* method){
    if(!config(self)){mission_original(self,id,method);return;}
    for(const auto& r:local_match_maps::routes){const auto* s=local_scenario(r.chapter);if(r.faction!=local_faction::active||!s||s->recorded!=id)continue;
        void* l=current(model);local_match::selected=r.chapter;ref(l,"missionDescription",mission(r.chapter));
        if(!filter_formats())return;
        if(ai_team_policy::seats(local_match::selected_mode)>r.max_seats){
            Roots roots;int desired=r.max_seats>=4?2:1;void* selected=nullptr;
            for(int i=0;i<*(int*)((char*)visible_formats+0x18);++i){void* f=list_item(visible_formats,i);if(mode(f)==desired)selected=f;}
            if(!selected)return;ref(l,"lobbyFormat",selected);local_match::selected_mode=desired;
            if(!slots(l,roots)||!publish(l))return;
        }
        void* ex=nullptr;int change=0;void* a[]={mission(r.chapter),&change};void* callback=get(self,0x58);if(callback)invoke_named(callback,"Invoke",2,a,&ex);
        LOG("Local training map selected=%d notification=%d",r.map,!exception(ex,"training-map-change"));return;
    }
    LOG("Local training rejected unsupported mission=%d",id);
}
static void change_format(void* self,int id,void* method){
    if(!config(self)){format_original(self,id,method);return;}
    for(int i=0;i<*(int*)((char*)formats+0x18);++i){void* f=list_item(formats,i);if(*(int*)((char*)f+0x10)!=id)continue;
        const auto* route=local_match_maps::find(local_match::selected);if(!route||ai_team_policy::seats(mode(f))>route->max_seats)return;
        Roots roots;void* l=current(model);ref(l,"lobbyFormat",f);local_match::selected_mode=mode(f);
        if(slots(l,roots))LOG("Local training format selected=%d updated=%d",local_match::selected_mode,publish(l));return;
    }
}
static void (*profile_original)(void*,void*,void*);
static void profile_click(void* self,void* profile,void* method){
    if(!ours(get(self,0x48))){profile_original(self,profile,method);return;}
    int id=profile?*(int*)((char*)profile+0x10):0;
    int seat=-1;for(unsigned i=1;i<preview_identities.actor_count;++i)if(preview_identities.actors[i].id==id)seat=i;
    if(seat<1){return;}
    if(!local_battle_format::free_for_all(local_match::selected_mode)&&seat%2!=1){profile_original(self,profile,method);return;}
    local_match::selected_opponent=3-local_match::selected_opponent;Roots roots;void* l=current(model);
    if(slots(l,roots))LOG("Local training opponent selected=%d updated=%d",local_match::selected_opponent,publish(l));
}
static void (*start_original)(void*,void*);
static void start(void* self,void* method){
    if(!ours(get(self,0x10))){start_original(self,method);return;}
    bool ok=start_local_match(local_match::selected,local_match::selected_opponent,local_match::selected_mode,false);
    LOG("Local training original battle button start=%d map=%d opponent=%d mode=%d",ok,local_match_maps::find(local_match::selected)->map,local_match::selected_opponent,local_match::selected_mode);
    if(!ok)local_match::dialog("未能开始训练","初始化或记录保存失败，当前进度已保留。",0);
}
static void (*leave_original)(void*,void*);
static void leave(void* self,void* method){
    if(!ours(self)){leave_original(self,method);return;}
    void* ex=nullptr;void* value=nullptr;void* a[]={value};invoke_named(get(self,0x48),"set_Value",1,a,&ex);
    for(int offset:{0x68,0x70})if(!ex&&get(self,offset))invoke_named(get(self,offset),"Invoke",0,nullptr,&ex);
    LOG("Local training leave=%d",!exception(ex,"training-leave"));
}
static bool (*rank_original)(void*,void*);
static bool rank(void* self,void* method){return offline_network&&self&&!strcmp(class_name(object_class(self)),"TrainingTilePresenter")?true:rank_original(self,method);}
static bool (*unlock_original)(void*,bool,bool,void*);
static bool unlock(void* self,bool by_rank,bool by_tile,void* method){
    if(offline_network&&self&&!strcmp(class_name(object_class(self)),"TrainingTilePresenter"))return true;
    return unlock_original(self,by_rank,by_tile,method);
}
static void (*exclude_original)(void*,int,void*),(*move_original)(void*,int,int,void*);
static void exclude(void* self,int id,void* method){if(!ours(self))exclude_original(self,id,method);}
static void move(void* self,int id,int slot,void* method){if(!ours(self))move_original(self,id,slot,method);}
static void (*send_original)(void*,void*,void*);
static void send(void* self,void* text,void* method){if(!ours(get(self,0x10)))send_original(self,text,method);}
static bool (*spectator_original)(void*,void*);
static bool spectator(void* self,void* method){return ours(self)?false:spectator_original(self,method);}
static void (*open_original)(void*,void*);
static void open(void* self,void* method){
    open_original(self,method);if(!ours(get(self,0x48)))return;
    void* ex=nullptr;void* view=invoke_named(self,"get_SectionRootView",0,nullptr,&ex);
    if(ex||!view){exception(ex,"training-section-view");return;}
    void* tabs=get(view,0x70),*bookmarks=get(get(tabs,0x30),0x28);
    bool no=false;void* args[]={&no};
    // Keep the original online tabs and chat in place, with input disabled.
    for(int i=1;bookmarks&&i<*(int*)((char*)bookmarks+0x18);++i){
        void* tab=list_item(bookmarks,i);if(tab)invoke_named(tab,"set_enabled",1,args,&ex);
    }
    void* input=get(get(get(view,0x78),0x28),0x28);
    if(input){invoke_named(input,"set_interactable",1,args,&ex);
        void* placeholder=!ex?invoke_named(input,"get_placeholder",0,nullptr,&ex):nullptr;
        void* caption[]={string_new("聊天（离线不可用）")};if(placeholder&&!ex)invoke_named(placeholder,"set_text",1,caption,&ex);}
    LOG("Local training original online controls disabled=%d",!exception(ex,"training-online-controls"));
}
// Signatures below are checked against the pinned release before installing.
static void install(){
    const uint32_t sig0[]={0xd10203ff,0xf90023fe,0xa9055ff8,0xa90657f6};
    create_original=(decltype(create_original))install_hook(0x7d6fa5c,sig0,(void*)create_lobby);
    // get_AvailableMissions is a one-instruction alias to this implementation.
    // Hook the shared body so direct callers also retain a valid entry point.
    const uint32_t sig1[]={0xd10203ff,0xa90467fe,0xa9055ff8,0xa90657f6};
    missions_original=(decltype(missions_original))install_hook(0x7d67b94,sig1,(void*)available_missions);
    const uint32_t sig2[]={0xf81e0ffe,0xa9014ff4,0xb0010254,0xaa0003f3};
    formats_original=(decltype(formats_original))install_hook(0x7d68030,sig2,(void*)available_formats);
    const uint32_t sig3[]={0xd100c3ff,0xa90157fe,0xa9024ff4,0x90010254};
    mission_type_original=(decltype(mission_type_original))install_hook(0x7d69360,sig3,(void*)mission_type);
    const uint32_t sig4[]={0xf81d0ffe,0xa90157f6,0xa9024ff4,0xb0010254};
    format_type_original=(decltype(format_type_original))install_hook(0x7d68f14,sig4,(void*)format_type);
    const uint32_t sig5[]={0xa9bd5ffe,0xa90157f6,0xa9024ff4,0x90010254};
    mission_original=(decltype(mission_original))install_hook(0x7d69168,sig5,(void*)change_mission);
    const uint32_t sig6[]={0xa9bd5ffe,0xa90157f6,0xa9024ff4,0xb0010254};
    format_original=(decltype(format_original))install_hook(0x7d68d1c,sig6,(void*)change_format);
    const uint32_t sig7[]={0xf81d0ffe,0xa90157f6,0xa9024ff4,0xf00102d5};
    profile_original=(decltype(profile_original))install_hook(0x7d56984,sig7,(void*)profile_click);
    const uint32_t sig8[]={0xa9bd5ffe,0xa90157f6,0xa9024ff4,0xf0010253};
    start_original=(decltype(start_original))install_hook(0x7d664c8,sig8,(void*)start);
    const uint32_t sig9[]={0xa9bd5ffe,0xa90157f6,0xa9024ff4,0xd0010213};
    leave_original=(decltype(leave_original))install_hook(0x7d6fe14,sig9,(void*)leave);
    const uint32_t sig10[]={0xf81e0ffe,0xa9014ff4,0xf000fdb4,0xaa0003f3};
    rank_original=(decltype(rank_original))install_hook(0x7dfa0c8,sig10,(void*)rank);
    const uint32_t unlock_sig[]={0xf81d0ffe,0xa90157f6,0xa9024ff4,0xf000fdb6};
    unlock_original=(decltype(unlock_original))install_hook(0x7dfa930,unlock_sig,(void*)unlock);
    const uint32_t extra0[]={0xa9bd5ffe,0xa90157f6,0xa9024ff4,0xb0010214};
    exclude_original=(decltype(exclude_original))install_hook(0x7d7003c,extra0,(void*)exclude);
    const uint32_t extra1[]={0xa9bd5ffe,0xa90157f6,0xa9024ff4,0xb0010214};
    move_original=(decltype(move_original))install_hook(0x7d70240,extra1,(void*)move);
    const uint32_t extra2[]={0xa9bc67fe,0xa9015ff8,0xa90257f6,0xa9034ff4};
    send_original=(decltype(send_original))install_hook(0x7d4809c,extra2,(void*)send);
    const uint32_t extra4[]={0xf81e0ffe,0xa9014ff4,0xf0010214,0xaa0003f3};
    spectator_original=(decltype(spectator_original))install_hook(0x7d6eea0,extra4,(void*)spectator);
    const uint32_t open_sig[]={0xf81c0ffe,0xa9015ff8,0xa90257f6,0xa9034ff4};
    open_original=(decltype(open_original))install_hook(0x7d5331c,open_sig,(void*)open);
    installed=open_original&&exclude_original&&move_original&&send_original&&spectator_original&&create_original&&missions_original&&formats_original&&mission_type_original&&format_type_original&&mission_original&&format_original&&profile_original&&start_original&&leave_original&&rank_original&&unlock_original;
    LOG("Local training original lobby hooks=%d",installed);
}
}
