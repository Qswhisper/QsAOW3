// Private canonical/decoded projections. No battle or player save is started.
#pragma once
namespace hero_wasp_audit {
using namespace generic_unit_research;
static bool check(void* type,void* source,const hero_effects::Delta& d,int grade,bool initialized){
    if(!type||!source)return false;auto integer=hero_class_effects_audit::integer;
    void* list=local_hero::get(type,0x98),*original=local_hero::get(source,0x98);
    void* a=list_item(list,0),*p=list_item(list,1),*oa=list_item(original,0),*op=list_item(original,1);
    void* w=local_hero::get(a,0x50),*ow=local_hero::get(oa,0x50);
    if(!a||!p||!oa||!op||!w||!ow||!integer(type,"get_HeroClass",grade))return false;
    bool pass=integer(a,"get_Cooldown",*(int*)((char*)oa+0x34)-d.cooldown[0])&&
        integer(p,"get_Cooldown",*(int*)((char*)op+0x34)-d.cooldown[1])&&
        integer(a,"get_Duration",*(int*)((char*)oa+0x40)+d.active_duration)&&
        integer(a,"get_Effect",*(int*)((char*)oa+0x44)+d.active_effect)&&
        integer(p,"get_Duration",*(int*)((char*)op+0x40)+d.duration)&&
        integer(p,"get_Effect",*(int*)((char*)op+0x44)+d.reduction)&&
        integer(p,"get_Radius",*(int*)((char*)op+0x38)+d.passive_radius)&&
        local_hero::get(a,0x70)==local_hero::get(oa,0x70)&&local_hero::get(p,0x70)==local_hero::get(op,0x70)&&
        !local_hero::get(p,0x50)&&local_hero::get(type,0x18)==local_hero::get(source,0x18);
    int range=*(int*)((char*)ow+0x4c)+d.range,padded=range>1000?range+50:range*105/100;
    pass=pass&&*(int*)((char*)w+0x4c)==(initialized?padded*padded:range)&&
        *(int*)((char*)w+0x7c)==*(int*)((char*)ow+0x7c)+d.explosion&&
        *(int16_t*)((char*)w+0x60)==*(int16_t*)((char*)ow+0x60)-d.shot_start;
    for(int j=0;j<3;++j)pass=pass&&*(int*)((char*)w+0x28+4*j)==*(int*)((char*)ow+0x28+4*j)+d.damage[j];
    if(grade>1)pass=pass&&a!=oa&&p!=op&&w!=ow&&integer(p,"get_RadiusInit",*(int*)((char*)op+0x38)+d.passive_radius);
    return pass;
}
static void run(){
    if(!progress_ready||access(DATA_ROOT "hero-wasp-audit.request",F_OK))return;
    unlink(DATA_ROOT "hero-wasp-audit.request");auto before=progress;
    FILE* f=fopen(DATA_ROOT "hero-wasp-audit.jsonl","w");if(!f)return;
    void* first=local_hero::first_level(170,2),*rows=local_hero::get(local_hero::get(first,0x48),0x18);
    void* owner=local_career::lookup("BATTLE","unitTypes",170),*old=local_hero::get(owner,0x98);
    void* active=list_item(old,0),*passive=list_item(old,1),*weapon=local_hero::get(active,0x50);
    bool ok=owner&&rows&&array_length(rows)==12&&active&&passive&&weapon;
    uint8_t a_before[0x80]={},p_before[0x80]={},w_before[0x110]={};
    if(ok){memcpy(a_before,active,sizeof(a_before));memcpy(p_before,passive,sizeof(p_before));memcpy(w_before,weapon,sizeof(w_before));}
    Plan plan;hero_effects::Delta total;int tested=0;
    auto fixture=local_progress::seed(20000000,0,0,10000);
    for(size_t i=0;ok&&i<array_length(rows);++i){
        Roots roots;void* row=((void**)((char*)rows+32))[i];Effect e;int hero=0,cards=0,grade=0;
        bool resolved=resolve(row,e),planned=resolved&&plan.add(e);
        bool price=local_hero::class_price(row,2,hero,cards,grade)&&hero==170&&grade==(int)i+1;
        bool bought=price&&local_progress::grant_stock(fixture,9,hero,cards)&&
            local_progress::buy_card_upgrade(fixture,*(int*)((char*)row+0x10),*(int*)((char*)row+0x20),*(int*)((char*)row+0x28),hero,cards);
        bool pass=planned&&bought;if(i)pass=pass&&hero_effects::add(total,e.ability);
        local_hero::Roots display_roots;void* display=pass?local_hero::display_type(owner,fixture,2,display_roots):nullptr;
        void* copy=pass?copy_type(owner,plan.families[0],roots,true):nullptr;
        bool display_ok=display&&check(display,owner,total,grade,false);
        bool native_ok=copy&&check(copy,owner,total,grade,i>0);
        if(i&&native_ok){void* w=local_hero::get(list_item(local_hero::get(copy,0x98),0),0x50);uint8_t original[0x110];memcpy(original,w,sizeof(original));
            void* ex=nullptr;invoke_named(w,"init",0,nullptr,&ex);native_ok=!ex&&!memcmp(original,w,sizeof(original));}
        pass=pass&&display_ok&&native_ok;ok=ok&&pass;++tested;
        fprintf(f,"{\"hero\":170,\"class\":%d,\"pass\":%s,\"resolved\":%s,\"planned\":%s,\"private_original_purchase\":%s,\"display\":%s,\"native_getters\":%s}\n",grade,pass?"true":"false",resolved?"true":"false",planned?"true":"false",bought?"true":"false",display_ok?"true":"false",native_ok?"true":"false");
    }
    bool battle_ok=false;
    if(ok&&tested==12){Roots roots;void* config=campaign_seeds::decode(DATA_ROOT "offline-seeds/fmc2m.ggs",roots);
        void* battle=local_hero::get(config,0x10),*side=local_dictionary_item(local_hero::get(battle,0x178),1),*enemy=local_dictionary_item(local_hero::get(battle,0x178),2);
        void* baseline=local_hero::get(side,0xf0),*other=local_hero::get(enemy,0xf0);
        uint64_t own_hash=hero_battle_projection_audit::type_hash(baseline),enemy_hash=hero_battle_projection_audit::type_hash(other);int matched=0;
        uintptr_t handle=0;void* values=local_dictionary_values(baseline,find_class("com.geargames.aow.entities","UnitType"),handle);if(handle)roots.take(handle);
        void* source=nullptr;for(size_t i=0;values&&i<array_length(values);++i){void* t=((void**)((char*)values+32))[i];if(identity(t,owner))source=t;}
        bool shape=source&&hero_ability_shape(source,plan.families[0]);
        bool applied=shape&&apply(side,baseline,plan,matched)&&matched==1;
        uintptr_t out_handle=0;void* out=applied?local_dictionary_values(local_hero::get(side,0xf0),find_class("com.geargames.aow.entities","UnitType"),out_handle):nullptr;if(out_handle)roots.take(out_handle);
        void* projected=nullptr;for(size_t i=0;out&&i<array_length(out);++i){void* t=((void**)((char*)out+32))[i];if(identity(t,owner))projected=t;}
        // Private ability weapons are explicitly native-initialized once.
        battle_ok=applied&&projected&&check(projected,source,total,12,true)&&apply(side,baseline,plan,matched)&&matched==1&&
            own_hash&&enemy_hash&&own_hash==hero_battle_projection_audit::type_hash(baseline)&&enemy_hash==hero_battle_projection_audit::type_hash(other);
        fprintf(f,"{\"phase\":\"decoded_battle\",\"shape\":%s,\"applied\":%s,\"pass\":%s}\n",shape?"true":"false",applied?"true":"false",battle_ok?"true":"false");
    }
    bool original_ok=ok&&!memcmp(a_before,active,sizeof(a_before))&&!memcmp(p_before,passive,sizeof(p_before))&&!memcmp(w_before,weapon,sizeof(w_before));
    bool isolated=!memcmp(&before,&progress,sizeof(progress));
    fprintf(f,"{\"summary\":true,\"hero\":170,\"pass\":%s,\"classes\":%d,\"battle_projection_and_repeat\":%s,\"canonical_unchanged\":%s,\"player_state_unchanged\":%s,\"save_written\":false,\"battle_started\":false}\n",ok&&battle_ok&&original_ok&&isolated?"true":"false",tested,battle_ok?"true":"false",original_ok?"true":"false",isolated?"true":"false");
    fclose(f);chmod(DATA_ROOT "hero-wasp-audit.jsonl",0644);LOG("Local Wasp class effects audit pass=%d classes=%d",ok&&battle_ok&&original_ok&&isolated,tested);
}
}
