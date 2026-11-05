// Compose existing reviewed scalar/weapon parsers, retaining whole-row refusal.
#pragma once
#include "unit_stat_effects.h"
#include "weapon_research_effects.h"
#include <string.h>
namespace research_effects {
inline bool composite_unit_weapon_parse(const char* text,UnitStats& scalar,WeaponParameters& weapon){
    if(!text||!*text)return false;char flat[2048]={},shots[2048]={};size_t nf=0,nw=0;
    const char* p=text;
    while(*p){const char* start=p;if(*p!='{')return false;++p;if(*p!='#')return false;++p;
        int token=0;if(!number(p,token)||*p!='<')return false;
        bool unit=token==20000||token==20001||token==20014||token==20034||token==20016||token==20017;
        bool gun=(token>=20004&&token<=20012)||token==20019;if(!unit&&!gun)return false;
        while(*p&&*p!='}')++p;if(*p!='}')return false;++p;
        size_t length=size_t(p-start);char* dest=unit?flat:shots;size_t& used=unit?nf:nw;
        if(used+length+2>2048)return false;if(used)dest[used++]='\n';memcpy(dest+used,start,length);used+=length;dest[used]=0;
        if(*p){if(*p!=' '&&*p!='\n'&&*p!='\r'&&*p!='?')return false;
            while(*p==' '||*p=='\n'||*p=='\r'||*p=='?')++p;if(!*p)return false;}
    }
    UnitStats a;WeaponParameters b;int unknown=0;
    if(!nf||!nw||unit_stats_parse(flat,a,unknown)!=UnitParse::ok||!weapon_parameters_parse(shots,b))return false;
    scalar=a;weapon=b;return true;
}
}
