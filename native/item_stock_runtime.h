// Typed non-card inventory projection. Consumption stays explicitly unavailable.
#pragma once
#include "item_content.generated.h"
namespace local_items {
using Roots=local_faction::Roots;
static bool ready,use_guards_installed;
static void* speedups[4];static uintptr_t speed_roots[4];
static void* get(void* p,int o){return p?*(void**)((char*)p+o):nullptr;}
static void ref(void* p,const char* name,void* value){field_set(p,class_field_from_name(object_class(p),name),value);}
static bool text_equal(void* actual,const char* expected){
    if(!expected)return !actual;if(!actual)return false;void* e=string_new(expected);if(!e)return false;
    int a=*(int*)((char*)actual+16),b=*(int*)((char*)e+16);return a>=0&&a==b&&!memcmp((char*)actual+20,(char*)e+20,a*2);
}
static bool speed_equal(void* v,const item_content::SpeedUp& d){return v&&*(int*)((char*)v+0x10)==d.id&&*(int*)((char*)v+0x30)==d.time&&
    text_equal(get(v,0x18),d.name)&&text_equal(get(v,0x20),d.description)&&text_equal(get(v,0x28),d.icon);}
static void* battery(int id){return local_career::lookup("SHARED","batteries",id);}
static bool initialize(){
    if(ready)return true;
    for(const auto& d:item_content::batteries){void* v=battery(d.id);if(!v||*(int*)((char*)v+0x14)!=d.capacity||*(int*)((char*)v+0x18)!=d.value||
        !text_equal(get(v,0x20),d.name)||!text_equal(get(v,0x28),d.description)||!text_equal(get(v,0x30),d.icon))return false;}
    for(const auto& d:item_content::speedups){int index=d.id-1;if(speedups[index])continue;Roots roots;void* v=object_new(find_class("com.geargames.aow.entities","SpeedUp"));if(!roots.keep(v))return false;
        void* ex=nullptr;invoke_named(v,".ctor",0,nullptr,&ex);if(ex)return false;*(int*)((char*)v+0x10)=d.id;*(int*)((char*)v+0x30)=d.time;
        ref(v,"name",d.name?string_new(d.name):nullptr);ref(v,"desc",d.description?string_new(d.description):nullptr);ref(v,"icon",d.icon?string_new(d.icon):nullptr);
        if(!speed_equal(v,d))return false;speed_roots[index]=gchandle_new(v,true);if(!speed_roots[index])return false;speedups[index]=v;
    }
    ready=true;LOG("Local original item definitions ready batteries=3 speedups=4");return true;
}
static void* speedup(int id){return id>=1&&id<=4&&initialize()?speedups[id-1]:nullptr;}
static int speedup_id(int box_type){return box_type==8?1:box_type==9?2:box_type==10?3:box_type==20?4:0;}
static bool supported(const local_progress::CareerReward& reward){
    if(reward.amount<=0||!initialize())return false;
    return reward.type==7?speedup(reward.id)!=nullptr:reward.type==10?use_guards_installed&&battery(reward.id)!=nullptr:reward.type==11&&reward.id==0;
}
static bool rebind_speedups(){
    if(!initialize())return false;void* screen=static_object("com.geargames.aow.scenes.main","ClientMainContext","<Screen>k__BackingField"),*repository=get(screen,0x290),*services=get(repository,0x20),*entries=get(services,0x18);
    int n=services?*(int*)((char*)services+0x20):0;if(!entries||n<0||n>64)return false;
    for(int i=0;i<n;++i){char* entry=(char*)entries+32+i*24;if(*(int*)entry<0)continue;void* service=*(void**)(entry+16);
        if(!service||strcmp(class_name(object_class(service)),"SpeedUpResourceService"))continue;
        void* rows=get(service,0x38);int count=rows?*(int*)((char*)rows+0x18):0;if(count!=4)return false;
        void* found[4]={};for(int j=0;j<count;++j){void* v=list_item(rows,j);int id=v?*(int*)((char*)v+0x10):0;if(id<1||id>4||found[id-1])return false;
            const auto& d=item_content::speedups[id-1];if(!speed_equal(v,d))return false;found[id-1]=v;}
        for(int j=0;j<4;++j)if(speedups[j]!=found[j]){uintptr_t root=gchandle_new(found[j],true);if(!root)return false;if(speed_roots[j])gchandle_free(speed_roots[j]);speed_roots[j]=root;speedups[j]=found[j];}
        return true;
    }return false;
}
static void* dictionary(void* old,const local_progress::State& s,int type,Roots& roots){
    if(!old||!initialize())return nullptr;void* next=object_new(object_class(old));if(!roots.keep(next))return nullptr;
    void* ex=nullptr;invoke_named(next,".ctor",0,nullptr,&ex);if(ex)return nullptr;
    int max=type==7?4:type==10?3:0;if(!max)return nullptr;
    for(int id=1;id<=max;++id){void* key=type==7?speedup(id):battery(id);int amount=local_progress::stock_amount(s,type,id);if(!key)return nullptr;
        void* args[]={key,&amount};invoke_named(next,"Add",2,args,&ex);if(ex)return nullptr;}
    return next;
}
static bool prepare(void* user,const local_progress::State& s){
    if(!user||!initialize())return false;
    for(uint32_t i=0;i<s.stock_count;++i){const auto& stock=s.stocks[i];if(stock.type==7||stock.type==10||stock.type==11){
        local_progress::CareerReward r{stock.type,stock.id,stock.amount};if(!supported(r))return false;}}
    Roots roots;void* hero=local_faction::clone(get(user,0x110),roots),*upgrade=local_faction::clone(get(user,0x100),roots);
    void* batteries=hero?dictionary(get(hero,0x18),s,10,roots):nullptr,*speed=upgrade?dictionary(get(upgrade,0x30),s,7,roots):nullptr;
    if(!batteries||!speed)return false;ref(hero,"batteries",batteries);ref(upgrade,"speedUps",speed);
    ref(user,"heroConfiguration",hero);ref(user,"upgradeConfiguration",upgrade);return true;
}
static bool publish(){
    if(!offline_network||!progress_ready)return false;void* user=saved("UserConfiguration"),*dispatcher=get(offline_network,0x88);if(!user||!dispatcher||!rebind_speedups()||!prepare(user,progress))return false;
    Roots roots;void* speed=object_new(find_class("com.geargames.aow.messages","SetSpeedUps")),*hero=object_new(find_class("com.geargames.aow.messages","SetHeroInfo"));
    if(!roots.keep(speed)||!roots.keep(hero))return false;void* ex=nullptr;invoke_named(speed,".ctor",0,nullptr,&ex);if(!ex)invoke_named(hero,".ctor",0,nullptr,&ex);
    void* config=get(user,0x110),*old_info=get(config,0x10),*info=old_info?object_new(object_class(old_info)):nullptr;if(!roots.keep(info))return false;
    invoke_named(info,".ctor",0,nullptr,&ex);int n=*(int*)((char*)old_info+0x18);if(n<0||n>128)return false;
    // UpdateBlueprints mutates its retained list. Dispatch an independent
    // collection so its enumerator cannot alias that destination list.
    for(int i=0;i<n&&!ex;++i){void* a[]={list_item(old_info,i)};invoke_named(info,"Add",1,a,&ex);}
    ref(speed,"values",get(get(user,0x100),0x30));ref(hero,"info",info);ref(hero,"batteries",get(config,0x18));
    *(int*)((char*)hero+0x30)=*(int*)((char*)config+0x20);memcpy((char*)hero+0x34,(char*)config+0x24,12);memcpy((char*)hero+0x40,(char*)config+0x30,8);
    for(void* message:{speed,hero}){void* args[]={message};if(!ex)invoke_named(dispatcher,"Dispatch",1,args,&ex);}
    LOG("Local item typed sources published=%d speedup_types=%u battery_types=%u reroll_keys=%d use_enabled=0",!ex,local_progress::stock_types(progress,7),local_progress::stock_types(progress,10),local_progress::stock_amount(progress,11,0));return !ex;
}
}
