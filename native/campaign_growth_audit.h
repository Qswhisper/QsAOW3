// One content batch: private decode/projection checks for newly opted-in routes.
#pragma once
namespace campaign_growth_audit {
using local_research_runtime::Roots;
static void* get(void* p,int o){return p?*(void**)((char*)p+o):nullptr;}
static uint32_t hash(uint32_t h,void* p,int offset,int size){return p?campaign_catalog::hash((char*)p+offset,size,h):h^0xffffffffu;}
static uint32_t type_hash(uint32_t h,void* type,bool building,bool gates=false){
    h=hash(h,type,0x10,4);
    if(!gates){if(building){h=hash(h,type,0x31,1);h=hash(h,type,0x34,4);}
        else{h=hash(h,type,0x32,0x1a);h=hash(h,type,0x77,5);}}
    void* rows=get(type,0x18);int n=rows?*(int*)((char*)rows+0x18):0;
    for(int i=0;i<n;++i){void* row=list_item(rows,i);if(gates){if(building)h=hash(h,row,0x28,8);continue;}
        if(building){h=hash(h,row,0x30,0x22);h=hash(h,row,0x60,0x14);h=hash(h,row,0x80,8);}
        else{h=hash(h,row,0x22,6);h=hash(h,row,0x48,0x1c);h=hash(h,row,0x70,0x17);h=hash(h,row,0x88,9);}
        void* weapons=get(row,0x18);for(int j=0;weapons&&j<*(int*)((char*)weapons+0x18);++j){void* w=list_item(weapons,j);
            h=hash(h,w,0x20,8);h=hash(h,w,0x28,0x18);h=hash(h,w,0x48,0x23);h=hash(h,w,0x78,0x38);}
    }return h;
}
static uint32_t side_hash(void* side,bool gates=false){
    uint32_t h=2166136261u;
    for(bool building:{false,true}){Roots roots;uintptr_t handle=0;void* values=local_dictionary_values(get(side,building?0xf8:0xf0),find_class("com.geargames.aow.entities",building?"BuildingType":"UnitType"),handle);
        if(!values||!roots.take(handle))return 0;
        for(size_t i=0;i<array_length(values);++i)h=type_hash(h,((void**)((char*)values+32))[i],building,gates);}
    return h;
}
static uint32_t canonical_hash(){uint32_t h=2166136261u;
    for(bool building:{false,true}){void* list=local_career::list("BATTLE",building?"buildingTypes":"unitTypes");
        for(int i=0;list&&i<*(int*)((char*)list+0x18);++i)h=type_hash(h,list_item(list,i),building);}
    return h;
}
static bool producers(void* side){Roots roots;uintptr_t handle=0;void* values=local_dictionary_values(get(side,0xf8),find_class("com.geargames.aow.entities","BuildingType"),handle);
    if(!values||!roots.take(handle))return false;
    for(size_t i=0;i<array_length(values);++i){void* levels=get(((void**)((char*)values+32))[i],0x18);
        for(int j=0;levels&&j<*(int*)((char*)levels+0x18);++j){void* units=get(list_item(levels,j),0x20);
            for(int k=0;units&&k<*(int*)((char*)units+0x18);++k){void* unit=list_item(units,k);
                if(!unit||local_dictionary_item(get(side,0xf0),*(int*)((char*)unit+0x10))!=unit)return false;}}}
    return true;
}
struct EntitySnapshot {
    struct Row {void* entity;void* type;void* state;bool building;unsigned char bytes[0x300];};
    Row* rows=(Row*)calloc(4096,sizeof(Row));int count=0;
    ~EntitySnapshot(){free(rows);}
    bool take(void* e,void* side,bool building){
        if(!e||get(e,0x18)!=side)return true;
        for(int i=0;i<count;++i)if(rows[i].entity==e)return true;
        if(!rows||count==4096)return false;auto& r=rows[count++];r.entity=e;r.building=building;
        r.type=get(e,building?0x38:0x40);int state=*(int8_t*)((char*)e+(building?0x60:0x6b));
        r.state=list_item(get(r.type,0x18),state);if(!r.type||!r.state)return false;
        memcpy(r.bytes,e,building?0x168:0x2e8);return true;
    }
    bool take(void* battle,void* side){
        for(bool b:{false,true}){void* a=get(side,b?0x148:0x140);size_t n=a?array_length(a):0;
            if(n>4096)return false;for(size_t i=0;i<n;++i)if(!take(((void**)((char*)a+32))[i],side,b))return false;}
        void* alliances=get(battle,0x170);for(size_t i=0;alliances&&i<array_length(alliances);++i){void* a=((void**)((char*)alliances+32))[i];
            for(int o:{0x78,0x80,0x88,0xa0}){void* list=get(a,o);int n=list?*(int*)((char*)list+0x18):0;
                if(n<0||n>4096)return false;for(int j=0;j<n;++j)if(!take(list_item(list,j),side,o>=0x88))return false;}}
        return true;
    }
    bool verify(void* side){
        for(int i=0;i<count;++i){auto& r=rows[i];void* e=r.entity;bool b=r.building;
            int to=b?0x38:0x40,ho=b?0x5c:0x184,so=b?0x58:0x182;void* type=get(e,to);
            int state=*(int8_t*)((char*)e+(b?0x60:0x6b));void* row=list_item(get(type,0x18),state);
            int oldmax=*(int*)((char*)(b?r.state:r.type)+0x34),newmax=*(int*)((char*)(b?row:type)+0x34);
            int hp=*(int*)(r.bytes+ho),expected=hp>0?hp+newmax-oldmax:hp;
            if(!type||!row||local_dictionary_item(get(side,b?0xf8:0xf0),*(int*)((char*)type+0x10))!=type||
               *(int*)((char*)e+ho)!=expected||*(int16_t*)((char*)e+so)!=expected)return false;
            unsigned char now[0x300];int size=b?0x168:0x2e8;memcpy(now,e,size);
            for(int o:{to,ho,so})memcpy(now+o,r.bytes+o,o==to?8:o==ho?4:2);
            if(b){for(int o:{0xa0,0xa8,0x150})memcpy(now+o,r.bytes+o,8);}
            else {int speed=(int8_t)r.bytes[0xba]+*(int8_t*)((char*)row+0x58)-*(int8_t*)((char*)r.state+0x58);
                int sight=*(int*)(r.bytes+0xbc)+*(int*)((char*)row+0x4c)-*(int*)((char*)r.state+0x4c);
                if(*(int8_t*)((char*)e+0xba)!=speed||*(int*)((char*)e+0xbc)!=sight)return false;
                memcpy(now+0xba,r.bytes+0xba,1);memcpy(now+0xbc,r.bytes+0xbc,4);}
            if(memcmp(now,r.bytes,size))return false;
            void* weapons=get(e,0x28),*types=get(row,0x18);int n=weapons?*(int*)((char*)weapons+0x18):0;
            for(int j=0;j<n;++j)if(get(list_item(weapons,j),0x10)!=list_item(types,j))return false;
        }return true;
    }
};
static void run(){
    if(!progress_ready||access(DATA_ROOT "campaign-growth-audit.request",F_OK))return;
    unlink(DATA_ROOT "campaign-growth-audit.request");FILE* f=fopen(DATA_ROOT "campaign-growth-audit.jsonl","w");if(!f)return;chmod(DATA_ROOT "campaign-growth-audit.jsonl",0644);
    auto before=progress;int active=local_faction::active;local_progress::State career{};
    bool ok=local_progress::load(local_faction::root(1),career)>0;uint32_t canonical=canonical_hash();int tested=0,expected=0;
    for(const auto& route:local_scenarios)if(route.rebase_growth)++expected;
    if(ok){local_faction::active=1;progress=career;
        for(const auto& route:local_scenarios){if(!route.rebase_growth)continue;Roots roots;
            local_progress::State route_career{};
            if(local_progress::load(local_faction::root(route.faction),route_career)<=0){ok=false;continue;}
            local_faction::active=route.faction;progress=route_career;
            void* message=read_scenario_message(route.file,5040,"@campaign-growth-audit");void* battle=get(message,0x20);
            void* side=local_dictionary_item(get(battle,0x178),1),*enemy=local_dictionary_item(get(battle,0x178),2);
            uint32_t own_before=side_hash(side),gates=side_hash(side,true),other=side_hash(enemy);void* script=get(battle,0x140),*map=get(battle,0x160);
            EntitySnapshot entities;
            bool fresh=battle&&*(int*)((char*)battle+0x28)==route.recorded&&message&&*(int*)((char*)message+0x30)<=0&&entities.take(battle,side);
            bool applied=fresh&&apply_local_battle_research(battle,1,true);uint32_t after=side_hash(side);
            bool entity_links=applied&&entities.verify(side);
            bool linked=applied&&producers(side),isolated=side_hash(enemy)==other&&canonical_hash()==canonical;
            bool unchanged_gates=side_hash(side,true)==gates,script_same=get(battle,0x140)==script&&get(battle,0x160)==map;
            bool again=applied&&apply_local_battle_research(battle,1,true)&&side_hash(side)==after;
            void* presented=local_presented_mission(route.chapter);bool identity=presented&&*(int*)((char*)presented+0x10)==route.chapter&&*(int*)((char*)presented+0x14)==route.map;
            bool pass=fresh&&applied&&entity_links&&linked&&isolated&&unchanged_gates&&script_same&&again&&identity;ok=ok&&pass;++tested;
            fprintf(f,"{\"mission\":%d,\"own_entities\":%d,\"entity_identity_damage_cache_links\":%s}\n",route.recorded,entities.count,entity_links?"true":"false");
            fprintf(f,"{\"mission\":%d,\"chapter\":%d,\"map\":%d,\"pass\":%s,\"fresh_empty_own\":%s,\"applied\":%s,\"producer_links\":%s,\"other_and_canonical_unchanged\":%s,\"scenario_gates_unchanged\":%s,\"script_map_unchanged\":%s,\"repeat_unchanged\":%s,\"original_variant_ui\":%s,\"own_before\":%u,\"own_after\":%u}\n",route.recorded,route.chapter,route.map,pass?"true":"false",fresh?"true":"false",applied?"true":"false",linked?"true":"false",isolated?"true":"false",unchanged_gates?"true":"false",script_same?"true":"false",again?"true":"false",identity?"true":"false",own_before,after);
        }
    }
    progress=before;local_faction::active=active;
    fprintf(f,"{\"summary\":true,\"pass\":%s,\"routes\":%d,\"player_state_unchanged\":%s,\"battle_runs\":0,\"save_writes\":0}\n",ok&&expected>0&&tested==expected?"true":"false",tested,!memcmp(&before,&progress,sizeof(before))?"true":"false");fclose(f);
}
}
