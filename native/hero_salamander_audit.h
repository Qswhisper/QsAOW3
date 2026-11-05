// Original class models, private native initializers and decoded seat projection.
// Does not start a battle, cast a player ability or write player progression.
#pragma once
namespace hero_salamander_audit {
using namespace generic_unit_research;
static bool parameter(void* copy,void* source,const hero_effects::Delta& d){
    if(!copy||!source)return false;int speed=0;
    if(!hero_effects::salamander_speed(*(int*)((char*)source+0x74),d,speed)||
       !original_stat_equals(copy,"get_SalamanderPassiveSpeedInc",4,speed))return false;
    for(int i=0x10;i<0xb4;++i)if((i<0x74||i>=0x78)&&((char*)copy)[i]!=((char*)source)[i])return false;
    return !d.salamander_speed||copy!=source;
}
static bool check(void* copy,void* source,const hero_effects::Delta& d,int grade,bool initialized){
    auto integer=hero_class_effects_audit::integer;
    if(!copy||!source||!integer(copy,"get_HeroClass",grade)||local_hero::get(copy,0x18)!=local_hero::get(source,0x18))return false;
    void* old=local_hero::get(source,0x98),*list=local_hero::get(copy,0x98);
    if(!old||!list||*(int*)((char*)list+0x18)!=2)return false;
    void* a=list_item(list,0),*p=list_item(list,1),*oa=list_item(old,0),*op=list_item(old,1);
    void* w=local_hero::get(a,0x50),*ow=local_hero::get(oa,0x50);
    if(!a||!p||!w||!oa||!op||!ow)return false;
    bool pass=integer(a,"get_Cooldown",*(int*)((char*)oa+0x34)-d.cooldown[0])&&
        integer(p,"get_Cooldown",*(int*)((char*)op+0x34)-d.cooldown[1])&&
        integer(p,"get_Duration",*(int*)((char*)op+0x40)+d.duration)&&
        integer(p,"get_Effect",*(int*)((char*)op+0x44)+d.passive_recovery)&&
        integer(p,"get_Radius",*(int*)((char*)op+0x38))&&
        local_hero::get(a,0x70)==local_hero::get(oa,0x70)&&local_hero::get(p,0x70)==local_hero::get(op,0x70)&&!local_hero::get(p,0x50);
    int range=*(int*)((char*)ow+0x4c)+d.range,padded=range>1000?range+50:range*105/100;
    pass=pass&&integer(w,"get_Distance",initialized?padded*padded:range)&&
        integer(w,"get_ExplosionRadius",*(int*)((char*)ow+0x7c)+d.explosion)&&
        *(int16_t*)((char*)w+0x60)==*(int16_t*)((char*)ow+0x60)-d.shot_start&&bool(*(bool*)((char*)w+0x14))==initialized;
    const char* names[]={"get_DamageLight","get_DamageMedium","get_DamageHeavy"};
    for(int i=0;i<3;++i)pass=pass&&integer(w,names[i],*(int*)((char*)ow+0x28+4*i)+d.damage[i]);
    for(int i=0x10;i<0x80;++i){
        if((i<0x34||i>=0x38)&&(i<0x50||i>=0x58)&&((char*)a)[i]!=((char*)oa)[i])pass=false;
        if((i<0x34||i>=0x38)&&(i<0x3c||i>=0x48)&&((char*)p)[i]!=((char*)op)[i])pass=false;
    }
    if(grade>1){
        pass=pass&&a!=oa&&p!=op&&w!=ow&&integer(p,"get_RadiusInit",*(int*)((char*)op+0x38));
        if(initialized){uint8_t bytes[0x110];memcpy(bytes,w,sizeof(bytes));void* ex=nullptr;invoke_named(w,"init",0,nullptr,&ex);pass=pass&&!ex&&!memcmp(bytes,w,sizeof(bytes));}
    }
    return pass;
}
static void run(){
    if(!progress_ready||access(DATA_ROOT "hero-salamander-audit.request",F_OK))return;unlink(DATA_ROOT "hero-salamander-audit.request");
    local_progress::StateBuffer saved_state,storage;if(!saved_state.data||!storage.data)return;*saved_state.data=progress;
    FILE* f=fopen(DATA_ROOT "hero-salamander-audit.jsonl","w");if(!f)return;chmod(DATA_ROOT "hero-salamander-audit.jsonl",0644);
    void* first=local_hero::first_level(174,2),*rows=local_hero::get(local_hero::get(first,0x48),0x18),*owner=local_career::lookup("BATTLE","unitTypes",174);
    void* source_parameter=local_career::list("BATTLE","heroParam"),*abilities=local_hero::get(owner,0x98);
    void* active=list_item(abilities,0),*passive=list_item(abilities,1),*weapon=local_hero::get(active,0x50);
    bool ok=rows&&array_length(rows)==12&&owner&&source_parameter&&active&&passive&&weapon&&hero_parameters::get_original;
    bool originals_ready=ok;
    uint8_t originals[0x2c4]={};
    if(ok){memcpy(originals,active,0x80);memcpy(originals+0x80,passive,0x80);memcpy(originals+0x100,weapon,0x110);memcpy(originals+0x210,source_parameter,0xb4);}
    Plan plan;hero_effects::Delta total;auto& fixture=*storage.data;fixture=local_progress::seed(20000000,0,0,10000);int tested=0;
    for(size_t i=0;ok&&i<array_length(rows);++i){Roots roots;local_hero::Roots display_roots;
        void* row=((void**)((char*)rows+32))[i];Effect e;int hero=0,cards=0,grade=0;
        bool resolved=resolve(row,e),planned=resolved&&plan.add(e);
        bool price=local_hero::class_price(row,2,hero,cards,grade)&&hero==174&&grade==int(i)+1;
        bool bought=price&&local_progress::grant_stock(fixture,9,hero,cards)&&local_progress::buy_card_upgrade(fixture,*(int*)((char*)row+0x10),*(int*)((char*)row+0x20),*(int*)((char*)row+0x28),hero,cards);
        bool pass=planned&&bought;
        if(i)pass=pass&&hero_effects::add(total,e.ability);
        void* display=pass?local_hero::display_type(owner,fixture,2,display_roots):nullptr;
        void* copy=pass?copy_type(owner,plan.families[0],roots,true):nullptr;
        void* presented=pass?hero_parameters::project(source_parameter,fixture,2,roots):nullptr;
        bool display_ok=display&&check(display,owner,total,grade,false)&&parameter(presented,source_parameter,total);
        bool native_ok=copy&&check(copy,owner,total,grade,i>0);
        pass=pass&&display_ok&&native_ok;ok&=pass;++tested;
        fprintf(f,"{\"phase\":\"class\",\"class\":%d,\"pass\":%s,\"resolved\":%s,\"planned\":%s,\"private_purchase\":%s,\"display_and_parameter\":%s,\"original_getters_init\":%s}\n",grade,pass?"true":"false",resolved?"true":"false",planned?"true":"false",bought?"true":"false",display_ok?"true":"false",native_ok?"true":"false");
    }
    bool battle_ok=false,atomic_ok=false;
    if(ok){Roots roots;void* config=campaign_seeds::decode(DATA_ROOT "offline-seeds/fmc2m.ggs",roots),*battle=local_hero::get(config,0x10);
        void* side=local_dictionary_item(local_hero::get(battle,0x178),1),*enemy=local_dictionary_item(local_hero::get(battle,0x178),2);
        void* baseline=local_hero::get(side,0xf0),*base_parameter=local_hero::get(side,0x238),*enemy_parameter=local_hero::get(enemy,0x238),*enemy_units=local_hero::get(enemy,0xf0);
        uint64_t hash=hero_battle_projection_audit::type_hash(baseline),other_hash=hero_battle_projection_audit::type_hash(enemy_units);
        uint8_t param_before[0xb4]={},other_before[0xb4]={};if(base_parameter)memcpy(param_before,base_parameter,sizeof(param_before));if(enemy_parameter)memcpy(other_before,enemy_parameter,sizeof(other_before));
        void* staged=local_research_runtime::clone(side,roots);int matched=0;
        void* source=hero_solaris_audit::family(baseline,owner,roots);
        bool applied=staged&&source&&baseline&&enemy_units&&enemy_parameter&&base_parameter&&apply(staged,baseline,plan,matched,base_parameter)&&matched==1;
        bool isolated=applied&&local_hero::get(side,0x238)==base_parameter&&parameter(local_hero::get(staged,0x238),base_parameter,total);
        if(isolated)publish_hero_parameter(side,staged);
        void* ex=nullptr;void* getter=isolated?invoke_named(side,"$ZY",0,nullptr,&ex):nullptr;
        bool published=isolated&&!ex&&getter==local_hero::get(staged,0x238)&&parameter(getter,base_parameter,total);
        bool repeated=published&&apply(side,baseline,plan,matched,base_parameter)&&matched==1&&parameter(local_hero::get(side,0x238),base_parameter,total);
        bool same=applied&&hash&&other_hash&&hero_battle_projection_audit::type_hash(baseline)==hash&&hero_battle_projection_audit::type_hash(enemy_units)==other_hash&&
            enemy_parameter==local_hero::get(enemy,0x238)&&!memcmp(param_before,base_parameter,sizeof(param_before))&&!memcmp(other_before,enemy_parameter,sizeof(other_before));
        void* projected=repeated?hero_solaris_audit::family(local_hero::get(side,0xf0),owner,roots):nullptr;
        battle_ok=repeated&&same&&check(projected,source,total,12,true);
        void* units_after=local_hero::get(side,0xf0),*parameter_after=local_hero::get(side,0x238);auto invalid=plan;invalid.families[0].ability.salamander_speed=100;
        bool missing_refused=!apply(side,baseline,plan,matched),bad_refused=!apply(side,baseline,invalid,matched,base_parameter);
        auto overflow=plan;overflow.families[0].ability.passive_recovery=INT_MAX;
        bool overflow_refused=!apply(side,baseline,overflow,matched,base_parameter);
        atomic_ok=missing_refused&&bad_refused&&overflow_refused&&units_after==local_hero::get(side,0xf0)&&parameter_after==local_hero::get(side,0x238);
        fprintf(f,"{\"phase\":\"decoded_battle\",\"pass\":%s,\"outer_publish_native_getter\":%s,\"repeat_from_original_parameter\":%s,\"original_and_enemy_unchanged\":%s,\"atomic_refusals\":%s}\n",battle_ok&&atomic_ok?"true":"false",published?"true":"false",repeated?"true":"false",same?"true":"false",atomic_ok?"true":"false");
    }
    bool unchanged=originals_ready&&!memcmp(originals,active,0x80)&&!memcmp(originals+0x80,passive,0x80)&&!memcmp(originals+0x100,weapon,0x110)&&!memcmp(originals+0x210,source_parameter,0xb4);
    bool player_same=!memcmp(saved_state.data,&progress,sizeof(progress));
    fprintf(f,"{\"summary\":true,\"pass\":%s,\"hero\":174,\"classes\":%d,\"decoded_battle_and_parameter\":%s,\"atomic_refusals\":%s,\"canonical_unchanged\":%s,\"player_state_unchanged\":%s,\"maximum_shared_speed\":%d,\"maximum_raw_recovery\":%d,\"player_save_writes\":0,\"battle_runs\":0}\n",ok&&battle_ok&&atomic_ok&&unchanged&&player_same?"true":"false",tested,battle_ok?"true":"false",atomic_ok?"true":"false",unchanged?"true":"false",player_same?"true":"false",25+total.salamander_speed,2400+total.passive_recovery);fclose(f);
}
}
