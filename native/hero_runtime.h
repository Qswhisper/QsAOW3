// Hero card inventory uses the career's atomic wallet/upgrade ledger.
// The original rank gate, card definitions, costs and views remain authoritative.
#pragma once
#include "hero_ability_effects.h"
namespace local_hero {
using Roots=local_faction::Roots;
static bool (*capacity_projector)(void*,const local_progress::State&);
static void ref(void* value,const char* field,void* data){field_set(value,class_field_from_name(object_class(value),field),data);}
static void* get(void* value,int offset){return value?*(void**)((char*)value+offset):nullptr;}
static void* card(int id,int faction){
    void* c=local_career::lookup("SHARED","heroCards",id);void* f=get(c,0x38);
    return id<200&&f&&*(int*)((char*)f+0x10)==faction&&*(int*)((char*)c+0x50)>0?c:nullptr;
}
static void* first_level(int id,int faction){
    void* upgrades=local_career::list(local_career::key(faction),"upgradeUnits");
    for(int i=0;upgrades&&i<*(int*)((char*)upgrades+0x18);++i){void* u=list_item(upgrades,i);void* owner=get(u,0x48);
        if(!owner||*((int8_t*)u+0x21)!=20||*(int*)((char*)owner+0x10)!=id)continue;
        void* levels=get(u,0x18);return levels&&array_length(levels)?((void**)((char*)levels+32))[0]:nullptr;}
    return nullptr;
}
static bool acquisition(void* level,int faction,int& hero,int& amount){
    void* upgrade=get(level,0x48),*owner=get(upgrade,0x48),*fraction=get(owner,0x20);
    if(!upgrade||strcmp(class_name(object_class(upgrade)),"UpgradeUnit")||*((int8_t*)upgrade+0x21)!=20||!fraction||*(int*)((char*)fraction+0x10)!=faction)return false;
    hero=*(int*)((char*)owner+0x10);void* definition=card(hero,faction),*first=first_level(hero,faction);
    if(!definition||!first||*(int*)((char*)first+0x10)!=*(int*)((char*)level+0x10))return false;
    amount=*(int*)((char*)definition+0x50);
    return !get(level,0x18)&&!get(first,0x18)&&!*(int*)((char*)level+0x20)&&!*(int*)((char*)first+0x20)&&
        !*(int*)((char*)level+0x28)&&!*(int*)((char*)first+0x28)&&!*(int*)((char*)level+0x50)&&
        *(int*)((char*)level+0x54)==amount&&*(int*)((char*)first+0x54)==amount&&amount>0;
}
static bool acquired(const local_progress::State& state,int hero,int faction){void* level=first_level(hero,faction);return level&&local_progress::has_upgrade(state,*(int*)((char*)level+0x10));}
static bool class_price(void* level,int faction,int& hero,int& amount,int& grade){
    void* upgrade=get(level,0x48),*owner=get(upgrade,0x48),*fraction=get(owner,0x20);
    if(!upgrade||strcmp(class_name(object_class(upgrade)),"UpgradeUnit")||*((int8_t*)upgrade+0x21)!=20||!fraction||*(int*)((char*)fraction+0x10)!=faction)return false;
    hero=*(int*)((char*)owner+0x10);if(!card(hero,faction)||*(int*)((char*)level+0x50))return false;
    void* rows=get(upgrade,0x18);grade=0;
    for(size_t i=0;rows&&i<array_length(rows);++i){void* row=((void**)((char*)rows+32))[i];if(row&&*(int*)((char*)row+0x10)==*(int*)((char*)level+0x10))grade=(int)i+1;}
    amount=*(int*)((char*)level+0x54);return grade>0&&grade<=12&&amount>0;
}
static void* (*project_display)(void*,const local_progress::State&,int,Roots&);
static void* display_type(void* type,const local_progress::State& state,int faction,Roots& roots){
    void* fraction=get(type,0x20);int id=type?*(int*)((char*)type+0x10):-1;
    if(!fraction||*(int*)((char*)fraction+0x10)!=faction||!card(id,faction))return nullptr;
    if(hero_effects::supported(id)&&project_display)return project_display(type,state,faction,roots);
    void* copy=local_faction::clone(type,roots);if(copy)*(int*)((char*)copy+0xa0)=acquired(state,id,faction)?1:0;return copy;
}
struct Display {void* data;uintptr_t handle;int hero,faction;};
static Display displays[16];static int display_count;
static void (*clear_extra_displays)();
static void clear_displays(){while(display_count)gchandle_free(displays[--display_count].handle);if(clear_extra_displays)clear_extra_displays();}
static void (*prototype_original)(void*,void*,void*);
static void prototype(void* self,void* type,void* method){
    Roots roots;void* adapted=offline_network&&progress_ready?display_type(type,progress,local_faction::active,roots):nullptr;
    prototype_original(self,adapted?adapted:type,method);
    if(adapted){int hero=*(int*)((char*)type+0x10),slot=-1;
        for(int i=0;i<display_count;++i)if(displays[i].hero==hero&&displays[i].faction==local_faction::active){slot=i;break;}
        if(slot<0&&display_count<16)slot=display_count++;
        if(slot>=0){uintptr_t handle=gchandle_new(self,true);if(handle){if(displays[slot].handle)gchandle_free(displays[slot].handle);displays[slot]={self,handle,hero,local_faction::active};}}
    }
}
struct Publication {void* source=nullptr;void* values=nullptr;void* type=nullptr;int hero=0;};
static void* infos(const local_progress::State&,int,Roots&);
static bool prepare_publication(const local_progress::State& state,int hero,Roots& roots,Publication& p){
    p.source=static_object("com.geargames.aow.scenes.main","ClientMainContext","<HeroConfigurationDataSource>k__BackingField");
    p.values=infos(state,local_faction::active,roots);p.hero=hero;
    p.type=display_type(local_career::lookup("BATTLE","unitTypes",hero),state,local_faction::active,roots);
    return p.source&&p.values&&p.type&&class_method(object_class(p.source),"UpdateBlueprints",1);
}
static bool publish(const Publication& p){
    void* ex=nullptr;void* args[]={p.values};invoke_named(p.source,"UpdateBlueprints",1,args,&ex);if(ex)return false;
    for(int i=0;i<display_count;++i){auto& d=displays[i];if(d.hero!=p.hero||d.faction!=local_faction::active)continue;
        void* type=get(d.data,0x18);if(!type||type==local_career::lookup("BATTLE","unitTypes",p.hero))return false;
        field_set(d.data,class_field_from_name(object_class(d.data),"m_prototype"),p.type);
        invoke_named(d.data,"InvokeDataUpdated",0,nullptr,&ex);if(ex)return false;}
    return true;
}
static bool append(void* list,void* value){void* ex=nullptr;void* args[]={value};invoke_named(list,"Add",1,args,&ex);return !ex;}
static void* infos(const local_progress::State& state,int faction,Roots& roots){
    void* klass=find_class("com.geargames.aow.entities","HeroConfiguration");
    void* field=klass?class_field_from_name(klass,"heroInfos"):nullptr;
    void* values=field?object_new(class_from_type(field_type(field))):nullptr;void* ex=nullptr;
    if(!roots.keep(values))return nullptr;invoke_named(values,".ctor",0,nullptr,&ex);if(ex)return nullptr;
    for(uint32_t i=0;i<state.stock_count;++i)if(state.stocks[i].type==9&&!card(state.stocks[i].id,faction)){
        LOG("Local hero inventory refused unknown/cross-faction card=%d",state.stocks[i].id);return nullptr;}
    void* definitions=local_career::list("SHARED","heroCards");int count=definitions?*(int*)((char*)definitions+0x18):0;
    for(int i=0;i<count;++i){void* definition=list_item(definitions,i);if(!definition||!card(*(int*)((char*)definition+0x10),faction))continue;
        void* value=object_new(find_class("com.geargames.aow.entities","HeroInfo"));if(!roots.keep(value))return nullptr;
        void* args[]={definition};invoke_named(value,".ctor",1,args,&ex);if(ex)return nullptr;
        *(int*)((char*)value+0x18)=local_progress::stock_amount(state,9,*(int*)((char*)definition+0x10));if(!append(values,value))return nullptr;
    }
    return values;
}
static void audit(){
    if(!progress_ready||access(DATA_ROOT "hero-audit.request",F_OK))return;
    unlink(DATA_ROOT "hero-audit.request");auto before=progress;void* user=saved("UserConfiguration");void* original=get(user,0x110);
    bool ok=true;int factions=0;
    for(int f:{1,2}){Roots roots;auto fixture=local_progress::seed(100,0,0,9);int id=f==1?70:170;
        void* definition=card(id,f);int amount=definition?*(int*)((char*)definition+0x50):0;
        ok=ok&&amount>0&&local_progress::grant_stock(fixture,9,id,amount);void* data=ok?infos(fixture,f,roots):nullptr;
        int total=0,hits=0,n=data?*(int*)((char*)data+0x18):0;
        for(int i=0;i<n;++i){void* row=list_item(data,i),*c=get(row,0x10),*fraction=get(c,0x38);int value=*(int*)((char*)row+0x18);
            ok=ok&&fraction&&*(int*)((char*)fraction+0x10)==f;total+=value;if(*(int*)((char*)c+0x10)==id&&value==amount)++hits;}
        ok=ok&&n==6&&total==amount&&hits==1;
        // Exercise the original reactive data source on private objects, including
        // refresh to zero after a spend. No fixture is published to the player.
        void* config=local_faction::clone(original,roots);
        void* source=object_new(find_class("com.geargames.aow.assets.scenes.main.network","HeroConfigurationDataSource"));
        void* logger=object_new(find_class("com.geargames.aow.logger.wrapper","LoggerWrapper"));
        void* ex=nullptr;if(!config||!roots.keep(source)||!roots.keep(logger)||!data){ok=false;break;}
        invoke_named(logger,".ctor",0,nullptr,&ex);
        ref(config,"heroInfos",data);void* ctor_args[]={logger};if(!ex)invoke_named(source,".ctor",1,ctor_args,&ex);
        void* set_args[]={config};if(!ex)invoke_named(source,"SetConfiguration",1,set_args,&ex);
        ok=ok&&!ex&&local_progress::consume_stock(fixture,9,id,amount);
        void* empty=ok?infos(fixture,f,roots):nullptr;void* update_args[]={empty};
        if(empty&&!ex)invoke_named(source,"UpdateBlueprints",1,update_args,&ex);else ok=false;
        void* actual=!ex?invoke_named(source,"get_Configuration",0,nullptr,&ex):nullptr;
        void* rows=get(actual,0x10);int remaining=0;
        for(int i=0;rows&&i<*(int*)((char*)rows+0x18);++i)remaining+=*(int*)((char*)list_item(rows,i)+0x18);
        LOG("Local hero private data source faction=%d rows=%d remaining=%d exception=%s",f,rows?*(int*)((char*)rows+0x18):-1,remaining,ex?class_name(object_class(ex)):"none");
        ok=ok&&!ex&&rows&&*(int*)((char*)rows+0x18)==6&&remaining==0;++factions;
    }
    bool isolated=!memcmp(&before,&progress,sizeof(progress))&&get(user,0x110)==original;
    FILE* file=fopen(DATA_ROOT "hero-audit.json","w");if(file){fprintf(file,"{\"pass\":%s,\"factions\":%d,\"private_hero_infos\":true,\"original_data_source_zero_refresh\":%s,\"player_state_unchanged\":%s,\"battle_started\":false,\"save_written\":false}\n",ok&&isolated?"true":"false",factions,ok?"true":"false",isolated?"true":"false");fclose(file);chmod(DATA_ROOT "hero-audit.json",0644);}
    LOG("Local hero private model audit passed=%d factions=%d player_state_unchanged=%d",ok&&isolated,factions,isolated);
}
static bool restore(void* user){
    clear_displays();
    void* source=get(user,0x110);if(!source)return false;Roots roots;
    void* config=local_faction::clone(source,roots);void* values=infos(progress,local_faction::active,roots);
    if(!config||!values||(capacity_projector&&!capacity_projector(config,progress)))return false;ref(config,"heroInfos",values);ref(user,"heroConfiguration",config);
    LOG("Local hero cards restored faction=%d inventory_types=%d rank_gate_unchanged=1",local_faction::active,*(int*)((char*)values+0x18));return true;
}
static void (*request_original)(void*,void*);
static void request(void* self,void* method){
    if(!offline_network){request_original(self,method);return;}
    if(!progress_ready)return;Roots roots;void* ex=nullptr;
    void* config=get(saved("UserConfiguration"),0x110);
    void* source=get(self,0x18);void* data=infos(progress,local_faction::active,roots);
    if(!config||!source||!data){LOG("Local hero refresh refused missing local model");return;}
    void* message=object_new(find_class("com.geargames.aow.messages","SetHeroInfo"));if(!roots.keep(message))return;
    invoke_named(message,".ctor",0,nullptr,&ex);if(ex)return;
    ref(message,"info",data);ref(message,"batteries",get(config,0x18));
    *(int*)((char*)message+0x30)=*(int*)((char*)config+0x20);
    memcpy((char*)message+0x34,(char*)config+0x24,12);memcpy((char*)message+0x40,(char*)config+0x30,8);
    void* args[]={message};invoke_named(self,"OnSetHeroInfoReceived",1,args,&ex);
    LOG("Local hero refresh applied=%d career_writes=0 exception=%s",!ex,ex?class_name(object_class(ex)):"none");
}
static void install(){
    const uint32_t sig[]={0xa9bd5ffe,0xa90157f6,0xa9024ff4,0xd0011513};
    request_original=(decltype(request_original))install_hook(0x7b0e058,sig,(void*)request);
    LOG("Local hero refresh hook=%d",request_original!=nullptr);
    const uint32_t prototype_sig[]={0xf81c0ffe,0xa9015ff8,0xa90257f6,0xa9034ff4};
    prototype_original=(decltype(prototype_original))install_hook(0x7c737e4,prototype_sig,(void*)prototype);
    LOG("Local hero private workshop type hook=%d",prototype_original!=nullptr);
}
}
