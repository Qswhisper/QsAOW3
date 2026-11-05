// Building-family adaptation on an unpublished side, before entity migration.
#pragma once
#include "building_research_effects.h"
namespace building_weapon_research {static bool supported_saved(void*);}
namespace building_research {
using local_research_runtime::Roots;
using research_effects::BuildingStats;
struct Effect {void* owner=nullptr;BuildingStats delta;unsigned unlocked_levels=0;};
static int count(void* list){return list?*(int*)((char*)list+0x18):0;}
static bool identity(void* a,void* b) {
    if(!a||!b||strcmp(class_name(object_class(a)),"BuildingType")||strcmp(class_name(object_class(b)),"BuildingType"))return false;
    void* fa=*(void**)((char*)a+0x20);void* fb=*(void**)((char*)b+0x20);char ma[256],mb[256];
    return fa&&fb&&*(int*)((char*)fa+0x10)==*(int*)((char*)fb+0x10)&&
        !memcmp((char*)a+0x28,(char*)b+0x28,3)&&*(int8_t*)((char*)a+0x30)==*(int8_t*)((char*)b+0x30)&&
        local_research_runtime::ascii(*(void**)((char*)a+0x38),ma,sizeof(ma))&&ma[0]&&
        local_research_runtime::ascii(*(void**)((char*)b+0x38),mb,sizeof(mb))&&!strcmp(ma,mb);
}
static bool resolve(void* level,Effect& effect) {
    void* upgrade=level?*(void**)((char*)level+0x48):nullptr;
    if(!upgrade||strcmp(class_name(object_class(upgrade)),"UpgradeBuilding"))return false;
    effect.owner=*(void**)((char*)upgrade+0x48);char text[4096];
    if(!identity(effect.owner,effect.owner))return false;
    int slot=*(int8_t*)((char*)upgrade+0x21);
    if(slot==-1){
        // Original BUILDING_LEVEL_UPGRADE_SLOT_ID, not a missing stat effect.
        // Exact original UpgLvl link selects the tier; no index/name inference.
        void* description=*(void**)((char*)level+0x18);
        if((description&&(!local_research_runtime::ascii(description,text,sizeof(text))||text[0]))||*(int*)((char*)level+0x54))return false;
        void* rows=*(void**)((char*)effect.owner+0x18);int n=count(rows),matches=0;
        if(n<1||n>5)return false;
        for(int i=0;i<n;++i){void* row=list_item(rows,i),*link=row?*(void**)((char*)row+0x90):nullptr;
            if(link&&*(int*)((char*)link+0x10)==*(int*)((char*)level+0x10)){effect.unlocked_levels|=1u<<i;++matches;}}
        return matches==1;
    }
    return slot>=0&&local_research_runtime::ascii(*(void**)((char*)level+0x18),text,sizeof(text))&&research_effects::building_parse(text,effect.delta);
}
static bool shape(void* source,const Effect& effect) {
    if(!identity(source,effect.owner))return false;
    void* levels=*(void**)((char*)source+0x18);void* baseline=*(void**)((char*)effect.owner+0x18);int n=count(baseline);
    if(n<1||n>5||count(levels)!=n||(effect.delta.flat_hp&&n!=1)||
       (effect.unlocked_levels&~((1u<<n)-1))||
       (effect.delta.hp_mask&&effect.delta.hp_mask!=((1u<<n)-1))||
       (effect.delta.flat_power&&(n!=1||*(int8_t*)((char*)source+0x30)!=2))||
       (effect.delta.power_mask&&(effect.delta.power_mask!=((1u<<n)-1)||*(int8_t*)((char*)source+0x30)!=1)))return false;
    if(effect.delta.training){int kind=*(int8_t*)((char*)source+0x30);bool factory=kind==7||kind==8||kind==9||kind==11||kind==18;
        if((effect.delta.training_token==20047&&kind!=5)||(effect.delta.training_token==20048&&!factory))return false;}
    bool producer=false;
    for(int i=0;i<n;++i){void* s=list_item(levels,i);void* b=list_item(baseline,i);if(!s||!b)return false;
        int64_t hp=(int64_t)*(int*)((char*)b+0x34)+effect.delta.hp[i];
        int64_t armor=(int64_t)*(int16_t*)((char*)b+0x38)+effect.delta.armor;
        if(hp<=0||hp>INT16_MAX||armor<0||armor>INT16_MAX)return false;}
    for(int i=0;i<n;++i){void* b=list_item(baseline,i);
        if(effect.delta.power[i]){
            int64_t energy=(int64_t)*(int16_t*)((char*)b+0x60)-effect.delta.power[i];
            if(*(int16_t*)((char*)b+0x60)>=0||energy<INT16_MIN||energy>=0)return false;}
        if(effect.delta.training){void* units=*(void**)((char*)b+0x20);if(count(units)<0||count(units)>256)return false;
            for(int j=0;j<count(units);++j){void* u=list_item(units,j);int value;
                if(!u||strcmp(class_name(object_class(u)),"UnitType"))return false;
                if(!research_effects::training_time(*(int*)((char*)u+0x38),effect.delta.training,value))return false;
                producer=true;}}}
    if(effect.delta.training&&!producer)return false;
    return true;
}
struct Plan {
    Effect families[64];int size=0;
    bool add(const Effect& effect){for(int i=0;i<size;++i)if(families[i].owner==effect.owner){
        BuildingStats next;if(!research_effects::building_add(families[i].delta,effect.delta,next))return false;
        families[i].delta=next;families[i].unlocked_levels|=effect.unlocked_levels;return true;}
        if(size==64)return false;families[size++]=effect;return true;}
};
static bool preexisting_starter(void* level){
    void* up=level?*(void**)((char*)level+0x48):nullptr;
    if(!up||strcmp(class_name(object_class(up)),"UpgradeBuilding")||*(int8_t*)((char*)up+0x21)!=-1||
       *(int8_t*)((char*)up+0x20)!=0||*(int*)((char*)level+0x20)||*(int*)((char*)level+0x28)||*(int*)((char*)level+0x54))return false;
    void* desc=*(void**)((char*)level+0x18);if(desc&&*(int*)((char*)desc+16))return false;
    void* owner=*(void**)((char*)up+0x48),*rows=owner?*(void**)((char*)owner+0x18):nullptr;
    void* first=count(rows)?list_item(rows,0):nullptr;
    return first&&*(int8_t*)((char*)first+0x28)==2&&!*(void**)((char*)first+0x90);
}
static bool collect_structural(Plan& plan,const local_progress::State& state,int faction){
    if(faction!=1&&faction!=2)return false;
    void* stones=local_career::list(local_career::key(faction),"careerStones");if(!stones)return false;
    for(int i=0;i<count(stones);++i){void* stone=list_item(stones,i);if(!stone)return false;
        if(!local_progress::has_career(state,*(int*)((char*)stone+0x10)))continue;
        if(*(int*)((char*)stone+0x20)>state.stars)return false;
        void* rewards=*(void**)((char*)stone+0x18);int n=count(rewards);if(n<1||n>128)return false;
        for(int j=0;j<n;++j){void* r=list_item(rewards,j);if(!r)return false;
            if(*(int*)((char*)r+0x14)!=20)continue;
            if(*(int*)((char*)r+0x1c)!=1)return false;
            void* level=local_career::lookup(local_career::key(faction),"upgradeLevels",*(int*)((char*)r+0x18));Effect e;
            if(!resolve(level,e)){if(preexisting_starter(level))continue;return false;}
            if(!e.unlocked_levels||!plan.add(e))return false;
        }
    }return true;
}
static bool collect(Plan& plan,void* extra=nullptr) {
    bool included=false;
    for(uint32_t i=0;i<progress.upgrade_count;++i){void* level=local_upgrade_level(progress.upgrade_ids[i]);
        void* upgrade=level?*(void**)((char*)level+0x48):nullptr;if(!upgrade)return false;
        if(extra&&*(int*)((char*)level+0x10)==*(int*)((char*)extra+0x10))included=true;
        if(!strcmp(class_name(object_class(upgrade)),"UpgradeUnit"))continue;
        if(strcmp(class_name(object_class(upgrade)),"UpgradeBuilding"))return false;
        research_effects::Delta legacy;void* owner=nullptr;
        if(local_research_runtime::level_delta(level,legacy,owner))continue;
        Effect e;if(!resolve(level,e)){if(building_weapon_research::supported_saved(level))continue;return false;}
        if(!plan.add(e))return false;
    }
    if(extra&&!included){Effect e;if(!resolve(extra,e)||!plan.add(e))return false;}
    if(!collect_structural(plan,progress,local_faction::active))return false;
    for(int i=0;i<plan.size;++i){void* faction=*(void**)((char*)plan.families[i].owner+0x20);
        if(!faction||*(int*)((char*)faction+0x10)!=local_faction::active||!shape(plan.families[i].owner,plan.families[i]))return false;}
    return true;
}
static bool purchase_supported(void* live) {
    void* archived=live?local_upgrade_level(*(int*)((char*)live+0x10)):nullptr;Effect a,b;
    if(!resolve(live,a)||!resolve(archived,b)||!identity(a.owner,b.owner)||
       *(int*)((char*)a.owner+0x10)!=*(int*)((char*)b.owner+0x10)||!research_effects::building_equal(a.delta,b.delta)||a.unlocked_levels!=b.unlocked_levels||
       *(int*)((char*)live+0x54)||*(int*)((char*)archived+0x54))return false;
    // Structural tier entries are granted by the original career reward type20.
    // Their CostC/Time fields are not a new manual purchase or claim fee.
    if(a.unlocked_levels||b.unlocked_levels)return false;
    Plan plan;return collect(plan,archived);
}
static void* copy_type(void* source,const Effect& effect,Roots& roots) {
    if(!shape(source,effect))return nullptr;
    void* copy=local_research_runtime::clone(source,roots);void* old=*(void**)((char*)source+0x18);
    void* levels=local_research_runtime::new_list(old,roots);if(!copy||!levels)return nullptr;
    void* baseline=*(void**)((char*)effect.owner+0x18);void* ex=nullptr;
    for(int i=0;i<count(old);++i){Roots lr;void* original=list_item(old,i);void* base=list_item(baseline,i);
        void* level=local_research_runtime::clone(original,lr);if(!level)return nullptr;
        // RESEARCHED=2 is the original BuildingLevelType enum. Other tiers retain
        // the scenario's availability, rank, contributions, price and producer list.
        if(effect.unlocked_levels&(1u<<i))*(int8_t*)((char*)level+0x28)=2;
        // Restore affected stats from canonical faction data, not the online
        // guest's capture-time research. Keep weapons, producers and availability.
        if(effect.delta.flat_hp||effect.delta.hp_mask){int hp=*(int*)((char*)base+0x34)+effect.delta.hp[i];
            *(int16_t*)((char*)level+0x30)=(int16_t)hp;*(int*)((char*)level+0x34)=hp;}
        if(effect.delta.armor)*(int16_t*)((char*)level+0x38)=*(int16_t*)((char*)base+0x38)+effect.delta.armor;
        if(effect.delta.power[i])for(int offset:{0x60,0x62})
            *(int16_t*)((char*)level+offset)=*(int16_t*)((char*)base+0x60)-effect.delta.power[i];
        void* args[]={level};invoke_named(levels,"Add",1,args,&ex);if(ex)return nullptr;}
    field_set(copy,class_field_from_name(object_class(copy),"levels"),levels);return copy;
}
// Caller provides an immutable baseline; repeated application never stacks.
static bool apply(void* side,void* baseline,const Plan& plan,int& matched) {
    matched=0;if(!side||!baseline)return false;Roots roots;uintptr_t handle=0;
    void* values=local_dictionary_values(baseline,find_class("com.geargames.aow.entities","BuildingType"),handle);
    if(!values||!roots.take(handle))return false;int indexes[64];for(int i=0;i<plan.size;++i)indexes[i]=-1;
    for(size_t j=0;j<array_length(values);++j){void* type=((void**)((char*)values+32))[j];if(!type)return false;
        int claims=0;for(int i=0;i<plan.size;++i)if(identity(type,plan.families[i].owner)){
            if(++claims>1||indexes[i]>=0)return false;indexes[i]=(int)j;++matched;}}
    void* replacement=object_new(object_class(baseline));if(!roots.keep(replacement))return false;
    void* ex=nullptr;invoke_named(replacement,".ctor",0,nullptr,&ex);if(ex)return false;
    for(size_t j=0;j<array_length(values);++j){Roots tr;void* type=((void**)((char*)values+32))[j];void* next=type;
        int id=*(int*)((char*)type+0x10);if(local_dictionary_item(baseline,id)!=type)return false;
        for(int i=0;i<plan.size;++i)if(indexes[i]==(int)j){next=copy_type(type,plan.families[i],tr);if(!next)return false;}
        void* args[]={&id,next};invoke_named(replacement,"Add",2,args,&ex);if(ex)return false;}
    field_set(side,class_field_from_name(object_class(side),"$Gg"),replacement);return true;
}
// Match canonical producer membership, then retain the current side's private
// combat stats. Time reduction is applied once to canonical times; the original
// BuildingAct training calculator still applies its scenario/boost modifiers.
static bool apply_training(void* side,const Plan& plan,int& changed) {
    struct Member {void* unit;void* owner;int percent;};Member members[256];int size=0;changed=0;
    for(int i=0;i<plan.size;++i){const auto& e=plan.families[i];if(!e.delta.training)continue;
        void* levels=*(void**)((char*)e.owner+0x18);
        for(int j=0;j<count(levels);++j){void* units=*(void**)((char*)list_item(levels,j)+0x20);
            for(int k=0;k<count(units);++k){void* u=list_item(units,k);bool found=false;
                for(int m=0;m<size;++m)if(members[m].unit==u){
                    if(members[m].owner!=e.owner||members[m].percent!=e.delta.training)return false;found=true;}
                if(!found){if(size==256)return false;members[size++]={u,e.owner,e.delta.training};}}}}
    if(!size)return true;Roots roots;uintptr_t handle=0;void* old=*(void**)((char*)side+0xf0);
    void* values=local_dictionary_values(old,find_class("com.geargames.aow.entities","UnitType"),handle);
    if(!values||!roots.take(handle))return false;
    void* replacement=object_new(object_class(old));if(!roots.keep(replacement))return false;
    void* ex=nullptr;invoke_named(replacement,".ctor",0,nullptr,&ex);if(ex)return false;
    bool seen[256]={};
    for(size_t i=0;i<array_length(values);++i){Roots one;void* u=((void**)((char*)values+32))[i];void* next=u;
        if(!u)return false;int id=*(int*)((char*)u+0x10);if(local_dictionary_item(old,id)!=u)return false;
        int match=-1;
        for(int m=0;m<size;++m)if(generic_unit_research::identity(u,members[m].unit)){
            if(match>=0||seen[m])return false;match=m;}
        if(match>=0){seen[match]=true;next=local_research_runtime::clone(u,one);if(!next)return false;
            for(int offset:{0x38,0x3c}){int value;if(!research_effects::training_time(
                *(int*)((char*)members[match].unit+0x38),members[match].percent,value))return false;
                *(int*)((char*)next+offset)=value;}++changed;}
        void* args[]={&id,next};invoke_named(replacement,"Add",2,args,&ex);if(ex)return false;}
    field_set(side,class_field_from_name(object_class(side),"$gg"),replacement);
    return local_research_runtime::relink_production_units(side);
}
static bool prepare_saved(void* side) {
    Plan plan;if(!side||!collect(plan))return false;if(!plan.size)return true;
    int matched=0,trained=0;bool ok=apply(side,*(void**)((char*)side+0xf8),plan,matched)&&apply_training(side,plan,trained);
    LOG("Building research prepared=%d side=%d families=%d present=%d absent_in_scenario=%d",ok,
        *(int*)((char*)side+0x10),plan.size,matched,plan.size-matched);return ok;
}
}
