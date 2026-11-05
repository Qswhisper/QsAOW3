// Shared pre-fire adapter: whole retained rows, native salvo generation, and
// cumulative bounds on private prototypes. No transactions or player saves.
#pragma once
namespace weapon_aiming_audit {
using namespace generic_unit_research;
static bool aiming(const char* p){return strstr(p,"{#20019<")||strstr(p,"{#20068<")||strstr(p,"{#20069<");}
static bool schedule(void* weapon,int& shots){
    if(!weapon||!*(bool*)((char*)weapon+0x14))return false;
    int start=*(int16_t*)((char*)weapon+0x60),interval=*(int16_t*)((char*)weapon+0x64),n=*(int8_t*)((char*)weapon+0x6a);
    if(start<0||interval<0||n<1||n>127||int64_t(start)+int64_t(n)*interval>INT16_MAX)return false;
    void* ex=nullptr,*ticks=invoke_named(weapon,"get_ShotTick",0,nullptr,&ex);
    if(ex||!ticks||array_length(ticks)!=size_t(n)||!original_stat_equals(weapon,"get_ShotStart",2,start)||
       !original_stat_equals(weapon,"get_ShotLength",2,start+n*interval))return false;
    for(int i=0;i<n;++i){if(((int16_t*)((char*)ticks+32))[i]!=start+i*interval)return false;++shots;}
    return true;
}
static bool unit_timing(void* copy,void* base,const Effect& e,FILE* out,int faction,bool initialized,int& shots){
    void* a=local_hero::get(base,0x18),*b=local_hero::get(copy,0x18);if(!copy||!a||!b)return false;
    for(int i=0;i<*(int*)((char*)a+0x18);++i){void* x=local_hero::get(list_item(a,i),0x18),*y=local_hero::get(list_item(b,i),0x18);
        for(int j=0;x&&j<*(int*)((char*)x+0x18);++j){void* old=list_item(x,j),*w=list_item(y,j);int t=target_index(old,e);
            int start=*(int16_t*)((char*)old+0x60)-(t>=0?e.shot_start[t]:0);
            if(!original_stat_equals(w,"get_ShotStart",2,start)||*(int8_t*)((char*)old+0x8b)!=*(int8_t*)((char*)w+0x8b))return false;
            if(t<0){if(old!=w)return false;continue;}
            if(initialized){if(!schedule(w,shots))return false;}
            else { // Every non-research byte, including target flags, stays original.
                uint8_t bytes[0x100];memcpy(bytes,(char*)w+0x10,sizeof(bytes));
                for(int o:{0x28,0x2c,0x30,0x4c,0x7c})memcpy(bytes+o-0x10,(char*)old+o,4);
                for(int o:{0x60,0x66,0x8e,0x90,0x92})memcpy(bytes+o-0x10,(char*)old+o,2);
                if(memcmp(bytes,(char*)old+0x10,sizeof(bytes)))return false;
                if(out&&e.shot_start[t])fprintf(out,"{\"phase\":\"original_timer\",\"faction\":%d,\"kind\":\"unit\",\"owner\":%d,\"state\":%d,\"weapon\":%d,\"type\":%d,\"before\":%d,\"reduction\":%d,\"after\":%d}\n",faction,*(int*)((char*)e.owner+0x10),i,e.targets[t].name,e.targets[t].type,*(int16_t*)((char*)old+0x60),e.shot_start[t],start);
            }
        }}return true;
}
static bool building_timing(void* copy,void* base,const building_weapon_research::Effect& e,FILE* out,int faction,bool initialized,int& shots){
    using building_weapon_research::get;using building_weapon_research::count;
    if(!copy)return false;void* a=get(base,0x18),*b=get(copy,0x18);
    for(int i=0;i<count(a);++i){void* x=get(list_item(a,i),0x18),*y=get(list_item(b,i),0x18);
        for(int j=0;j<count(x);++j){void* old=list_item(x,j),*w=list_item(y,j);int t=building_weapon_research::target(old,e);
            int start=*(int16_t*)((char*)old+0x60)-(t>=0?e.delta[t].shot_start:0);
            if(!original_stat_equals(w,"get_ShotStart",2,start)||*(int8_t*)((char*)old+0x8b)!=*(int8_t*)((char*)w+0x8b))return false;
            if(t<0){if(old!=w)return false;continue;}
            if(initialized){if(!schedule(w,shots))return false;}
            else if(out&&e.delta[t].shot_start)fprintf(out,"{\"phase\":\"original_timer\",\"faction\":%d,\"kind\":\"building\",\"owner\":%d,\"state\":%d,\"weapon\":%d,\"type\":%d,\"before\":%d,\"reduction\":%d,\"after\":%d}\n",faction,*(int*)((char*)e.owner+0x10),i,e.keys[t].name,e.keys[t].type,*(int16_t*)((char*)old+0x60),e.delta[t].shot_start,start);
        }}return true;
}
static __attribute__((noinline)) void run(){
    if(!progress_ready||access(DATA_ROOT "weapon-aiming-audit.request",F_OK))return;unlink(DATA_ROOT "weapon-aiming-audit.request");
    local_progress::StateBuffer before;if(!before.data)return;*before.data=progress;
    FILE* f=fopen(DATA_ROOT "weapon-aiming-audit.jsonl","w");if(!f)return;chmod(DATA_ROOT "weapon-aiming-audit.jsonl",0644);
    auto* units=new Plan[2];auto* buildings=new building_weapon_research::Plan[2];
    bool ok=true;int candidates=0,aim_rows=0,accepted=0,refused=0,reused=0,native=0,getters=0,predebit=0,building_rows=0,shots=0;
    for(const char* source:{"CONFEDERATION","RESISTANCE"}){int faction=!strcmp(source,"CONFEDERATION")?1:2;void* rows=levels(source);
        for(int i=0;rows&&i<*(int*)((char*)rows+0x18);++i){void* row=list_item(rows,i),*up=local_hero::get(row,0x48);char text[4096];
            if(!up||!local_research_runtime::ascii(local_hero::get(row,0x18),text,sizeof(text))||(!aiming(text)&&!weapon_groups_audit::labelled(text)))continue;
            ++candidates;bool has_aim=aiming(text);if(has_aim)++aim_rows;
            int id=*(int*)((char*)row+0x10);const char* kind=class_name(object_class(up));bool pass=true;
            if(!strcmp(kind,"UpgradeUnit")){
                Effect e;const char* why=nullptr;
                if(!resolve(row,e)||!shape(e.owner,e,why)){++refused;fprintf(f,"{\"phase\":\"refused\",\"faction\":%d,\"id\":%d,\"kind\":\"unit\",\"aiming\":%s,\"reason\":\"%s\"}\n",faction,id,has_aim?"true":"false",why?why:e.reason);continue;}
                if(e.hero_ability){++reused;fprintf(f,"{\"phase\":\"reused_hero_adapter\",\"faction\":%d,\"id\":%d}\n",faction,id);continue;}
                Roots roots;uint64_t hash=weapon_parameters_audit::hash(e.owner);void* copy=copy_type(e.owner,e,roots);
                pass=weapon_groups_audit::unit_values(copy,e.owner,e,getters)&&unit_timing(copy,e.owner,e,nullptr,faction,false,shots)&&
                    weapon_parameters_audit::native_init(copy,e,native)&&unit_timing(copy,e.owner,e,nullptr,faction,true,shots)&&
                    units[faction-1].add(e)&&hash==weapon_parameters_audit::hash(e.owner);
                if(faction==local_faction::active){pass=pass&&purchase_supported(row);++predebit;}
                fprintf(f,"{\"phase\":\"original_row\",\"faction\":%d,\"id\":%d,\"owner\":%d,\"kind\":\"unit\",\"aiming\":%s,\"targets\":%d,\"pass\":%s}\n",faction,id,*(int*)((char*)e.owner+0x10),has_aim?"true":"false",e.target_count,pass?"true":"false");
            }else if(!strcmp(kind,"UpgradeBuilding")){
                building_weapon_research::Effect e;
                if(!building_weapon_research::resolve(row,e)||!building_weapon_research::shape(e.owner,e)){++refused;fprintf(f,"{\"phase\":\"refused\",\"faction\":%d,\"id\":%d,\"kind\":\"building\",\"aiming\":%s,\"reason\":\"whole_group_formula_or_timer_bounds_unverified\"}\n",faction,id,has_aim?"true":"false");continue;}
                Roots roots;uint64_t hash=building_weapon_audit::hash(e.owner);void* copy=building_weapon_research::copy_type(e.owner,e,roots);
                pass=building_weapon_audit::raw(copy,e.owner,e)&&building_timing(copy,e.owner,e,nullptr,faction,false,shots)&&
                    building_weapon_audit::initialize(copy,e,native)&&building_timing(copy,e.owner,e,nullptr,faction,true,shots)&&
                    buildings[faction-1].add(e)&&hash==building_weapon_audit::hash(e.owner);
                if(faction==local_faction::active){pass=pass&&building_weapon_research::purchase_supported(row);++predebit;}
                ++building_rows;fprintf(f,"{\"phase\":\"original_row\",\"faction\":%d,\"id\":%d,\"owner\":%d,\"kind\":\"building\",\"aiming\":%s,\"targets\":%d,\"pass\":%s}\n",faction,id,*(int*)((char*)e.owner+0x10),has_aim?"true":"false",e.targets,pass?"true":"false");
            }else{ok=false;continue;}ok=ok&&pass;++accepted;
        }}
    int cumulative=0,guarded=0;
    for(int faction=0;faction<2;++faction){
        for(int i=0;i<units[faction].count;++i){const Effect& e=units[faction].families[i];const char* why=nullptr;
            if(!shape(e.owner,e,why)){Roots roots;uint64_t hash=weapon_parameters_audit::hash(e.owner);
                bool pass=!copy_type(e.owner,e,roots)&&hash==weapon_parameters_audit::hash(e.owner);++guarded;ok&=pass;
                fprintf(f,"{\"phase\":\"cumulative_refused\",\"faction\":%d,\"owner\":%d,\"kind\":\"unit\",\"reason\":\"%s\",\"pass\":%s,\"complete_chain_supported\":false}\n",faction+1,*(int*)((char*)e.owner+0x10),why?why:"unknown",pass?"true":"false");continue;}
            Roots roots;uint64_t hash=weapon_parameters_audit::hash(e.owner);void* a=copy_type(e.owner,e,roots),*b=copy_type(e.owner,e,roots);
            bool pass=weapon_groups_audit::unit_values(a,e.owner,e,getters)&&weapon_groups_audit::unit_values(b,e.owner,e,getters)&&
                unit_timing(a,e.owner,e,f,faction+1,false,shots)&&unit_timing(b,e.owner,e,nullptr,faction+1,false,shots)&&
                weapon_parameters_audit::native_init(a,e,native)&&unit_timing(a,e.owner,e,nullptr,faction+1,true,shots);
            Effect bad=e;bad.shot_start[0]=INT16_MAX;pass=pass&&!copy_type(e.owner,bad,roots)&&hash==weapon_parameters_audit::hash(e.owner);ok&=pass;++cumulative;
            fprintf(f,"{\"phase\":\"cumulative\",\"faction\":%d,\"owner\":%d,\"kind\":\"unit\",\"pass\":%s}\n",faction+1,*(int*)((char*)e.owner+0x10),pass?"true":"false");
        }
        for(int i=0;i<buildings[faction].size;++i){const auto& e=buildings[faction].families[i];
            if(!building_weapon_research::shape(e.owner,e)){Roots roots;uint64_t hash=building_weapon_audit::hash(e.owner);
                bool pass=!building_weapon_research::copy_type(e.owner,e,roots)&&hash==building_weapon_audit::hash(e.owner);++guarded;ok&=pass;
                fprintf(f,"{\"phase\":\"cumulative_refused\",\"faction\":%d,\"owner\":%d,\"kind\":\"building\",\"pass\":%s,\"complete_chain_supported\":false}\n",faction+1,*(int*)((char*)e.owner+0x10),pass?"true":"false");continue;}
            Roots roots;uint64_t hash=building_weapon_audit::hash(e.owner);void* a=building_weapon_research::copy_type(e.owner,e,roots),*b=a?building_weapon_research::copy_type(a,e,roots):nullptr;
            bool pass=building_weapon_audit::raw(a,e.owner,e)&&b&&building_weapon_audit::hash(a)==building_weapon_audit::hash(b)&&
                building_timing(a,e.owner,e,f,faction+1,false,shots)&&building_weapon_audit::initialize(a,e,native)&&building_timing(a,e.owner,e,nullptr,faction+1,true,shots);
            auto bad=e;bad.delta[0].shot_start=INT16_MAX;pass=pass&&!building_weapon_research::copy_type(e.owner,bad,roots)&&hash==building_weapon_audit::hash(e.owner);ok&=pass;++cumulative;
            fprintf(f,"{\"phase\":\"cumulative\",\"faction\":%d,\"owner\":%d,\"kind\":\"building\",\"pass\":%s}\n",faction+1,*(int*)((char*)e.owner+0x10),pass?"true":"false");
        }
    }
    delete[] units;delete[] buildings;bool unchanged=!memcmp(before.data,&progress,sizeof(progress));
    fprintf(f,"{\"summary\":true,\"pass\":%s,\"candidates\":%d,\"aiming_rows\":%d,\"accepted\":%d,\"refused\":%d,\"existing_hero_rows_reused\":%d,\"building_rows\":%d,\"native_initializers\":%d,\"native_salvo_ticks\":%d,\"original_accuracy_getters\":%d,\"active_faction_predebit\":%d,\"cumulative_families\":%d,\"cumulative_guarded\":%d,\"player_state_unchanged\":%s,\"save_writes\":0,\"battle_runs\":0}\n",ok&&accepted>0&&cumulative>0&&unchanged?"true":"false",candidates,aim_rows,accepted,refused,reused,building_rows,native,shots,getters,predebit,cumulative,guarded,unchanged?"true":"false");fclose(f);
}
}
