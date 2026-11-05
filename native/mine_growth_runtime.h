#pragma once
#include "mine_growth_effects.h"
namespace mine_growth {
using mine_research::get;using mine_research::same;using local_research_runtime::Roots;using research_effects::MineGrowth;
struct Effect {int mine=-1;MineGrowth delta;};
struct Plan {MineGrowth mines[4];};
static void* original(int index){return index>=0&&index<4?local_career::lookup("BATTLE","mineTypes",index*2):nullptr;}
static bool candidate(void* row){
    void* up=get(row,0x48),*owner=get(up,0x48),*f=get(owner,0x20);if(!up||strcmp(class_name(object_class(up)),"UpgradeUnit")||!owner||!f||*(int*)((char*)f+0x10)!=2)return false;
    int id=*(int*)((char*)owner+0x10),slot=*(int8_t*)((char*)up+0x21);char text[4096];
    if(!((id==111&&slot>=5&&slot<=7)||(id==125&&(slot==6||slot==7)))||!local_research_runtime::ascii(get(row,0x18),text,sizeof(text)))return false;
    return strstr(text,"{#20051<")||strstr(text,"{#20091<");
}
static bool resolve(void* row,Effect& out){
    if(!candidate(row))return false;void* up=get(row,0x48),*owner=get(up,0x48);int id=*(int*)((char*)owner+0x10),slot=*(int8_t*)((char*)up+0x21);char text[4096];Effect next;
    if(!local_research_runtime::ascii(get(row,0x18),text,sizeof(text))||!research_effects::mine_growth_parse(text,next.delta))return false;
    next.mine=id==111?slot-5:3;
    if(next.mine<0||next.mine>3||(next.delta.time&&(id!=125||slot!=6))||(!next.delta.time&&id==125&&slot!=7))return false;
    out=next;return true;
}
static MineGrowth read(void* mine){MineGrowth r;if(!mine)return r;for(int i=0;i<3;++i)r.damage[i]=*(int*)((char*)mine+0x1c+4*i);
    r.radius=*(int*)((char*)mine+0x28);r.time=*(int*)((char*)mine+0x34);r.life=*(int16_t*)((char*)mine+0x38);return r;}
static bool shape(const Plan& p){for(int i=0;i<4;++i)if(research_effects::mine_growth_any(p.mines[i])){void* base=original(i);MineGrowth values;
    if(!base||*(int*)((char*)base+0x10)!=i*2||!research_effects::mine_growth_values(read(base),p.mines[i],values))return false;}return true;}
static bool supported_saved(void* row){Effect e;Plan p;if(!resolve(row,e))return false;p.mines[e.mine]=e.delta;return shape(p);}
static bool collect(Plan& p,void* extra=nullptr){Plan next;bool included=false;
    for(uint32_t i=0;i<progress.upgrade_count;++i){void* row=local_upgrade_level(progress.upgrade_ids[i]);if(!row)return false;
        if(extra&&*(int*)((char*)extra+0x10)==*(int*)((char*)row+0x10))included=true;if(!candidate(row))continue;Effect e;
        if(local_faction::active!=2||!resolve(row,e)||!research_effects::mine_growth_add(next.mines[e.mine],e.delta,next.mines[e.mine]))return false;}
    if(extra&&!included){Effect e;if(local_faction::active!=2||!resolve(extra,e)||!research_effects::mine_growth_add(next.mines[e.mine],e.delta,next.mines[e.mine]))return false;}
    if(!shape(next))return false;p=next;return true;
}
static bool purchase_supported(void* live){Effect a,b;Plan p;void* archived=live?local_upgrade_level(*(int*)((char*)live+0x10)):nullptr;
    return resolve(live,a)&&resolve(archived,b)&&a.mine==b.mine&&!memcmp(&a.delta,&b.delta,sizeof(a.delta))&&
        !*(int*)((char*)live+0x54)&&!*(int*)((char*)archived+0x54)&&collect(p,archived);}
static bool prepare(void* side,const Plan& p,int& matched){
    matched=0;if(!side||!shape(p))return false;bool any=false;for(const auto& d:p.mines)any|=research_effects::mine_growth_any(d);if(!any)return true;
    void* f=get(side,0xd8),*old=get(side,0x100);if(!f||*(int*)((char*)f+0x10)!=2||!old)return false;
    Roots roots;uintptr_t h=0;void* values=local_dictionary_values(old,find_class("com.geargames.aow.entities","MineType"),h);if(!values||!roots.take(h))return false;
    void* dictionary=object_new(object_class(old));if(!roots.keep(dictionary))return false;void* ex=nullptr;invoke_named(dictionary,".ctor",0,nullptr,&ex);if(ex)return false;bool seen[4]={};
    for(size_t j=0;j<array_length(values);++j){Roots one;void* source=((void**)((char*)values+32))[j],*next=source;if(!source)return false;int id=*(int*)((char*)source+0x10);
        if(local_dictionary_item(old,id)!=source)return false;
        for(int i=0;i<4;++i)if(research_effects::mine_growth_any(p.mines[i])&&same(source,original(i))){
            if(seen[i])return false;seen[i]=true;MineGrowth v;const auto& d=p.mines[i];if(!research_effects::mine_growth_values(read(original(i)),d,v))return false;
            next=local_research_runtime::clone(source,one);if(!next)return false;
            for(int k=0;k<3;++k)if(d.damage[k])*(int*)((char*)next+0x1c+4*k)=v.damage[k];
            if(d.radius)*(int*)((char*)next+0x28)=v.radius;if(d.life)*(int16_t*)((char*)next+0x38)=v.life;
            if(d.time)*(int*)((char*)next+0x34)=v.time;
            // Retained Frog/Lizard definitions are initially unavailable. A
            // purchased named mine-development row supplies that category;
            // absent scenario types and categories without receipts stay put.
            if((i==1||i==2)&&d.life&&*(int8_t*)((char*)original(i)+0x18)==0)*(int8_t*)((char*)next+0x18)=1;
            ++matched;}
        void* args[]={&id,next};invoke_named(dictionary,"Add",2,args,&ex);if(ex)return false;}
    field_set(side,class_field_from_name(object_class(side),"$hg"),dictionary);return true;
}
static bool prepare_saved(void* side){Plan p;if(!collect(p))return false;int matched=0;bool ok=prepare(side,p,matched);
    if(matched)LOG("Local mine growth prepared=%d side=%d types=%d",ok,*(int*)((char*)side+0x10),matched);return ok;}
}
