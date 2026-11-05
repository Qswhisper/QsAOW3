// Whole original descriptions that explicitly label weapon I / weapon II.
#pragma once
#include "unit_stat_effects.h"
#include "weapon_research_effects.h"
#include <stdio.h>
#include <string.h>
namespace research_effects {
struct WeaponGroups {UnitStats scalar;WeaponParameters weapons[2];bool used[2]={};};
inline bool grouped_token(int token,int& group,int& normalized){
    if(token==20053||token==20054){group=token-20053;normalized=20008;return true;}
    if(token==20092||token==20093){group=token-20092;normalized=20007;return true;}
    if(token==20068||token==20069){group=token-20068;normalized=20019;return true;}
    return false;
}
inline bool append_group_token(char* out,size_t capacity,size_t& used,int token,const char* value,size_t length){
    char head[24];int n=snprintf(head,sizeof(head),"%s{#%d<",used?"\n":"",token);
    if(n<1||size_t(n)>=sizeof(head)||used+size_t(n)+length+3>capacity)return false;
    memcpy(out+used,head,n);used+=size_t(n);memcpy(out+used,value,length);used+=length;
    out[used++]='>';out[used++]='}';out[used]=0;return true;
}
inline bool weapon_groups_parse(const char* text,WeaponGroups& result){
    if(!text||!*text)return false;
    struct Token {int id;const char* value;size_t length;};Token tokens[32];int count=0;unsigned groups=0;
    const char* p=text;
    while(*p){int token=0;if(count==32||*p++!='{'||*p++!='#'||!number(p,token)||*p++!='<')return false;
        const char* value=p;while(*p&&*p!='>')++p;if(*p!='>'||p==value)return false;size_t length=size_t(p-value);
        ++p;if(*p++!='}')return false;tokens[count++]={token,value,length};int group=-1,normalized=0;
        if(grouped_token(token,group,normalized))groups|=1u<<group;
        if(*p){if(*p!=' '&&*p!='\n'&&*p!='\r'&&*p!='?')return false;
            while(*p==' '||*p=='\n'||*p=='\r'||*p=='?')++p;if(!*p)return false;}}
    if(!groups)return false;
    char scalar[2048]={},weapons[2][2048]={};size_t ns=0,nw[2]={};
    int current=groups==1?0:groups==2?1:-1;
    for(int i=0;i<count;++i){const auto& t=tokens[i];int group=-1,normalized=t.id;
        bool explicit_group=grouped_token(t.id,group,normalized);
        bool flat=t.id==20000||t.id==20001||t.id==20014||t.id==20034||t.id==20016||t.id==20017;
        if(flat){if(!append_group_token(scalar,sizeof(scalar),ns,t.id,t.value,t.length))return false;continue;}
        if(explicit_group)current=group;
        else {
            if(((t.id<20004||t.id>20012)&&t.id!=20019)||current<0)return false;
            // Plain accuracy immediately following an explicit weapon heading
            // belongs to that heading. Other unlabelled fields in a two-weapon
            // description have no established owner and refuse the whole row.
            if(groups==3&&t.id!=20009&&t.id!=20012)return false;
            group=current;
        }
        if(!append_group_token(weapons[group],sizeof(weapons[group]),nw[group],normalized,t.value,t.length))return false;
    }
    WeaponGroups next;int unknown=0;
    if(ns&&unit_stats_parse(scalar,next.scalar,unknown)!=UnitParse::ok)return false;
    for(int group=0;group<2;++group)if(nw[group]){
        if(!weapon_parameters_parse(weapons[group],next.weapons[group]))return false;next.used[group]=true;}
    result=next;return true;
}
}
