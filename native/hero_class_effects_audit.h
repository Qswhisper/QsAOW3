// Private original-model/decoded-battle audit. No player grants or writes.
#pragma once
namespace hero_class_effects_audit {
using namespace generic_unit_research;
static bool integer(void* object,const char* getter,int expected){
    void* ex=nullptr;void* box=invoke_named(object,getter,0,nullptr,&ex);
    return !ex&&box&&*(int*)((char*)box+16)==expected;
}
static void run(){
    if(!progress_ready||access(DATA_ROOT "hero-class-effects.request",F_OK))return;
    unlink(DATA_ROOT "hero-class-effects.request");auto before=progress;bool ok=true;int tested=0;
    void* first=local_hero::first_level(172,2),*rows=local_hero::get(local_hero::get(first,0x48),0x18);
    void* owner=local_career::lookup("BATTLE","unitTypes",172),*old=local_hero::get(owner,0x98);
    void* active=list_item(old,0),*passive=list_item(old,1),*weapon=local_hero::get(active,0x50);
    unsigned char original[0x110];if(weapon)memcpy(original,weapon,sizeof(original));else ok=false;
    Plan plan;hero_effects::Delta total;auto fixture=local_progress::seed(20000000,0,0,10000);
    for(size_t i=0;ok&&rows&&i<array_length(rows);++i){
        Roots roots;
        void* row=((void**)((char*)rows+32))[i];Effect e;int hero=0,cards=0,grade=0;
        bool pass=resolve(row,e)&&plan.add(e)&&local_hero::class_price(row,2,hero,cards,grade)&&hero==172&&grade==(int)i+1&&
            local_progress::grant_stock(fixture,9,hero,cards)&&local_progress::buy_card_upgrade(fixture,*(int*)((char*)row+0x10),*(int*)((char*)row+0x20),*(int*)((char*)row+0x28),hero,cards);
        if(i)pass=pass&&hero_effects::add(total,e.ability);
        local_hero::Roots display_roots;void* display=pass?display_projection(owner,fixture,2,display_roots):nullptr;
        void* projected=pass?copy_type(owner,plan.families[0],roots,true):nullptr;
        void* abilities=local_hero::get(projected,0x98),*a=list_item(abilities,0),*p=list_item(abilities,1),*w=local_hero::get(a,0x50);
        pass=pass&&display&&projected&&integer(projected,"get_HeroClass",grade)&&integer(display,"get_HeroClass",grade);
        if(i){
            int64_t range=*(int*)((char*)weapon+0x4c)+total.range;int padded=(int)(range>1000?range+50:range*105/100);
            pass=pass&&a!=active&&p!=passive&&w!=weapon&&
                integer(a,"get_Cooldown",1250-total.cooldown[0])&&integer(p,"get_Cooldown",270-total.cooldown[1])&&
                integer(p,"get_Duration",20+total.duration)&&integer(p,"get_Effect",50+total.reduction)&&
                *(int*)((char*)w+0x4c)==padded*padded&&*(int*)((char*)w+0x54)==padded&&
                *(int*)((char*)w+0x7c)==350+total.explosion&&*(int16_t*)((char*)w+0x60)==*(int16_t*)((char*)weapon+0x60)-total.shot_start;
            for(int j=0;j<3;++j)pass=pass&&*(int*)((char*)w+0x28+4*j)==*(int*)((char*)weapon+0x28+4*j)+total.damage[j]&&
                *(int*)((char*)w+0x34+4*j)==*(int*)((char*)w+0x28+4*j);
            // The original initializer must leave an initialized private weapon
            // unchanged when UnitType.init traverses it a second time.
            unsigned char initialized[0x110];memcpy(initialized,w,sizeof(initialized));void* ex=nullptr;invoke_named(w,"init",0,nullptr,&ex);
            pass=pass&&!ex&&!memcmp(initialized,w,sizeof(initialized));
        }
        ok=ok&&pass;++tested;LOG("Local hero class effects private grade=%d pass=%d",grade,pass);
    }
    bool battle_ok=false;
    if(ok&&tested==12){Roots retained;void* config=campaign_seeds::decode(DATA_ROOT "offline-seeds/fmc2m.ggs",retained);
        void* battle=local_hero::get(config,0x10),*side=local_dictionary_item(local_hero::get(battle,0x178),1);
        void* enemy=local_dictionary_item(local_hero::get(battle,0x178),2),*baseline=local_hero::get(side,0xf0),*enemy_types=local_hero::get(enemy,0xf0);
        auto own_hash=hero_battle_projection_audit::type_hash(baseline),enemy_hash=hero_battle_projection_audit::type_hash(enemy_types);int matched=0;
        battle_ok=apply(side,baseline,plan,matched)&&matched==1;
        int again=0;battle_ok=battle_ok&&apply(side,baseline,plan,again)&&again==1&&own_hash==hero_battle_projection_audit::type_hash(baseline)&&enemy_hash==hero_battle_projection_audit::type_hash(enemy_types);
    }
    bool isolated=!memcmp(&before,&progress,sizeof(progress))&&weapon&&!memcmp(original,weapon,sizeof(original))&&*(int*)((char*)owner+0xa0)==0;
    FILE* file=fopen(DATA_ROOT "hero-class-effects.json","w");if(file){fprintf(file,"{\"pass\":%s,\"hero\":172,\"grades\":%d,\"original_getters\":true,\"native_weapon_init\":true,\"decoded_battle_repeat\":%s,\"canonical_and_player_unchanged\":%s,\"save_written\":false,\"battle_started\":false}\n",ok&&battle_ok&&isolated?"true":"false",tested,battle_ok?"true":"false",isolated?"true":"false");fclose(file);chmod(DATA_ROOT "hero-class-effects.json",0644);}
}
}
