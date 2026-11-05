// New ability route checks on private original objects, without player grants.
#pragma once
namespace hero_seraphim_audit {
using namespace generic_unit_research;
static void run_one(int hero_id,const char* request,const char* report){
    if(!progress_ready||access(request,F_OK))return;
    unlink(request);auto before=progress;
    void* first=local_hero::first_level(hero_id,1),*rows=local_hero::get(local_hero::get(first,0x48),0x18);
    void* owner=local_career::lookup("BATTLE","unitTypes",hero_id),*old=local_hero::get(owner,0x98);
    void* active=list_item(old,0),*passive=list_item(old,1),*aura=list_item(old,2),*weapon=local_hero::get(hero_id==70?aura:active,0x50);
    bool ok=owner&&rows&&array_length(rows)==12&&active&&passive&&aura&&weapon;
    uint8_t original[0x110]={},original_passive[0x80]={};
    if(ok){memcpy(original,weapon,sizeof(original));memcpy(original_passive,passive,sizeof(original_passive));}
    Plan plan;hero_effects::Delta total;auto fixture=local_progress::seed(20000000,0,0,10000);int tested=0;
    FILE* f=fopen(report,"w");if(!f)return;
    for(size_t i=0;ok&&i<array_length(rows);++i){
        Roots roots;void* row=((void**)((char*)rows+32))[i];Effect e;int hero=0,cards=0,grade=0;
        bool pass=resolve(row,e)&&plan.add(e)&&local_hero::class_price(row,1,hero,cards,grade)&&hero==hero_id&&grade==(int)i+1&&
            local_progress::grant_stock(fixture,9,hero,cards)&&local_progress::buy_card_upgrade(fixture,*(int*)((char*)row+0x10),*(int*)((char*)row+0x20),*(int*)((char*)row+0x28),hero,cards);
        if(i)pass=pass&&hero_effects::add(total,e.ability);
        local_hero::Roots display_roots;void* display=pass?local_hero::display_type(owner,fixture,1,display_roots):nullptr;
        void* projected=pass?copy_type(owner,plan.families[0],roots,true):nullptr;
        auto integer=hero_class_effects_audit::integer;
        pass=pass&&display&&projected&&integer(display,"get_HeroClass",grade)&&integer(projected,"get_HeroClass",grade);
        void* list=local_hero::get(projected,0x98),*a=list_item(list,0),*p=list_item(list,1),*selected=list_item(list,hero_id==70?2:0),*w=local_hero::get(selected,0x50);
        bool route=hero_id==71?list_item(list,2)==aura:local_hero::get(a,0x50)==local_hero::get(active,0x50)&&local_hero::get(projected,0x18)==local_hero::get(owner,0x18);
        pass=pass&&list&&*(int*)((char*)list+0x18)==3&&route;
        if(i){int range=*(int*)((char*)weapon+0x4c)+total.range,padded=range>1000?range+50:range*105/100;
            pass=pass&&a!=active&&p!=passive&&w!=weapon&&integer(a,"get_Cooldown",*(int*)((char*)active+0x34)-total.cooldown[0])&&
                integer(p,"get_Cooldown",*(int*)((char*)passive+0x34)-total.cooldown[1])&&integer(p,"get_Duration",*(int*)((char*)passive+0x40)+total.duration)&&integer(p,"get_Effect",*(int*)((char*)passive+0x44)+total.reduction)&&
                local_hero::get(p,0x50)==local_hero::get(passive,0x50)&&local_hero::get(p,0x70)==local_hero::get(passive,0x70)&&
                *(int*)((char*)w+0x4c)==padded*padded&&*(int*)((char*)w+0x54)==padded&&
                *(int*)((char*)w+0x7c)==*(int*)((char*)weapon+0x7c)+total.explosion&&*(int16_t*)((char*)w+0x60)==*(int16_t*)((char*)weapon+0x60)-total.shot_start;
            for(int j=0;j<3;++j)pass=pass&&*(int*)((char*)w+0x28+4*j)==*(int*)((char*)weapon+0x28+4*j)+total.damage[j];
            uint8_t initialized[0x110];memcpy(initialized,w,sizeof(initialized));void* ex=nullptr;invoke_named(w,"init",0,nullptr,&ex);
            pass=pass&&!ex&&!memcmp(initialized,w,sizeof(initialized));
        }
        ok=ok&&pass;++tested;
        fprintf(f,"{\"hero\":%d,\"class\":%d,\"pass\":%s,\"correct_ability_weapon_route\":%s,\"passive_weapon_and_buff_preserved\":%s}\n",hero_id,grade,pass?"true":"false",route?"true":"false",p&&local_hero::get(p,0x50)==local_hero::get(passive,0x50)&&local_hero::get(p,0x70)==local_hero::get(passive,0x70)?"true":"false");
    }
    bool battle_ok=false;
    if(ok&&tested==12){Roots roots;void* config=campaign_seeds::decode(DATA_ROOT "offline-seeds/fmc1m.ggs",roots);
        void* battle=local_hero::get(config,0x10),*side=local_dictionary_item(local_hero::get(battle,0x178),1),*enemy=local_dictionary_item(local_hero::get(battle,0x178),2);
        void* baseline=local_hero::get(side,0xf0),*enemy_types=local_hero::get(enemy,0xf0);
        uint64_t own_hash=hero_battle_projection_audit::type_hash(baseline),enemy_hash=hero_battle_projection_audit::type_hash(enemy_types);int matched=0;
        uintptr_t pre_handle=0;void* pre=local_dictionary_values(baseline,find_class("com.geargames.aow.entities","UnitType"),pre_handle);if(pre_handle)roots.take(pre_handle);
        for(size_t i=0;pre&&i<array_length(pre);++i){void* t=((void**)((char*)pre+32))[i];if(!identity(t,owner))continue;
            fprintf(f,"{\"phase\":\"battle_source\",\"id\":%d,\"abilities\":%d,\"shape\":%s}\n",*(int*)((char*)t+0x10),local_hero::get(t,0x98)?*(int*)((char*)local_hero::get(t,0x98)+0x18):-1,hero_ability_shape(t,plan.families[0])?"true":"false");
            void* abilities=local_hero::get(t,0x98);
            for(int j=0;abilities&&j<*(int*)((char*)abilities+0x18);++j){void* a=list_item(abilities,j),*w=local_hero::get(a,0x50),*b=local_hero::get(a,0x70);
                fprintf(f,"{\"phase\":\"battle_source_ability\",\"index\":%d,\"ability_id\":%d,\"role\":%d,\"buff_id\":%d,\"weapon_name\":%d,\"weapon_type\":%d,\"weapon_initialized\":%s}\n",j,*(int*)((char*)a+0x10),*(int*)((char*)a+0x30),b?*(int*)((char*)b+0x10):-1,w?*(int16_t*)((char*)w+0x20):-1,w?*(int16_t*)((char*)w+0x24):-1,w&&*(bool*)((char*)w+0x14)?"true":"false");}
            }
        bool applied=own_hash&&enemy_hash&&apply(side,baseline,plan,matched);
        fprintf(f,"{\"phase\":\"battle_apply\",\"applied\":%s,\"matched\":%d}\n",applied?"true":"false",matched);
        battle_ok=applied&&matched==1;
        void* types=local_hero::get(side,0xf0);uintptr_t handle=0;void* all=local_dictionary_values(types,find_class("com.geargames.aow.entities","UnitType"),handle);if(handle)roots.take(handle);
        void* projected=nullptr;for(size_t i=0;all&&i<array_length(all);++i){void* t=((void**)((char*)all+32))[i];if(identity(t,owner))projected=t;}
        battle_ok=battle_ok&&projected&&hero_class_effects_audit::integer(projected,"get_HeroClass",12)&&apply(side,baseline,plan,matched)&&matched==1&&
            own_hash==hero_battle_projection_audit::type_hash(baseline)&&enemy_hash==hero_battle_projection_audit::type_hash(enemy_types)&&enemy_types==local_hero::get(enemy,0xf0);
    }
    bool original_ok=ok&&!memcmp(original,weapon,sizeof(original))&&!memcmp(original_passive,passive,sizeof(original_passive));
    bool isolated=!memcmp(&before,&progress,sizeof(progress));
    fprintf(f,"{\"summary\":true,\"hero\":%d,\"pass\":%s,\"classes\":%d,\"battle_projection_and_repeat\":%s,\"canonical_unchanged\":%s,\"player_state_unchanged\":%s,\"save_written\":false,\"battle_started\":false}\n",hero_id,ok&&battle_ok&&original_ok&&isolated?"true":"false",tested,battle_ok?"true":"false",original_ok?"true":"false",isolated?"true":"false");
    fclose(f);chmod(report,0644);LOG("Local hero class route audit hero=%d pass=%d classes=%d",hero_id,ok&&battle_ok&&original_ok&&isolated,tested);
}
static void run(){
    run_one(71,DATA_ROOT "hero-seraphim-audit.request",DATA_ROOT "hero-seraphim-audit.jsonl");
    run_one(70,DATA_ROOT "hero-cerberus-audit.request",DATA_ROOT "hero-cerberus-audit.jsonl");
}
}
