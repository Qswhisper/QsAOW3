// Private HeroParam projection for class effects outside the hero's UnitType.
// Original ShotAct keeps its own chaining, attenuation and integer arithmetic.
#pragma once
namespace hero_parameters {
using Roots=local_research_runtime::Roots;
static bool collect(const local_progress::State& state,int faction,hero_effects::Delta& result){
    hero_effects::Delta total;
    for(int hero:{72,174}){
        if(!local_hero::card(hero,faction))continue;
        void* first=local_hero::first_level(hero,faction),*rows=local_hero::get(local_hero::get(first,0x48),0x18);
        if(!rows||array_length(rows)!=12)return false;
        generic_unit_research::Plan plan;bool gap=false;
        for(size_t i=0;i<array_length(rows);++i){void* row=((void**)((char*)rows+32))[i];
            if(!row)return false;
            if(!local_progress::has_upgrade(state,*(int*)((char*)row+0x10))){gap=true;continue;}
            generic_unit_research::Effect e;
            if(gap||!generic_unit_research::resolve(row,e)||!plan.add(e))return false;
        }
        if(plan.count>1||(plan.count&&!hero_effects::add(total,plan.families[0].ability)))return false;
    }
    result=total;return true;
}
static void* project(void* source,const local_progress::State& state,int faction,Roots& roots){
    hero_effects::Delta delta;if(!collect(state,faction,delta))return nullptr;
    if(!delta.chain_distance&&!delta.chain_loss_reduction&&!delta.salamander_speed)return source;
    return generic_unit_research::copy_hero_parameter(source,delta,roots);
}
static void* (*get_original)(void*,void*);
static void* cached_source,*cached_copy;static uintptr_t cached_root;
static int cached_distance,cached_loss,cached_speed,cached_faction;
static void* get(void* self,void* method){
    void* source=get_original(self,method);
    // Battle providers may already expose a seat-private projected parameter.
    // Only the immutable main dictionary needs this presentation projection.
    if(!offline_network||!progress_ready||!source||source!=local_career::list("BATTLE","heroParam"))return source;
    hero_effects::Delta delta;
    if(!collect(progress,local_faction::active,delta)){LOG("Local hero shared parameter presentation refused invalid class ledger");return nullptr;}
    if(!delta.chain_distance&&!delta.chain_loss_reduction&&!delta.salamander_speed)return source;
    if(cached_source==source&&cached_faction==local_faction::active&&cached_distance==delta.chain_distance&&cached_loss==delta.chain_loss_reduction&&cached_speed==delta.salamander_speed)return cached_copy;
    Roots roots;void* copy=generic_unit_research::copy_hero_parameter(source,delta,roots);if(!copy)return nullptr;
    uintptr_t root=gchandle_new(copy,true);if(!root)return nullptr;
    if(cached_root)gchandle_free(cached_root);
    cached_source=source;cached_copy=copy;cached_root=root;cached_faction=local_faction::active;
    cached_distance=delta.chain_distance;cached_loss=delta.chain_loss_reduction;cached_speed=delta.salamander_speed;return copy;
}
static void install(){
    const uint32_t signature[]={0xf81e0ffe,0xa9014ff4,0xf0010374,0xaa0003f3};
    get_original=(decltype(get_original))install_hook(0x7d42214,signature,(void*)get);
    LOG("Local hero shared parameter presentation hook=%d canonical_source_preserved=1",get_original!=nullptr);
}
}
