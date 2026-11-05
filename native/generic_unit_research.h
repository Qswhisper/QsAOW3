#include "composite_research_effects.h"
// Shared unit-family adapter. Included after the retained legacy helpers.
#pragma once
#include "unit_stat_effects.h"
#include "weapon_research_effects.h"
#include "weapon_route_contracts.h"
#include "hero_ability_effects.h"
#include "state_armor_effects.h"
#include "unit_transition_effects.h"
#include "weapon_group_effects.h"
#include "unit_energy_effects.h"
#include "local_mobility_runtime.h"
namespace generic_unit_research {
using local_research_runtime::Roots;
using research_effects::UnitStats;
struct WeaponKey {int name=0,type=0,refs=0;};
struct Effect {void* owner=nullptr;int local_turn=0;UnitStats delta;research_effects::StateArmor state_armor;research_effects::UnitTransition transition;research_effects::UnitEnergy energy;int range[2]={},explosion[2]={},reload[2]={},accuracy[2][3]={},shot_start[2]={};int hero_class=0;bool hero_ability=false;hero_effects::Delta ability;bool legacy_damage=false;WeaponKey targets[2];int target_count=0;const char* reason="not_unit_upgrade";};
static bool same_key(const WeaponKey& a,const WeaponKey& b){return a.name==b.name&&a.type==b.type;}
static bool weapon_matches(void* weapon,const WeaponKey& key){return weapon&&*(int16_t*)((char*)weapon+0x20)==key.name&&*(int16_t*)((char*)weapon+0x24)==key.type;}
#include "weapon_group_routes.h"
static int target_index(void* weapon,const Effect& e){for(int i=0;i<e.target_count;++i)if(weapon_matches(weapon,e.targets[i]))return i;return -1;}
static bool select_weapon(void* type,void* upgrade,WeaponKey& key,const char*& reason,bool common_only=false) {
    if(!type)return false;
    void* abilities=*(void**)((char*)type+0x98);
    if(abilities&&*(int*)((char*)abilities+0x18)){reason="weapon_ability_routing_unverified";return false;}
    void* states=*(void**)((char*)type+0x18);int n=states?*(int*)((char*)states+0x18):0;
    if(n<1||n>32)return false;
    bool common=true;int total=0;void* first=nullptr;WeaponKey shared;
    for(int i=0;i<n;++i) {
        void* state=list_item(states,i);void* list=state?*(void**)((char*)state+0x18):nullptr;
        int count=list?*(int*)((char*)list+0x18):0;if(!state||count<0||count>32)return false;
        for(int j=0;j<count;++j) {
            void* w=list_item(list,j);if(!w)return false;++total;
            WeaponKey current{*(int16_t*)((char*)w+0x20),*(int16_t*)((char*)w+0x24),0};
            if(!first){first=w;shared=current;}
            else if(!same_key(shared,current)||memcmp((char*)first+0x28,(char*)w+0x28,12))common=false;
        }
    }
    if(first&&common&&shared.name>0&&shared.type>0){key=shared;key.refs=total;reason="shared_weapon_damage";return true;}
    if(common_only)return false;
    void* f=*(void**)((char*)type+0x20);char model[256];
    if(!f||!local_research_runtime::ascii(*(void**)((char*)type+0x60),model,sizeof(model)))return false;
    for(const auto& route:route_contracts)if(route.faction==*(int*)((char*)f+0x10)&&route.owner==*(int*)((char*)type+0x10)&&
        route.slot==*((int8_t*)upgrade+0x21)&&!strcmp(route.model,model)) {
        WeaponKey selected{route.name,route.type,0};
        for(int i=0;i<n;++i){void* list=*(void**)((char*)list_item(states,i)+0x18);int count=list?*(int*)((char*)list+0x18):0;
            for(int j=0;j<count;++j)if(weapon_matches(list_item(list,j),selected))++selected.refs;}
        if(!selected.refs)return false;key=selected;reason="reviewed_weapon_route";return true;
    }
    return false;
}
static void* levels(const char* source) {
    void* config=saved(source);void* f=config?class_field_from_name(object_class(config),"upgradeLevels"):nullptr;
    return f?*(void**)((char*)config+field_offset(f)):nullptr;
}
static bool identity(void* a,void* b) {
    if(!a||!b)return false;
    void* fa=*(void**)((char*)a+0x20);void* fb=*(void**)((char*)b+0x20);
    char ma[256],mb[256];
    return fa&&fb&&*(int*)((char*)fa+0x10)==*(int*)((char*)fb+0x10)&&
        *((int8_t*)a+0x31)==*((int8_t*)b+0x31)&&
        local_research_runtime::ascii(*(void**)((char*)a+0x60),ma,sizeof(ma))&&
        local_research_runtime::ascii(*(void**)((char*)b+0x60),mb,sizeof(mb))&&ma[0]&&!strcmp(ma,mb);
}
#include "state_armor_runtime.h"
#include "hero_ability_runtime.h"
static bool resolve(void* level,Effect& effect) {
    void* upgrade=level?*(void**)((char*)level+0x48):nullptr;
    if(!upgrade||strcmp(class_name(object_class(upgrade)),"UpgradeUnit"))return false;
    effect.owner=*(void**)((char*)upgrade+0x48);
    if(!effect.owner){effect.reason="missing_unit_owner";return false;}
    if(!identity(effect.owner,effect.owner)){effect.reason="missing_unit_identity";return false;}
    if(*((int8_t*)upgrade+0x21)==20){int hero=0,amount=0;void* fraction=*(void**)((char*)effect.owner+0x20);
        if(fraction&&local_hero::acquisition(level,*(int*)((char*)fraction+0x10),hero,amount)){
            effect.hero_class=1;effect.reason="original_first_hero_acquisition";return true;}
        effect.reason="hero_class_effect_not_implemented";
        int hero_id=*(int*)((char*)effect.owner+0x10);if(!hero_effects::supported(hero_id))return false;
        void* rows=local_hero::get(upgrade,0x18);if(!rows||array_length(rows)!=12)return false;
        int index=-1;for(int i=1;i<12;++i){void* row=((void**)((char*)rows+32))[i];if(row&&*(int*)((char*)row+0x10)==*(int*)((char*)level+0x10))index=i;}
        char text[2048];if(index<1||!local_research_runtime::ascii(local_hero::get(level,0x18),text,sizeof(text))||!hero_effects::parse(text,effect.ability,hero_id))return false;
        effect.hero_class=index+1;effect.hero_ability=true;effect.reason=hero_id==75?"original_atlant_weapon_frontal_shield_class":hero_id==174?"original_salamander_weapon_shared_speed_recovery_class":hero_id==72?"original_solaris_dual_weapon_shared_chain_parameters":hero_id==173?"original_gatling_dual_weapon_smoke_aura_class":hero_id==73?"original_beholder_radius_duration_effect_class":(hero_id==170?"original_wasp_weapon_corrosion_smoke_class":(hero_id==172?"original_leviathan_missile_shield_class":(hero_id==71?"original_seraphim_weapon_passive_class":"original_cerberus_third_weapon_class")));
        return hero_ability_shape(effect.owner,effect);
    }
    char desc[4096];
    const char* original=local_mobility_runtime::original(level);
    if(original)snprintf(desc,sizeof(desc),"%s",original);
    else if(!local_research_runtime::ascii(*(void**)((char*)level+0x18),desc,sizeof(desc))){effect.reason="non_ascii_or_long_description";return false;}
    if(const auto* mobility=local_mobility_runtime::row(level)){
        char other[4096];if(!local_mobility::strip(*mobility,desc,other,sizeof(other))){effect.reason="local_mobility_original_identity_or_description_mismatch";return false;}
        memcpy(desc,other,strlen(other)+1);effect.local_turn=1;
        if(!*desc){effect.reason="approved_local_turn_adaptation";return true;}
    }
    if(research_effects::unit_energy_parse(desc,effect.energy)){effect.reason="original_unit_energy_and_jump";return true;}
    int unknown=0;auto parsed=research_effects::unit_stats_parse(desc,effect.delta,unknown);
    if(parsed==research_effects::UnitParse::ok){effect.reason="flat_unit_stats";return true;}
    int transition_profile=0;
    if(research_effects::unit_transition_parse(desc,effect.transition,&transition_profile)){
        int slot=*((int8_t*)upgrade+0x21);void* f=*(void**)((char*)effect.owner+0x20);
        if(transition_profile&&(!f||*(int*)((char*)effect.owner+0x10)!=(transition_profile==1?17:117)||
            *(int8_t*)((char*)effect.owner+0x31)!=(transition_profile==1?22:23)||*(int*)((char*)f+0x10)!=transition_profile))return false;
        effect.reason=transition_profile==1?"original_shield_activation_cooldown_ticks":transition_profile==2?"original_fog_activation_cooldown_ticks":"original_unit_mode_transition_ticks";return slot>=0&&slot<20;
    }
    if(research_effects::state_armor_parse(desc,effect.state_armor)){
        int deltas[2];effect.reason="reviewed_original_state_armor_pair";
        int slot=*((int8_t*)upgrade+0x21);
        return slot>=0&&slot<20&&state_armor_deltas(effect.owner,effect,deltas);
    }
    // Preserve the previously verified two-weapon rifle profile. Do not infer
    // weapon routing from another unit's UI slot or silently accept half a level.
    void* legacy_owner=nullptr;UnitStats legacy;
    if(local_research_runtime::unit_level_delta(level,legacy,legacy_owner)) {
        effect.delta=legacy;effect.legacy_damage=true;effect.target_count=2;
        void* state=list_item(*(void**)((char*)effect.owner+0x18),0);void* weapons=state?*(void**)((char*)state+0x18):nullptr;
        if(!weapons||*(int*)((char*)weapons+0x18)!=2)return false;
        for(int i=0;i<2;++i){void* w=list_item(weapons,i);if(!w)return false;
            effect.targets[i]={*(int16_t*)((char*)w+0x20),*(int16_t*)((char*)w+0x24),1};}
        effect.reason="verified_legacy_rifle_profile";return true;
    }
    research_effects::WeaponGroups grouped;
    if(research_effects::weapon_groups_parse(desc,grouped)){
        int slot=*((int8_t*)upgrade+0x21);WeaponKey keys[2];
        effect.reason="explicit_weapon_group_identity_unverified";
        if(slot<0||slot>=20)return false;
        if(!grouped_weapon_keys(effect.owner,keys)){
            // A sole canonical weapon can only be weapon I. Do not use a
            // slot-based ordinary route as an inferred I/II identity.
            if(!grouped.used[0]||grouped.used[1]||!select_weapon(effect.owner,upgrade,keys[0],effect.reason,true))return false;
        }
        effect.delta=grouped.scalar;
        for(int group=0;group<2;++group)if(grouped.used[group]){
            int index=effect.target_count++;effect.targets[index]=keys[group];const auto& d=grouped.weapons[group];
            for(int t=0;t<3;++t){effect.delta.damage[index][t]=d.damage[t];effect.accuracy[index][t]=d.accuracy[t];}
            effect.range[index]=d.range;effect.explosion[index]=d.explosion;effect.reload[index]=d.reload;effect.shot_start[index]=d.shot_start;
        }
        effect.reason="explicit_original_weapon_groups";return true;
    }
    research_effects::WeaponParameters parameters;
    UnitStats composite_scalar;bool composed=research_effects::composite_unit_weapon_parse(desc,composite_scalar,parameters);
    if(composed||research_effects::weapon_parameters_parse(desc,parameters)) {
        if(composed)effect.delta=composite_scalar;
        int slot=*((int8_t*)upgrade+0x21);
        if(slot<0||slot>=20){effect.reason="weapon_skill_or_invalid_slot";return false;}
        effect.reason="weapon_target_ambiguous";
        if(!select_weapon(effect.owner,upgrade,effect.targets[0],effect.reason))return false;
        effect.target_count=1;if(composed)effect.reason="reviewed_composite_unit_and_weapon";
        for(int t=0;t<3;++t)effect.delta.damage[0][t]=parameters.damage[t];
        effect.range[0]=parameters.range;effect.explosion[0]=parameters.explosion;effect.reload[0]=parameters.reload;effect.shot_start[0]=parameters.shot_start;
        for(int i=0;i<3;++i)effect.accuracy[0][i]=parameters.accuracy[i];
        return true;
    }
    effect.reason=research_effects::unit_parse_reason(parsed);return false;
}
static bool sum_int(int base,int delta,int max,int& result) {
    int64_t value=(int64_t)base+delta;
    if(base<0||delta<0||value>max)return false;result=(int)value;return true;
}
static bool accuracy_values(void* weapon,const int (&improvement)[3],int (&output)[3],const char*& reason){
    if(!weapon)return false;const int offsets[]={0x8e,0x92,0x90};int base[3];
    for(int k=0;k<3;++k)base[k]=*(int16_t*)((char*)weapon+offsets[k]);
    return research_effects::weapon_accuracy_apply(*(int16_t*)((char*)weapon+0x24),
        *(int8_t*)((char*)weapon+0x94),*(int8_t*)((char*)weapon+0x8a),base,improvement,output,reason);
}
static bool shape(void* type,const Effect& e,const char*& reason) {
    reason="prototype_shape_or_range";
    if(!type||e.local_turn<0||e.local_turn>6)return false;int next=0;int mode_armor[2];
    if(!state_armor_deltas(type,e,mode_armor)){reason="state_armor_family_or_mode_mismatch";return false;}
    if(e.hero_class&&(e.hero_class<1||e.hero_class>12||*(int*)((char*)type+0xa0)!=0||!hero_ability_shape(type,e)))return false;
    if(e.delta.hp&&(*(int*)((char*)type+0x34)<=0||!sum_int(*(int*)((char*)type+0x34),e.delta.hp,INT16_MAX,next)||next<=0))return false;
    if(e.transition.to||e.transition.from){void* states=*(void**)((char*)type+0x18);int a,b;
        if(!states||*(int*)((char*)states+0x18)!=2||!research_effects::unit_transition_apply(
            *(int8_t*)((char*)type+0x4d),*(int8_t*)((char*)type+0x4e),e.transition,a,b)){reason="unit_transition_modes_or_timer_bounds";return false;}}
    if(e.target_count<0||e.target_count>2)return false;
    for(int w=e.target_count;w<2;++w){if(e.range[w]||e.explosion[w]||e.reload[w]||e.shot_start[w])return false;for(int t=0;t<3;++t)if(e.delta.damage[w][t]||e.accuracy[w][t])return false;}
    int hits[2]={},energy_recovery_targets=0;
    for(int i=0;i<e.target_count;++i)if(e.targets[i].name<=0||e.targets[i].type<=0||e.targets[i].refs<1||e.targets[i].refs>1024||
        (i&&same_key(e.targets[0],e.targets[1])))return false;
    if(!e.delta.armor&&!e.state_armor.group&&!e.delta.speed&&!e.delta.sight&&!e.delta.regen&&!e.target_count&&!e.local_turn&&!research_effects::unit_energy_any(e.energy))return true;
    void* states=*(void**)((char*)type+0x18);int n=states?*(int*)((char*)states+0x18):0;
    if(n<1||n>32)return false;
    for(int i=0;i<n;++i) {
        void* s=list_item(states,i);if(!s)return false;
        if(e.local_turn&&*(int16_t*)((char*)s+0x5a)<0){reason="local_turn_negative_native_baseline";return false;}
        if(research_effects::unit_energy_any(e.energy)){research_effects::UnitEnergy values;
            if(e.energy.recovery&&*(int16_t*)((char*)s+0x8c)>0)++energy_recovery_targets;
            if(*(int8_t*)((char*)type+0x31)>=60||!research_effects::unit_energy_values(*(int16_t*)((char*)s+0x88),
                *(int16_t*)((char*)s+0x8c),*(int8_t*)((char*)s+0x77),e.energy,values)){reason="unit_energy_or_jump_baseline_and_bounds";return false;}}
        if(e.delta.armor&&!sum_int(*(int16_t*)((char*)s+0x22),e.delta.armor,INT16_MAX,next))return false;
        if(e.state_armor.group){int combined=0;
            if(!sum_int(e.delta.armor,mode_armor[i],INT16_MAX,combined)||!sum_int(*(int16_t*)((char*)s+0x22),combined,INT16_MAX,next))return false;}
        // A zero-speed deployment state needs a verified state selector first.
        if(e.delta.speed&&(*(int8_t*)((char*)s+0x58)<=0||!sum_int(*(int8_t*)((char*)s+0x58),e.delta.speed,INT8_MAX,next))){reason="immobile_state_or_speed_range";return false;}
        if(e.delta.sight&&!sum_int(*(int*)((char*)s+0x4c),e.delta.sight,INT_MAX,next))return false;
        if(e.delta.regen&&!sum_int(*(int16_t*)((char*)s+0x72),e.delta.regen,INT16_MAX,next))return false;
        if(e.target_count) {
            void* weapons=*(void**)((char*)s+0x18);int count=weapons?*(int*)((char*)weapons+0x18):0;
            if(count<0||count>32)return false;
            for(int w=0;w<count;++w){void* weapon=list_item(weapons,w);if(!weapon)return false;int target=target_index(weapon,e);
                if(target>=0){++hits[target];
                    if(e.accuracy[target][0]||e.accuracy[target][1]||e.accuracy[target][2]){
                        int accuracy[3];if(!accuracy_values(weapon,e.accuracy[target],accuracy,reason))return false;
                    }
                    if(e.reload[target]&&(*(bool*)((char*)weapon+0x14)||e.reload[target]<0||*(int16_t*)((char*)weapon+0x66)<=e.reload[target])){
                        reason="weapon_reload_baseline_or_minimum";return false;}
                    if(e.shot_start[target]&&(*(bool*)((char*)weapon+0x14)||!research_effects::weapon_shot_start(
                        *(int16_t*)((char*)weapon+0x60),e.shot_start[target],next))){reason="weapon_prefire_baseline_or_underflow";return false;}
                    if(e.range[target]||e.explosion[target]){
                        if(*(bool*)((char*)weapon+0x14)){reason="weapon_range_requires_uninitialized_baseline";return false;}
                        int range=0,radius=0;
                        if(!sum_int(*(int*)((char*)weapon+0x4c),e.range[target],INT_MAX,range)||
                           !sum_int(*(int*)((char*)weapon+0x7c),e.explosion[target],46340,radius))return false;
                        int64_t padded=range>1000?int64_t(range)+50:int64_t(range)*105/100;
                        if(range<=0||padded>46340){reason="weapon_native_distance_square_overflow";return false;}
                    }
                    for(int t=0;t<3;++t)
                    if(!sum_int(*(int*)((char*)weapon+0x28+t*4),e.delta.damage[target][t],INT_MAX,next))return false;}}
        }
    }
    for(int i=0;i<e.target_count;++i)if(hits[i]!=e.targets[i].refs){reason="weapon_target_reference_mismatch";return false;}
    if(e.energy.recovery&&!energy_recovery_targets){reason="unit_energy_recovery_disabled_in_all_states";return false;}
    return true;
}
static void* copy_type(void* source,const Effect& e,Roots& roots,bool battle=false) {
    const char* reason=nullptr;if(!shape(source,e,reason))return nullptr;
    void* copy=local_research_runtime::clone(source,roots);if(!copy)return nullptr;
    if(e.hero_class)*(int*)((char*)copy+0xa0)=e.hero_class;
    if(!copy_hero_abilities(copy,source,e,roots,battle))return nullptr;
    if(e.delta.hp) {
        int hp=*(int*)((char*)source+0x34)+e.delta.hp;
        *(int16_t*)((char*)copy+0x32)=(int16_t)hp;*(int*)((char*)copy+0x34)=hp;
    }
    if(e.transition.to||e.transition.from){int to,from;
        if(!research_effects::unit_transition_apply(*(int8_t*)((char*)source+0x4d),*(int8_t*)((char*)source+0x4e),e.transition,to,from))return nullptr;
        *(int8_t*)((char*)copy+0x4d)=to;*(int8_t*)((char*)copy+0x4e)=from;}
    if(!e.delta.armor&&!e.state_armor.group&&!e.delta.speed&&!e.delta.sight&&!e.delta.regen&&!e.target_count&&!e.local_turn&&!research_effects::unit_energy_any(e.energy))return copy;
    int mode_armor[2];if(!state_armor_deltas(source,e,mode_armor))return nullptr;
    void* old=*(void**)((char*)source+0x18);void* states=local_research_runtime::new_list(old,roots);
    if(!states)return nullptr;void* exception=nullptr;
    // Keep aliases across states and duplicate weapon slots. A source weapon
    // is copied once; distinct source instances remain distinct instances.
    void* weapon_sources[1024]={};void* weapon_copies[1024]={};int weapon_copy_count=0;
    for(int i=0;i<*(int*)((char*)old+0x18);++i) {
        Roots sr;void* base=list_item(old,i);void* state=local_research_runtime::clone(base,sr);if(!state)return nullptr;
        *(int16_t*)((char*)state+0x22)+=e.delta.armor;
        if(e.state_armor.group)*(int16_t*)((char*)state+0x22)+=mode_armor[i];
        *(int8_t*)((char*)state+0x58)+=e.delta.speed;
        if(e.local_turn)*(int16_t*)((char*)state+0x5a)=local_mobility::apply(*(int16_t*)((char*)base+0x5a),e.local_turn);
        *(int*)((char*)state+0x4c)+=e.delta.sight;
        *(int16_t*)((char*)state+0x72)+=e.delta.regen;
        if(research_effects::unit_energy_any(e.energy)){research_effects::UnitEnergy values;
            if(!research_effects::unit_energy_values(*(int16_t*)((char*)base+0x88),*(int16_t*)((char*)base+0x8c),
                *(int8_t*)((char*)base+0x77),e.energy,values))return nullptr;
            *(int16_t*)((char*)state+0x88)=values.capacity;
            *(int16_t*)((char*)state+0x8c)=values.recovery;*(int16_t*)((char*)state+0x8e)=values.recovery;
            *(int8_t*)((char*)state+0x77)=values.jump;}
        if(e.target_count) {
            void* old_weapons=*(void**)((char*)base+0x18);void* weapons=old_weapons?local_research_runtime::new_list(old_weapons,sr):nullptr;
            if(old_weapons&&!weapons)return nullptr;
            int count=old_weapons?*(int*)((char*)old_weapons+0x18):0;
            for(int w=0;w<count;++w) {
                Roots wr;void* original=list_item(old_weapons,w);int target=target_index(original,e);void* weapon=original;
                if(target>=0){weapon=nullptr;
                    for(int k=0;k<weapon_copy_count;++k)if(weapon_sources[k]==original){weapon=weapon_copies[k];break;}
                    if(!weapon){if(weapon_copy_count==1024)return nullptr;weapon=local_research_runtime::clone(original,wr);if(!weapon)return nullptr;
                        for(int t=0;t<3;++t)*(int*)((char*)weapon+0x28+t*4)+=e.delta.damage[target][t];
                        *(int*)((char*)weapon+0x4c)+=e.range[target];*(int*)((char*)weapon+0x7c)+=e.explosion[target];
                        *(int16_t*)((char*)weapon+0x66)-=e.reload[target];
                        *(int16_t*)((char*)weapon+0x60)-=e.shot_start[target];
                        if(e.accuracy[target][0]||e.accuracy[target][1]||e.accuracy[target][2]){
                            int accuracy[3];const char* why=nullptr;if(!accuracy_values(original,e.accuracy[target],accuracy,why))return nullptr;
                            *(int16_t*)((char*)weapon+0x8e)=accuracy[0];
                            *(int16_t*)((char*)weapon+0x92)=accuracy[1];
                            *(int16_t*)((char*)weapon+0x90)=accuracy[2];}
                        weapon_sources[weapon_copy_count]=original;weapon_copies[weapon_copy_count++]=weapon;}}
                void* args[]={weapon};invoke_named(weapons,"Add",1,args,&exception);if(exception)return nullptr;
            }
            field_set(state,class_field_from_name(object_class(state),"weapons"),weapons);
        }
        void* args[]={state};invoke_named(states,"Add",1,args,&exception);if(exception)return nullptr;
    }
    field_set(copy,class_field_from_name(object_class(copy),"states"),states);return copy;
}
struct Plan {
    Effect families[128];int count=0;
    bool add(const Effect& e) {
        for(int i=0;i<count;++i)if(families[i].owner==e.owner) {
            Effect next=families[i];UnitStats flat_a=next.delta,flat_b=e.delta;
            next.local_turn=std::min(6,next.local_turn+e.local_turn);
            if(!research_effects::state_armor_add(next.state_armor,e.state_armor,next.state_armor))return false;
            if(!research_effects::unit_transition_add(next.transition,e.transition,next.transition))return false;
            if(!research_effects::unit_energy_add(next.energy,e.energy,next.energy))return false;
            if(e.hero_class){if(e.hero_class!=next.hero_class+1)return false;next.hero_class=e.hero_class;}
            if(e.hero_ability){if(!hero_effects::add(next.ability,e.ability))return false;next.hero_ability=true;}
            memset(flat_a.damage,0,sizeof(flat_a.damage));memset(flat_b.damage,0,sizeof(flat_b.damage));
            UnitStats flat;if(!research_effects::rifle_add(flat_a,flat_b,flat))return false;
            next.delta.hp=flat.hp;next.delta.armor=flat.armor;next.delta.speed=flat.speed;next.delta.sight=flat.sight;next.delta.regen=flat.regen;
            for(int w=0;w<e.target_count;++w) {
                int index=-1;for(int j=0;j<next.target_count;++j)if(same_key(next.targets[j],e.targets[w]))index=j;
                if(index<0){if(next.target_count==2)return false;index=next.target_count++;next.targets[index]=e.targets[w];}
                else if(next.targets[index].refs!=e.targets[w].refs)return false;
                for(int t=0;t<3;++t){int64_t total=(int64_t)next.delta.damage[index][t]+e.delta.damage[w][t];
                    if(total<0||total>INT_MAX)return false;next.delta.damage[index][t]=(int)total;}
                if(!sum_int(next.range[index],e.range[w],INT_MAX,next.range[index])||
                   !sum_int(next.explosion[index],e.explosion[w],INT_MAX,next.explosion[index])||
                   !sum_int(next.reload[index],e.reload[w],INT16_MAX,next.reload[index])||
                   !sum_int(next.shot_start[index],e.shot_start[w],INT16_MAX,next.shot_start[index]))return false;
                for(int k=0;k<3;++k)if(!sum_int(next.accuracy[index][k],e.accuracy[w][k],100,next.accuracy[index][k]))return false;
            }
            next.legacy_damage|=e.legacy_damage;families[i]=next;return true;
        }
        if(count==128||(e.hero_class&&e.hero_class!=1))return false;
        research_effects::StateArmor checked;
        if(!research_effects::state_armor_add({},e.state_armor,checked))return false;
        families[count++]=e;return true;
    }
};
static bool collect(Plan& plan,void* extra=nullptr) {
    bool included=false;
    for(uint32_t i=0;i<progress.upgrade_count;++i) {
        void* level=local_upgrade_level(progress.upgrade_ids[i]);if(!level)return false;
        if(extra&&*(int*)((char*)level+0x10)==*(int*)((char*)extra+0x10))included=true;
        void* upgrade=*(void**)((char*)level+0x48);if(!upgrade)return false;
        if(strcmp(class_name(object_class(upgrade)),"UpgradeUnit"))continue;
        if(mine_research::supported_saved(level))continue;
        if(mine_growth::supported_saved(level))continue;
        if(support_ability::supported_saved(level))continue;
        Effect e;if(!resolve(level,e)||!plan.add(e))return false;
    }
    if(extra&&!included){Effect e;if(!resolve(extra,e)||!plan.add(e))return false;}
    for(int i=0;i<plan.count;++i){const char* reason=nullptr;if(!shape(plan.families[i].owner,plan.families[i],reason))return false;}
    return true;
}
static bool purchase_supported(void* live) {
    void* archived=live?local_upgrade_level(*(int*)((char*)live+0x10)):nullptr;
    Effect a,b;if(!archived||!resolve(live,a)||!resolve(archived,b)||!identity(a.owner,b.owner)||
        *(int*)((char*)a.owner+0x10)!=*(int*)((char*)b.owner+0x10)||
        !research_effects::rifle_equal(a.delta,b.delta)||a.local_turn!=b.local_turn||memcmp(&a.state_armor,&b.state_armor,sizeof(a.state_armor))||memcmp(&a.transition,&b.transition,sizeof(a.transition))||memcmp(&a.energy,&b.energy,sizeof(a.energy))||memcmp(a.range,b.range,sizeof(a.range))||memcmp(a.explosion,b.explosion,sizeof(a.explosion))||memcmp(a.reload,b.reload,sizeof(a.reload))||memcmp(a.shot_start,b.shot_start,sizeof(a.shot_start))||memcmp(a.accuracy,b.accuracy,sizeof(a.accuracy))||a.hero_class!=b.hero_class||a.legacy_damage!=b.legacy_damage||a.target_count!=b.target_count)return false;
    if(*(int*)((char*)live+0x54)||*(int*)((char*)archived+0x54)){
        int hero_a=0,hero_b=0,cards_a=0,cards_b=0;
        if(!a.hero_class||a.hero_class!=b.hero_class||a.hero_ability!=b.hero_ability||memcmp(&a.ability,&b.ability,sizeof(a.ability)))return false;
        if(a.hero_class==1&&(!local_hero::acquisition(live,local_faction::active,hero_a,cards_a)||!local_hero::acquisition(archived,local_faction::active,hero_b,cards_b)||hero_a!=hero_b||cards_a!=cards_b))return false;
    }
    for(int i=0;i<a.target_count;++i)if(!same_key(a.targets[i],b.targets[i])||a.targets[i].refs!=b.targets[i].refs)return false;
    Plan plan;return collect(plan,archived);
}
static void* display_projection(void* type,const local_progress::State& state,int faction,local_hero::Roots& retained){
    if(!type)return nullptr;int hero=*(int*)((char*)type+0x10);
    if(!hero_effects::supported(hero)||!local_hero::card(hero,faction))return nullptr;
    void* first=local_hero::first_level(hero,faction),*upgrade=local_hero::get(first,0x48),*rows=local_hero::get(upgrade,0x18);
    Plan plan;bool gap=false;
    for(size_t i=0;rows&&i<array_length(rows);++i){void* row=((void**)((char*)rows+32))[i];if(!row)return nullptr;
        if(!local_progress::has_upgrade(state,*(int*)((char*)row+0x10))){gap=true;continue;}
        Effect e;if(gap||!resolve(row,e)||!plan.add(e))return nullptr;}
    Roots roots;void* result=plan.count?copy_type(type,plan.families[0],roots):local_research_runtime::clone(type,roots);
    return retained.keep(result)?result:nullptr;
}
// The caller retains this immutable baseline for deterministic reapplication.
// All unit/type/list copies and producer links are staged on a private side;
// failure publishes nothing to the actual side.
static void publish_hero_parameter(void* side,void* prepared){
    field_set(side,class_field_from_name(object_class(side),"$vG"),*(void**)((char*)prepared+0x238));
}
static bool apply(void* side,void* baseline,const Plan& plan,int& matched,void* baseline_parameter=nullptr) {
    matched=0;if(!side||!baseline)return false;Roots roots;
    void* candidate=local_research_runtime::clone(side,roots);if(!candidate)return false;
    uintptr_t handle=0;void* values=local_dictionary_values(baseline,find_class("com.geargames.aow.entities","UnitType"),handle);
    if(!values||!roots.take(handle))return false;
    int indexes[128];for(int i=0;i<plan.count;++i)indexes[i]=-1;
    for(size_t j=0;j<array_length(values);++j) {
        void* type=((void**)((char*)values+32))[j];if(!type)return false;
        int claims=0;
        for(int i=0;i<plan.count;++i)if(identity(type,plan.families[i].owner)) {
            if(++claims>1)return false;
            if(indexes[i]>=0){LOG("Generic research ambiguous unit family=%d",*(int*)((char*)plan.families[i].owner+0x10));return false;}
            indexes[i]=(int)j;++matched;
        }
    }
    // Parameter effects share the same immutable-baseline contract as units.
    // An absent hero family cannot modify unrelated units on this side.
    hero_effects::Delta parameter_delta;
    for(int i=0;i<plan.count;++i)if(indexes[i]>=0){
        int hero=*(int*)((char*)plan.families[i].owner+0x10);
        if(hero==72){parameter_delta.chain_distance=plan.families[i].ability.chain_distance;
            parameter_delta.chain_loss_reduction=plan.families[i].ability.chain_loss_reduction;}
        if(hero==174)parameter_delta.salamander_speed=plan.families[i].ability.salamander_speed;
    }
    void* parameter=nullptr;
    if(parameter_delta.chain_distance||parameter_delta.chain_loss_reduction||parameter_delta.salamander_speed){
        parameter=copy_hero_parameter(baseline_parameter,parameter_delta,roots);if(!parameter)return false;
        field_set(candidate,class_field_from_name(object_class(candidate),"$vG"),parameter);
    }
    void* replacement=object_new(object_class(baseline));if(!roots.keep(replacement))return false;
    void* exception=nullptr;invoke_named(replacement,".ctor",0,nullptr,&exception);if(exception)return false;
    for(size_t j=0;j<array_length(values);++j) {
        Roots tr;void* type=((void**)((char*)values+32))[j];void* published=type;
        for(int i=0;i<plan.count;++i)if(indexes[i]==(int)j){published=copy_type(type,plan.families[i],tr,true);if(!published)return false;}
        int id=*(int*)((char*)type+0x10);void* args[]={&id,published};invoke_named(replacement,"Add",2,args,&exception);if(exception)return false;
    }
    field_set(candidate,class_field_from_name(object_class(candidate),"$gg"),replacement);
    if(!local_research_runtime::relink_production_units(candidate))return false;
    field_set(side,class_field_from_name(object_class(side),"$gg"),replacement);
    field_set(side,class_field_from_name(object_class(side),"$Gg"),*(void**)((char*)candidate+0xf8));
    if(parameter)publish_hero_parameter(side,candidate);
    return true;
}
// Prepare only a private, unpublished side; caller migrates preplaced entities
// and commits both dictionaries and entities after all checks pass.
static bool prepare_saved(void* side) {
    Plan plan;if(!side||!collect(plan))return false;if(!plan.count)return true;
    int matched=0;bool ok=apply(side,*(void**)((char*)side+0xf0),plan,matched,*(void**)((char*)side+0x238));
    LOG("Generic unit research prepared=%d side=%d families=%d present=%d absent_in_scenario=%d",ok,*(int*)((char*)side+0x10),plan.count,matched,plan.count-matched);
    return ok;
}
static bool apply_saved(void* battle,int player) {
    void* side=local_dictionary_item(battle?*(void**)((char*)battle+0x178):nullptr,player);
    return side&&local_research_runtime::empty_player_units(battle,side)&&prepare_saved(side);
}
static void audit_hero_acquisition(){
    if(!progress_ready||access(DATA_ROOT "hero-acquisition-audit.request",F_OK))return;
    unlink(DATA_ROOT "hero-acquisition-audit.request");auto before=progress;bool ok=true;int tested=0,rejected=0;
    void* definitions=local_career::list("SHARED","heroCards");int count=definitions?*(int*)((char*)definitions+0x18):0;
    for(int i=0;i<count;++i){void* definition=list_item(definitions,i);void* fraction=local_hero::get(definition,0x38);if(!fraction)continue;
        int faction=*(int*)((char*)fraction+0x10),id=*(int*)((char*)definition+0x10);if(!local_hero::card(id,faction))continue;
        Roots roots;void* level=local_hero::first_level(id,faction);void* owner=local_career::lookup("BATTLE","unitTypes",id);
        int hero=0,cards=0;Effect effect;bool pass=local_hero::acquisition(level,faction,hero,cards)&&hero==id&&resolve(level,effect)&&effect.hero_class==1;
        auto fixture=local_progress::seed(0,0,0,10000);
        pass=pass&&local_progress::grant_stock(fixture,9,id,cards)&&local_progress::buy_card_upgrade(fixture,*(int*)((char*)level+0x10),0,0,id,cards)&&!local_progress::stock_amount(fixture,9,id);
        void* copy=pass?copy_type(owner,effect,roots):nullptr;
        local_hero::Roots display_roots;void* display=pass?local_hero::display_type(owner,fixture,faction,display_roots):nullptr;
        auto opened=(bool(*)(void*,void*))(image_base+0x7a2ce24);
        pass=pass&&copy&&display&&copy!=owner&&display!=owner&&!opened(owner,nullptr)&&opened(copy,nullptr)&&opened(display,nullptr)&&
            !memcmp((char*)owner+0x10,(char*)copy+0x10,0x90)&&!memcmp((char*)owner+0xa4,(char*)copy+0xa4,0x30);
        void* ui=object_new(find_class("com.geargames.aow.ugui.army","ArmyPrototypeData"));
        if(pass&&roots.keep(ui)&&local_hero::prototype_original){local_hero::prototype_original(ui,display,nullptr);pass=local_hero::get(ui,0x18)==display;}
        else pass=false;
        void* upgrade=local_hero::get(level,0x48),*levels=local_hero::get(upgrade,0x18);void* second=levels&&array_length(levels)>1?((void**)((char*)levels+32))[1]:nullptr;
        Effect unsupported;bool refused=second&&!resolve(second,unsupported);rejected+=refused;
        bool next_expected=hero_effects::supported(id)?!refused&&unsupported.hero_ability:refused;
        LOG("Local hero acquisition private audit hero=%d faction=%d pass=%d next_class_refused=%d",id,faction,pass,refused);
        ok=ok&&pass&&next_expected;++tested;
    }
    bool isolated=!memcmp(&before,&progress,sizeof(progress));FILE* file=fopen(DATA_ROOT "hero-acquisition-audit.json","w");
    if(file){fprintf(file,"{\"pass\":%s,\"heroes\":%d,\"unsupported_next_classes_refused\":%d,\"original_opened_predicate\":true,\"original_workshop_constructor\":true,\"canonical_types_unchanged\":%s,\"player_state_unchanged\":%s,\"save_written\":false,\"battle_started\":false}\n",ok&&isolated&&tested==12?"true":"false",tested,rejected,ok?"true":"false",isolated?"true":"false");fclose(file);chmod(DATA_ROOT "hero-acquisition-audit.json",0644);}
}
}
