// Original class models and a detached decoded side; no player unlock or cast.
#pragma once
namespace hero_solaris_audit {
using namespace generic_unit_research;
static void* family(void* dictionary,void* owner,Roots& roots){
    uintptr_t handle=0;void* values=local_dictionary_values(dictionary,find_class("com.geargames.aow.entities","UnitType"),handle);
    if(!values||!roots.take(handle))return nullptr;void* result=nullptr;
    for(size_t i=0;i<array_length(values);++i){void* type=((void**)((char*)values+32))[i];if(identity(type,owner)){if(result)return nullptr;result=type;}}
    return result;
}
static bool parameter(void* copy,void* source,const hero_effects::Delta& d){
    if(!copy||!source)return false;int distance=0,loss=0;
    if(!hero_effects::chain_parameters(*(int*)((char*)source+0x20),*(int*)((char*)source+0x28),d,distance,loss))return false;
    if(!original_stat_equals(copy,"get_SolarisA1JumpDistance",4,distance)||
       !original_stat_equals(copy,"get_SolarisA1JumpDmgDecr",4,loss))return false;
    for(int i=0x10;i<0xb4;++i)if((i<0x20||i>=0x24)&&(i<0x28||i>=0x2c)&&((char*)copy)[i]!=((char*)source)[i])return false;
    return (!d.chain_distance&&!d.chain_loss_reduction)||copy!=source;
}
static bool check(void* copy,void* source,const hero_effects::Delta& d,int grade,bool initialized){
    if(!copy||!source||!hero_class_effects_audit::integer(copy,"get_HeroClass",grade))return false;
    void* old=local_hero::get(source,0x98),*list=local_hero::get(copy,0x98);
    if(!old||!list||*(int*)((char*)list+0x18)!=2||local_hero::get(copy,0x18)!=local_hero::get(source,0x18))return false;
    for(int role=0;role<2;++role){void* a=list_item(list,role),*b=list_item(old,role),*w=local_hero::get(a,0x50),*base=local_hero::get(b,0x50);
        if(!a||!b||!w||!base||(grade>1&&(a==b||w==base)))return false;
        const auto delta=role?d.passive_weapon:active_weapon_delta(d);
        int raw_range=*(int*)((char*)base+0x4c)+delta.range;
        int padded=raw_range>1000?raw_range+50:raw_range*105/100;
        if(!original_stat_equals(a,"get_Cooldown",4,*(int*)((char*)b+0x34)-d.cooldown[role])||
           !original_stat_equals(w,"get_Distance",4,initialized?padded*padded:raw_range)||
           !original_stat_equals(w,"get_ExplosionRadius",4,*(int*)((char*)base+0x7c))||
           bool(*(bool*)((char*)w+0x14))!=initialized||
           *(int16_t*)((char*)w+0x60)!=*(int16_t*)((char*)base+0x60)-delta.shot_start)return false;
        const char* getters[]={"get_DamageLight","get_DamageMedium","get_DamageHeavy"};
        for(int i=0;i<3;++i)if(!original_stat_equals(w,getters[i],4,*(int*)((char*)base+0x28+i*4)+delta.damage[i]))return false;
        if(initialized){uint8_t before[0x110];memcpy(before,w,sizeof(before));void* ex=nullptr;invoke_named(w,"init",0,nullptr,&ex);if(ex||memcmp(before,w,sizeof(before)))return false;}
    }
    return true;
}
static void run(){
    if(!progress_ready||access(DATA_ROOT "hero-solaris-audit.request",F_OK))return;unlink(DATA_ROOT "hero-solaris-audit.request");
    local_progress::StateBuffer saved_state,storage;if(!saved_state.data||!storage.data)return;*saved_state.data=progress;
    FILE* f=fopen(DATA_ROOT "hero-solaris-audit.jsonl","w");if(!f)return;chmod(DATA_ROOT "hero-solaris-audit.jsonl",0644);
    void* first=local_hero::first_level(72,1),*rows=local_hero::get(local_hero::get(first,0x48),0x18),*owner=local_career::lookup("BATTLE","unitTypes",72);
    void* source_parameter=local_career::list("BATTLE","heroParam"),*abilities=local_hero::get(owner,0x98);
    void* active=list_item(abilities,0),*passive=list_item(abilities,1),*aw=local_hero::get(active,0x50),*pw=local_hero::get(passive,0x50);
    bool ok=rows&&array_length(rows)==12&&owner&&source_parameter&&active&&passive&&aw&&pw&&hero_parameters::get_original;
    uint8_t originals[0x3d4]={};
    if(ok){memcpy(originals,active,0x80);memcpy(originals+0x80,passive,0x80);memcpy(originals+0x100,aw,0x110);memcpy(originals+0x210,pw,0x110);memcpy(originals+0x320,source_parameter,0xb4);}
    Plan plan;hero_effects::Delta total;auto& fixture=*storage.data;fixture=local_progress::seed(20000000,0,0,10000);int tested=0;
    for(size_t i=0;ok&&i<array_length(rows);++i){Roots roots;local_hero::Roots display_roots;
        void* row=((void**)((char*)rows+32))[i];Effect e;int hero=0,cards=0,grade=0;
        bool resolved=resolve(row,e),planned=resolved&&plan.add(e);
        bool price=local_hero::class_price(row,1,hero,cards,grade)&&hero==72&&grade==int(i)+1;
        bool bought=price&&local_progress::grant_stock(fixture,9,hero,cards)&&local_progress::buy_card_upgrade(fixture,*(int*)((char*)row+0x10),*(int*)((char*)row+0x20),*(int*)((char*)row+0x28),hero,cards);
        bool pass=planned&&bought;
        if(i)pass=pass&&hero_effects::add(total,e.ability);
        void* display=pass?local_hero::display_type(owner,fixture,1,display_roots):nullptr;
        void* copy=pass?copy_type(owner,plan.families[0],roots,true):nullptr;
        void* presented_parameter=pass?hero_parameters::project(source_parameter,fixture,1,roots):nullptr;
        bool display_ok=display&&check(display,owner,total,grade,false)&&parameter(presented_parameter,source_parameter,total);
        bool native_ok=copy&&check(copy,owner,total,grade,i>0);
        pass=pass&&display_ok&&native_ok;ok&=pass;++tested;
        fprintf(f,"{\"phase\":\"class\",\"class\":%d,\"pass\":%s,\"private_purchase\":%s,\"display_and_parameter\":%s,\"original_getters_init\":%s}\n",grade,pass?"true":"false",bought?"true":"false",display_ok?"true":"false",native_ok?"true":"false");
    }
    bool battle_ok=false,atomic_ok=false;
    if(ok){Roots roots;void* config=campaign_seeds::decode(DATA_ROOT "offline-seeds/fmc1m.ggs",roots),*battle=local_hero::get(config,0x10);
        void* side=local_dictionary_item(local_hero::get(battle,0x178),1),*enemy=local_dictionary_item(local_hero::get(battle,0x178),2);
        void* baseline=local_hero::get(side,0xf0),*base_parameter=local_hero::get(side,0x238),*enemy_parameter=local_hero::get(enemy,0x238),*enemy_units=local_hero::get(enemy,0xf0);
        uint64_t hash=hero_battle_projection_audit::type_hash(baseline),other_hash=hero_battle_projection_audit::type_hash(enemy_units);
        uint8_t param_before[0xb4]={},other_before[0xb4]={};if(base_parameter)memcpy(param_before,base_parameter,sizeof(param_before));if(enemy_parameter)memcpy(other_before,enemy_parameter,sizeof(other_before));
        void* staged=local_research_runtime::clone(side,roots);int matched=0;
        void* source=family(baseline,owner,roots);
        bool applied=staged&&source&&baseline&&enemy_units&&enemy_parameter&&base_parameter&&apply(staged,baseline,plan,matched,base_parameter)&&matched==1;
        bool isolated=applied&&local_hero::get(side,0x238)==base_parameter&&parameter(local_hero::get(staged,0x238),base_parameter,total);
        if(isolated)publish_hero_parameter(side,staged); // Same outer loading-path publisher.
        void* ex=nullptr;void* getter=isolated?invoke_named(side,"$ZY",0,nullptr,&ex):nullptr;
        bool published=isolated&&!ex&&getter==local_hero::get(staged,0x238)&&parameter(getter,base_parameter,total);
        bool repeated=published&&apply(side,baseline,plan,matched,base_parameter)&&matched==1&&parameter(local_hero::get(side,0x238),base_parameter,total);
        bool same=applied&&hash&&other_hash&&hero_battle_projection_audit::type_hash(baseline)==hash&&hero_battle_projection_audit::type_hash(enemy_units)==other_hash&&
            enemy_parameter==local_hero::get(enemy,0x238)&&!memcmp(param_before,base_parameter,sizeof(param_before))&&!memcmp(other_before,enemy_parameter,sizeof(other_before));
        void* projected=repeated?family(local_hero::get(side,0xf0),owner,roots):nullptr;
        battle_ok=repeated&&same&&check(projected,source,total,12,true);
        void* units_after=local_hero::get(side,0xf0),*parameter_after=local_hero::get(side,0x238);auto invalid=plan;invalid.families[0].ability.chain_loss_reduction=101;
        bool missing_refused=!apply(side,baseline,plan,matched),bad_refused=!apply(side,baseline,invalid,matched,base_parameter);
        atomic_ok=missing_refused&&bad_refused&&units_after==local_hero::get(side,0xf0)&&parameter_after==local_hero::get(side,0x238);
        fprintf(f,"{\"phase\":\"decoded_battle\",\"pass\":%s,\"outer_publish_native_getter\":%s,\"repeat_from_original_parameter\":%s,\"original_and_enemy_unchanged\":%s,\"atomic_refusals\":%s}\n",battle_ok&&atomic_ok?"true":"false",published?"true":"false",repeated?"true":"false",same?"true":"false",atomic_ok?"true":"false");
    }
    bool unchanged=ok&&!memcmp(originals,active,0x80)&&!memcmp(originals+0x80,passive,0x80)&&!memcmp(originals+0x100,aw,0x110)&&!memcmp(originals+0x210,pw,0x110)&&!memcmp(originals+0x320,source_parameter,0xb4);
    bool player_same=!memcmp(saved_state.data,&progress,sizeof(progress));
    fprintf(f,"{\"summary\":true,\"pass\":%s,\"classes\":%d,\"decoded_battle_and_parameter\":%s,\"atomic_refusals\":%s,\"canonical_unchanged\":%s,\"player_state_unchanged\":%s,\"player_save_writes\":0,\"battle_runs\":0}\n",ok&&battle_ok&&atomic_ok&&unchanged&&player_same?"true":"false",tested,battle_ok?"true":"false",atomic_ok?"true":"false",unchanged?"true":"false",player_same?"true":"false");fclose(f);
}
}
