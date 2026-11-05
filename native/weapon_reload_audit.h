// Whole original reload descriptions, native type/runtime getters and init.
#pragma once
namespace weapon_reload_audit {
using namespace generic_unit_research;
static bool runtime_values(void* copy,void* source,const Effect& e,int& checked){
    void* states=weapon_parameters_audit::get(copy,0x18),*old=weapon_parameters_audit::get(source,0x18);
    Roots roots;void* klass=find_class("com.geargames.aow.entities","Weapon");if(!klass)return false;
    for(int i=0;states&&i<*(int*)((char*)states+0x18);++i){void* ws=weapon_parameters_audit::get(list_item(states,i),0x18),*os=weapon_parameters_audit::get(list_item(old,i),0x18);
        for(int j=0;ws&&j<*(int*)((char*)ws+0x18);++j){void* w=list_item(ws,j),*base=list_item(os,j);int index=target_index(base,e);if(index<0)continue;
            int expected=*(int16_t*)((char*)base+0x66)-e.reload[index];
            void* runtime=object_new(klass);if(!roots.keep(runtime)||!local_bot_runtime::put(runtime,0x10,w))return false;
            if(!original_stat_equals(runtime,"get_RoundLen",4,expected))return false;
            ++checked;
        }}return true;
}
static void run(){
    if(!progress_ready||access(DATA_ROOT "weapon-reload-audit.request",F_OK))return;
    unlink(DATA_ROOT "weapon-reload-audit.request");auto before=progress;
    FILE* f=fopen(DATA_ROOT "weapon-reload-audit.jsonl","w");if(!f)return;chmod(DATA_ROOT "weapon-reload-audit.jsonl",0644);
    Plan plans[2];bool ok=true;int candidates=0,accepted=0,refused=0,native=0,runtime=0,predebit=0;
    for(const char* source:{"CONFEDERATION","RESISTANCE"}){int faction=!strcmp(source,"CONFEDERATION")?1:2;
        void* rows=levels(source);int n=rows?*(int*)((char*)rows+0x18):0;if(n<=0)ok=false;
        for(int i=0;i<n;++i){void* level=list_item(rows,i),*up=weapon_parameters_audit::get(level,0x48);char text[4096];
            if(!up||strcmp(class_name(object_class(up)),"UpgradeUnit")||!local_research_runtime::ascii(weapon_parameters_audit::get(level,0x18),text,sizeof(text)))continue;
            research_effects::WeaponParameters p;if(!research_effects::weapon_parameters_parse(text,p)||!p.reload)continue;
            ++candidates;Effect e;const char* reason=nullptr;
            if(!resolve(level,e)||!shape(e.owner,e,reason)){++refused;fprintf(f,"{\"phase\":\"refused_target\",\"faction\":%d,\"id\":%d,\"reason\":\"%s\"}\n",faction,*(int*)((char*)level+0x10),reason?reason:e.reason);continue;}
            Roots roots;uint64_t original=weapon_parameters_audit::hash(e.owner);void* copy=copy_type(e.owner,e,roots);
            bool pass=copy&&weapon_parameters_audit::raw_values(copy,e.owner,e)&&runtime_values(copy,e.owner,e,runtime)&&
                weapon_parameters_audit::native_init(copy,e,native)&&runtime_values(copy,e.owner,e,runtime)&&plans[faction-1].add(e)&&original==weapon_parameters_audit::hash(e.owner);
            if(faction==local_faction::active){pass=pass&&purchase_supported(level);++predebit;}
            ok=ok&&pass;++accepted;
            fprintf(f,"{\"phase\":\"original_reload\",\"faction\":%d,\"id\":%d,\"owner\":%d,\"ticks\":%d,\"pass\":%s,\"canonical_unchanged\":%s}\n",faction,*(int*)((char*)level+0x10),*(int*)((char*)e.owner+0x10),p.reload,pass?"true":"false",original==weapon_parameters_audit::hash(e.owner)?"true":"false");fflush(f);
        }}
    int cumulative=0,guarded=0;
    for(int faction=1;faction<=2;++faction)for(int i=0;i<plans[faction-1].count;++i){Roots roots;Effect e=plans[faction-1].families[i];const char* reason=nullptr;
        if(!shape(e.owner,e,reason)){++guarded;fprintf(f,"{\"phase\":\"cumulative_refusal\",\"faction\":%d,\"owner\":%d,\"reason\":\"%s\"}\n",faction,*(int*)((char*)e.owner+0x10),reason?reason:"unknown");continue;}
        uint64_t original=weapon_parameters_audit::hash(e.owner);void* a=copy_type(e.owner,e,roots),*b=copy_type(e.owner,e,roots);
        bool pass=a&&b&&weapon_parameters_audit::raw_values(a,e.owner,e)&&weapon_parameters_audit::raw_values(b,e.owner,e)&&original==weapon_parameters_audit::hash(e.owner);
        // A period of zero/negative must fail before any purchase/projection.
        Effect bad=e;bad.reload[0]=INT16_MAX;const char* rejected=nullptr;pass=pass&&!shape(bad.owner,bad,rejected)&&!copy_type(bad.owner,bad,roots);
        ok=ok&&pass;++cumulative;fprintf(f,"{\"phase\":\"cumulative_reload\",\"faction\":%d,\"owner\":%d,\"pass\":%s,\"repeat_and_zero_guard\":%s}\n",faction,*(int*)((char*)e.owner+0x10),pass?"true":"false",pass?"true":"false");
    }
    bool isolated=!memcmp(&before,&progress,sizeof(progress));
    fprintf(f,"{\"summary\":true,\"pass\":%s,\"candidates\":%d,\"accepted\":%d,\"refused\":%d,\"native_initializers\":%d,\"runtime_getters\":%d,\"active_faction_predebit\":%d,\"cumulative_families\":%d,\"cumulative_guarded\":%d,\"player_state_unchanged\":%s,\"player_save_writes\":0,\"battle_runs\":0}\n",ok&&accepted>0&&cumulative>0&&isolated?"true":"false",candidates,accepted,refused,native,runtime,predebit,cumulative,guarded,isolated?"true":"false");fclose(f);
}
}
