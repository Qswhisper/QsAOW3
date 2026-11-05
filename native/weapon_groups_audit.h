// One batch over original grouped descriptions; no player transactions or battle.
#pragma once
namespace weapon_groups_audit {
using namespace generic_unit_research;
static bool labelled(const char* p){return strstr(p,"{#20053<")||strstr(p,"{#20054<")||strstr(p,"{#20092<")||strstr(p,"{#20093<");}
static bool unit_values(void* copy,void* source,const Effect& e,int& getters,bool initialized=false){
    if(!copy||(initialized?!research_batch_audit::getters(copy,source,e):!weapon_parameters_audit::raw_values(copy,source,e)))return false;
    void* states=weapon_parameters_audit::get(source,0x18),*actual=weapon_parameters_audit::get(copy,0x18);
    const int offsets[]={0x8e,0x92,0x90};const char* names[]={"get_AccuracyStatic","get_AccuracyWalk","get_AccuracyDynamic"};
    for(int i=0;states&&i<*(int*)((char*)states+0x18);++i){
        void* ws=weapon_parameters_audit::get(list_item(states,i),0x18),*cs=weapon_parameters_audit::get(list_item(actual,i),0x18);
        for(int j=0;ws&&j<*(int*)((char*)ws+0x18);++j){void* old=list_item(ws,j),*w=list_item(cs,j);int t=target_index(old,e),expected[3];
            for(int k=0;k<3;++k)expected[k]=*(int16_t*)((char*)old+offsets[k]);
            if(t>=0&&(e.accuracy[t][0]||e.accuracy[t][1]||e.accuracy[t][2])){
                const char* why=nullptr;if(!accuracy_values(old,e.accuracy[t],expected,why))return false;}
            for(int k=0;k<3;++k){if(!original_stat_equals(w,names[k],2,expected[k]))return false;++getters;}
        }}return true;
}
static __attribute__((noinline)) void run(){
    if(!progress_ready||access(DATA_ROOT "weapon-groups-audit.request",F_OK))return;unlink(DATA_ROOT "weapon-groups-audit.request");
    local_progress::StateBuffer before;if(!before.data)return;*before.data=progress;
    FILE* f=fopen(DATA_ROOT "weapon-groups-audit.jsonl","w");if(!f)return;chmod(DATA_ROOT "weapon-groups-audit.jsonl",0644);
    auto* plans=new Plan[2];bool ok=true;int candidates=0,accepted=0,refused=0,native=0,getters=0,predebit=0,buildings=0;
    for(const char* source:{"CONFEDERATION","RESISTANCE"}){int faction=!strcmp(source,"CONFEDERATION")?1:2;void* rows=levels(source);
        for(int i=0;rows&&i<*(int*)((char*)rows+0x18);++i){void* row=list_item(rows,i),*up=local_hero::get(row,0x48);char text[4096];
            if(!up||!local_research_runtime::ascii(local_hero::get(row,0x18),text,sizeof(text))||!labelled(text))continue;++candidates;
            int id=*(int*)((char*)row+0x10);const char* kind=class_name(object_class(up));bool pass=true;
            if(!strcmp(kind,"UpgradeUnit")){
                Effect e;const char* why=nullptr;
                if(!resolve(row,e)||!shape(e.owner,e,why)){++refused;fprintf(f,"{\"phase\":\"refused\",\"faction\":%d,\"id\":%d,\"kind\":\"unit\",\"reason\":\"%s\"}\n",faction,id,why?why:e.reason);continue;}
                Roots roots;uint64_t hash=weapon_parameters_audit::hash(e.owner);void* copy=copy_type(e.owner,e,roots);
                pass=unit_values(copy,e.owner,e,getters)&&weapon_parameters_audit::native_init(copy,e,native)&&unit_values(copy,e.owner,e,getters,true)&&plans[faction-1].add(e)&&hash==weapon_parameters_audit::hash(e.owner);
                if(faction==local_faction::active){pass=pass&&purchase_supported(row);++predebit;}
                fprintf(f,"{\"phase\":\"original_row\",\"faction\":%d,\"id\":%d,\"owner\":%d,\"kind\":\"unit\",\"targets\":%d,\"pass\":%s}\n",faction,id,*(int*)((char*)e.owner+0x10),e.target_count,pass?"true":"false");
            }else if(!strcmp(kind,"UpgradeBuilding")){
                building_weapon_research::Effect e;
                if(!building_weapon_research::resolve(row,e)||!building_weapon_research::shape(e.owner,e)){++refused;fprintf(f,"{\"phase\":\"refused\",\"faction\":%d,\"id\":%d,\"kind\":\"building\",\"reason\":\"whole_group_or_native_formula_unverified\"}\n",faction,id);continue;}
                Roots roots;uint64_t hash=building_weapon_audit::hash(e.owner);void* copy=building_weapon_research::copy_type(e.owner,e,roots);
                pass=building_weapon_audit::raw(copy,e.owner,e)&&building_weapon_audit::initialize(copy,e,native)&&hash==building_weapon_audit::hash(e.owner);
                if(faction==local_faction::active){pass=pass&&building_weapon_research::purchase_supported(row);++predebit;}
                ++buildings;fprintf(f,"{\"phase\":\"original_row\",\"faction\":%d,\"id\":%d,\"owner\":%d,\"kind\":\"building\",\"targets\":%d,\"pass\":%s}\n",faction,id,*(int*)((char*)e.owner+0x10),e.targets,pass?"true":"false");
            }else{ok=false;continue;}ok=ok&&pass;++accepted;
        }}
    int families=0,guarded=0;
    for(int faction=0;faction<2;++faction)for(int i=0;i<plans[faction].count;++i){const Effect& e=plans[faction].families[i];const char* why=nullptr;
        if(!shape(e.owner,e,why)){++guarded;continue;}
        Roots roots;void* a=copy_type(e.owner,e,roots),*b=copy_type(e.owner,e,roots);
        bool pass=unit_values(a,e.owner,e,getters)&&unit_values(b,e.owner,e,getters);Effect invalid=e;invalid.accuracy[0][0]=101;
        pass=pass&&!copy_type(e.owner,invalid,roots);ok&=pass;++families;
        fprintf(f,"{\"phase\":\"cumulative\",\"faction\":%d,\"owner\":%d,\"pass\":%s,\"repeat_from_baseline\":true,\"invalid_whole_effect_refused\":true}\n",faction+1,*(int*)((char*)e.owner+0x10),pass?"true":"false");
    }
    delete[] plans;bool unchanged=!memcmp(before.data,&progress,sizeof(progress));
    fprintf(f,"{\"summary\":true,\"pass\":%s,\"candidates\":%d,\"accepted\":%d,\"refused\":%d,\"building_rows\":%d,\"native_initializers\":%d,\"original_accuracy_getters\":%d,\"active_faction_predebit\":%d,\"cumulative_families\":%d,\"cumulative_guarded\":%d,\"player_state_unchanged\":%s,\"save_writes\":0,\"battle_runs\":0}\n",ok&&accepted>0&&families>0&&unchanged?"true":"false",candidates,accepted,refused,buildings,native,getters,predebit,families,guarded,unchanged?"true":"false");fclose(f);
}
}
