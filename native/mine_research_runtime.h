// Mine definitions belong to a battle side, not to a UnitStateType weapon list.
#pragma once
#include "mine_research_effects.h"
namespace mine_research {
using local_research_runtime::Roots;
using research_effects::MineTimes;
static void* get(void* p,int o){return p?*(void**)((char*)p+o):nullptr;}
static bool candidate(void* row){
    void* up=get(row,0x48),*owner=get(up,0x48),*fraction=get(owner,0x20);
    bool identity=up&&!strcmp(class_name(object_class(up)),"UpgradeUnit")&&owner&&fraction&&
        *(int*)((char*)fraction+0x10)==2&&*(int*)((char*)owner+0x10)==111&&*(int8_t*)((char*)owner+0x31)==21&&
        *(int8_t*)((char*)up+0x21)==4;
    if(!identity)return false;char text[4096];return local_research_runtime::ascii(get(row,0x18),text,sizeof(text))&&
        (strstr(text,"{#20088<")||strstr(text,"{#20089<")||strstr(text,"{#20090<"));
}
static bool resolve(void* row,MineTimes& out){
    if(!candidate(row))return false;char text[4096];
    return local_research_runtime::ascii(get(row,0x18),text,sizeof(text))&&research_effects::mine_times_parse(text,out);
}
static void* original(int index){return index>=0&&index<3?local_career::lookup("BATTLE","mineTypes",index*2):nullptr;}
static bool same(void* a,void* b){return a&&b&&!strcmp(class_name(object_class(a)),"MineType")&&object_class(a)==object_class(b)&&
    *(int16_t*)((char*)a+0x14)==*(int16_t*)((char*)b+0x14)&&*(int8_t*)((char*)a+0x19)==*(int8_t*)((char*)b+0x19)&&
    *(int8_t*)((char*)a+0x1a)==*(int8_t*)((char*)b+0x1a);}
static bool shape(const MineTimes& d){
    for(int i=0;i<3;++i){void* mine=original(i);int next=0;if(!mine||*(int*)((char*)mine+0x10)!=i*2||
        !research_effects::mine_time_value(*(int*)((char*)mine+0x34),d.reduction[i],next))return false;}return true;
}
static bool supported_saved(void* row){MineTimes d;return resolve(row,d)&&shape(d);}
static bool collect(MineTimes& out,void* extra=nullptr){
    MineTimes sum;bool included=false;
    for(uint32_t i=0;i<progress.upgrade_count;++i){void* row=local_upgrade_level(progress.upgrade_ids[i]);if(!row)return false;
        if(extra&&*(int*)((char*)row+0x10)==*(int*)((char*)extra+0x10))included=true;
        if(!candidate(row))continue;MineTimes d;if(local_faction::active!=2||!resolve(row,d)||!research_effects::mine_times_add(sum,d,sum))return false;}
    if(extra&&!included){MineTimes d;if(local_faction::active!=2||!resolve(extra,d)||!research_effects::mine_times_add(sum,d,sum))return false;}
    bool any=false;for(int value:sum.reduction)any|=value!=0;
    if(any&&!shape(sum))return false;out=sum;return true;
}
static bool purchase_supported(void* live){
    void* archived=live?local_upgrade_level(*(int*)((char*)live+0x10)):nullptr;MineTimes a,b,sum;
    return live&&archived&&resolve(live,a)&&resolve(archived,b)&&!memcmp(&a,&b,sizeof(a))&&
        !*(int*)((char*)live+0x54)&&!*(int*)((char*)archived+0x54)&&collect(sum,archived);
}
static bool prepare(void* side,const MineTimes& d,int& matched){
    matched=0;if(!side)return false;bool any=false;for(int v:d.reduction)any|=v!=0;if(!any)return true;if(!shape(d))return false;
    void* fraction=get(side,0xd8),*old=get(side,0x100);if(!fraction||*(int*)((char*)fraction+0x10)!=2||!old)return false;
    Roots roots;uintptr_t handle=0;void* values=local_dictionary_values(old,find_class("com.geargames.aow.entities","MineType"),handle);
    if(!values||!roots.take(handle))return false;
    void* next=object_new(object_class(old));if(!roots.keep(next))return false;void* ex=nullptr;invoke_named(next,".ctor",0,nullptr,&ex);if(ex)return false;
    bool seen[3]={};
    for(size_t j=0;j<array_length(values);++j){Roots one;void* source=((void**)((char*)values+32))[j],*published=source;if(!source)return false;
        int id=*(int*)((char*)source+0x10);if(local_dictionary_item(old,id)!=source)return false;
        for(int i=0;i<3;++i)if(same(source,original(i))){
            if(seen[i])return false;seen[i]=true;int time=0;
            if(!research_effects::mine_time_value(*(int*)((char*)original(i)+0x34),d.reduction[i],time))return false;
            published=local_research_runtime::clone(source,one);if(!published)return false;*(int*)((char*)published+0x34)=time;++matched;}
        void* args[]={&id,published};invoke_named(next,"Add",2,args,&ex);if(ex)return false;
    }
    field_set(side,class_field_from_name(object_class(side),"$hg"),next);return true;
}
static bool prepare_saved(void* side){
    MineTimes sum;if(!collect(sum))return false;int matched=0;bool ok=prepare(side,sum,matched);
    if(sum.reduction[0]||sum.reduction[1]||sum.reduction[2])LOG("Local mine timing prepared=%d side=%d types=%d absent=%d",ok,*(int*)((char*)side+0x10),matched,3-matched);
    return ok;
}
}
