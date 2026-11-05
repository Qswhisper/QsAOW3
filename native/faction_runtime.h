#pragma once
#include "cooperative_store.h"
namespace local_faction {
// Keep the decoded, retained avatar seed separate from the player's editable UI.
static void* commander_avatar_seed;
static uintptr_t commander_avatar_root;
static void* commander_level_seed;
static uintptr_t commander_level_root;
static void* commander_country_seed;
static uintptr_t commander_country_root;
struct Roots {
    uintptr_t handles[32]={};int count=0;
    ~Roots(){while(count)gchandle_free(handles[--count]);}
    bool keep(void* object){if(!object||count==32)return false;handles[count++]=gchandle_new(object,true);return true;}
};
static void* clone(void* value,Roots& roots){void* ex=nullptr;
    void* copy=value?runtime_invoke(class_method(find_class("System","Object"),"MemberwiseClone",0),value,nullptr,&ex):nullptr;
    return !ex&&roots.keep(copy)?copy:nullptr;}
static bool empty_field(void* object,const char* name,Roots& roots){
    void* field=object?class_field_from_name(object_class(object),name):nullptr;
    void* old=field?*(void**)((char*)object+field_offset(field)):nullptr;
    if(!old)return true;void* value=object_new(object_class(old));if(!roots.keep(value))return false;
    void* ex=nullptr;invoke_named(value,".ctor",0,nullptr,&ex);if(ex)return false;field_set(object,field,value);return true;}
static void* fraction(int id){
    void* base=saved("BATTLE");void* field=base?class_field_from_name(object_class(base),"unitTypes"):nullptr;
    void* list=field?*(void**)((char*)base+field_offset(field)):nullptr;int count=list?*(int*)((char*)list+0x18):0;
    for(int i=0;i<count;++i){void* item=list_item(list,i);void* f=item?*(void**)((char*)item+0x20):nullptr;
        if(f&&*(int*)((char*)f+0x10)==id)return f;}
    return nullptr;
}
static bool prepare_user(){
    void* user=saved("UserConfiguration");void* original=user?*(void**)((char*)user+0x28):nullptr;
    if(!original)return false;Roots roots;
    if(!commander_avatar_root){commander_avatar_seed=*(void**)((char*)original+0x50);
        if(!commander_avatar_seed||(commander_avatar_root=gchandle_new(commander_avatar_seed,true))==0)return false;
        commander_level_seed=*(void**)((char*)original+0x20);
        if(commander_level_seed&&(commander_level_root=gchandle_new(commander_level_seed,true))==0)return false;
        commander_country_seed=*(void**)((char*)original+0x48);
        if(!commander_country_seed||(commander_country_root=gchandle_new(commander_country_seed,true))==0)return false;}
    void* profile=clone(original,roots);void* secondary=clone(original,roots);
    void* f=fraction(active);void* other_fraction=fraction(active==1?2:1);
    if(!profile||!secondary||!f||!other_fraction)return false;
    void* public_class=find_class("com.geargames.aow.entities","PublicProfile");
    field_set(profile,class_field_from_name(public_class,"fraction"),f);
    field_set(secondary,class_field_from_name(public_class,"fraction"),other_fraction);
    if(active==1){*(int*)((char*)profile+0x10)=0x7f000001;
        field_set(profile,class_field_from_name(public_class,"name"),string_new("本地联盟军"));
        *(int*)((char*)profile+0xd0)=0;*(int*)((char*)profile+0xd4)=0;
        for(const char* name:{"boosts","troops","quests","invoices","upgrades","activeUpgrades"})if(!empty_field(profile,name,roots))return false;}
    field_set(profile,class_field_from_name(public_class,"name"),string_new(offline_world::player_name(active)));
    field_set(profile,class_field_from_name(public_class,"nameAlias"),string_new(""));
    // The original second-faction panel only needs a PublicProfile; never
    // import achievements, research or inventory from the other faction.
    local_progress::State other{};if(local_progress::load(root(active==1?2:1),other)<0)return false;
    *(int*)((char*)secondary+0x10)=active==2?0x7f000001:*(int*)((char*)original+0x10);
    *(int*)((char*)secondary+0x30)=other.experience;*(int*)((char*)secondary+0x68)=other.stars;
    field_set(secondary,class_field_from_name(public_class,"name"),string_new(offline_world::player_name(active==1?2:1)));
    field_set(secondary,class_field_from_name(public_class,"nameAlias"),string_new(""));
    if(!empty_field(user,"missionResults",roots))return false;
    void* upgrade=clone(*(void**)((char*)user+0x100),roots);if(!upgrade)return false;
    for(const char* name:{"finishUpgrades","activeUpgrades","unitsAvailable"})if(!empty_field(upgrade,name,roots))return false;
    if(active==1){void* mechanics=clone(*(void**)((char*)user+0x210),roots);if(!mechanics||!empty_field(mechanics,"unlockedGameMechanics",roots))return false;
        *(int*)((char*)mechanics+0x18)=0;field_set(user,class_field_from_name(object_class(user),"gameMechanicsConfiguration"),mechanics);}
    // Career receipts reconstruct inventory/structural grants per faction.
    if(!empty_field(profile,"boosts",roots))return false;
    field_set(user,class_field_from_name(object_class(user),"currentProfile"),profile);
    field_set(user,class_field_from_name(object_class(user),"secondary"),secondary);
    field_set(user,class_field_from_name(object_class(user),"upgradeConfiguration"),upgrade);
    LOG("Local faction user prepared faction=%d secondary=%d",active,active==1?2:1);return true;
}
static void (*profile_entry_policy)(void*);
static void (*profile_show_original)(void*,void*,void*);
static void profile_show(void* self,void* profile,void* method){
    profile_show_original(self,profile,method);
    if(!offline_network||!profile)return;
    if(profile_entry_policy)profile_entry_policy(self);
    if(offline_world_runtime::publish_profile(profile))return;
    void* f=*(void**)((char*)profile+0x28);int faction=f?*(int*)((char*)f+0x10):0;
    if(faction!=1&&faction!=2)return;
    local_progress::State state{};int status=local_progress::load(root(faction),state);if(status<0){LOG("Local profile statistics refused unreadable career");return;}
    if(!profile_statistics_runtime::available()){LOG("Local profile statistics consumer unavailable");return;}
    local_match_store::State pvp{};
    if(local_match_store::load(root(faction),pvp)<0){LOG("Local profile statistics refused unreadable AI ledger faction=%d",faction);return;}
    std::vector<local_history_store::Record> history;unsigned damaged=0;
    if(profile_statistics_policy::count(pvp)>50&&!local_history_store::load(root(faction),faction,history,damaged))
        LOG("Local profile recent history unavailable faction=%d; no guessed recent rate",faction);
    profile_statistics_policy::Snapshot statistics;
    if(!profile_statistics_policy::project(state,pvp,history,faction,statistics)){LOG("Local profile statistics projection refused faction=%d",faction);return;}
    Roots roots;void* answer=object_new(find_class("com.geargames.aow.messages","ProfileStatisticsAnswer"));if(!roots.keep(answer))return;
    void* ex=nullptr;invoke_named(answer,".ctor",0,nullptr,&ex);if(ex)return;
    field_set(answer,class_field_from_name(object_class(answer),"profile"),profile);
    *(int*)((char*)answer+0x34)=state.experience;*(int*)((char*)answer+0x38)=state.stars;
    *(int*)((char*)answer+0x58)=statistics.campaign_xp;*(int*)((char*)answer+0x5c)=statistics.campaign_stars;
    *(int*)((char*)answer+0x60)=statistics.campaign_battles;*(int*)((char*)answer+0x64)=statistics.campaign_wins;
    *(int*)((char*)answer+0x68)=statistics.completed;*(int*)((char*)answer+0x6c)=statistics.three_star;*(int*)((char*)answer+0x70)=statistics.three_star_hard;
    // Read existing independent ledgers without settling or modifying them.
    // formal rank stars remain separate from campaign best stars.
    auto count=[](const local_match_store::State& s){
        uint64_t n=(uint64_t)s.wins+s.losses+s.draws;return n>INT_MAX?INT_MAX:(int)n;
    };
    *(int*)((char*)answer+0x78)=statistics.pvp_stars;
    *(int*)((char*)answer+0x7c)=statistics.pvp_battles;*(int*)((char*)answer+0x80)=statistics.pvp_wins;
    cooperative_store::State coop{};
    if(faction==2){
        if(cooperative_store::load(offline_accounts::cooperative_root(),coop)<0){LOG("Local profile statistics refused unreadable cooperative ledger");return;}
        *(int*)((char*)answer+0x9c)=count(coop.match);*(int*)((char*)answer+0xa0)=coop.match.wins>INT_MAX?INT_MAX:(int)coop.match.wins;
    }
    *(int*)((char*)answer+0x48)=statistics.total_ratio;*(int*)((char*)answer+0x4c)=statistics.current_ratio;
    void* shared=saved("SHARED");void* field=shared?class_field_from_name(object_class(shared),"profileRanks"):nullptr;
    void* ranks=field?*(void**)((char*)shared+field_offset(field)):nullptr;int n=ranks?*(int*)((char*)ranks+0x18):0;
    void* rank=nullptr;int best=-1;
    for(int i=0;i<n;++i){void* item=list_item(ranks,i);int stars=item?*(int*)((char*)item+0x14):INT_MAX;
        if(stars<=state.stars&&stars>best){rank=item;best=stars;}}
    if(!rank)return;field_set(answer,class_field_from_name(object_class(answer),"rank"),rank);
    profile_statistics_runtime::remember(profile,faction,statistics);
    void* dispatcher=*(void**)((char*)offline_network+0x88);void* args[]={answer};
    if(dispatcher)invoke_named(dispatcher,"Dispatch",1,args,&ex);
    LOG("Local profile statistics faction=%d dispatched=%d stars=%d campaign_completed=%d three_star=%d hard=%d pve_stars=%d pvp_stars=%d total_rate=%d current_rate=%d recent_known=%d damaged_history=%u read_only=1",faction,dispatcher&&!ex,state.stars,statistics.completed,statistics.three_star,statistics.three_star_hard,statistics.campaign_stars,statistics.pvp_stars,statistics.total_ratio,statistics.current_ratio,statistics.current_known,damaged);
    LOG("Local profile mode statistics faction=%d pvp=%u/%d cooperative=%u/%d read_only=1",faction,pvp.wins,count(pvp),faction==2?coop.match.wins:0,faction==2?count(coop.match):0);
}
static void switch_request(void* self,void*){
    void* profile=self?*(void**)((char*)self+0x48):nullptr;void* f=profile?*(void**)((char*)profile+0x28):nullptr;
    int target=f?*(int*)((char*)f+0x10):0;
    bool saved=progress_ready&&!local_run_active&&target!=active&&choose(target);
    LOG("Local faction switch from=%d to=%d persisted=%d",active,target,saved);
    // The original switch completion ends by dispatching LogoutGame(-6).
    // Use that exact bounded path, then republish fresh local configuration in
    // the original logon scene. No online request or server profile mutation.
    if(saved){progress_ready=false;progress_checked=false;main_attempted=false;}
    reinterpret_cast<void(*)(void*,int,void*)>(image_base+0x7e81e38)(self,-6,nullptr);
}
}
