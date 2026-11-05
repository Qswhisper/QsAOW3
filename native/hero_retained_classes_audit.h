// Content-only alternate-class probe. Decodes retained inputs; no scene/init/purchase.
#pragma once
#include <new>
namespace hero_retained_classes_audit {
using local_research_runtime::Roots;
static void* get(void* p,int o){return p?*(void**)((char*)p+o):nullptr;}
static int count(void* p){return p?*(int*)((char*)p+0x18):0;}
static void mix(uint64_t& h,int v){for(int i=0;i<4;++i)h=(h^uint8_t(uint32_t(v)>>(i*8)))*1099511628211ull;}
static bool weapon(FILE* f,void* w,uint64_t& hash){
    if(!w){fputs("null",f);mix(hash,-1);return true;}
    if(strcmp(class_name(object_class(w)),"WeaponType"))return false;
    int v[]={*(int16_t*)((char*)w+0x20),*(int16_t*)((char*)w+0x24),*(bool*)((char*)w+0x14),
        *(int*)((char*)w+0x28),*(int*)((char*)w+0x2c),*(int*)((char*)w+0x30),*(int*)((char*)w+0x4c),
        *(int16_t*)((char*)w+0x60),*(int16_t*)((char*)w+0x66),*(int*)((char*)w+0x7c),*(int16_t*)((char*)w+0x88)};
    for(int n:v)mix(hash,n);
    fprintf(f,"{\"name\":%d,\"type\":%d,\"initialized\":%d,\"damage\":[%d,%d,%d],\"range\":%d,\"shot_start\":%d,\"reload\":%d,\"explosion\":%d,\"rotate_speed\":%d}",v[0],v[1],v[2],v[3],v[4],v[5],v[6],v[7],v[8],v[9],v[10]);return true;
}
static bool states(FILE* f,void* unit,uint64_t& hash){
    void* rows=get(unit,0x18);int n=count(rows);if(n<1||n>16)return false;mix(hash,n);fputs("[",f);
    for(int i=0;i<n;++i){void* row=list_item(rows,i);if(!row)return false;int armor=*(int16_t*)((char*)row+0x22),sight=*(int*)((char*)row+0x4c),speed=*(int8_t*)((char*)row+0x58);
        mix(hash,armor);mix(hash,sight);mix(hash,speed);
        fprintf(f,"%s{\"armor\":%d,\"sight\":%d,\"speed\":%d,\"weapons\":[",i?",":"",armor,sight,speed);
        void* list=get(row,0x18);int k=count(list);if(k<0||k>16)return false;mix(hash,k);
        for(int j=0;j<k;++j){if(j)fputc(',',f);if(!weapon(f,list_item(list,j),hash))return false;}fputs("]}",f);}
    fputc(']',f);return true;
}
static bool unit(FILE* f,void* u,const char* source,int chapter,int side,int& records,int& later){
    if(!u)return false;int kind=*(int8_t*)((char*)u+0x31),hero=kind==64?74:kind==71?171:0;
    if(!hero)return true;void* fraction=get(u,0x20);int faction=fraction?*(int*)((char*)fraction+0x10):0;
    if((hero==74&&faction!=1)||(hero==171&&faction!=2))return true;
    int grade=*(int*)((char*)u+0xa0),life=*(int*)((char*)u+0x34);uint64_t hash=1469598103934665603ull;
    mix(hash,kind);mix(hash,life);++records;if(grade>1)++later;
    fprintf(f,"{\"phase\":\"hero_definition\",\"source\":\"%s\",\"chapter\":%d,\"side\":%d,\"hero\":%d,\"class\":%d,\"life\":%d,\"states\":",source,chapter,side,hero,grade,life);
    if(!states(f,u,hash))return false;
    fputs(",\"abilities\":[",f);void* abilities=get(u,0x98);int n=count(abilities);if(n<0||n>8)return false;mix(hash,n);
    for(int i=0;i<n;++i){void* a=list_item(abilities,i);if(!a)return false;
        int v[]={*(int*)((char*)a+0x30),*(int*)((char*)a+0x34),*(int*)((char*)a+0x38),*(int*)((char*)a+0x40),*(int*)((char*)a+0x44)};
        for(int value:v)mix(hash,value);
        fprintf(f,"%s{\"role\":%d,\"cooldown\":%d,\"radius\":%d,\"duration\":%d,\"effect\":%d,\"weapon\":",i?",":"",v[0],v[1],v[2],v[3],v[4]);
        if(!weapon(f,get(a,0x50),hash))return false;
        fputs(",\"dependent_unit\":",f);void* child=get(a,0x58);
        if(child){if(strcmp(class_name(object_class(child)),"UnitType"))return false;
            int child_kind=*(int8_t*)((char*)child+0x31),hp=*(int*)((char*)child+0x34);mix(hash,child_kind);mix(hash,hp);
            fprintf(f,"{\"kind\":%d,\"life\":%d,\"states\":",child_kind,hp);if(!states(f,child,hash))return false;fputc('}',f);
        }else {fputs("null",f);mix(hash,-1);}fputc('}',f);
    }
    fprintf(f,"],\"parameter_signature\":\"%016llx\",\"pass\":true}\n",(unsigned long long)hash);return true;
}
static bool battle(FILE* f,void* input,const char* source,int chapter,bool embedded,int& records,int& later){
    void* b=get(input,embedded?0x10:0x20);Roots roots;uintptr_t sh=0;
    void* sides=b?local_dictionary_values(get(b,0x178),find_class("com.geargames.aow.entities.battle","BattleSide"),sh):nullptr;
    if(!sides||!roots.take(sh)||array_length(sides)>32)return false;
    for(size_t i=0;i<array_length(sides);++i){void* side=((void**)((char*)sides+32))[i];Roots one;uintptr_t uh=0;
        void* types=side?local_dictionary_values(get(side,0xf0),find_class("com.geargames.aow.entities","UnitType"),uh):nullptr;
        if(!types||!one.take(uh)||array_length(types)>1024)return false;
        for(size_t j=0;j<array_length(types);++j)if(!unit(f,((void**)((char*)types+32))[j],source,chapter,*(int*)((char*)side+0x10),records,later))return false;
    }return true;
}
static __attribute__((noinline)) void run(){
    if(!progress_ready||access(DATA_ROOT "hero-retained-classes-audit.request",F_OK))return;unlink(DATA_ROOT "hero-retained-classes-audit.request");
    FILE* f=fopen(DATA_ROOT "hero-retained-classes-audit.jsonl","w");if(!f)return;chmod(DATA_ROOT "hero-retained-classes-audit.jsonl",0644);
    local_progress::StateBuffer before;if(!before.data){fclose(f);return;}*before.data=progress;
    bool ok=true;int inputs=0,records=0,later=0;
    for(int hero:{74,171})ok=unit(f,local_career::lookup("BATTLE","unitTypes",hero),"canonical",-1,-1,records,later)&&ok;
    for(const auto& route:local_scenarios){Roots roots;bool valid=local_scenarios.verify(route);
        void* input=!valid?nullptr:route.embedded?campaign_seeds::decode(route.file,roots):read_scenario_message(route.file,5040,"@hero-retained-class-input");
        bool good=input&&battle(f,input,"campaign",route.chapter,route.embedded,records,later);ok=ok&&good;++inputs;
        fprintf(f,"{\"phase\":\"input\",\"source\":\"campaign\",\"chapter\":%d,\"pass\":%s}\n",route.chapter,good?"true":"false");
    }
    // Do not inline a large catalogue/decoder stack into ordinary main setup.
    auto coop=new(std::nothrow) campaign_catalog::Catalog();bool loaded=coop&&coop->load(DATA_ROOT,"offline-cooperative/catalog.bin");ok=ok&&loaded;
    if(loaded)for(const auto& route:*coop){void* input=read_scenario_message(route.file,5040,"@hero-retained-class-input");
        bool good=input&&battle(f,input,"cooperative",route.chapter,false,records,later);ok=ok&&good;++inputs;
        fprintf(f,"{\"phase\":\"input\",\"source\":\"cooperative\",\"chapter\":%d,\"pass\":%s}\n",route.chapter,good?"true":"false");}delete coop;
    bool unchanged=!memcmp(before.data,&progress,sizeof(progress));ok=ok&&unchanged&&inputs==56&&records>=2;
    fprintf(f,"{\"summary\":true,\"pass\":%s,\"original_inputs\":%d,\"hero_definition_records\":%d,\"later_class_records\":%d,\"player_state_unchanged\":%s,\"player_save_writes\":0,\"battle_runs\":0,\"battle_initializations\":0,\"upgrade_projection\":false}\n",ok?"true":"false",inputs,records,later,unchanged?"true":"false");fclose(f);
}
}
