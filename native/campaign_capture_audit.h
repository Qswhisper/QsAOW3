// Bounded content input inspection; no battle initialization or player writes.
#pragma once
namespace campaign_capture_audit {
static void* get(void* p,int o){return p?*(void**)((char*)p+o):nullptr;}
static void prototype(FILE* f,void* p,bool building){
    visited_count=0;dump_object(f,p,0,1);
    void* rows=get(p,0x18);int n=rows?*(int*)((char*)rows+0x18):0;
    if(n<0||n>32)return;
    for(int i=0;i<n;++i){void* row=list_item(rows,i);fprintf(f,"ROW %d\n",i);visited_count=0;dump_object(f,row,0,1);
        void* weapons=get(row,0x18);int m=weapons?*(int*)((char*)weapons+0x18):0;
        if(m<0||m>32)return;for(int j=0;j<m;++j){fprintf(f,"WEAPON %d\n",j);visited_count=0;dump_object(f,list_item(weapons,j),0,1);}}
}
static void run(){
    if(access(DATA_ROOT "campaign-capture-audit.request",F_OK))return;
    unlink(DATA_ROOT "campaign-capture-audit.request");auto before=progress;
    FILE* f=fopen(DATA_ROOT "campaign-capture-audit.txt","w");if(!f)return;chmod(DATA_ROOT "campaign-capture-audit.txt",0644);
    for(int mission:{11,211}){char path[512];snprintf(path,sizeof(path),DATA_ROOT "offline-candidates/%d.ggs",mission);
        void* message=read_scenario_message(path,5040,"@campaign-capture-audit");void* battle=get(message,0x20);
        fprintf(f,"MISSION expected=%d actual=%d tick=%d\n",mission,battle?*(int*)((char*)battle+0x28):-1,message?*(int*)((char*)message+0x30):999);
        if(!battle)continue;void* side=local_dictionary_item(get(battle,0x178),1);
        visited_count=0;dump_object(f,side,0,1);
        for(bool building:{false,true}){local_research_runtime::Roots roots;uintptr_t handle=0;
            void* values=local_dictionary_values(get(side,building?0xf8:0xf0),find_class("com.geargames.aow.entities",building?"BuildingType":"UnitType"),handle);
            if(!values||!roots.take(handle))continue;
            void* bases=local_career::list("BATTLE",building?"buildingTypes":"unitTypes");
            for(size_t i=0;i<array_length(values);++i){void* type=((void**)((char*)values+32))[i];
                fprintf(f,"SOURCE kind=%s id=%d\n",building?"building":"unit",type?*(int*)((char*)type+0x10):-1);prototype(f,type,building);
                for(int j=0;bases&&j<*(int*)((char*)bases+0x18);++j){void* base=list_item(bases,j);
                    bool match=building?building_research::identity(type,base):generic_unit_research::identity(type,base);
                    if(match){fprintf(f,"BASE kind=%s id=%d\n",building?"building":"unit",*(int*)((char*)base+0x10));prototype(f,base,building);}}
            }
        }
        void* alliances=get(battle,0x170);for(size_t i=0;alliances&&i<array_length(alliances);++i){void* a=((void**)((char*)alliances+32))[i];
            for(int offset:{0x78,0x80,0x88,0xa0}){void* list=get(a,offset);for(int j=0;list&&j<*(int*)((char*)list+0x18);++j){void* e=list_item(list,j);
                if(get(e,0x18)==side){fprintf(f,"OWN_ENTITY\n");visited_count=0;dump_object(f,e,0,1);}}}}
    }
    fprintf(f,"END player_state_unchanged=%d battle_runs=0 save_writes=0\n",!memcmp(&before,&progress,sizeof(before)));fclose(f);
}
}
