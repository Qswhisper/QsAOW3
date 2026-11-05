// Flat unit effects shared by all prototype families. The legacy damage
// selector remains separate until the weapon/slot contracts are recovered.
#pragma once
#include "rifle_research_effects.h"
namespace research_effects {
using UnitStats = RifleDelta;
enum class UnitParse { ok, empty, syntax, unknown_token, duplicate, nonpositive };
inline UnitParse unit_stats_parse(const char* p,UnitStats& result,int& unknown) {
    if(!p||!*p)return UnitParse::empty;
    UnitStats next;unsigned seen=0;unknown=0;
    while(*p) {
        if(*p++!='{'||*p++!='#')return UnitParse::syntax;
        int token=0,value=0;
        if(!number(p,token)||*p++!='<'||!number(p,value)||*p++!='>'||*p++!='}')return UnitParse::syntax;
        int index=token==20000?0:token==20001?1:token==20014?2:token==20034?3:
                  (token==20016||token==20017)?4:-1;
        if(index<0){unknown=token;return UnitParse::unknown_token;}
        if(seen&(1u<<index))return UnitParse::duplicate;
        if(value<=0)return UnitParse::nonpositive;
        seen|=1u<<index;
        if(index==0)next.hp=value;else if(index==1)next.armor=value;else if(index==2)next.speed=value;
        else if(index==3)next.sight=value;else next.regen=value;
        if(*p) {
            if(*p!=' '&&*p!='\n'&&*p!='\r'&&*p!='?')return UnitParse::syntax;
            while(*p==' '||*p=='\n'||*p=='\r'||*p=='?')++p;
            if(!*p)return UnitParse::syntax;
        }
    }
    result=next;return UnitParse::ok;
}
inline const char* unit_parse_reason(UnitParse p) {
    switch(p){case UnitParse::ok:return "flat_unit_stats";case UnitParse::empty:return "empty_description";
    case UnitParse::unknown_token:return "unknown_effect_token";case UnitParse::duplicate:return "duplicate_effect_token";
    case UnitParse::nonpositive:return "nonpositive_effect";default:return "unverified_description_format";}
}
}
