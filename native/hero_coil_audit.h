// Private full-family models/init and decoded seat. No player grants or battle.
#pragma once
#include <memory>
namespace hero_coil_audit {
using namespace generic_unit_research;
static bool weapon_check(void* copy,void* source,const hero_effects::WeaponDelta& d,bool initialized){
    auto integer=hero_class_effects_audit::integer;if(!copy||!source)return false;
    int range=*(int*)((char*)source+0x4c)+d.range,padded=range>1000?range+50:range*105/100;
    bool ok=integer(copy,"get_Distance",initialized?padded*padded:range)&&
        integer(copy,"get_ExplosionRadius",*(int*)((char*)source+0x7c)+d.explosion)&&
        *(int16_t*)((char*)copy+0x60)==*(int16_t*)((char*)source+0x60)-d.shot_start&&bool(*(bool*)((char*)copy+0x14))==initialized;
    const char* names[]={"get_DamageLight","get_DamageMedium","get_DamageHeavy"};
    for(int i=0;i<3;++i)ok=ok&&integer(copy,names[i],*(int*)((char*)source+0x28+4*i)+d.damage[i]);
    if(initialized){uint8_t bytes[0x110];memcpy(bytes,copy,sizeof(bytes));void* ex=nullptr;invoke_named(copy,"init",0,nullptr,&ex);ok=ok&&!ex&&!memcmp(bytes,copy,sizeof(bytes));}
    return ok;
}
static bool check(void* copy,void* source,const hero_effects::Delta& d,int grade,bool initialized){
    auto integer=hero_class_effects_audit::integer;
    if(!copy||!source||!integer(copy,"get_HeroClass",grade)||local_hero::get(copy,0x18)!=local_hero::get(source,0x18))return false;
    void* list=local_hero::get(copy,0x98),*old=local_hero::get(source,0x98);
    if(!list||!old||*(int*)((char*)list+0x18)!=3||list_item(list,2)!=list_item(old,2))return false;
    void* a=list_item(list,0),*p=list_item(list,1),*oa=list_item(old,0),*op=list_item(old,1);
    if(!a||!p||!oa||!op)return false;
    bool ok=integer(a,"get_Cooldown",*(int*)((char*)oa+0x34)-d.cooldown[0])&&
        integer(p,"get_Cooldown",*(int*)((char*)op+0x34)-d.cooldown[1])&&
        integer(a,"get_Duration",*(int*)((char*)oa+0x40)+d.active_duration)&&
        integer(a,"get_Effect",*(int*)((char*)oa+0x44)+d.active_effect)&&
        integer(p,"get_Effect",*(int*)((char*)op+0x44)+d.passive_stun)&&
        integer(p,"get_Duration",*(int*)((char*)op+0x40))&&
        local_hero::get(a,0x70)==local_hero::get(oa,0x70)&&local_hero::get(p,0x70)==local_hero::get(op,0x70)&&
        weapon_check(local_hero::get(a,0x50),local_hero::get(oa,0x50),active_weapon_delta(d),initialized)&&
        weapon_check(local_hero::get(p,0x50),local_hero::get(op,0x50),d.passive_weapon,initialized);
    for(int i=0x10;i<0x80;++i){
        if((i<0x34||i>=0x38)&&(i<0x40||i>=0x48)&&(i<0x50||i>=0x58)&&((char*)a)[i]!=((char*)oa)[i])ok=false;
        if((i<0x34||i>=0x38)&&(i<0x3c||i>=0x40)&&(i<0x44||i>=0x48)&&(i<0x50||i>=0x58)&&((char*)p)[i]!=((char*)op)[i])ok=false;
    }
    if(grade>1)ok=ok&&a!=oa&&p!=op&&integer(p,"get_RadiusInit",*(int*)((char*)op+0x38));
    return ok;
}
static void run(){
    if(!progress_ready||access(DATA_ROOT "hero-coil-audit.request",F_OK))return;unlink(DATA_ROOT "hero-coil-audit.request");
    local_progress::StateBuffer before,storage;std::unique_ptr<Plan> plan(new(std::nothrow) Plan),invalid(new(std::nothrow) Plan);
    if(!before.data||!storage.data||!plan||!invalid)return;*before.data=progress;
    FILE* f=fopen(DATA_ROOT "hero-coil-audit.jsonl","w");if(!f)return;chmod(DATA_ROOT "hero-coil-audit.jsonl",0644);
    void* first=local_hero::first_level(175,2),*rows=local_hero::get(local_hero::get(first,0x48),0x18),*owner=local_career::lookup("BATTLE","unitTypes",175);
    void* old=local_hero::get(owner,0x98),*a=list_item(old,0),*p=list_item(old,1),*third=list_item(old,2),*aw=local_hero::get(a,0x50),*pw=local_hero::get(p,0x50);
    bool ok=rows&&array_length(rows)==12&&owner&&a&&p&&third&&aw&&pw,ready=ok;uint8_t originals[0x3a0]={};
    if(ok){memcpy(originals,a,0x80);memcpy(originals+0x80,p,0x80);memcpy(originals+0x100,third,0x80);memcpy(originals+0x180,aw,0x110);memcpy(originals+0x290,pw,0x110);}
    hero_effects::Delta total;auto& state=*storage.data;state=local_progress::seed(150000000,0,0,10000);int tested=0;
    for(size_t i=0;ok&&i<array_length(rows);++i){Roots roots;local_hero::Roots retained;
        void* row=((void**)((char*)rows+32))[i];Effect e;int hero=0,cards=0,grade=0;
        bool resolved=resolve(row,e),planned=resolved&&plan->add(e),price=local_hero::class_price(row,2,hero,cards,grade)&&hero==175&&grade==int(i)+1;
        bool bought=price&&local_progress::grant_stock(state,9,hero,cards)&&local_progress::buy_card_upgrade(state,*(int*)((char*)row+0x10),*(int*)((char*)row+0x20),*(int*)((char*)row+0x28),hero,cards);
        bool pass=planned&&bought;if(i)pass=pass&&hero_effects::add(total,e.ability);
        void* shown=pass?local_hero::display_type(owner,state,2,retained):nullptr,*copy=pass?copy_type(owner,plan->families[0],roots,true):nullptr;
        bool display_ok=shown&&check(shown,owner,total,grade,false),native_ok=copy&&check(copy,owner,total,grade,i>0);
        pass=pass&&display_ok&&native_ok;ok=ok&&pass;++tested;
        fprintf(f,"{\"phase\":\"class\",\"class\":%d,\"pass\":%s,\"resolved\":%s,\"planned\":%s,\"private_purchase\":%s,\"display\":%s,\"original_getters_init\":%s}\n",grade,pass?"true":"false",resolved?"true":"false",planned?"true":"false",bought?"true":"false",display_ok?"true":"false",native_ok?"true":"false");
    }
    bool battle_ok=false,atomic=false;
    if(ok){Roots roots;void* config=campaign_seeds::decode(DATA_ROOT "offline-seeds/fmc2m.ggs",roots),*battle=local_hero::get(config,0x10);
        void* side=local_dictionary_item(local_hero::get(battle,0x178),1),*enemy=local_dictionary_item(local_hero::get(battle,0x178),2);
        void* baseline=local_hero::get(side,0xf0),*other=local_hero::get(enemy,0xf0),*parameter=local_hero::get(side,0x238);
        uint64_t own_hash=hero_battle_projection_audit::type_hash(baseline),other_hash=hero_battle_projection_audit::type_hash(other);int matched=0;
        void* source=hero_solaris_audit::family(baseline,owner,roots);
        battle_ok=source&&own_hash&&other_hash&&apply(side,baseline,*plan,matched)&&matched==1&&
            check(hero_solaris_audit::family(local_hero::get(side,0xf0),owner,roots),source,total,12,true)&&
            apply(side,baseline,*plan,matched)&&matched==1&&
            check(hero_solaris_audit::family(local_hero::get(side,0xf0),owner,roots),source,total,12,true)&&
            own_hash==hero_battle_projection_audit::type_hash(baseline)&&other_hash==hero_battle_projection_audit::type_hash(other)&&parameter==local_hero::get(side,0x238);
        void* after=local_hero::get(side,0xf0);*invalid=*plan;invalid->families[0].ability.active_effect=61;
        bool percent=!apply(side,baseline,*invalid,matched);*invalid=*plan;invalid->families[0].ability.passive_stun=INT_MAX;
        bool overflow=!apply(side,baseline,*invalid,matched);*invalid=*plan;invalid->families[0].ability.passive_weapon.range=1;
        bool range=!apply(side,baseline,*invalid,matched);atomic=percent&&overflow&&range&&after==local_hero::get(side,0xf0)&&parameter==local_hero::get(side,0x238);
        fprintf(f,"{\"phase\":\"decoded_battle\",\"pass\":%s,\"original_and_enemy_unchanged\":%s,\"atomic_refusals\":%s,\"zero_passive_range_retained\":%s}\n",battle_ok&&atomic?"true":"false",battle_ok?"true":"false",atomic?"true":"false",battle_ok?"true":"false");
    }
    bool same=ready&&!memcmp(originals,a,0x80)&&!memcmp(originals+0x80,p,0x80)&&!memcmp(originals+0x100,third,0x80)&&!memcmp(originals+0x180,aw,0x110)&&!memcmp(originals+0x290,pw,0x110);
    bool unchanged=!memcmp(before.data,&progress,sizeof(progress));
    fprintf(f,"{\"summary\":true,\"pass\":%s,\"hero\":175,\"classes\":%d,\"canonical_unchanged\":%s,\"player_state_unchanged\":%s,\"decoded_battle_and_repeat\":%s,\"atomic_refusals\":%s,\"building_stun_ticks\":%d,\"energy_percent\":%d,\"passive_stun_ticks\":%d,\"player_save_writes\":0,\"battle_runs\":0}\n",ok&&tested==12&&battle_ok&&atomic&&same&&unchanged?"true":"false",tested,same?"true":"false",unchanged?"true":"false",battle_ok?"true":"false",atomic?"true":"false",60+total.active_duration,40+total.active_effect,20+total.passive_stun);fclose(f);
}
}
