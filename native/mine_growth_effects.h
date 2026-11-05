#pragma once
#include "mine_research_effects.h"
namespace research_effects {
struct MineGrowth {int damage[3]={},life=0,radius=0,time=0;};
inline bool mine_growth_parse(const char* p,MineGrowth& out){
    if(!p||!*p)return false;MineGrowth next;unsigned seen=0;
    while(*p){int token=0,value=0;if(*p++!='{'||*p++!='#'||!number(p,token)||*p++!='<')return false;
        int bit=token>=20004&&token<=20006?token-20004:token==20051?3:token==20010?4:token==20091?5:-1;
        if(bit<0||(seen&(1u<<bit)))return false;seen|=1u<<bit;
        if(bit==4||bit==5){if(!hero_effects::decimal100(p,value))return false;}
        else if(!number(p,value)||value<=0)return false;
        if(*p++!='>'||*p++!='}')return false;
        if(bit<3)next.damage[bit]=value;else if(bit==3)next.life=value;else if(bit==4)next.radius=value;
        else {if(value%10)return false;next.time=value/10;}
        if(*p){if(*p!=' '&&*p!='\n'&&*p!='\r'&&*p!='?')return false;
            while(*p==' '||*p=='\n'||*p=='\r'||*p=='?')++p;if(!*p)return false;}}
    if(seen!=15&&seen!=31&&seen!=32)return false;out=next;return true;
}
inline bool mine_growth_any(const MineGrowth& e){return e.damage[0]||e.damage[1]||e.damage[2]||e.life||e.radius||e.time;}
inline bool mine_growth_add(const MineGrowth& a,const MineGrowth& b,MineGrowth& out){
    MineGrowth next;auto add=[](int a,int b,int limit,int& out){int64_t n=int64_t(a)+b;if(a<0||b<0||n>limit)return false;out=int(n);return true;};
    for(int i=0;i<3;++i)if(!add(a.damage[i],b.damage[i],INT_MAX,next.damage[i]))return false;
    if(!add(a.life,b.life,INT16_MAX,next.life)||!add(a.radius,b.radius,46340,next.radius)||!add(a.time,b.time,INT_MAX,next.time))return false;out=next;return true;
}
inline bool mine_growth_values(const MineGrowth& base,const MineGrowth& d,MineGrowth& out){
    MineGrowth next,positive=d;positive.time=0;
    if(!mine_growth_add(base,positive,next)||base.life<1||base.radius<1||!mine_time_value(base.time,d.time,next.time))return false;
    out=next;return true;
}
}
