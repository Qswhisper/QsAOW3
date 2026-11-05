// Whole-level integer damage deltas. Target routing is checked separately.
#pragma once
#include "rifle_research_effects.h"
#include "hero_ability_effects.h"
#include "weapon_accuracy_effects.h"
namespace research_effects {
// Accuracy order: static probability, walking multiplier, moving-target lead.
struct WeaponParameters {int damage[3]={},range=0,explosion=0,reload=0,accuracy[3]={},shot_start=0;};
// ShotStart is the pre-fire delay, in original 100ms battle ticks. Aiming is a
// target-category bit mask and must never be changed by an aiming-time row.
inline bool weapon_shot_start(int base,int reduction,int& result){
    if(base<0||base>INT16_MAX||reduction<0||reduction>base)return false;
    result=base-reduction;return true;
}
inline bool weapon_parameters_parse(const char* p,WeaponParameters& result){
    if(!p||!*p)return false;WeaponParameters next;unsigned mask=0;
    while(*p){int token=0,value=0;
        if(*p++!='{'||*p++!='#'||!number(p,token)||*p++!='<')return false;
        int bit=token>=20004&&token<=20007?token-20004:token==20010?4:token==20011?5:token==20008?6:token==20009?7:token==20012?8:token==20019?9:-1;
        if(bit<0||(mask&(1u<<bit)))return false;mask|=1u<<bit;
        if(bit<3||(bit>=6&&bit<=8)){if(!number(p,value)||value<=0||(bit>=6&&value>100))return false;if(bit<3)next.damage[bit]=value;else next.accuracy[bit-6]=value;}
        else if(!hero_effects::decimal100(p,value))return false;
        if(*p++!='>'||*p++!='}')return false;
        if(bit==3)next.range=value;else if(bit==4)next.explosion=value;
        else if(bit==5){if(value%10)return false;next.reload=value/10;}
        else if(bit==9){if(value%10||value/10>INT16_MAX)return false;next.shot_start=value/10;}
        if(*p){if(*p!=' '&&*p!='\n'&&*p!='\r'&&*p!='?')return false;
            while(*p==' '||*p=='\n'||*p=='\r'||*p=='?')++p;if(!*p)return false;}
    }
    if(!mask)return false;result=next;return true;
}
inline bool weapon_damage_parse(const char* p,int (&damage)[3]) {
    if(!p||!*p)return false;int next[3]={};unsigned mask=0;
    while(*p) {
        int token=0,value=0;
        if(*p++!='{'||*p++!='#'||!number(p,token)||*p++!='<'||
           !number(p,value)||*p++!='>'||*p++!='}'||value<=0)return false;
        int t=token-20004;if(t<0||t>2||(mask&(1u<<t)))return false;
        mask|=1u<<t;next[t]=value;
        if(*p){if(*p!=' '&&*p!='\n'&&*p!='\r'&&*p!='?')return false;
            while(*p==' '||*p=='\n'||*p=='\r'||*p=='?')++p;if(!*p)return false;}
    }
    if(!mask)return false;for(int t=0;t<3;++t)damage[t]=next[t];return true;
}
}
