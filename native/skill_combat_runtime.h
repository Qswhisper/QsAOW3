// Skill percentage mechanisms on private, research-prepared player prototypes.
// Uses actual SkillCard.Effect / 1000, never the rounded EffectVisual value.
#pragma once
#include "boost_conversion_policy.h"
#include "skill_chest_runtime.h"
namespace skill_combat {
using local_research_runtime::Roots;
static void* get(void* o,int n){return o?*(void**)((char*)o+n):nullptr;}
static bool supported(int card){return skill_chest::native_card(card)||local_progress::waived_skill_card(card)||((card==24||card==36||card==37)&&local_reward_skills::installed)||(card==25&&local_faction::active==2&&local_reward_skills::cooperative_credit_installed)||(card==27&&local_contracts::ready)||card==1||card==2||card==3||card==4||card==5||card==8||card==19||card==23||card==28||card==32||card==33||card==38||card==39||card==40||(card==30&&local_boost_conversion::installed);}
static bool protection(void* source,void* target,int offset,int amount){
    // Native CheckAndCalc.$GC reads ArmorBonus independently of raw Armor and
    // applies damage * (100 - ArmorBonus) / 100. Skill effects use /1000;
    // the retained protection cards have exact integer-percent increments.
    if(!amount)return true;if(amount<0||amount%10)return false;
    int old=*(int16_t*)((char*)source+offset),next=old+amount/10;
    if(old<0||next>INT16_MAX)return false;
    *(int16_t*)((char*)target+offset)=next;return true;
}
static bool scaled(int base,int amount,bool down,int& value){
    if(base<0||amount<0||(down&&amount>=1000))return false;
    int64_t n=down?int64_t(base)*(1000-amount)/1000:int64_t(base)+int64_t(base)*amount/1000;
    if(n<0||n>INT_MAX)return false;value=(int)n;return true;
}
struct Weapons {
    void* sources[2048]={},*copies[2048]={};int bonuses[2048]={},count=0;
    void* copy(void* source,int bonus,Roots& roots){
        if(!source||!bonus)return source;
        for(int i=0;i<count;++i)if(sources[i]==source&&bonuses[i]==bonus)return copies[i];
        if(count==2048)return nullptr;void* result=local_research_runtime::clone(source,roots);if(!result)return nullptr;
        for(int i=0;i<3;++i){int next=0;if(!scaled(*(int*)((char*)source+0x28+i*4),bonus,false,next))return nullptr;
            *(int*)((char*)result+0x28+i*4)=next;*(int*)((char*)result+0x34+i*4)=next;}
        for(const char* name:{"rounds_made","shots_made","target_impacts","target_impacts_precise","target_damage","off_target_impacts","off_target_impacts_precise","off_target_damage"}){
            void* field=class_field_from_name(object_class(source),name);void* old=field?get(source,field_offset(field)):nullptr;
            if(old){void* private_values=local_research_runtime::clone(old,roots);if(!private_values)return nullptr;field_set(result,field,private_values);}}
        void* ex=nullptr;invoke_named(result,"init",0,nullptr,&ex);if(ex)return nullptr;
        sources[count]=source;copies[count]=result;bonuses[count++]=bonus;return result;
    }
    void* list(void* source,int bonus,Roots& roots){
        if(!source)return nullptr;void* result=local_research_runtime::new_list(source,roots);if(!result)return nullptr;
        int n=*(int*)((char*)source+0x18);if(n<0||n>64)return nullptr;
        for(int i=0;i<n;++i){Roots one;void* old=list_item(source,i),*next=copy(old,bonus,one);
            if(!next||!local_skill::append(result,next))return nullptr;}
        return result;
    }
};
static int unit_card(void* type){
    int kind=*(int8_t*)((char*)type+0x31),category=*(int8_t*)((char*)type+0x30);
    // Hero/subordinate weapon routes require their own reviewed skill contract.
    if(kind<10||kind>=60)return 0;
    return category==1?1:(category==2||category==6)?32:category==3?33:category==4?19:0;
}
static int protection_card(void* type){
    // Original UnitType.Hero checks abilities.Count > 0 (get_Hero RVA45e9888).
    // Hero protection has its own card; ordinary unit card23 must not stack on it.
    void* abilities=get(type,0x98);
    if(abilities&&*(int*)((char*)abilities+0x18)>0)return 40;
    return unit_card(type)?23:0;
}
static int attack_card(void* type){
    // The original Hero predicate also covers the retained dependent forms.
    // Keep it distinct from ordinary category cards and their protection route.
    void* abilities=get(type,0x98);
    return abilities&&*(int*)((char*)abilities+0x18)>0?39:unit_card(type);
}
static void* unit(void* source,int bonus,Weapons& weapons,Roots& roots,int reduction=0){
    if(!bonus&&!reduction)return source;void* result=local_research_runtime::clone(source,roots),*old=get(source,0x18);
    void* states=old?local_research_runtime::new_list(old,roots):nullptr;if(!result||!states)return nullptr;
    int n=*(int*)((char*)old+0x18);if(n<1||n>32)return nullptr;
    for(int i=0;i<n;++i){Roots one;void* s=list_item(old,i),*copy=local_research_runtime::clone(s,one),*old_weapons=get(s,0x18);
        if(!copy||!protection(s,copy,0x24,reduction))return nullptr;
        if(bonus&&old_weapons){void* ws=weapons.list(old_weapons,bonus,one);if(!ws)return nullptr;local_skill::ref(copy,"weapons",ws);}
        if(!local_skill::append(states,copy))return nullptr;}
    local_skill::ref(result,"states",states);
    if(bonus&&attack_card(source)==39){
        void* original=get(source,0x98),*abilities=local_research_runtime::new_list(original,roots);int count=original?*(int*)((char*)original+0x18):0;
        if(!abilities||count<1||count>32)return nullptr;
        for(int i=0;i<count;++i){Roots one;void* ability=list_item(original,i),*copy=local_research_runtime::clone(ability,one),*weapon=get(ability,0x50);
            if(!copy)return nullptr;
            if(weapon){void* adapted=weapons.copy(weapon,bonus,one);if(!adapted)return nullptr;local_skill::ref(copy,"weapon",adapted);}
            if(!local_skill::append(abilities,copy))return nullptr;}
        // Preserve buff/effect percentages, timers, spawn counts and all other
        // ability fields. Shared state/active/passive weapon aliases use one copy.
        local_skill::ref(result,"ability",abilities);
    }
    return result;
}
static void* building(void* source,int attack,int construction,Weapons& weapons,Roots& roots,int reduction=0){
    if(!attack&&!construction&&!reduction)return source;void* result=local_research_runtime::clone(source,roots),*old=get(source,0x18);
    void* levels=old?local_research_runtime::new_list(old,roots):nullptr;if(!result||!levels)return nullptr;
    int n=*(int*)((char*)old+0x18);if(n<1||n>16)return nullptr;
    for(int i=0;i<n;++i){Roots one;void* level=list_item(old,i),*copy=local_research_runtime::clone(level,one);if(!copy||!protection(level,copy,0x3a,reduction))return nullptr;
        if(attack&&get(level,0x18)){void* ws=weapons.list(get(level,0x18),attack,one);if(!ws)return nullptr;local_skill::ref(copy,"weapons",ws);}
        if(construction&&*(int*)((char*)level+0x64)>0){int next=0;if(!scaled(*(int*)((char*)level+0x64),construction,true,next))return nullptr;
            *(int*)((char*)copy+0x64)=next;*(int*)((char*)copy+0x68)=next;}
        if(!local_skill::append(levels,copy))return nullptr;}
    local_skill::ref(result,"levels",levels);return result;
}
static bool project(void* side,const local_progress::State& state,void* unit_baseline,void* building_baseline){
    if(!state.skill_count)return skill_chest::prepare(side,0,0);if(!local_skill::initialize())return false;
    skill_tree_policy::Summary summary;if(!local_skill::graph.summarize(state,summary))return false;
    int bonus[100]={};for(int i=1;i<100;++i)if(summary.level[i]){
        if(!supported(i))return false;
        // Card38 is consumed by the original hall UpgradeCostCalculator.
        // It must stay neutral in the battle passive dictionary/prototypes.
        if(local_progress::waived_skill_card(i)||i==38||i==30||i==24||i==25||i==36||i==37)continue;
        void* card=local_career::skill_card(i);if(!card)return false;
        int64_t value=int64_t(*(int*)((char*)card+0x38))*summary.level[i];if(value<0||value>INT_MAX)return false;bonus[i]=(int)value;}
    Roots roots;Weapons weapons;void* candidate=local_research_runtime::clone(side,roots);if(!candidate)return false;
    void* dictionaries[]={unit_baseline,building_baseline};void* results[2]={};
    for(int k=0;k<2;++k){void* dictionary=dictionaries[k];if(!dictionary)return false;
        void* next=object_new(object_class(dictionary));if(!roots.keep(next))return false;
        void* ex=nullptr;invoke_named(next,".ctor",0,nullptr,&ex);if(ex)return false;results[k]=next;
        uintptr_t handle=0;void* values=local_dictionary_values(dictionary,find_class("com.geargames.aow.entities",k?"BuildingType":"UnitType"),handle);if(!values||!roots.take(handle))return false;
        for(size_t i=0;i<array_length(values);++i){Roots one;void* type=((void**)((char*)values+32))[i];if(!type)return false;
            int card=k?0:attack_card(type);
            int reduction=k?0:protection_card(type);
            void* adapted=k?building(type,bonus[2],bonus[3],weapons,one,bonus[8]):unit(type,bonus[card],weapons,one,bonus[reduction]);
            if(!adapted)return false;int id=*(int*)((char*)type+0x10);void* args[]={&id,adapted};invoke_named(next,"Add",2,args,&ex);if(ex)return false;}
    }
    local_skill::ref(candidate,"$gg",results[0]);local_skill::ref(candidate,"$Gg",results[1]);
    if(!local_research_runtime::relink_production_units(candidate)||!skill_chest::prepare(candidate,bonus[4],bonus[5],bonus))return false;
    local_skill::ref(side,"$gg",get(candidate,0xf0));local_skill::ref(side,"$Gg",get(candidate,0xf8));
    local_skill::ref(side,"$IH",get(candidate,0x3e8));*(int16_t*)((char*)side+0x218)=*(int16_t*)((char*)candidate+0x218);
    LOG("Local skill combat prepared=1 nodes=%u unique_weapon_projections=%d",state.skill_count,weapons.count);return true;
}
static bool prepare(void* side,const local_progress::State& state){return project(side,state,get(side,0xf0),get(side,0xf8));}
}
