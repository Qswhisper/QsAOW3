#pragma once
#include "support_ability_effects.h"
namespace support_ability {
using local_research_runtime::Roots;using research_effects::SupportAura;
static void* get(void* p,int o){return p?*(void**)((char*)p+o):nullptr;}
struct Effect {int faction=0;SupportAura delta;};
struct Plan {int faction=0;SupportAura delta;};
static bool owner(void* type,int& faction){
    void* f=get(type,0x20);if(!f||!type)return false;faction=*(int*)((char*)f+0x10);int id=*(int*)((char*)type+0x10);
    char model[256];return ((faction==1&&id==17&&*(int8_t*)((char*)type+0x31)==22)||
        (faction==2&&id==117&&*(int8_t*)((char*)type+0x31)==23))&&
        local_research_runtime::ascii(get(type,0x60),model,sizeof(model))&&!strcmp(model,faction==1?"f1/f1_veh_shield":"f2/f2_veh_chameleon");
}
static bool candidate(void* row){void* up=get(row,0x48);if(!up||strcmp(class_name(object_class(up)),"UpgradeUnit"))return false;
    int faction=0;char text[4096];return owner(get(up,0x48),faction)&&local_research_runtime::ascii(get(row,0x18),text,sizeof(text))&&
        (strstr(text,"{#20022<")||strstr(text,"{#20023<"));}
static bool resolve(void* row,Effect& out){
    if(!candidate(row))return false;void* up=get(row,0x48);int slot=*(int8_t*)((char*)up+0x21);char text[4096];Effect next;
    if(slot<0||slot>=20||!owner(get(up,0x48),next.faction)||!local_research_runtime::ascii(get(row,0x18),text,sizeof(text))||
        !research_effects::support_aura_parse(text,next.delta)||(next.faction==2&&next.delta.bonus))return false;out=next;return true;
}
struct Base {void* config=nullptr;void* fraction=nullptr;};static Base bases[2];
static void* base(int faction){
    if(faction<1||faction>2)return nullptr;auto& cache=bases[faction-1];void* cfg=saved(faction==1?"CONFEDERATION":"RESISTANCE");
    if(cache.config==cfg&&cache.fraction)return cache.fraction;void* field=cfg?class_field_from_name(object_class(cfg),"upgradeLevels"):nullptr;
    void* rows=field?get(cfg,field_offset(field)):nullptr;
    for(int i=0;rows&&i<*(int*)((char*)rows+0x18);++i){void* up=get(list_item(rows,i),0x48);if(!up||strcmp(class_name(object_class(up)),"UpgradeUnit"))continue;
        int found=0;void* type=get(up,0x48);if(owner(type,found)&&found==faction){cache={cfg,get(type,0x20)};return cache.fraction;}}
    return nullptr;
}
static bool shape(const Plan& p){if(!p.delta.radius&&!p.delta.bonus)return true;void* f=base(p.faction);SupportAura values;
    return f&&research_effects::support_aura_values(*(int*)((char*)f+0x44),*(int*)((char*)f+0x48),p.delta,values);}
static bool supported_saved(void* row){Effect e;return resolve(row,e)&&shape({e.faction,e.delta});}
static bool collect(Plan& out,void* extra=nullptr){Plan next;bool included=false;
    auto add=[&](void* row){Effect e;if(!resolve(row,e)||e.faction!=local_faction::active||(next.faction&&next.faction!=e.faction)||
        !research_effects::support_aura_add(next.delta,e.delta,next.delta))return false;next.faction=e.faction;return true;};
    for(uint32_t i=0;i<progress.upgrade_count;++i){void* row=local_upgrade_level(progress.upgrade_ids[i]);if(!row)return false;
        if(extra&&*(int*)((char*)row+0x10)==*(int*)((char*)extra+0x10))included=true;if(candidate(row)&&!add(row))return false;}
    if(extra&&!included&&!add(extra))return false;if(!shape(next))return false;out=next;return true;
}
static bool purchase_supported(void* live){Effect a,b;Plan p;void* archived=live?local_upgrade_level(*(int*)((char*)live+0x10)):nullptr;
    return resolve(live,a)&&resolve(archived,b)&&a.faction==b.faction&&!memcmp(&a.delta,&b.delta,sizeof(a.delta))&&
        !*(int*)((char*)live+0x54)&&!*(int*)((char*)archived+0x54)&&collect(p,archived);}
static bool prepare(void* side,const Plan& p){
    if(!side||!shape(p))return false;if(!p.delta.radius&&!p.delta.bonus)return true;
    void* old_f=get(side,0xd8),*canonical=base(p.faction);if(!old_f||*(int*)((char*)old_f+0x10)!=p.faction||!canonical)return false;
    SupportAura values;if(!research_effects::support_aura_values(*(int*)((char*)canonical+0x44),*(int*)((char*)canonical+0x48),p.delta,values))return false;
    Roots roots;void* sources[128]={},*copies[128]={};int n=0;
    auto fraction=[&](void* source)->void*{
        if(!source||*(int*)((char*)source+0x10)!=p.faction)return nullptr;
        for(int i=0;i<n;++i)if(sources[i]==source)return copies[i];if(n==128)return nullptr;
        void* copy=local_research_runtime::clone(source,roots);if(!copy)return nullptr;
        if(p.delta.radius)*(int*)((char*)copy+0x44)=values.radius;if(p.delta.bonus)*(int*)((char*)copy+0x48)=values.bonus;
        sources[n]=source;copies[n++]=copy;return copy;};
    void* next_f=fraction(old_f);if(!next_f)return false;
    void* dictionaries[2]={};
    for(int kind=0;kind<2;++kind){bool building=kind==1;void* old=get(side,building?0xf8:0xf0);uintptr_t handle=0;
        void* list=local_dictionary_values(old,find_class("com.geargames.aow.entities",building?"BuildingType":"UnitType"),handle);
        if(!list||!roots.take(handle))return false;void* next=object_new(object_class(old));if(!roots.keep(next))return false;
        void* ex=nullptr;invoke_named(next,".ctor",0,nullptr,&ex);if(ex)return false;
        for(size_t j=0;j<array_length(list);++j){Roots one;void* type=((void**)((char*)list+32))[j],*copy=type,*f=get(type,0x20);if(!type)return false;
            if(f&&*(int*)((char*)f+0x10)==p.faction){copy=local_research_runtime::clone(type,one);if(!copy)return false;
                void* changed=fraction(f);if(!changed)return false;field_set(copy,class_field_from_name(object_class(copy),"fraction"),changed);}
            int id=*(int*)((char*)type+0x10);if(local_dictionary_item(old,id)!=type)return false;void* args[]={&id,copy};invoke_named(next,"Add",2,args,&ex);if(ex)return false;}
        dictionaries[kind]=next;
    }
    field_set(side,class_field_from_name(object_class(side),"$gg"),dictionaries[0]);field_set(side,class_field_from_name(object_class(side),"$Gg"),dictionaries[1]);
    field_set(side,class_field_from_name(object_class(side),"$Eg"),next_f);return local_research_runtime::relink_production_units(side);
}
static bool prepare_saved(void* side){Plan p;return collect(p)&&prepare(side,p);}
}
