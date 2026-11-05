#pragma once
namespace composite_research_audit {
using namespace generic_unit_research;
static bool accuracy(const Effect& e){for(auto& row:e.accuracy)for(int n:row)if(n)return true;return false;}
static bool values(void* copy,void* source,const Effect& e,int& getters,bool initialized=false){
    if(accuracy(e))return weapon_accuracy_audit::values(copy,source,e,getters,initialized);
    return initialized?research_batch_audit::getters(copy,source,e):weapon_parameters_audit::raw_values(copy,source,e);
}
static void run(){
    if(!progress_ready||access(DATA_ROOT "composite-research-audit.request",F_OK))return;unlink(DATA_ROOT "composite-research-audit.request");
    local_progress::StateBuffer saved_state;if(!saved_state.data)return;*saved_state.data=progress;
    FILE* f=fopen(DATA_ROOT "composite-research-audit.jsonl","w");if(!f)return;chmod(DATA_ROOT "composite-research-audit.jsonl",0644);
    Plan* plans=new(std::nothrow) Plan[2];if(!plans){fclose(f);return;}bool ok=true;int candidates=0,accepted=0,refused=0,native=0,getters=0,predebit=0,families=0,guarded=0;
    for(const char* key:{"CONFEDERATION","RESISTANCE"}){int faction=!strcmp(key,"CONFEDERATION")?1:2;void* rows=levels(key);int n=rows?*(int*)((char*)rows+0x18):0;
        for(int i=0;i<n;++i){void* row=list_item(rows,i),*up=local_hero::get(row,0x48);char desc[4096];research_effects::UnitStats scalar;research_effects::WeaponParameters gun;
            if(!up||strcmp(class_name(object_class(up)),"UpgradeUnit")||!local_research_runtime::ascii(local_hero::get(row,0x18),desc,sizeof(desc))||!research_effects::composite_unit_weapon_parse(desc,scalar,gun))continue;
            ++candidates;Effect e;const char* reason=nullptr;
            if(!resolve(row,e)||!shape(e.owner,e,reason)){++refused;fprintf(f,"{\"phase\":\"refused\",\"faction\":%d,\"id\":%d,\"reason\":\"%s\"}\n",faction,*(int*)((char*)row+0x10),reason?reason:e.reason);continue;}
            Roots roots;uint64_t before=weapon_parameters_audit::hash(e.owner);void* copy=copy_type(e.owner,e,roots);
            bool pass=copy&&values(copy,e.owner,e,getters)&&weapon_parameters_audit::native_init(copy,e,native)&&values(copy,e.owner,e,getters,true)&&before==weapon_parameters_audit::hash(e.owner)&&plans[faction-1].add(e);
            if(faction==local_faction::active){pass=pass&&purchase_supported(row);++predebit;}
            ok&=pass;++accepted;fprintf(f,"{\"phase\":\"original_composite\",\"faction\":%d,\"id\":%d,\"owner\":%d,\"hp\":%d,\"armor\":%d,\"pass\":%s}\n",faction,*(int*)((char*)row+0x10),*(int*)((char*)e.owner+0x10),e.delta.hp,e.delta.armor,pass?"true":"false");
        }
    }
    for(int faction=0;faction<2;++faction)for(int i=0;i<plans[faction].count;++i){Effect e=plans[faction].families[i];const char* reason=nullptr;
        if(!shape(e.owner,e,reason)){++guarded;continue;}Roots roots;uint64_t hash=weapon_parameters_audit::hash(e.owner);
        void* a=copy_type(e.owner,e,roots),*b=copy_type(e.owner,e,roots);bool pass=a&&b&&values(a,e.owner,e,getters)&&values(b,e.owner,e,getters)&&hash==weapon_parameters_audit::hash(e.owner);
        auto invalid=e;invalid.delta.hp=INT16_MAX+1;pass=pass&&!copy_type(e.owner,invalid,roots);
        ok&=pass;++families;fprintf(f,"{\"phase\":\"family_repeat_and_atomic_guard\",\"faction\":%d,\"owner\":%d,\"pass\":%s}\n",faction+1,*(int*)((char*)e.owner+0x10),pass?"true":"false");
    }
    bool unchanged=!memcmp(saved_state.data,&progress,sizeof(progress));delete[] plans;
    fprintf(f,"{\"summary\":true,\"pass\":%s,\"candidates\":%d,\"accepted\":%d,\"refused\":%d,\"families\":%d,\"cumulative_guarded\":%d,\"native_initializers\":%d,\"original_accuracy_values\":%d,\"active_faction_predebit\":%d,\"player_state_unchanged\":%s,\"player_save_writes\":0,\"battle_runs\":0}\n",ok&&accepted>0&&families>0&&unchanged?"true":"false",candidates,accepted,refused,families,guarded,native,getters,predebit,unchanged?"true":"false");fclose(f);
}
}
