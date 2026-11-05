// Whole retained range/explosion descriptions; detached original prototypes.
#pragma once
namespace weapon_parameters_audit {
using namespace generic_unit_research;
static void* get(void* p,int o){return p?*(void**)((char*)p+o):nullptr;}
static uint64_t hash(void* type){
    uint64_t h=research_batch_audit::type_hash(type);void* states=get(type,0x18);
    for(int i=0;states&&i<*(int*)((char*)states+0x18);++i){void* ws=get(list_item(states,i),0x18);
        for(int j=0;ws&&j<*(int*)((char*)ws+0x18);++j){void* w=list_item(ws,j);if(!w)return 0;
            for(int k=0x10;k<0x110;++k)h=(h^((uint8_t*)w)[k])*1099511628211ULL;}}
    return h;
}
static bool raw_values(void* copy,void* base,const Effect& e){
    if(!copy||!research_batch_audit::getters(copy,base,e))return false;
    void* states=get(base,0x18),*actual=get(copy,0x18);
    for(int i=0;i<*(int*)((char*)states+0x18);++i){void* a=get(list_item(states,i),0x18),*b=get(list_item(actual,i),0x18);
        for(int j=0;a&&j<*(int*)((char*)a+0x18);++j){void* old=list_item(a,j),*w=list_item(b,j);int index=target_index(old,e);
            int range=*(int*)((char*)old+0x4c)+(index>=0?e.range[index]:0),radius=*(int*)((char*)old+0x7c)+(index>=0?e.explosion[index]:0);
            int reload=*(int16_t*)((char*)old+0x66)-(index>=0?e.reload[index]:0);
            int shot_start=*(int16_t*)((char*)old+0x60)-(index>=0?e.shot_start[index]:0);
            if(!original_stat_equals(w,"get_Distance",4,range)||!original_stat_equals(w,"get_ExplosionRadius",4,radius)||!original_stat_equals(w,"get_RoundLen",2,reload)||!original_stat_equals(w,"get_ShotStart",2,shot_start))return false;
        }}return true;
}
static bool native_init(void* copy,const Effect& e,int& count){
    void* done[1024]={};int n=0;void* states=get(copy,0x18);
    for(int i=0;i<*(int*)((char*)states+0x18);++i){void* ws=get(list_item(states,i),0x18);
        for(int j=0;ws&&j<*(int*)((char*)ws+0x18);++j){void* w=list_item(ws,j);if(target_index(w,e)<0)continue;
            bool known=false;for(int k=0;k<n;++k)if(done[k]==w)known=true;if(known)continue;if(n==1024)return false;done[n++]=w;
            if(*(bool*)((char*)w+0x14))return false;int raw=*(int*)((char*)w+0x4c),radius=*(int*)((char*)w+0x7c);
            int padded=raw>1000?raw+50:raw*105/100;void* ex=nullptr;invoke_named(w,"init",0,nullptr,&ex);
            if(ex||!*(bool*)((char*)w+0x14)||*(int*)((char*)w+0x4c)!=padded*padded||
               *(int*)((char*)w+0x50)!=padded||*(int*)((char*)w+0x54)!=padded||
               *(int*)((char*)w+0x7c)!=radius||*(int*)((char*)w+0x80)!=radius)return false;
            ++count;
        }}return n>0;
}
static void run(){
    if(access(DATA_ROOT "weapon-parameters-audit.request",F_OK))return;
    unlink(DATA_ROOT "weapon-parameters-audit.request");auto before=progress;
    FILE* f=fopen(DATA_ROOT "weapon-parameters-audit.jsonl","w");if(!f)return;chmod(DATA_ROOT "weapon-parameters-audit.jsonl",0644);
    Plan plans[2];int candidates=0,accepted=0,refused=0,initializers=0,purchase_checks=0;bool ok=true;
    for(const char* source:{"CONFEDERATION","RESISTANCE"}){
        int faction=!strcmp(source,"CONFEDERATION")?1:2;void* rows=levels(source);int n=rows?*(int*)((char*)rows+0x18):0;
        if(n<=0){ok=false;continue;}
        for(int i=0;i<n;++i){void* level=list_item(rows,i),*up=get(level,0x48);char description[4096];
            if(!up||strcmp(class_name(object_class(up)),"UpgradeUnit")||
               !local_research_runtime::ascii(get(level,0x18),description,sizeof(description)))continue;
            research_effects::WeaponParameters parameters;
            if(!research_effects::weapon_parameters_parse(description,parameters)||parameters.reload||(!parameters.range&&!parameters.explosion))continue;
            ++candidates;Effect e;const char* reason=nullptr;bool available=resolve(level,e)&&shape(e.owner,e,reason);
            if(!available){++refused;fprintf(f,"{\"phase\":\"unsupported_original_target\",\"faction\":%d,\"id\":%d,\"reason\":\"%s\"}\n",faction,*(int*)((char*)level+0x10),reason?reason:e.reason);continue;}
            Roots roots;uint64_t original=hash(e.owner);void* copy=copy_type(e.owner,e,roots);
            bool pass=raw_values(copy,e.owner,e)&&native_init(copy,e,initializers)&&original==hash(e.owner)&&plans[faction-1].add(e);
            if(faction==local_faction::active){pass=pass&&purchase_supported(level);++purchase_checks;}
            ok=ok&&pass;++accepted;
            fprintf(f,"{\"phase\":\"original_weapon_parameters\",\"faction\":%d,\"id\":%d,\"owner\":%d,\"range_delta\":%d,\"explosion_delta\":%d,\"pass\":%s,\"canonical_unchanged\":%s}\n",faction,*(int*)((char*)level+0x10),*(int*)((char*)e.owner+0x10),e.range[0],e.explosion[0],pass?"true":"false",original==hash(e.owner)?"true":"false");
            fflush(f);
        }
    }
    int families=0;
    for(int faction=1;faction<=2;++faction)for(int i=0;i<plans[faction-1].count;++i){Roots roots;
        const Effect& e=plans[faction-1].families[i];uint64_t original=hash(e.owner);const char* reason=nullptr;
        void* a=copy_type(e.owner,e,roots),*b=copy_type(e.owner,e,roots);
        bool pass=shape(e.owner,e,reason)&&raw_values(a,e.owner,e)&&raw_values(b,e.owner,e)&&original==hash(e.owner);
        ok=ok&&pass;++families;fprintf(f,"{\"phase\":\"cumulative_weapon_parameters\",\"faction\":%d,\"owner\":%d,\"pass\":%s,\"repeat_from_baseline\":%s}\n",faction,*(int*)((char*)e.owner+0x10),pass?"true":"false",pass?"true":"false");
    }
    bool unchanged=!memcmp(&before,&progress,sizeof(before));
    fprintf(f,"{\"summary\":true,\"pass\":%s,\"candidates\":%d,\"accepted\":%d,\"refused_ambiguous_targets\":%d,\"families\":%d,\"native_initializers\":%d,\"active_faction_predebit_checks\":%d,\"player_state_unchanged\":%s,\"player_save_writes\":0,\"battle_runs\":0}\n",ok&&accepted>0&&families>0&&unchanged?"true":"false",candidates,accepted,refused,families,initializers,purchase_checks,unchanged?"true":"false");fclose(f);
}
}
