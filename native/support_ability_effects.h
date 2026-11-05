#pragma once
#include "hero_ability_effects.h"
#include "rifle_research_effects.h"
namespace research_effects {
struct SupportAura {int radius=0,bonus=0;};
inline bool support_aura_parse(const char* p,SupportAura& out){
    if(!p||!*p)return false;SupportAura next;unsigned seen=0;
    while(*p){int token=0,value=0;if(*p++!='{'||*p++!='#'||!number(p,token)||*p++!='<'||
        (token!=20022&&token!=20023)||!hero_effects::decimal100(p,value)||*p++!='>'||*p++!='}')return false;
        unsigned bit=token==20022?1:2;if(seen&bit)return false;seen|=bit;
        if(bit==1)next.radius=value;else next.bonus=value;
        if(*p){if(*p!=' '&&*p!='\n'&&*p!='\r'&&*p!='?')return false;
            while(*p==' '||*p=='\n'||*p=='\r'||*p=='?')++p;if(!*p)return false;}}
    if(!seen)return false;out=next;return true;
}
inline bool support_aura_add(const SupportAura& a,const SupportAura& b,SupportAura& out){
    if(a.radius<0||b.radius<0||a.bonus<0||b.bonus<0||int64_t(a.radius)+b.radius>46340||int64_t(a.bonus)+b.bonus>=100)return false;
    SupportAura next{a.radius+b.radius,a.bonus+b.bonus};out=next;return true;
}
inline bool support_aura_values(int radius,int bonus,const SupportAura& d,SupportAura& out){
    if(radius<1||bonus<0||bonus>=100)return false;return support_aura_add({radius,bonus},d,out);
}
}
