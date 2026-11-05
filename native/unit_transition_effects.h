// Original research pair: seconds off the two UnitType mode-switch timers.
#pragma once
#include "research_effects.h"
#include "hero_ability_effects.h"
namespace research_effects {
struct UnitTransition {int to=0,from=0;};
inline bool unit_transition_parse(const char* p,UnitTransition& output,int* profile=nullptr){
    if(!p||!*p)return false;UnitTransition next;unsigned mask=0;int group=-1;
    while(*p){int token=0,hundredths=0;
        if(*p++!='{'||*p++!='#'||!number(p,token)||*p++!='<')return false;
        if(token<20026||token>20031)return false;int current=(token-20026)/2;
        if((current&&!profile)||(group>=0&&group!=current))return false;group=current;
        unsigned bit=(token-20026)%2?2:1;
        if(!bit||(mask&bit)||!hero_effects::decimal100(p,hundredths)||hundredths%10||hundredths/10>INT8_MAX||*p++!='>'||*p++!='}')return false;
        mask|=bit;if(bit==1)next.to=hundredths/10;else next.from=hundredths/10;
        if(*p){if(*p!=' '&&*p!='\n'&&*p!='\r'&&*p!='?')return false;
            while(*p==' '||*p=='\n'||*p=='\r'||*p=='?')++p;if(!*p)return false;}
    }
    if(mask!=3)return false;output=next;if(profile)*profile=group;return true;
}
inline bool unit_transition_add(const UnitTransition& a,const UnitTransition& b,UnitTransition& output){
    if(a.to<0||a.from<0||b.to<0||b.from<0||int64_t(a.to)+b.to>INT8_MAX||int64_t(a.from)+b.from>INT8_MAX)return false;
    UnitTransition next{a.to+b.to,a.from+b.from};output=next;return true;
}
inline bool unit_transition_apply(int base_to,int base_from,const UnitTransition& delta,int& to,int& from){
    if(base_to<1||base_to>INT8_MAX||base_from<1||base_from>INT8_MAX||delta.to<0||delta.from<0||delta.to>=base_to||delta.from>=base_from)return false;
    to=base_to-delta.to;from=base_from-delta.from;return true;
}
}
