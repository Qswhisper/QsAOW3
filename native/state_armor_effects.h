// Whole original mode-specific armor pairs; quantities are additive integers.
#pragma once
#include "rifle_research_effects.h"
namespace research_effects {
struct StateArmor {int group=0,value[2]={};};
inline bool state_armor_parse(const char* p,StateArmor& result){
    if(!p||!*p)return false;int values[7]={};unsigned mask=0;
    const int tokens[]={20066,20067,20065,20103,20104,20107,20108};
    while(*p){int token=0,value=0;
        if(*p++!='{'||*p++!='#'||!number(p,token)||*p++!='<'||!number(p,value)||*p++!='>'||*p++!='}')return false;
        int index=-1;for(int i=0;i<7;++i)if(tokens[i]==token)index=i;
        if(index<0||value<=0||value>INT16_MAX||(mask&(1u<<index)))return false;
        mask|=1u<<index;values[index]=value;
        if(*p){if(*p!=' '&&*p!='\n'&&*p!='\r'&&*p!='?')return false;
            while(*p==' '||*p=='\n'||*p=='\r'||*p=='?')++p;if(!*p)return false;}
    }
    StateArmor next;
    if(mask==3){next.group=1;next.value[0]=values[0];next.value[1]=values[1];}
    else if(mask==5){next.group=2;next.value[0]=values[0];next.value[1]=values[2];}
    else if(mask==24){next.group=3;next.value[0]=values[3];next.value[1]=values[4];}
    else if(mask==96){next.group=4;next.value[0]=values[5];next.value[1]=values[6];}
    else return false;
    result=next;return true;
}
inline bool state_armor_add(const StateArmor& a,const StateArmor& b,StateArmor& out){
    if(a.group<0||a.group>4||b.group<0||b.group>4||(a.group&&b.group&&a.group!=b.group))return false;
    StateArmor next;next.group=a.group?a.group:b.group;
    for(int i=0;i<2;++i){int64_t sum=int64_t(a.value[i])+b.value[i];
        if(a.value[i]<0||b.value[i]<0||(!a.group&&a.value[i])||(!b.group&&b.value[i])||sum>INT16_MAX)return false;
        next.value[i]=(int)sum;}
    out=next;return true;
}
}
