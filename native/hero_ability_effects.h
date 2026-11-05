// Exact decimal units from the original ability parameter presenters.
// Deliberately whole-row: an unknown token rejects the complete upgrade.
#pragma once
#include <climits>
#include <cstdint>
namespace hero_effects {
// 173 smoke area uses AuraType.Radius in original AuraAct.A (46687fc/4668bf0).
// Ability.Radius is separately consumed by the original launcher (4888e7c).
inline bool supported(int hero){return hero==172||hero==71||hero==70||hero==170||hero==73||hero==173||hero==72||hero==174||hero==75||hero==175;}
inline bool parsable(int hero){return supported(hero)||hero==173;}
inline bool take(const char*& p,char c){if(*p!=c)return false;++p;return true;}
struct WeaponDelta {int damage[3]={},explosion=0,range=0,shot_start=0;};
struct Delta {
    int cooldown[2]={},damage[3]={},explosion=0,range=0,shot_start=0,duration=0,reduction=0;
    int active_duration=0,active_effect=0,passive_radius=0;
    WeaponDelta passive_weapon;
    int aura_duration=0,aura_radius=0,aura_effect=0;
    int chain_distance=0,chain_loss_reduction=0;
    int salamander_speed=0,passive_recovery=0;
    int shield_half_angle=0,passive_absorption=0;
    int passive_stun=0;
};
inline bool shield_angle(int base,const Delta& d,int& result){
    int64_t next=(int64_t)base+d.shield_half_angle;
    if(base<1||base>180||d.shield_half_angle<0||next>180)return false;
    result=(int)next;return true;
}
inline bool salamander_speed(int base,const Delta& d,int& result){
    // Original UnitAct.AI subtracts base movement interval * this percent /100.
    // A nonpositive movement interval is not an acceptable projected profile.
    int64_t next=(int64_t)base+d.salamander_speed;
    if(base<0||base>=100||d.salamander_speed<0||next>=100)return false;
    result=(int)next;return true;
}
inline bool chain_parameters(int distance,int loss,const Delta& d,int& out_distance,int& out_loss){
    int64_t next=(int64_t)distance+d.chain_distance;
    if(distance<1||loss<0||loss>100||d.chain_distance<0||d.chain_loss_reduction<0||
       next>46340||d.chain_loss_reduction>loss)return false;
    out_distance=(int)next;out_loss=loss-d.chain_loss_reduction;return true;
}
inline bool decimal100(const char*& p,int& result){
    if(*p<'0'||*p>'9')return false;int64_t value=0;
    while(*p>='0'&&*p<='9'){value=value*10+*p++-'0';if(value>INT_MAX/100)return false;}
    value*=100;
    if(*p==','){++p;int places=0;while(*p>='0'&&*p<='9'){if(places==2)return false;value+=(*p++-'0')*(places++?1:10);}if(!places)return false;}
    if(value<=0||value>INT_MAX)return false;result=(int)value;return true;
}
inline bool parse(const char* p,Delta& out,int hero=172){
    if(!p||!parsable(hero))return false;Delta next;unsigned masks[2]={};int section=0;
    auto active_complete=[hero](unsigned mask){if(hero==175)return mask==191u||mask==367u||mask==479u||mask==495u||mask==511u;return hero==75?(mask==31u||mask==63u||mask==79u||mask==95u||mask==127u):hero==174?(mask==63u||mask==95u||mask==111u||mask==127u):hero==72?(mask==47u||mask==111u):(hero==173?(mask==63u||mask==127u):(hero==73?(mask==433u||mask==465u||mask==497u):(hero==170?(mask&159u)==159u:(hero==172?mask==127:(hero==70?mask==63:(mask==63||mask==95||mask==127))))));};
    while(*p){
        if(*p=='|'){if(section||!active_complete(masks[0]))return false;++section;++p;}
        if(!take(p,'{')||!take(p,'#'))return false;int token=0,value=0;
        while(*p>='0'&&*p<='9'){token=token*10+*p++-'0';if(token>99999)return false;}
        if(!take(p,'<')||!decimal100(p,value)||!take(p,'>')||!take(p,'}'))return false;
        int bit=-1,scale=100,multiplier=1;int* target=nullptr;
        if(token==20330){bit=0;scale=10;target=&next.cooldown[section];}
        else if(hero==175&&section){
            if(token>=20004&&token<=20006){bit=token-20003;target=&next.passive_weapon.damage[token-20004];}
            else if(token==20335){bit=4;scale=1;target=&next.passive_weapon.explosion;}
            // Original kind75 passive stun presenter reads Effect /10.
            else if(token==20362){bit=12;scale=10;target=&next.passive_stun;}
        }
        else if(hero==175&&!section&&token==20366){bit=7;scale=10;target=&next.active_duration;}
        else if(hero==175&&!section&&token==20367){bit=8;target=&next.active_effect;}
        else if(hero==75&&section){
            if(token==20336){bit=2;scale=10;target=&next.duration;}
            else if(token==20365){bit=15;target=&next.passive_absorption;}
            // Original Atlant angle presenter doubles AbilityType.Radius.
            else if(token==20368){bit=14;scale=200;target=&next.shield_half_angle;}
        }
        else if(hero==174&&section){
            if(token==20336){bit=2;scale=10;target=&next.duration;}
            // Original HealthRecovery presenter divides raw Effect by 600.
            else if(token==20364){bit=12;scale=1;multiplier=6;target=&next.passive_recovery;}
            else if(token==20342){bit=13;target=&next.salamander_speed;}
        }
        else if(hero==72&&section){
            if(token>=20004&&token<=20006){bit=token-20003;target=&next.passive_weapon.damage[token-20004];}
            else if(token==20346){bit=10;scale=1;target=&next.chain_distance;}
            else if(token==20347){bit=11;target=&next.chain_loss_reduction;}
        }
        else if(hero==173&&section){
            if(token>=20004&&token<=20006){bit=token-20003;target=&next.passive_weapon.damage[token-20004];}
            else if(token==20010){bit=4;scale=1;target=&next.passive_weapon.explosion;}
            else if(token==20007){bit=5;scale=1;target=&next.passive_weapon.range;}
            else if(token==20336){bit=7;scale=10;target=&next.aura_duration;}
            else if(token==20335){bit=8;scale=1;target=&next.aura_radius;}
            else if(token==20341){bit=9;target=&next.aura_effect;}
        }
        else if(!section&&token>=20004&&token<=20006){bit=token-20003;target=&next.damage[token-20004];}
        else if(!section&&token==20010){bit=4;scale=1;target=&next.explosion;}
        else if(!section&&token==20339){bit=5;scale=1;target=&next.range;}
        else if(hero==173&&!section&&token==20007){bit=5;scale=1;target=&next.range;}
        else if(hero==174&&!section&&token==20007){bit=5;scale=1;target=&next.range;}
        else if(hero==75&&!section&&token==20007){bit=5;scale=1;target=&next.range;}
        else if(hero==175&&!section&&token==20007){bit=5;scale=1;target=&next.range;}
        else if(!section&&token==20349){bit=6;scale=10;target=&next.shot_start;}
        // This retained class profile uses the legacy aiming-time label for
        // active pre-fire delay. Do not modify WeaponType.Aiming's bit flags.
        else if(hero==73&&!section&&token==20019){bit=6;scale=10;target=&next.shot_start;}
        else if(section&&token==20337&&hero!=71&&hero!=170){bit=1;target=&next.reduction;}
        else if(section&&token==20336){bit=2;scale=10;target=&next.duration;}
        else if((hero==170||hero==73)&&!section&&token==20336){bit=7;scale=10;target=&next.active_duration;}
        else if((hero==170||hero==73)&&!section&&token==20340){bit=8;target=&next.active_effect;}
        // Beholder active RADIUS is its ability weapon's explosion radius;
        // original 7d3faa8 selects this path for unit kind63/active role1.
        else if(hero==73&&!section&&token==20335){bit=4;scale=1;target=&next.explosion;}
        else if(hero==170&&section&&token==20338){bit=1;target=&next.reduction;}
        else if(hero==170&&section&&token==20335){bit=3;scale=1;target=&next.passive_radius;}
        if(!target||(masks[section]&(1u<<bit))||value%scale||(int64_t)(value/scale)*multiplier>INT_MAX)return false;
        masks[section]|=1u<<bit;*target=(value/scale)*multiplier;
        while(*p=='\n'||*p=='\r'||*p==' ')++p;
    }
    bool passive_complete=hero==175?(masks[1]==15u||masks[1]==4111u||masks[1]==4127u):hero==75?(masks[1]==49157u||masks[1]==32773u):hero==174?(masks[1]==12293u||masks[1]==4101u):hero==72?(masks[1]==1039u||masks[1]==2063u):masks[1]==(hero==173?959u:(hero==170?15u:(hero==71?5u:7u)));
    if(section!=1||!active_complete(masks[0])||!passive_complete)return false;out=next;return true;
}
inline bool add(Delta& a,const Delta& b){
    Delta next=a;
    for(int i=0;i<2;++i){if((int64_t)a.cooldown[i]+b.cooldown[i]>INT_MAX)return false;next.cooldown[i]+=b.cooldown[i];}
    for(int i=0;i<3;++i){if((int64_t)a.damage[i]+b.damage[i]>INT_MAX)return false;next.damage[i]+=b.damage[i];}
    int* targets[]={&next.explosion,&next.range,&next.shot_start,&next.duration,&next.reduction,&next.active_duration,&next.active_effect,&next.passive_radius};
    const int values[]={b.explosion,b.range,b.shot_start,b.duration,b.reduction,b.active_duration,b.active_effect,b.passive_radius};
    for(int i=0;i<8;++i){if((int64_t)*targets[i]+values[i]>INT_MAX)return false;*targets[i]+=values[i];}
    int* extra[]={next.passive_weapon.damage,next.passive_weapon.damage+1,next.passive_weapon.damage+2,
        &next.passive_weapon.explosion,&next.passive_weapon.range,&next.passive_weapon.shot_start,
        &next.aura_duration,&next.aura_radius,&next.aura_effect,&next.chain_distance,&next.chain_loss_reduction,
        &next.salamander_speed,&next.passive_recovery,&next.shield_half_angle,&next.passive_absorption,&next.passive_stun};
    const int more[]={b.passive_weapon.damage[0],b.passive_weapon.damage[1],b.passive_weapon.damage[2],
        b.passive_weapon.explosion,b.passive_weapon.range,b.passive_weapon.shot_start,b.aura_duration,b.aura_radius,b.aura_effect,b.chain_distance,b.chain_loss_reduction,
        b.salamander_speed,b.passive_recovery,b.shield_half_angle,b.passive_absorption,b.passive_stun};
    for(int i=0;i<16;++i){if((int64_t)*extra[i]+more[i]>INT_MAX)return false;*extra[i]+=more[i];}
    a=next;return true;
}
}
