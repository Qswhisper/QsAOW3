// Detached original dual-weapon/Aura projection; no player unlock/card grants.
#pragma once
namespace hero_gatling_audit {
using namespace generic_unit_research;
static bool check(void* copy,void* source,const hero_effects::Delta& d,int grade,bool initialized){
    if(!copy||!source)return false;void* old=local_hero::get(source,0x98),*rows=local_hero::get(copy,0x98);
    if(!old||!rows||*(int*)((char*)rows+0x18)!=2||!hero_class_effects_audit::integer(copy,"get_HeroClass",grade))return false;
    for(int role=0;role<2;++role){void* a=list_item(rows,role),*b=list_item(old,role),*w=local_hero::get(a,0x50),*base=local_hero::get(b,0x50);
        if(!a||!b||!w||!base||(grade>1&&(a==b||w==base)))return false;
        auto delta=role?d.passive_weapon:active_weapon_delta(d);
        if(!original_stat_equals(a,"get_Cooldown",4,*(int*)((char*)b+0x34)-d.cooldown[role])||
           !original_stat_equals(a,"get_Radius",4,*(int*)((char*)b+0x38))||
           !original_stat_equals(w,"get_Distance",4,initialized?([](int v){int p=v>1000?v+50:v*105/100;return p*p;})(*(int*)((char*)base+0x4c)+delta.range):*(int*)((char*)base+0x4c)+delta.range)||
           !original_stat_equals(w,"get_ExplosionRadius",4,*(int*)((char*)base+0x7c)+delta.explosion)||
           *(int16_t*)((char*)w+0x60)!=*(int16_t*)((char*)base+0x60)-delta.shot_start||
           bool(*(bool*)((char*)w+0x14))!=initialized)return false;
        const char* damage[]={"get_DamageLight","get_DamageMedium","get_DamageHeavy"};
        for(int k=0;k<3;++k)if(!original_stat_equals(w,damage[k],4,*(int*)((char*)base+0x28+k*4)+delta.damage[k]))return false;
        if(role){void* aura=local_hero::get(w,0x18),*old_aura=local_hero::get(base,0x18);if(!aura||!old_aura||(grade>1&&aura==old_aura))return false;
            if(!original_stat_equals(aura,"get_Radius",4,*(int*)((char*)old_aura+0x14)+d.aura_radius)||
               !original_stat_equals(aura,"get_Duration",4,*(int*)((char*)old_aura+0x18)+d.aura_duration)||
               !original_stat_equals(aura,"get_EffectValue",4,*(int*)((char*)old_aura+0x20)+d.aura_effect)||
               *(int*)((char*)aura+0x24)!=*(int*)((char*)old_aura+0x24)||
               local_hero::get(aura,0x28)!=local_hero::get(old_aura,0x28)||local_hero::get(a,0x70)!=local_hero::get(b,0x70))return false;
        }
        if(initialized){uint8_t prior[0x110];memcpy(prior,w,sizeof(prior));void* ex=nullptr;invoke_named(w,"init",0,nullptr,&ex);if(ex||memcmp(prior,w,sizeof(prior)))return false;}
    }
    // Both private ability weapons leave ordinary state attacks untouched.
    return local_hero::get(copy,0x18)==local_hero::get(source,0x18);
}
static void run(){
    if(!progress_ready||access(DATA_ROOT "hero-gatling-audit.request",F_OK))return;unlink(DATA_ROOT "hero-gatling-audit.request");
    local_progress::StateBuffer saved_state;if(!saved_state.data)return;*saved_state.data=progress;
    FILE* f=fopen(DATA_ROOT "hero-gatling-audit.jsonl","w");if(!f)return;chmod(DATA_ROOT "hero-gatling-audit.jsonl",0644);
    void* first=local_hero::first_level(173,2),*rows=local_hero::get(local_hero::get(first,0x48),0x18),*owner=local_career::lookup("BATTLE","unitTypes",173);
    void* original=local_hero::get(owner,0x98);void* active=list_item(original,0),*passive=list_item(original,1),*aw=local_hero::get(active,0x50),*pw=local_hero::get(passive,0x50),*aura=local_hero::get(pw,0x18);
    bool ok=rows&&array_length(rows)==12&&owner&&active&&passive&&aw&&pw&&aura&&hero_effects::supported(173);
    uint8_t originals[0x440]={};if(ok){memcpy(originals,active,0x80);memcpy(originals+0x80,passive,0x80);memcpy(originals+0x100,aw,0x110);memcpy(originals+0x210,pw,0x110);memcpy(originals+0x320,aura,0x40);}
    Plan plan;hero_effects::Delta total;int tested=0;
    local_progress::StateBuffer storage;if(!storage.data)ok=false;auto& fixture=*storage.data;fixture=local_progress::seed(20000000,0,0,10000);
    for(size_t i=0;ok&&i<array_length(rows);++i){Roots roots;void* row=((void**)((char*)rows+32))[i];Effect e;int hero=0,cards=0,grade=0;
        bool resolved=resolve(row,e),planned=resolved&&plan.add(e);
        bool price=local_hero::class_price(row,2,hero,cards,grade)&&hero==173&&grade==int(i)+1;
        bool bought=price&&local_progress::grant_stock(fixture,9,hero,cards)&&local_progress::buy_card_upgrade(fixture,*(int*)((char*)row+0x10),*(int*)((char*)row+0x20),*(int*)((char*)row+0x28),hero,cards);
        bool pass=planned&&bought;if(i)pass=pass&&hero_effects::add(total,e.ability);
        local_hero::Roots display_roots;void* display=pass?local_hero::display_type(owner,fixture,2,display_roots):nullptr,*copy=pass?copy_type(owner,plan.families[0],roots,true):nullptr;
        bool display_ok=display&&check(display,owner,total,grade,false),native_ok=copy&&check(copy,owner,total,grade,i>0);
        pass=pass&&display_ok&&native_ok;ok&=pass;++tested;
        fprintf(f,"{\"phase\":\"class\",\"class\":%d,\"pass\":%s,\"resolved\":%s,\"planned\":%s,\"private_purchase\":%s,\"display\":%s,\"original_getters_init\":%s}\n",grade,pass?"true":"false",resolved?"true":"false",planned?"true":"false",bought?"true":"false",display_ok?"true":"false",native_ok?"true":"false");
    }
    bool battle_ok=false;
    if(ok){Roots roots;void* cfg=campaign_seeds::decode(DATA_ROOT "offline-seeds/fmc1m.ggs",roots),*battle=local_hero::get(cfg,0x10);
        void* side=local_dictionary_item(local_hero::get(battle,0x178),2),*other=local_dictionary_item(local_hero::get(battle,0x178),1);
        void* baseline=local_hero::get(side,0xf0),*enemy=local_hero::get(other,0xf0);uint64_t hash=hero_battle_projection_audit::type_hash(baseline),enemy_hash=hero_battle_projection_audit::type_hash(enemy);
        uintptr_t handle=0;void* values=local_dictionary_values(baseline,find_class("com.geargames.aow.entities","UnitType"),handle);if(handle)roots.take(handle);
        void* source=nullptr;for(size_t i=0;values&&i<array_length(values);++i){void* t=((void**)((char*)values+32))[i];if(identity(t,owner))source=t;}
        int matched=0;bool shape=source&&hero_ability_shape(source,plan.families[0]),applied=shape&&apply(side,baseline,plan,matched)&&matched==1;
        handle=0;values=applied?local_dictionary_values(local_hero::get(side,0xf0),find_class("com.geargames.aow.entities","UnitType"),handle):nullptr;if(handle)roots.take(handle);
        void* projected=nullptr;for(size_t i=0;values&&i<array_length(values);++i){void* t=((void**)((char*)values+32))[i];if(identity(t,owner))projected=t;}
        battle_ok=applied&&projected&&check(projected,source,total,12,true)&&apply(side,baseline,plan,matched)&&matched==1&&hash&&enemy_hash&&hash==hero_battle_projection_audit::type_hash(baseline)&&enemy_hash==hero_battle_projection_audit::type_hash(enemy);
        fprintf(f,"{\"phase\":\"decoded_battle\",\"shape\":%s,\"applied\":%s,\"pass\":%s}\n",shape?"true":"false",applied?"true":"false",battle_ok?"true":"false");
    }
    bool unchanged=ok&&!memcmp(originals,active,0x80)&&!memcmp(originals+0x80,passive,0x80)&&!memcmp(originals+0x100,aw,0x110)&&!memcmp(originals+0x210,pw,0x110)&&!memcmp(originals+0x320,aura,0x40);
    bool player_same=!memcmp(saved_state.data,&progress,sizeof(progress));
    fprintf(f,"{\"summary\":true,\"pass\":%s,\"classes\":%d,\"battle_projection_repeat\":%s,\"canonical_unchanged\":%s,\"player_state_unchanged\":%s,\"player_save_writes\":0,\"battle_runs\":0}\n",ok&&battle_ok&&unchanged&&player_same?"true":"false",tested,battle_ok?"true":"false",unchanged?"true":"false",player_same?"true":"false");fclose(f);
}
}
