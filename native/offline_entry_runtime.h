// Original settings/account flow and explicit unavailable UI states.
#pragma once
#include "new_start_policy.h"
namespace offline_entry {
static bool installed=false,reset_dialog=false,tutorial_queued=false;
static void* get(void* p,int offset){return p?*(void**)((char*)p+offset):nullptr;}
static void label(void* p,const char* text){if(!p)return;void* ex=nullptr;void* args[]={string_new(text)};invoke_named(p,"set_text",1,args,&ex);if(ex)LOG("Offline label failed class=%s",class_name(object_class(p)));}
static void active(void* p,bool value,bool is_game_object=false){if(!p)return;void* ex=nullptr;void* go=is_game_object?p:invoke_named(p,"get_gameObject",0,nullptr,&ex);
    void* args[]={&value};if(go&&!ex)invoke_named(go,"SetActive",1,args,&ex);}
static void button(void* p,const char* text,bool enabled){
    if(!p)return;if(!strcmp(class_name(object_class(p)),"GUIMainActionGoodBuy"))p=get(p,0x38);if(!p)return;
    bool typed=false;for(void* c=object_class(p);c;c=class_parent(c))if(!strcmp(class_name(c),"GUIMainActionButton")){typed=true;break;}
    if(!typed){LOG("Offline button type refused class=%s",class_name(object_class(p)));return;}
    void* ex=nullptr;void* labels=get(p,0x28);for(size_t i=0;labels&&i<array_length(labels);++i)label(((void**)((char*)labels+32))[i],i?"":text);
    bool force=true;void* args[]={&force};if(!enabled)invoke_named(p,"ShowHidden",1,args,&ex);
    else {void* action=get(p,0x90);void* show[]={action,&force};invoke_named(p,"Show",2,show,&ex);}
    void* collider=get(p,0x88);void* state[]={&enabled};if(collider)invoke_named(collider,"set_enabled",1,state,&ex);
}
static void ui_audit(){
    if(access(DATA_ROOT "new-start-ui-audit.request",F_OK))return;unlink(DATA_ROOT "new-start-ui-audit.request");
    FILE* out=fopen(DATA_ROOT "new-start-ui-audit.txt","w");if(!out)return;fchmod(fileno(out),0644);
    void* cls=find_class("UnityEngine","Component"),*resources=find_class("UnityEngine","Resources"),*ex=nullptr;
    void* t=cls?type_object(class_type(cls)):nullptr;void* args[]={t};const void* method=resources?class_method(resources,"FindObjectsOfTypeAll",1):nullptr;
    void* all=method&&t?runtime_invoke(method,nullptr,args,&ex):nullptr;local_research_runtime::Roots roots;
    if(!all||ex||!roots.keep(all)||array_length(all)>200000){fprintf(out,"FAILED component inventory\n");fclose(out);return;}
    fprintf(out,"@account\t%llu\tprivate=%d\tfaction=%d\tstars=%d\n",(unsigned long long)offline_accounts::choice.active,offline_accounts::private_session,local_faction::active,progress.stars);
    for(size_t i=0;i<array_length(all);++i){void* p=((void**)((char*)all+32))[i];if(!p)continue;
        const char* name=class_name(object_class(p)),*space=class_namespace(object_class(p));
        if(space&&(!strncmp(space,"UnityEngine",11)||!strncmp(space,"TMPro",5)||!strncmp(space,"Spine",5)))continue;
        void* go=invoke_named(p,"get_gameObject",0,nullptr,&ex);void* visible=go&&!ex?invoke_named(go,"get_activeInHierarchy",0,nullptr,&ex):nullptr;
        if(ex||!visible||!*((bool*)visible+16)){ex=nullptr;continue;}char label[160]={};void* value=invoke_named(go,"get_name",0,nullptr,&ex);
        if(value&&!ex)local_research_runtime::ascii(value,label,sizeof(label));fprintf(out,"%s\t%s\n",name,label);
        if(!strcmp(name,"UGUIActionButton"))fprintf(out,"@button\t%s\tvisible=%d\n",label,*(bool*)((char*)p+0x58));
        bool tile=false;for(void* c=object_class(p);c;c=class_parent(c))if(!strcmp(class_name(c),"UGUIBaseTile")){tile=true;break;}
        if(tile){void* presenter=get(p,0x88);void* available=presenter?invoke_named(presenter,"IsTileAvailableByRank",0,nullptr,&ex):nullptr;
            fprintf(out,"@tile\t%s\tunlocked=%d\terror=%d\n",name,available&&!ex&&*((bool*)available+16),ex!=nullptr);ex=nullptr;}
    }fclose(out);LOG("Offline original UI inventory collected");
}
static bool contains(void* s,const char16_t* needle){if(!s)return false;int n=*(int*)((char*)s+16);size_t k=0;while(needle[k])++k;
    if(n<0||n>4096||k>size_t(n))return false;const char16_t* text=(const char16_t*)((char*)s+20);
    for(int i=0;i<=n-int(k);++i)if(!memcmp(text+i,needle,k*2))return true;return false;}
static void (*text_original)(void*,void*,void*);
static void text(void* self,void* value,void* method){
    if(offline_network&&value){int n=*(int*)((char*)value+16);
        if(n<=20&&contains(value,u"重置或删除账号"))value=string_new("本地生涯");
        else if(contains(value,u"您可以重置或删除您的账号"))value=string_new("[FFFFFF]新开局会创建独立生涯，从原教程开始。当前两个阵营的进度、研究和库存均完整保留。\n可在右侧本地存档中切换。每份生涯独立成长，不合并资源和奖励。[-]");
        else if(n==4&&contains(value,u"重置账号"))value=string_new("新开局（保留旧进度）");
        else if(n==3&&contains(value,u"礼品码"))value=string_new("礼品码（停用）");
    }
    text_original(self,value,method);
}
static bool idle(){return offline_network&&progress_ready&&!local_run_active&&!local_match::playing&&!local_coop::playing;}
static bool can_reload(){return static_object("com.geargames.aow.scenes.main","ClientMainContext","<NetworkConnector>k__BackingField")!=nullptr;}
static bool reload(){
    // Selection changes are never followed by writes from the previous cached
    // profile, even if the original connector refuses a reload.
    progress_ready=false;
    void* ex=nullptr;void* connector=static_object("com.geargames.aow.scenes.main","ClientMainContext","<NetworkConnector>k__BackingField");
    if(!connector||ex){LOG("Local account reload refused connector missing");return false;}
    int reason=-6;void* args[]={&reason};progress_ready=false;progress_checked=false;main_attempted=false;tutorial_queued=false;local_match::selection_owner=0;
    invoke_named(connector,"ToReloading",1,args,&ex);LOG("Local account original relogin=%d selected=%llu private=%d",!ex,(unsigned long long)offline_accounts::choice.active,offline_accounts::private_session);return !ex;
}
static void settings_present(void* self){
    if(!offline_network||!self)return;
    button(get(self,0x38),"新开局（保留旧进度）",idle());
    active(get(self,0x40),false);active(get(self,0x48),false); // Original QA reset and server deletion.
    active(get(self,0x98),false);active(get(self,0xa0),false); // Ads and online invitations.
    ui_audit();
}
static void (*settings_enable_original)(void*,void*),(*settings_localize_original)(void*,void*);
static void settings_enable(void* self,void* method){settings_enable_original(self,method);settings_present(self);}
static void settings_localize(void* self,void* method){settings_localize_original(self,method);settings_present(self);}
static void (*reset_click_original)(void*,void*);
static void reset_click(void* self,void* method){if(!offline_network){reset_click_original(self,method);return;}if(!idle())return;
    reset_dialog=true;reset_click_original(self,method);reset_dialog=false;
}
static void (*reset_result_original)(void*,bool,int,void*);
static void reset_result(void* self,bool yes,int type,void* method){
    if(!offline_network){reset_result_original(self,yes,type,method);return;}if(!yes||!idle()||!can_reload())return;
    if(!offline_accounts::create()){LOG("Local new account failed; original career preserved");return;}
    if(!reload())LOG("Local new account selected; restart required, no career replaced");
}
static bool free_reset(void*){return true;}
static void* (*code_confirm_original)(void*,void*,void*,void*);
static void* code_confirm(void* self,void* action,void* text,void* method){
    if(reset_dialog)text=string_new("开始新的独立生涯？当前进度将完整保留，可在设置的本地存档处切换。新档将从原教程开始。");
    return code_confirm_original(self,action,text,method);
}
static void account_caption(void* self){
    if(!self)return;char title[96];uint64_t target=offline_accounts::cycle_target();
    if(target==UINT64_MAX||target==offline_accounts::choice.active)snprintf(title,sizeof(title),"暂无其他存档");
    else if(!target)snprintf(title,sizeof(title),"切回原有进度");else snprintf(title,sizeof(title),"切换到存档 %llu",(unsigned long long)target);
    label(get(self,0x40),title);bool can=idle()&&target!=UINT64_MAX&&target!=offline_accounts::choice.active;
    void* ex=nullptr;void* go=invoke_named(self,"get_gameObject",0,nullptr,&ex);
    void* collider_class=find_class("UnityEngine","Collider");void* t=collider_class?type_object(class_type(collider_class)):nullptr;void* args[]={t};
    void* collider=go&&t?invoke_named(go,"GetComponent",1,args,&ex):nullptr;void* enable[]={&can};if(collider&&!ex)invoke_named(collider,"set_enabled",1,enable,&ex);
}
static void (*sign_localize_original)(void*,void*),(*sign_click_original)(void*,void*);
static void sign_localize(void* self,void* method){sign_localize_original(self,method);if(offline_network)account_caption(self);}
static void sign_click(void* self,void* method){if(!offline_network){sign_click_original(self,method);return;}if(idle()&&can_reload()&&offline_accounts::cycle())reload();}
static void (*right_original)(void*,bool,void*);
static void right(void* self,bool avatar,void* method){
    right_original(self,avatar,method);if(!offline_network||!self)return;char text[112];
    if(!offline_accounts::choice.active)snprintf(text,sizeof(text),"本地存档 · 原有进度");else snprintf(text,sizeof(text),"本地存档 · %llu",(unsigned long long)offline_accounts::choice.active);
    label(get(self,0x20),text);label(get(self,0x48),"进度保存在本机。新开局会保留当前生涯；使用上方按钮切换已保存的进度。");
    label(get(self,0x50),"");active(get(self,0x38),false,true);active(get(self,0x58),true,true);account_caption(get(self,0x60));
    void* table=get(self,0x28),*ex=nullptr;if(table)invoke_named(table,"Reposition",0,nullptr,&ex);
    ui_audit();
}
static bool unavailable(){return false;}
static void blocked(void*,void*){LOG("Offline unavailable entry refused before async request");}
static void profile_policy(void* self){button(get(self,0x60),"本地身份",false);button(get(self,0x68),"",false);}
static void* (*profile_tabs_original)(bool,bool,bool,bool,void*);
static void* profile_tabs(bool friends,bool recruits_rank,bool recruits,bool daily,void* method){
    return profile_tabs_original(offline_network?false:friends,offline_network?false:recruits_rank,offline_network?false:recruits,offline_network?false:daily,method);
}
static void (*ads_update_original)(void*,void*);
static void ads_update(void* self,void* method){ads_update_original(self,method);if(offline_network&&self){
    active(get(self,0x28),false,true);active(get(self,0x40),false,true);active(get(self,0x20),false);active(get(self,0x60),false);
}}
static void (*seller_original)(void*,void*,bool,void*,int,bool,void*);
static void seller(void* self,void* goods,bool available,void* promo,int count,bool mini,void* method){
    seller_original(self,goods,offline_network?false:available,promo,count,mini,method);if(!offline_network||!self)return;
    button(get(self,0x50),"商城购买未接入",false);button(get(self,0x58),"商城购买未接入",false);
    active(get(self,0x60),false);active(get(self,0x68),false);
    ui_audit();
}
static void (*goods_original)(void*,bool,bool,void*);
static void goods(void* self,bool scroll,bool toggle,void* method){goods_original(self,scroll,toggle,method);if(offline_network)active(get(self,0x40),false);}
static int ads_goods(void*,int index,void*){return index;}
static void* scoped_pref(void* key,local_research_runtime::Roots& roots){
    if(!offline_accounts::ready)return key;char original[192],scoped[320];
    if(!local_research_runtime::ascii(key,original,sizeof(original))||!new_start_policy::scoped_key(offline_accounts::base,offline_accounts::choice.active,offline_accounts::private_session,original,scoped,sizeof(scoped)))return key;
    void* value=string_new(scoped);if(!roots.keep(value)){LOG("Local tutorial preference scope allocation refused");return nullptr;}return value;
}
static int (*pref_get_original)(void*,int,void*);
static bool (*pref_set_original)(void*,int,void*),(*pref_has_original)(void*,void*);
static void (*pref_delete_original)(void*,void*);
static int pref_get(void* key,int fallback,void* method){local_research_runtime::Roots roots;return pref_get_original(scoped_pref(key,roots),fallback,method);}
static bool pref_set(void* key,int value,void* method){local_research_runtime::Roots roots;return pref_set_original(scoped_pref(key,roots),value,method);}
static bool pref_has(void* key,void* method){local_research_runtime::Roots roots;return pref_has_original(scoped_pref(key,roots),method);}
static void pref_delete(void* key,void* method){local_research_runtime::Roots roots;pref_delete_original(scoped_pref(key,roots),method);}
// Shared modern main-panel button boundary. Existing disabled colors and layout
// remain visible; no unsupported callback can enter a network wait.
static void* parent_component(void* self,const char* space,const char* name){
    void* c=find_class(space,name),*t=c?type_object(class_type(c)):nullptr,*ex=nullptr;
    bool include=true;void* args[]={t,&include};void* result=t?invoke_named(self,"GetComponentInParent",2,args,&ex):nullptr;
    if(ex){LOG("Offline parent button policy lookup failed class=%s",name);return nullptr;}return result;
}
static bool unsupported_button(void* self){
    if(!offline_network||!self||strcmp(class_name(object_class(self)),"UGUIActionButton"))return false;
    if(local_training::ours(local_training::model)){
        void* player=parent_component(self,"com.geargames.aow.ugui","UGUILobbyPlayerView");
        if(player&&(self==get(player,0x38)||self==get(player,0x40)))return true;
        void* filled=parent_component(self,"com.geargames.aow.ugui.lobby","UGUILobbyFilledSlotView");
        if(filled&&self==get(filled,0x38))return true;
    }
    void* ex=nullptr;void* go=invoke_named(self,"get_gameObject",0,nullptr,&ex);void* n=go&&!ex?invoke_named(go,"get_name",0,nullptr,&ex):nullptr;char name[80];
    if(ex||!local_research_runtime::ascii(n,name,sizeof(name)))return false;
    static const char* names[]={"button_chat","button_mail","button_ratings","btn_clan","item_casino","item_box","element_premium","item_daily_14","item_daily_30"};
    for(const char* item:names)if(!strcmp(item,name))return true;return false;
}
static void (*state_original)(void*,bool,void*),(*pointer_original)(void*,void*,void*);
static void state(void* self,bool visible,void* method){state_original(self,unsupported_button(self)?false:visible,method);}
static void pointer(void* self,void* event,void* method){ui_audit();if(unsupported_button(self)){LOG("Offline disabled UI entry click refused");return;}pointer_original(self,event,method);}
static void (*daily30_original)(void*,bool,void*),(*daily14_original)(void*,bool,void*);
static void daily30(void* self,bool value,void* method){daily30_original(self,offline_network?false:value,method);}
static void daily14(void* self,bool value,void* method){daily14_original(self,offline_network?false:value,method);}
// Complete replacement of the verified 64-byte setter: its original body only
// sets the first-purchase GameObject active when either bonus flag is true.
// No relocated trampoline is called (the original starts with a conditional).
static void first_purchase(void* self,bool gold,bool other,void*){active(get(self,0x28),offline_network?false:(gold||other));}
static bool promo_tab(void* self){return offline_network&&self&&*(int*)((char*)self+0x48)==3;}
static void (*settings_tab_original)(void*,int,void*),(*settings_tab_click_original)(void*,void*);
static void settings_tab(void* self,int category,void* method){
    settings_tab_original(self,category,method);if(!promo_tab(self))return;
    float alpha=.35f;void* a[]={&alpha};void* ex=nullptr;
    for(int offset:{0x20,0x28})if(void* p=get(self,offset))invoke_named(p,"set_alpha",1,a,&ex);
    void* toggle=get(self,0x50);bool no=false;void* disabled[]={&no};if(toggle)invoke_named(toggle,"set_enabled",1,disabled,&ex);
    void* c=find_class("UnityEngine","Collider"),*t=c?type_object(class_type(c)):nullptr;void* args[]={t};
    void* collider=t?invoke_named(self,"GetComponent",1,args,&ex):nullptr;if(collider&&!ex)invoke_named(collider,"set_enabled",1,disabled,&ex);
}
static void settings_tab_click(void* self,void* method){if(!promo_tab(self))settings_tab_click_original(self,method);}
static void clean_menu(void* self){
    if(!offline_network||!self)return;void* tabs=get(self,0x20);
    for(size_t i=0;tabs&&i<array_length(tabs);++i){void* tab=((void**)((char*)tabs+32))[i];if(!tab)continue;const char* name=class_name(object_class(tab));
        if(!strcmp(name,"GUIMainWindowTabChat")||!strcmp(name,"GUIMainWindowTabClan")||!strcmp(name,"GUIMainWindowTabRating")||!strcmp(name,"GUIMainCasinoTab"))active(get(tab,0x20),false);
    }ui_audit();
}
static void (*main_show_original)(void*,void*),(*main_last_original)(void*,void*,void*);
static void main_show(void* self,void* method){main_show_original(self,method);clean_menu(self);}
static void main_last(void* self,void* last,void* method){main_last_original(self,last,method);clean_menu(self);}
#include "offline_entry_hooks.generated.h"
static bool install(){
    settings_enable_original=(decltype(settings_enable_original))hook(SettingsEnable,(void*)settings_enable);
    settings_localize_original=(decltype(settings_localize_original))hook(SettingsLocalize,(void*)settings_localize);
    reset_click_original=(decltype(reset_click_original))hook(ResetClick,(void*)reset_click);
    reset_result_original=(decltype(reset_result_original))hook(ResetResult,(void*)reset_result);
    bool a=hook(ResetFree,(void*)free_reset)&&hook(ResetSecondFree,(void*)free_reset);
    code_confirm_original=(decltype(code_confirm_original))hook(CodeConfirm,(void*)code_confirm);
    right_original=(decltype(right_original))hook(AccountPanel,(void*)right);
    sign_localize_original=(decltype(sign_localize_original))hook(AccountButtonText,(void*)sign_localize);
    sign_click_original=(decltype(sign_click_original))hook(AccountButtonClick,(void*)sign_click);
    a=hook(DailyVisible,(void*)unavailable)&&hook(Day14Visible,(void*)unavailable)&&a;
    a=hook(DailyClick,(void*)blocked)&&hook(Day14Click,(void*)blocked)&&a;
    ads_update_original=(decltype(ads_update_original))hook(StoreAdsUpdate,(void*)ads_update);
    seller_original=(decltype(seller_original))hook(StoreSeller,(void*)seller);
    goods_original=(decltype(goods_original))hook(StoreGoods,(void*)goods);
    a=hook(StoreAdsGoods,(void*)ads_goods)&&a;
    main_show_original=(decltype(main_show_original))hook(MainShow,(void*)main_show);
    main_last_original=(decltype(main_last_original))hook(MainLast,(void*)main_last);
    pref_get_original=(decltype(pref_get_original))hook(PrefGet,(void*)pref_get);
    pref_set_original=(decltype(pref_set_original))hook(PrefSet,(void*)pref_set);
    pref_has_original=(decltype(pref_has_original))hook(PrefHas,(void*)pref_has);
    pref_delete_original=(decltype(pref_delete_original))hook(PrefDelete,(void*)pref_delete);
    state_original=(decltype(state_original))hook(ModernState,(void*)state);
    pointer_original=(decltype(pointer_original))hook(ModernPointer,(void*)pointer);
    daily30_original=(decltype(daily30_original))hook(ModernDaily30,(void*)daily30);
    daily14_original=(decltype(daily14_original))hook(ModernDaily14,(void*)daily14);
    text_original=(decltype(text_original))hook(LocalizedText,(void*)text);
    a=hook(ModernFirstPurchase,(void*)first_purchase)&&a;
    settings_tab_original=(decltype(settings_tab_original))hook(SettingsTab,(void*)settings_tab);
    settings_tab_click_original=(decltype(settings_tab_click_original))hook(SettingsTabClick,(void*)settings_tab_click);
    profile_tabs_original=(decltype(profile_tabs_original))hook(ProfileTabs,(void*)profile_tabs);
    local_faction::profile_entry_policy=profile_policy;
    installed=a&&settings_enable_original&&settings_localize_original&&reset_click_original&&reset_result_original&&code_confirm_original&&right_original&&sign_localize_original&&sign_click_original&&ads_update_original&&seller_original&&goods_original&&main_show_original&&main_last_original&&pref_get_original&&pref_set_original&&pref_has_original&&pref_delete_original&&state_original&&pointer_original&&daily30_original&&daily14_original&&text_original&&settings_tab_original&&settings_tab_click_original&&profile_tabs_original;
    LOG("Local new start/account/original unavailable entry hooks=%d",installed);return installed;
}
static void on_hall_ready(){
    ui_audit();
    if(!offline_accounts::fresh_account()||!progress_ready||tutorial_queued||progress.next_run||progress.count)return;
    tutorial_queued=true;int chapter=local_faction::active==1?0:200;
    bool started=start_local_campaign(chapter);LOG("Local first tutorial automatic=%d chapter=%d profile=%llu private=%d",started,chapter,(unsigned long long)offline_accounts::choice.active,offline_accounts::private_session);
}
}
