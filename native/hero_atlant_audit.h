// Original class models and decoded seat; no player acquisition or live combat.
#pragma once
namespace hero_atlant_audit {
using namespace generic_unit_research;
static bool check(void* copy,void* source,const hero_effects::Delta& d,int grade,bool initialized){
    auto integer=hero_class_effects_audit::integer;
    if(!copy||!source||!integer(copy,"get_HeroClass",grade)||local_hero::get(copy,0x18)!=local_hero::get(source,0x18))return false;
    void* list=local_hero::get(copy,0x98),*old=local_hero::get(source,0x98);
    if(!list||!old||*(int*)((char*)list+0x18)!=3||list_item(list,2)!=list_item(old,2))return false;
    void* a=list_item(list,0),*p=list_item(list,1),*oa=list_item(old,0),*op=list_item(old,1);
    void* w=local_hero::get(a,0x50),*ow=local_hero::get(oa,0x50);int angle=0;
    if(!a||!p||!oa||!op||!w||!ow||!hero_effects::shield_angle(*(int*)((char*)op+0x38),d,angle))return false;
    bool pass=integer(a,"get_Cooldown",*(int*)((char*)oa+0x34)-d.cooldown[0])&&
        integer(p,"get_Cooldown",*(int*)((char*)op+0x34)-d.cooldown[1])&&
        integer(p,"get_Duration",*(int*)((char*)op+0x40)+d.duration)&&
        integer(p,"get_Effect",*(int*)((char*)op+0x44)+d.passive_absorption)&&integer(p,"get_Radius",angle)&&
        !local_hero::get(a,0x70)&&!local_hero::get(p,0x50)&&!local_hero::get(p,0x70);
    int range=*(int*)((char*)ow+0x4c)+d.range,padded=range>1000?range+50:range*105/100;
    pass=pass&&integer(w,"get_Distance",initialized?padded*padded:range)&&
        integer(w,"get_ExplosionRadius",*(int*)((char*)ow+0x7c)+d.explosion)&&
        *(int16_t*)((char*)w+0x60)==*(int16_t*)((char*)ow+0x60)-d.shot_start&&bool(*(bool*)((char*)w+0x14))==initialized;
    const char* names[]={"get_DamageLight","get_DamageMedium","get_DamageHeavy"};
    for(int i=0;i<3;++i)pass=pass&&integer(w,names[i],*(int*)((char*)ow+0x28+4*i)+d.damage[i]);
    for(int i=0x10;i<0x80;++i){
        if((i<0x34||i>=0x38)&&(i<0x50||i>=0x58)&&((char*)a)[i]!=((char*)oa)[i])pass=false;
        if((i<0x34||i>=0x48)&&((char*)p)[i]!=((char*)op)[i])pass=false;
    }
    if(grade>1){
        pass=pass&&a!=oa&&p!=op&&w!=ow&&integer(p,"get_RadiusInit",angle);
        if(initialized){uint8_t bytes[0x110];memcpy(bytes,w,sizeof(bytes));void* ex=nullptr;invoke_named(w,"init",0,nullptr,&ex);pass=pass&&!ex&&!memcmp(bytes,w,sizeof(bytes));}
    }
    return pass;
}
static void run(){
    if(!progress_ready||access(DATA_ROOT "hero-atlant-audit.request",F_OK))return;unlink(DATA_ROOT "hero-atlant-audit.request");
    local_progress::StateBuffer saved_state,storage;if(!saved_state.data||!storage.data)return;*saved_state.data=progress;
    FILE* f=fopen(DATA_ROOT "hero-atlant-audit.jsonl","w");if(!f)return;chmod(DATA_ROOT "hero-atlant-audit.jsonl",0644);
    void* first=local_hero::first_level(75,1),*rows=local_hero::get(local_hero::get(first,0x48),0x18),*owner=local_career::lookup("BATTLE","unitTypes",75);
    void* abilities=local_hero::get(owner,0x98),*active=list_item(abilities,0),*passive=list_item(abilities,1),*third=list_item(abilities,2),*weapon=local_hero::get(active,0x50);
    bool ok=rows&&array_length(rows)==12&&owner&&active&&passive&&third&&weapon;
    bool originals_ready=ok;uint8_t originals[0x290]={};
    if(ok){memcpy(originals,active,0x80);memcpy(originals+0x80,passive,0x80);memcpy(originals+0x100,third,0x80);memcpy(originals+0x180,weapon,0x110);}
    Plan plan;hero_effects::Delta total;auto& fixture=*storage.data;fixture=local_progress::seed(100000000,0,0,10000);int tested=0;
    for(size_t i=0;ok&&i<array_length(rows);++i){Roots roots;local_hero::Roots display_roots;
        void* row=((void**)((char*)rows+32))[i];Effect e;int hero=0,cards=0,grade=0;
        bool resolved=resolve(row,e),planned=resolved&&plan.add(e);
        bool price=local_hero::class_price(row,1,hero,cards,grade)&&hero==75&&grade==int(i)+1;
        bool bought=price&&local_progress::grant_stock(fixture,9,hero,cards)&&local_progress::buy_card_upgrade(fixture,*(int*)((char*)row+0x10),*(int*)((char*)row+0x20),*(int*)((char*)row+0x28),hero,cards);
        bool pass=planned&&bought;if(i)pass=pass&&hero_effects::add(total,e.ability);
        void* display=pass?local_hero::display_type(owner,fixture,1,display_roots):nullptr;
        void* copy=pass?copy_type(owner,plan.families[0],roots,true):nullptr;
        bool display_ok=display&&check(display,owner,total,grade,false),native_ok=copy&&check(copy,owner,total,grade,i>0);
        pass=pass&&display_ok&&native_ok;ok&=pass;++tested;
        fprintf(f,"{\"phase\":\"class\",\"class\":%d,\"pass\":%s,\"resolved\":%s,\"planned\":%s,\"private_purchase\":%s,\"display\":%s,\"original_getters_init\":%s}\n",grade,pass?"true":"false",resolved?"true":"false",planned?"true":"false",bought?"true":"false",display_ok?"true":"false",native_ok?"true":"false");
    }
    bool battle_ok=false,atomic_ok=false;
    if(ok){Roots roots;void* config=campaign_seeds::decode(DATA_ROOT "offline-seeds/fmc1m.ggs",roots),*battle=local_hero::get(config,0x10);
        void* side=local_dictionary_item(local_hero::get(battle,0x178),1),*enemy=local_dictionary_item(local_hero::get(battle,0x178),2);
        void* baseline=local_hero::get(side,0xf0),*other=local_hero::get(enemy,0xf0),*parameter=local_hero::get(side,0x238),*other_parameter=local_hero::get(enemy,0x238);
        uint8_t param_before[0xb4]={},other_before[0xb4]={};if(parameter)memcpy(param_before,parameter,sizeof(param_before));if(other_parameter)memcpy(other_before,other_parameter,sizeof(other_before));
        uint64_t hash=hero_battle_projection_audit::type_hash(baseline),other_hash=hero_battle_projection_audit::type_hash(other);
        void* source=hero_solaris_audit::family(baseline,owner,roots);int matched=0;
        bool applied=source&&parameter&&other_parameter&&hash&&other_hash&&apply(side,baseline,plan,matched)&&matched==1;
        void* projected=applied?hero_solaris_audit::family(local_hero::get(side,0xf0),owner,roots):nullptr;
        bool repeated=applied&&check(projected,source,total,12,true)&&apply(side,baseline,plan,matched)&&matched==1;
        bool isolated=repeated&&hero_battle_projection_audit::type_hash(baseline)==hash&&hero_battle_projection_audit::type_hash(other)==other_hash&&
            parameter==local_hero::get(side,0x238)&&other_parameter==local_hero::get(enemy,0x238)&&!memcmp(param_before,parameter,sizeof(param_before))&&!memcmp(other_before,other_parameter,sizeof(other_before));
        battle_ok=isolated&&check(hero_solaris_audit::family(local_hero::get(side,0xf0),owner,roots),source,total,12,true);
        void* after=local_hero::get(side,0xf0);auto invalid=plan;invalid.families[0].ability.shield_half_angle=181;
        bool angle_refused=!apply(side,baseline,invalid,matched);invalid=plan;invalid.families[0].ability.passive_absorption=INT_MAX;
        bool overflow_refused=!apply(side,baseline,invalid,matched);
        atomic_ok=angle_refused&&overflow_refused&&after==local_hero::get(side,0xf0)&&parameter==local_hero::get(side,0x238);
        fprintf(f,"{\"phase\":\"decoded_battle\",\"pass\":%s,\"repeat_from_original\":%s,\"original_and_enemy_unchanged\":%s,\"shared_parameters_unchanged\":%s,\"atomic_refusals\":%s}\n",battle_ok&&atomic_ok?"true":"false",repeated?"true":"false",isolated?"true":"false",isolated?"true":"false",atomic_ok?"true":"false");
    }
    bool same=originals_ready&&!memcmp(originals,active,0x80)&&!memcmp(originals+0x80,passive,0x80)&&!memcmp(originals+0x100,third,0x80)&&!memcmp(originals+0x180,weapon,0x110);
    bool player_same=!memcmp(saved_state.data,&progress,sizeof(progress));
    fprintf(f,"{\"summary\":true,\"pass\":%s,\"hero\":75,\"classes\":%d,\"decoded_battle_and_repeat\":%s,\"atomic_refusals\":%s,\"canonical_unchanged\":%s,\"player_state_unchanged\":%s,\"maximum_display_angle\":%d,\"maximum_absorption\":%d,\"player_save_writes\":0,\"battle_runs\":0}\n",ok&&battle_ok&&atomic_ok&&same&&player_same?"true":"false",tested,battle_ok?"true":"false",atomic_ok?"true":"false",same?"true":"false",player_same?"true":"false",(75+total.shield_half_angle)*2,400+total.passive_absorption);fclose(f);
}
}
