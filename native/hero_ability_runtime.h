// Included inside generic_unit_research, after Effect. Only the original
// Shared private weapon projection for reviewed original ability routes.
static bool hero_parameter_shape(void* parameter,const hero_effects::Delta& d){
    if(!parameter||strcmp(class_name(object_class(parameter)),"HeroParam"))return false;
    int distance=0,loss=0,speed=0;
    return (!(d.chain_distance||d.chain_loss_reduction)||hero_effects::chain_parameters(*(int*)((char*)parameter+0x20),*(int*)((char*)parameter+0x28),d,distance,loss))&&
        (!d.salamander_speed||hero_effects::salamander_speed(*(int*)((char*)parameter+0x74),d,speed));
}
static void* copy_hero_parameter(void* parameter,const hero_effects::Delta& d,Roots& roots){
    if(!hero_parameter_shape(parameter,d))return nullptr;
    if(!d.chain_distance&&!d.chain_loss_reduction&&!d.salamander_speed)return parameter;
    void* copy=local_research_runtime::clone(parameter,roots);int distance=0,loss=0;
    if(!copy)return nullptr;
    if(d.chain_distance||d.chain_loss_reduction){
        if(!hero_effects::chain_parameters(*(int*)((char*)parameter+0x20),*(int*)((char*)parameter+0x28),d,distance,loss))return nullptr;
        *(int*)((char*)copy+0x20)=distance;*(int*)((char*)copy+0x28)=loss;
    }
    if(d.salamander_speed){int speed=0;if(!hero_effects::salamander_speed(*(int*)((char*)parameter+0x74),d,speed))return nullptr;*(int*)((char*)copy+0x74)=speed;}
    return copy;
}
static bool ability_weapon_shape(void* weapon,const hero_effects::WeaponDelta& d,bool zero_distance=false){
    if(!weapon||*(bool*)((char*)weapon+0x14)||*(int16_t*)((char*)weapon+0x60)<d.shot_start)return false;
    for(int i=0;i<3;++i)if(*(int*)((char*)weapon+0x28+4*i)<0||(int64_t)*(int*)((char*)weapon+0x28+4*i)+d.damage[i]>INT_MAX)return false;
    int64_t range=(int64_t)*(int*)((char*)weapon+0x4c)+d.range;
    int64_t padded=range>1000?range+50:range*105/100;
    return (range>0||(zero_distance&&range==0&&!d.range))&&padded<=46340&&*(int*)((char*)weapon+0x7c)>=0&&(int64_t)*(int*)((char*)weapon+0x7c)+d.explosion<=INT_MAX;
}
static hero_effects::WeaponDelta active_weapon_delta(const hero_effects::Delta& d){
    hero_effects::WeaponDelta out;for(int i=0;i<3;++i)out.damage[i]=d.damage[i];
    out.range=d.range;out.explosion=d.explosion;out.shot_start=d.shot_start;return out;
}
static bool apply_ability_weapon(void* weapon,const hero_effects::WeaponDelta& d,bool battle,bool zero_distance=false){
    if(!ability_weapon_shape(weapon,d,zero_distance))return false;
    for(int i=0;i<3;++i)*(int*)((char*)weapon+0x28+4*i)+=d.damage[i];
    *(int*)((char*)weapon+0x4c)+=d.range;*(int*)((char*)weapon+0x7c)+=d.explosion;*(int16_t*)((char*)weapon+0x60)-=d.shot_start;
    void* ex=nullptr;if(battle){invoke_named(weapon,"init",0,nullptr,&ex);if(ex||!*(bool*)((char*)weapon+0x14))return false;}return true;
}
static bool hero_ability_shape(void* type,const Effect& e){
    if(!e.hero_ability)return true;
    int hero=*(int*)((char*)e.owner+0x10);
    if(!identity(type,e.owner)||!hero_effects::supported(hero))return false;
    void* list=local_hero::get(type,0x98);if(!list||*(int*)((char*)list+0x18)!=((hero==172||hero==170||hero==73||hero==173||hero==72||hero==174)?2:3))return false;
    void* active=list_item(list,0),*passive=list_item(list,1),*weapon=local_hero::get(list_item(list,hero==70?2:0),0x50);
    if(!active||!passive||!weapon||*(int*)((char*)active+0x30)!=1||*(int*)((char*)passive+0x30)!=2||
        *(bool*)((char*)weapon+0x14))return false;
    if(hero==172){if(local_hero::get(passive,0x50)||*(int16_t*)((char*)weapon+0x20)!=2750||*(int16_t*)((char*)weapon+0x24)!=22)return false;}
    else if(hero==71){void* aura=list_item(list,2),*passive_weapon=local_hero::get(passive,0x50),*buff=local_hero::get(passive,0x70),*aura_weapon=local_hero::get(aura,0x50);
        // Battle prototypes have seat-scoped IDs. Family identity, the native
        // ability roles and weapon keys define this route; retain the exact
        // source buff reference instead of requiring its global dictionary ID.
        if(!aura||*(int*)((char*)aura+0x30)!=3||!passive_weapon||!aura_weapon||!buff||
            *(int16_t*)((char*)weapon+0x20)!=2720||*(int16_t*)((char*)weapon+0x24)!=23||
            *(int16_t*)((char*)passive_weapon+0x20)!=2747||*(int16_t*)((char*)passive_weapon+0x24)!=21||
            *(int16_t*)((char*)aura_weapon+0x20)!=2747||*(int16_t*)((char*)aura_weapon+0x24)!=21||e.ability.reduction)return false;}
    else if(hero==173){
        void* second=local_hero::get(passive,0x50),*aura=local_hero::get(second,0x18),*buff=local_hero::get(passive,0x70);
        if(*(int8_t*)((char*)type+0x31)!=73||local_hero::get(active,0x70)||!buff||!second||!aura||
           *(int16_t*)((char*)weapon+0x20)!=2751||*(int16_t*)((char*)weapon+0x24)!=23||
           *(int16_t*)((char*)second+0x20)!=2751||*(int16_t*)((char*)second+0x24)!=21||
           *(int8_t*)((char*)aura+0x1c)!=10||local_hero::get(aura,0x28)!=buff||
           !ability_weapon_shape(second,e.ability.passive_weapon))return false;
        const auto& d=e.ability;
        if(*(int*)((char*)aura+0x14)<0||(int64_t)*(int*)((char*)aura+0x14)+d.aura_radius>46340||
           *(int*)((char*)aura+0x18)<0||(int64_t)*(int*)((char*)aura+0x18)+d.aura_duration>INT_MAX||
           *(int*)((char*)aura+0x20)<0||(int64_t)*(int*)((char*)aura+0x20)+d.aura_effect>100)return false;
        // These original weapons are independent of ordinary state attacks.
        // Refuse a new alias contract instead of silently missing a reference.
        void* states=local_hero::get(type,0x18);if(!states)return false;
        for(int i=0;i<*(int*)((char*)states+0x18);++i){void* ws=local_hero::get(list_item(states,i),0x18);
            for(int j=0;ws&&j<*(int*)((char*)ws+0x18);++j){void* w=list_item(ws,j);
                if(!w||w==weapon||w==second||(*(int16_t*)((char*)w+0x20)==2751&&
                   (*(int16_t*)((char*)w+0x24)==21||*(int16_t*)((char*)w+0x24)==23)))return false;}}
    }
    else if(hero==175){
        void* second=local_hero::get(passive,0x50),*third=list_item(list,2),*third_weapon=local_hero::get(third,0x50);
        void* active_buff=local_hero::get(active,0x70),*passive_buff=local_hero::get(passive,0x70);
        void* originals=local_hero::get(e.owner,0x98);
        void* source_active_buff=local_hero::get(list_item(originals,0),0x70),*source_passive_buff=local_hero::get(list_item(originals,1),0x70);
        if(*(int8_t*)((char*)type+0x31)!=75||!second||!third||*(int*)((char*)third+0x30)!=3||!third_weapon||
           !active_buff||!passive_buff||!source_active_buff||!source_passive_buff||
           *(int*)((char*)source_active_buff+0x10)!=1005||*(int*)((char*)source_passive_buff+0x10)!=1004||
           *(int8_t*)((char*)active_buff+0x18)!=2||*(int8_t*)((char*)passive_buff+0x18)!=2||
           *(int16_t*)((char*)weapon+0x20)!=2776||*(int16_t*)((char*)weapon+0x24)!=46||
           *(int16_t*)((char*)second+0x20)!=2776||*(int16_t*)((char*)second+0x24)!=20||*(int*)((char*)second+0x4c)!=0||
           *(int16_t*)((char*)third_weapon+0x20)!=20404||*(int16_t*)((char*)third_weapon+0x24)!=10||
           !ability_weapon_shape(second,e.ability.passive_weapon,true))return false;
        void* states=local_hero::get(type,0x18);if(!states)return false;
        for(int i=0;i<*(int*)((char*)states+0x18);++i){void* ws=local_hero::get(list_item(states,i),0x18);
            for(int j=0;ws&&j<*(int*)((char*)ws+0x18);++j){void* w=list_item(ws,j);
                if(!w||w==weapon||w==second||(*(int16_t*)((char*)w+0x20)==2776&&
                   (*(int16_t*)((char*)w+0x24)==46||*(int16_t*)((char*)w+0x24)==20)))return false;}}
    }
    else if(hero==75){
        void* third=list_item(list,2);int angle=0;
        if(*(int8_t*)((char*)type+0x31)!=65||!third||*(int*)((char*)third+0x30)!=3||
           local_hero::get(active,0x70)||local_hero::get(passive,0x50)||local_hero::get(passive,0x70)||
           local_hero::get(third,0x50)||local_hero::get(third,0x70)||
           *(int16_t*)((char*)weapon+0x20)!=2727||*(int16_t*)((char*)weapon+0x24)!=23||
           !hero_effects::shield_angle(*(int*)((char*)passive+0x38),e.ability,angle))return false;
        void* states=local_hero::get(type,0x18);if(!states)return false;
        for(int i=0;i<*(int*)((char*)states+0x18);++i){void* ws=local_hero::get(list_item(states,i),0x18);
            for(int j=0;ws&&j<*(int*)((char*)ws+0x18);++j){void* w=list_item(ws,j);
                if(!w||w==weapon||(*(int16_t*)((char*)w+0x20)==2727&&*(int16_t*)((char*)w+0x24)==23))return false;}}
    }
    else if(hero==174){
        void* buff=local_hero::get(passive,0x70);
        void* original_buff=local_hero::get(list_item(local_hero::get(e.owner,0x98),1),0x70);
        // REGEN_SPEED=8 is the original definition ID, not BuffType.Type.
        // Its retained type is 1. Decoded prototype IDs may be seat-scoped.
        if(*(int8_t*)((char*)type+0x31)!=74||local_hero::get(active,0x70)||local_hero::get(passive,0x50)||!buff||
           !original_buff||*(int*)((char*)original_buff+0x10)!=8||*(int8_t*)((char*)original_buff+0x18)!=1||
           *(int8_t*)((char*)buff+0x18)!=1||*(int16_t*)((char*)weapon+0x20)!=20395||*(int16_t*)((char*)weapon+0x24)!=30||
           !hero_parameter_shape(local_career::list("BATTLE","heroParam"),e.ability))return false;
        void* states=local_hero::get(type,0x18);if(!states)return false;
        for(int i=0;i<*(int*)((char*)states+0x18);++i){void* ws=local_hero::get(list_item(states,i),0x18);
            for(int j=0;ws&&j<*(int*)((char*)ws+0x18);++j){void* w=list_item(ws,j);
                if(!w||w==weapon||(*(int16_t*)((char*)w+0x20)==20395&&*(int16_t*)((char*)w+0x24)==30))return false;}}
    }
    else if(hero==72){
        void* second=local_hero::get(passive,0x50);
        if(*(int8_t*)((char*)type+0x31)!=62||!second||local_hero::get(active,0x70)||local_hero::get(passive,0x70)||
           *(int16_t*)((char*)weapon+0x20)!=2720||*(int16_t*)((char*)weapon+0x24)!=42||
           *(int16_t*)((char*)second+0x20)!=2720||*(int16_t*)((char*)second+0x24)!=70||
           !ability_weapon_shape(second,e.ability.passive_weapon)||
           !hero_parameter_shape(local_career::list("BATTLE","heroParam"),e.ability))return false;
        void* states=local_hero::get(type,0x18);if(!states)return false;
        for(int i=0;i<*(int*)((char*)states+0x18);++i){void* ws=local_hero::get(list_item(states,i),0x18);
            for(int j=0;ws&&j<*(int*)((char*)ws+0x18);++j){void* w=list_item(ws,j);
                if(!w||w==weapon||w==second||(*(int16_t*)((char*)w+0x20)==2720&&
                   (*(int16_t*)((char*)w+0x24)==42||*(int16_t*)((char*)w+0x24)==70)))return false;}}
    }
    else if(hero==170||hero==73){
        if(local_hero::get(passive,0x50)||!local_hero::get(active,0x70)||
           (hero==170?!local_hero::get(passive,0x70):local_hero::get(passive,0x70)!=nullptr)||
           *(int16_t*)((char*)weapon+0x20)!=2750||*(int16_t*)((char*)weapon+0x24)!=(hero==170?23:21)||*((int8_t*)type+0x31)!=(hero==170?70:63))return false;
        // Preserve exact source buff references. Refuse unseen weapon aliases
        // into ordinary attacks instead of applying only part of a class row.
        void* states=local_hero::get(type,0x18);if(!states)return false;
        for(int i=0;i<*(int*)((char*)states+0x18);++i){void* ws=local_hero::get(list_item(states,i),0x18);
            for(int j=0;ws&&j<*(int*)((char*)ws+0x18);++j){void* w=list_item(ws,j);
                if(!w||w==weapon||(*(int16_t*)((char*)w+0x20)==2750&&*(int16_t*)((char*)w+0x24)==(hero==170?23:21)))return false;}}
    }
    else {void* aura=list_item(list,2),*active_weapon=local_hero::get(active,0x50);
        if(!aura||*(int*)((char*)aura+0x30)!=3||!active_weapon||local_hero::get(passive,0x50)||
            *(int16_t*)((char*)weapon+0x20)!=2720||*(int16_t*)((char*)weapon+0x24)!=20||
            *(int16_t*)((char*)active_weapon+0x20)!=2720||*(int16_t*)((char*)active_weapon+0x24)!=30||e.ability.shot_start)return false;
        // Reviewed original route has no aliases into ordinary state weapons.
        // Refuse an unseen source requiring a further runtime migration.
        void* states=local_hero::get(type,0x18);if(!states)return false;
        for(int i=0;i<*(int*)((char*)states+0x18);++i){void* ws=local_hero::get(list_item(states,i),0x18);
            for(int j=0;ws&&j<*(int*)((char*)ws+0x18);++j){void* w=list_item(ws,j);
                if(!w||w==weapon||(*(int16_t*)((char*)w+0x20)==2720&&*(int16_t*)((char*)w+0x24)==20))return false;}}}
    const auto& d=e.ability;
    if(hero!=175&&d.passive_stun)return false;
    if(hero==175&&(d.reduction||d.duration||d.passive_radius||d.passive_weapon.range||d.passive_weapon.shot_start||
       *(int*)((char*)passive+0x44)<0||(int64_t)*(int*)((char*)passive+0x44)+d.passive_stun>INT_MAX))return false;
    if(hero!=75&&(d.shield_half_angle||d.passive_absorption))return false;
    if(hero==75&&(d.reduction||d.passive_radius||d.salamander_speed||d.passive_recovery||
       *(int*)((char*)passive+0x44)<0||(int64_t)*(int*)((char*)passive+0x44)+d.passive_absorption>INT_MAX))return false;
    if(hero!=174&&(d.salamander_speed||d.passive_recovery))return false;
    if(hero==174&&(d.reduction||d.passive_radius||d.chain_distance||d.chain_loss_reduction||
       *(int*)((char*)passive+0x44)<=0||(int64_t)*(int*)((char*)passive+0x44)+d.passive_recovery>INT_MAX))return false;
    if(hero!=170&&hero!=73&&hero!=175&&(d.active_duration||d.active_effect||d.passive_radius))return false;
    if(hero==73&&(d.passive_radius||d.damage[0]||d.damage[1]||d.damage[2]))return false;
    if((hero==170||hero==73||hero==175)&&(*(int*)((char*)active+0x40)<0||(int64_t)*(int*)((char*)active+0x40)+d.active_duration>INT_MAX||
        *(int*)((char*)active+0x44)<0||(int64_t)*(int*)((char*)active+0x44)+d.active_effect>100||
        *(int*)((char*)passive+0x38)<0||(int64_t)*(int*)((char*)passive+0x38)+d.passive_radius>46340))return false;
    if(*(int*)((char*)active+0x34)<=d.cooldown[0]||*(int*)((char*)passive+0x34)<=d.cooldown[1]||
        *(int16_t*)((char*)weapon+0x60)<d.shot_start||
        *(int*)((char*)passive+0x44)<0||(hero!=174&&hero!=75&&hero!=175&&(int64_t)*(int*)((char*)passive+0x44)+d.reduction>100)||
        *(int*)((char*)passive+0x40)<0||(int64_t)*(int*)((char*)passive+0x40)+d.duration>INT_MAX)return false;
    // Native WeaponType.init pads and squares distance in an int. Bound the
    // eventual initialized value before charging or allocating a projection.
    return ability_weapon_shape(weapon,active_weapon_delta(d));
}
static bool copy_hero_abilities(void* copy,void* source,const Effect& e,Roots& roots,bool battle){
    if(!e.hero_ability)return true;if(!hero_ability_shape(source,e))return false;
    void* old=local_hero::get(source,0x98),*list=local_research_runtime::new_list(old,roots);
    void* active=local_research_runtime::clone(list_item(old,0),roots);
    void* passive=local_research_runtime::clone(list_item(old,1),roots);
    int hero=*(int*)((char*)e.owner+0x10);void* selected=hero==70?local_research_runtime::clone(list_item(old,2),roots):active;
    void* weapon=selected?local_research_runtime::clone(local_hero::get(selected,0x50),roots):nullptr;if(!list||!active||!passive||!weapon)return false;
    const auto& d=e.ability;
    *(int*)((char*)active+0x34)-=d.cooldown[0];*(int*)((char*)passive+0x34)-=d.cooldown[1];
    *(int*)((char*)passive+0x40)+=d.duration;*(int*)((char*)passive+0x44)+=hero==174?d.passive_recovery:hero==75?d.passive_absorption:hero==175?d.passive_stun:d.reduction;
    if(hero==75)*(int*)((char*)passive+0x38)+=d.shield_half_angle;
    if(hero==174||hero==75){void* ex=nullptr;invoke_named(passive,"init",0,nullptr,&ex);if(ex)return false;}
    if(hero==170||hero==73||hero==175){
        *(int*)((char*)active+0x40)+=d.active_duration;*(int*)((char*)active+0x44)+=d.active_effect;
        *(int*)((char*)passive+0x38)+=d.passive_radius;
        // Original AbilityType.init only snapshots raw radius to RadiusInit.
        void* ex=nullptr;invoke_named(passive,"init",0,nullptr,&ex);if(ex)return false;
    }
    // Weapon clones are outside the original global weapon dictionary. Run
    // its real initializer explicitly for battle, once, on the private clone.
    void* ex=nullptr;if(!apply_ability_weapon(weapon,active_weapon_delta(d),battle))return false;
    field_set(selected,class_field_from_name(object_class(selected),"weapon"),weapon);
    if(hero==173||hero==72||hero==175){
        void* second=local_research_runtime::clone(local_hero::get(passive,0x50),roots);
        if(!second)return false;
        if(hero==173){
        void* aura=local_research_runtime::clone(local_hero::get(second,0x18),roots);if(!aura)return false;
        // Smoke duration/accuracy are original Aura getters (7d3f504/7d3ff94).
        // Original AuraAct.A calls AuraType.get_Radius at 46687fc and
        // compares actual target distance at 4668bf0. Token20335 is the
        // smoke activity radius; Weapon.radius remains token20010.
        // Keep Ability.radius separately: the launcher consumes it at
        // 4888e7c; it is absent from 173's original displayed params.
        *(int*)((char*)aura+0x14)+=d.aura_radius;*(int*)((char*)aura+0x18)+=d.aura_duration;*(int*)((char*)aura+0x20)+=d.aura_effect;
        field_set(second,class_field_from_name(object_class(second),"aura"),aura);
        }
        if(!apply_ability_weapon(second,d.passive_weapon,battle,hero==175))return false;
        field_set(passive,class_field_from_name(object_class(passive),"weapon"),second);
    }
    for(void* row:{active,passive}){void* args[]={row};invoke_named(list,"Add",1,args,&ex);if(ex)return false;}
    // Seraphim's third native aura has no class delta in this route. Preserve
    // its exact original weapon/buff links rather than dropping or upgrading it.
    for(int i=2;i<*(int*)((char*)old+0x18);++i){void* args[]={hero==70&&i==2?selected:list_item(old,i)};invoke_named(list,"Add",1,args,&ex);if(ex)return false;}
    field_set(copy,class_field_from_name(object_class(copy),"ability"),list);return true;
}
