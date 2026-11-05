// Three original mine categories form one whole research operation.
#pragma once
#include "weapon_research_effects.h"
namespace research_effects {
struct MineTimes {int reduction[3]={};};
inline bool mine_times_parse(const char* p,MineTimes& out){
    if(!p||!*p)return false;MineTimes next;unsigned mask=0;
    while(*p){int token=0,hundredths=0;
        if(*p++!='{'||*p++!='#'||!number(p,token)||*p++!='<'||token<20088||token>20090||
           !hero_effects::decimal100(p,hundredths)||hundredths%10||*p++!='>'||*p++!='}')return false;
        int i=token-20088;if(mask&(1u<<i))return false;mask|=1u<<i;next.reduction[i]=hundredths/10;
        if(*p){if(*p!=' '&&*p!='\n'&&*p!='\r'&&*p!='?')return false;
            while(*p==' '||*p=='\n'||*p=='\r'||*p=='?')++p;if(!*p)return false;}}
    if(mask!=7)return false;out=next;return true;
}
inline bool mine_times_add(const MineTimes& a,const MineTimes& b,MineTimes& out){
    MineTimes next;for(int i=0;i<3;++i){int64_t n=int64_t(a.reduction[i])+b.reduction[i];
        if(a.reduction[i]<0||b.reduction[i]<0||n>INT_MAX)return false;next.reduction[i]=int(n);}out=next;return true;
}
inline bool mine_time_value(int base,int reduction,int& out){
    if(base<1||reduction<0||reduction>=base)return false;out=base-reduction;return true;
}
}
