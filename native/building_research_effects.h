// Exact, whole-level building stat tokens. Unknown/mixed mechanics refuse.
#pragma once
#include "research_effects.h"
namespace research_effects {
struct BuildingStats {
    int hp[5]={},armor=0;
    unsigned hp_mask=0;bool flat_hp=false;
    int power[5]={},training=0,training_token=0;
    unsigned power_mask=0;bool flat_power=false;
};
inline bool building_parse(const char* p,BuildingStats& result) {
    if(!p||!*p)return false;BuildingStats next;bool seen_armor=false;
    while(*p) {
        if(*p!='{')return false;++p;if(*p!='#')return false;++p;
        int token=0,value=0;
        if(!number(p,token)||*p!='<')return false;++p;
        if(!number(p,value)||value<=0||*p!='>')return false;++p;
        if(*p!='}')return false;++p;
        int index=token>=20075&&token<=20078?token-20075:token==20094?4:-1;
        if(token==20000){if(next.hp_mask||next.flat_hp)return false;next.flat_hp=true;next.hp[0]=value;}
        else if(index>=0){if(next.flat_hp||(next.hp_mask&(1u<<index)))return false;next.hp_mask|=1u<<index;next.hp[index]=value;}
        else if(token==20001){if(seen_armor)return false;seen_armor=true;next.armor=value;}
        else if(token==20041){if(next.power_mask||next.flat_power)return false;next.flat_power=true;next.power[0]=value;}
        else if((token>=20079&&token<=20082)||token==20095){
            int i=token==20095?4:token-20079;
            if(next.flat_power||(next.power_mask&(1u<<i)))return false;
            next.power_mask|=1u<<i;next.power[i]=value;
        }
        else if(token==20047||token==20048){if(next.training||value>=100)return false;next.training=value;next.training_token=token;}
        else return false;
        if(*p){if(*p!=' '&&*p!='\n'&&*p!='\r'&&*p!='?')return false;
            while(*p==' '||*p=='\n'||*p=='\r'||*p=='?')++p;
            if(!*p)return false;}
    }
    if(next.hp_mask && next.hp_mask!=7 && next.hp_mask!=31)return false;
    if(next.power_mask && next.power_mask!=7 && next.power_mask!=31)return false;
    result=next;return true;
}
inline bool building_add(const BuildingStats& a,const BuildingStats& b,BuildingStats& out) {
    BuildingStats n=a;
    if((a.flat_hp&&b.hp_mask)||(b.flat_hp&&a.hp_mask)||(a.hp_mask&&b.hp_mask&&a.hp_mask!=b.hp_mask))return false;
    for(int i=0;i<5;++i){int64_t v=(int64_t)a.hp[i]+b.hp[i];if(v<0||v>INT_MAX)return false;n.hp[i]=(int)v;}
    int64_t armor=(int64_t)a.armor+b.armor;if(armor<0||armor>INT_MAX)return false;
    if((a.flat_power&&b.power_mask)||(b.flat_power&&a.power_mask)||
       (a.power_mask&&b.power_mask&&a.power_mask!=b.power_mask)||
       (a.training_token&&b.training_token&&a.training_token!=b.training_token))return false;
    for(int i=0;i<5;++i){int64_t v=(int64_t)a.power[i]+b.power[i];if(v<0||v>INT_MAX)return false;n.power[i]=(int)v;}
    int64_t training=(int64_t)a.training+b.training;if(training<0||training>=100)return false;
    n.training=(int)training;n.training_token=a.training_token?a.training_token:b.training_token;
    n.flat_power=a.flat_power||b.flat_power;n.power_mask=a.power_mask|b.power_mask;
    n.armor=(int)armor;n.flat_hp=a.flat_hp||b.flat_hp;n.hp_mask=a.hp_mask|b.hp_mask;out=n;return true;
}
inline bool building_equal(const BuildingStats& a,const BuildingStats& b) {
    if(a.armor!=b.armor||a.flat_hp!=b.flat_hp||a.hp_mask!=b.hp_mask)return false;
    if(a.training!=b.training||a.training_token!=b.training_token||a.flat_power!=b.flat_power||a.power_mask!=b.power_mask)return false;
    for(int i=0;i<5;++i)if(a.hp[i]!=b.hp[i]||a.power[i]!=b.power[i])return false;return true;
}
inline bool training_time(int baseline,int reduction,int& result){
    if(baseline<=0||reduction<0||reduction>=100)return false;
    int64_t next=(int64_t)baseline*(100-reduction)/100;
    if(next<1||next>INT_MAX)return false;result=(int)next;return true;
}
}
