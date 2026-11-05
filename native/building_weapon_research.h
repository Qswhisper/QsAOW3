// Building-specific original weapon routes; all preparation stays unpublished.
#pragma once
#include "building_combat_effects.h"
namespace building_weapon_research {
using local_research_runtime::Roots;
using generic_unit_research::WeaponKey;
using research_effects::WeaponParameters;
static void* get(void* p,int o){return p?*(void**)((char*)p+o):nullptr;}
static int count(void* p){return p?*(int*)((char*)p+0x18):0;}
struct Effect {void* owner=nullptr;WeaponKey keys[2];WeaponParameters delta[2];int targets=0,sight=0;};
static int target(void* w,const Effect& e){for(int i=0;i<e.targets;++i)if(generic_unit_research::weapon_matches(w,e.keys[i]))return i;return -1;}
static bool select(void* owner,int slot,WeaponKey& key,bool common_only=false){
    void* levels=get(owner,0x18);int n=count(levels);if(n<1||n>5||slot<0||slot>=20)return false;
    WeaponKey found;bool common=true;int total=0;
    for(int i=0;i<n;++i){void* list=get(list_item(levels,i),0x18);int k=count(list);if(k<1||k>32)return false;
        for(int j=0;j<k;++j){void* w=list_item(list,j);if(!w)return false;
            WeaponKey current{*(int16_t*)((char*)w+0x20),*(int16_t*)((char*)w+0x24),0};
            if(!total)found=current;else if(!generic_unit_research::same_key(found,current))common=false;++total;}}
    if(common&&found.name>0&&found.type>0){found.refs=total;key=found;return true;}
    if(common_only)return false;
    // Original localized research names distinguish torpedo slots2/3 and AA4/5.
    // Reviewed mapping, not a recovered server modifier table. See binding JSON.
    int faction=get(owner,0x20)?*(int*)((char*)get(owner,0x20)+0x10):0,id=*(int*)((char*)owner+0x10);
    char model[256];if(!local_research_runtime::ascii(get(owner,0x38),model,sizeof(model)))return false;
    WeaponKey routed;
    if(faction==1&&id==19&&!strcmp(model,"f1/f1_bld_torpedo")){
        if(slot==2||slot==3)routed={2742,34,0};else if(slot==4||slot==5)routed={2741,30,0};else return false;
    }else if(faction==2&&id==119&&!strcmp(model,"f2/f2_bld_seagun")){
        if(slot==2||slot==3)routed={2768,34,0};else if(slot==4||slot==5)routed={2770,10,0};else return false;
    }else return false;
    for(int i=0;i<n;++i){void* list=get(list_item(levels,i),0x18);
        for(int j=0;j<count(list);++j)if(generic_unit_research::weapon_matches(list_item(list,j),routed))++routed.refs;}
    if(!routed.refs)return false;key=routed;return true;
}
static bool resolve(void* level,Effect& e){
    void* upgrade=get(level,0x48);if(!upgrade||strcmp(class_name(object_class(upgrade)),"UpgradeBuilding"))return false;
    void* owner=get(upgrade,0x48);if(!building_research::identity(owner,owner))return false;
    char text[4096];WeaponParameters delta;int sight=0,slot=*(int8_t*)((char*)upgrade+0x21);bool armed=false;
    if(slot<0||slot>=20||!local_research_runtime::ascii(get(level,0x18),text,sizeof(text)))return false;
    if(!research_effects::building_combat_parse(text,delta,sight,armed)){
        research_effects::WeaponGroups groups;
        if(!research_effects::weapon_groups_parse(text,groups)||groups.scalar.hp||groups.scalar.armor||
           groups.scalar.speed||groups.scalar.regen)return false;
        for(const auto& damage:groups.scalar.damage)for(int value:damage)if(value)return false;
        // These are the existing reviewed torpedo and secondary-AA slot
        // identities, reused for explicit I/II labels, not array positions.
        int faction=get(owner,0x20)?*(int*)((char*)get(owner,0x20)+0x10):0,id=*(int*)((char*)owner+0x10);
        bool pair=(faction==1&&id==19)||(faction==2&&id==119);
        if(!pair&&(!groups.used[0]||groups.used[1]))return false;
        Effect next;next.owner=owner;next.sight=groups.scalar.sight;
        for(int group=0;group<2;++group)if(groups.used[group]){
            WeaponKey key;if(!select(owner,group==0?2:4,key,!pair))return false;
            next.keys[next.targets]=key;next.delta[next.targets++]=groups.weapons[group];}
        e=next;return true;
    }
    WeaponKey key;if(armed&&!select(owner,slot,key))return false;
    e.owner=owner;e.sight=sight;if(armed){e.keys[0]=key;e.delta[0]=delta;e.targets=1;}return true;
}
static WeaponParameters read(void* w){
    WeaponParameters s;for(int i=0;i<3;++i)s.damage[i]=*(int*)((char*)w+0x28+i*4);
    s.range=*(int*)((char*)w+0x4c);s.explosion=*(int*)((char*)w+0x7c);s.reload=*(int16_t*)((char*)w+0x66);s.shot_start=*(int16_t*)((char*)w+0x60);
    s.accuracy[0]=*(int16_t*)((char*)w+0x8e);s.accuracy[1]=*(int16_t*)((char*)w+0x92);s.accuracy[2]=*(int16_t*)((char*)w+0x90);return s;
}
static bool values(void* w,const WeaponParameters& delta,WeaponParameters& next){
    return w&&!*(bool*)((char*)w+0x14)&&research_effects::building_weapon_values(read(w),delta,
        *(int16_t*)((char*)w+0x24),*(int8_t*)((char*)w+0x94),*(int8_t*)((char*)w+0x8a),next);
}
static bool shape(void* source,const Effect& e){
    if(!building_research::identity(source,e.owner)||e.targets<0||e.targets>2||e.sight<0||(!e.targets&&!e.sight))return false;
    void* source_levels=get(source,0x18),*base_levels=get(e.owner,0x18);int n=count(source_levels);
    if(n<1||n>5||n!=count(base_levels))return false;int hits[2]={};
    for(int i=0;i<e.targets;++i)if(e.keys[i].name<=0||e.keys[i].type<=0||e.keys[i].refs<=0||
        (i&&generic_unit_research::same_key(e.keys[0],e.keys[i])))return false;
    for(int i=0;i<n;++i){void* source_level=list_item(source_levels,i),*base_level=list_item(base_levels,i);int sight=0;
        if(!source_level||!base_level||(e.sight&&!research_effects::building_sight_value(*(int*)((char*)base_level+0x40),e.sight,sight)))return false;
        if(!e.targets)continue;
        void* a=get(source_level,0x18),*b=get(base_level,0x18);int k=count(a);
        if(k<1||k>32||k!=count(b))return false;
        for(int j=0;j<k;++j){void* w=list_item(a,j),*base=list_item(b,j);if(!w||!base||
            *(int16_t*)((char*)w+0x20)!=*(int16_t*)((char*)base+0x20)||*(int16_t*)((char*)w+0x24)!=*(int16_t*)((char*)base+0x24))return false;
            int t=target(w,e);if(t>=0){++hits[t];WeaponParameters next;
                if(*(bool*)((char*)w+0x14)||!values(base,e.delta[t],next))return false;}}}
    for(int i=0;i<e.targets;++i)if(hits[i]!=e.keys[i].refs)return false;return true;
}
static bool supported_saved(void* level){Effect e;return resolve(level,e)&&shape(e.owner,e);}
struct Plan {
    Effect families[64];int size=0;
    bool add(const Effect& e){
        for(int i=0;i<size;++i)if(families[i].owner==e.owner){Effect next=families[i];
            if(!research_effects::building_sight_value(next.sight,e.sight,next.sight))return false;
            for(int j=0;j<e.targets;++j){int k=-1;for(int t=0;t<next.targets;++t)if(generic_unit_research::same_key(next.keys[t],e.keys[j]))k=t;
                if(k<0){if(next.targets==2)return false;k=next.targets++;next.keys[k]=e.keys[j];}
                if(next.keys[k].refs!=e.keys[j].refs||!research_effects::building_weapon_add(next.delta[k],e.delta[j],next.delta[k]))return false;}
            families[i]=next;return true;}
        if(size==64)return false;families[size++]=e;return true;
    }
};
static bool collect(Plan& plan,void* extra=nullptr){
    bool included=false;
    for(uint32_t i=0;i<progress.upgrade_count;++i){void* level=local_upgrade_level(progress.upgrade_ids[i]);void* up=get(level,0x48);if(!up)return false;
        if(extra&&*(int*)((char*)level+0x10)==*(int*)((char*)extra+0x10))included=true;
        if(!strcmp(class_name(object_class(up)),"UpgradeUnit"))continue;
        if(strcmp(class_name(object_class(up)),"UpgradeBuilding"))return false;
        research_effects::Delta hq;void* owner=nullptr;building_research::Effect other;
        if(local_research_runtime::level_delta(level,hq,owner)||building_research::resolve(level,other))continue;
        Effect e;if(!resolve(level,e)||!plan.add(e))return false;
    }
    if(extra&&!included){Effect e;if(!resolve(extra,e)||!plan.add(e))return false;}
    for(int i=0;i<plan.size;++i)if(!get(plan.families[i].owner,0x20)||
        *(int*)((char*)get(plan.families[i].owner,0x20)+0x10)!=local_faction::active||!shape(plan.families[i].owner,plan.families[i]))return false;
    return true;
}
static bool purchase_supported(void* live){
    Effect a,b;void* archived=live?local_upgrade_level(*(int*)((char*)live+0x10)):nullptr;
    if(!resolve(live,a)||!resolve(archived,b)||!building_research::identity(a.owner,b.owner)||
        *(int*)((char*)a.owner+0x10)!=*(int*)((char*)b.owner+0x10)||a.targets!=b.targets||a.sight!=b.sight||
        memcmp(a.delta,b.delta,sizeof(a.delta))||*(int*)((char*)live+0x54)||*(int*)((char*)archived+0x54))return false;
    for(int i=0;i<a.targets;++i)if(!generic_unit_research::same_key(a.keys[i],b.keys[i])||a.keys[i].refs!=b.keys[i].refs)return false;
    Plan plan;return collect(plan,archived);
}
static void* copy_type(void* source,const Effect& e,Roots& roots){
    if(!shape(source,e))return nullptr;
    void* copy=local_research_runtime::clone(source,roots),*old=get(source,0x18),*base=get(e.owner,0x18);
    void* levels=local_research_runtime::new_list(old,roots);if(!copy||!levels)return nullptr;
    void* sources[160]={},*bases[160]={},*copies[160]={};int copied=0;void* ex=nullptr;
    for(int i=0;i<count(old);++i){Roots lr;void* row=local_research_runtime::clone(list_item(old,i),lr);if(!row)return nullptr;
        if(e.sight){int sight=0;if(!research_effects::building_sight_value(*(int*)((char*)list_item(base,i)+0x40),e.sight,sight))return nullptr;
            *(int*)((char*)row+0x40)=sight;*(int*)((char*)row+0x44)=sight;}
        if(e.targets){void* a=get(list_item(old,i),0x18),*b=get(list_item(base,i),0x18),*weapons=local_research_runtime::new_list(a,lr);if(!weapons)return nullptr;
        for(int j=0;j<count(a);++j){Roots wr;void* original=list_item(a,j),*canonical=list_item(b,j),*next=original;int t=target(original,e);
            if(t>=0){next=nullptr;for(int k=0;k<copied;++k)if(sources[k]==original){if(bases[k]!=canonical)return nullptr;next=copies[k];break;}
                if(!next){WeaponParameters v;if(copied==160||!values(canonical,e.delta[t],v))return nullptr;
                    next=local_research_runtime::clone(original,wr);if(!next)return nullptr;
                    for(int k=0;k<3;++k)*(int*)((char*)next+0x28+k*4)=v.damage[k];
                    *(int*)((char*)next+0x4c)=v.range;*(int*)((char*)next+0x7c)=v.explosion;*(int16_t*)((char*)next+0x66)=v.reload;
                    *(int16_t*)((char*)next+0x60)=v.shot_start;
                    *(int16_t*)((char*)next+0x8e)=v.accuracy[0];*(int16_t*)((char*)next+0x92)=v.accuracy[1];*(int16_t*)((char*)next+0x90)=v.accuracy[2];
                    sources[copied]=original;bases[copied]=canonical;copies[copied++]=next;}}
            void* args[]={next};invoke_named(weapons,"Add",1,args,&ex);if(ex)return nullptr;}
        field_set(row,class_field_from_name(object_class(row),"weapons"),weapons);}
        void* args[]={row};invoke_named(levels,"Add",1,args,&ex);if(ex)return nullptr;
    }
    field_set(copy,class_field_from_name(object_class(copy),"levels"),levels);return copy;
}
static bool apply(void* side,void* baseline,const Plan& plan,int& matched){
    matched=0;if(!side||!baseline)return false;Roots roots;uintptr_t handle=0;
    void* values=local_dictionary_values(baseline,find_class("com.geargames.aow.entities","BuildingType"),handle);
    if(!values||!roots.take(handle))return false;int indices[64];for(int i=0;i<plan.size;++i)indices[i]=-1;
    for(size_t j=0;j<array_length(values);++j){void* source=((void**)((char*)values+32))[j];if(!source)return false;int hits=0;
        for(int i=0;i<plan.size;++i)if(building_research::identity(source,plan.families[i].owner)){
            if(++hits>1||indices[i]>=0)return false;indices[i]=(int)j;++matched;}}
    void* replacement=object_new(object_class(baseline));if(!roots.keep(replacement))return false;void* ex=nullptr;
    invoke_named(replacement,".ctor",0,nullptr,&ex);if(ex)return false;
    for(size_t j=0;j<array_length(values);++j){Roots tr;void* source=((void**)((char*)values+32))[j],*next=source;int id=*(int*)((char*)source+0x10);
        if(local_dictionary_item(baseline,id)!=source)return false;
        for(int i=0;i<plan.size;++i)if(indices[i]==(int)j){next=copy_type(source,plan.families[i],tr);if(!next)return false;}
        void* args[]={&id,next};invoke_named(replacement,"Add",2,args,&ex);if(ex)return false;}
    field_set(side,class_field_from_name(object_class(side),"$Gg"),replacement);return true;
}
static bool prepare_saved(void* side){
    Plan plan;if(!side||!collect(plan))return false;if(!plan.size)return true;int matched=0;
    bool ok=apply(side,get(side,0xf8),plan,matched);
    LOG("Building weapon research prepared=%d side=%d families=%d present=%d absent_in_scenario=%d",ok,*(int*)((char*)side+0x10),plan.size,matched,plan.size-matched);return ok;
}
}
