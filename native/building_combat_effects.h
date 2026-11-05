// Whole building rows composed from already reviewed weapon fields and sight.
#pragma once
#include "building_weapon_effects.h"
#include <string.h>
namespace research_effects {
inline bool building_sight_value(int base,int increase,int& output){
    if(base<0||increase<0||(int64_t)base+increase>INT_MAX)return false;output=base+increase;return true;
}
inline bool building_combat_parse(const char* text,WeaponParameters& weapon,int& sight,bool& armed){
    if(!text||!*text)return false;char gun[2048]={};size_t used=0;int vision=0;bool seen=false;const char* p=text;
    while(*p){const char* start=p;int token=0;
        if(*p++!='{'||*p++!='#'||!number(p,token)||*p++!='<')return false;
        if(token==20034){int value=0;if(seen||!number(p,value)||value<=0||*p++!='>'||*p++!='}')return false;seen=true;vision=value;}
        else {if((token<20004||token>20012)&&token!=20019)return false;while(*p&&*p!='}')++p;if(*p!='}')return false;++p;
            size_t n=size_t(p-start);if(used+n+2>sizeof(gun))return false;if(used)gun[used++]='\n';memcpy(gun+used,start,n);used+=n;gun[used]=0;}
        if(*p){if(*p!=' '&&*p!='\n'&&*p!='\r'&&*p!='?')return false;
            while(*p==' '||*p=='\n'||*p=='\r'||*p=='?')++p;if(!*p)return false;}}
    WeaponParameters parsed;if(used&&!weapon_parameters_parse(gun,parsed))return false;
    if(!used&&!seen)return false;weapon=parsed;sight=vision;armed=used>0;return true;
}
}
