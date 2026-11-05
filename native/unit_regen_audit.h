// Shared repair/recovery projection, on detached original models only.
#pragma once
namespace unit_regen_audit {
using namespace generic_unit_research;
static bool values(void* copy,void* source,const Effect& e){
    if(!copy||!research_batch_audit::getters(copy,source,e))return false;
    void* old=local_hero::get(source,0x18),*rows=local_hero::get(copy,0x18);
    for(int i=0;i<*(int*)((char*)old+0x18);++i){void* s=list_item(old,i),*c=list_item(rows,i);
        if(c==s||!original_stat_equals(c,"get_Regen",2,*(int16_t*)((char*)s+0x72)+e.delta.regen)||
           !original_stat_equals(c,"get_RegenInit",2,*(int16_t*)((char*)s+0x74)))return false;
    }return true;
}
static bool initialized_state(void* state,Roots& roots){
    // UnitState.init visits all weapons: privatize those references too, so
    // this field-contract test cannot initialize a shared canonical weapon.
    void* list=local_hero::get(state,0x18),*weapons=local_research_runtime::new_list(list,roots);
    if(!weapons)return false;void* ex=nullptr;
    for(int j=0;list&&j<*(int*)((char*)list+0x18);++j){void* w=local_research_runtime::clone(list_item(list,j),roots);if(!w)return false;
        void* args[]={w};invoke_named(weapons,"Add",1,args,&ex);if(ex)return false;}
    field_set(state,class_field_from_name(object_class(state),"weapons"),weapons);
    int raw=*(int16_t*)((char*)state+0x72);invoke_named(state,"init",0,nullptr,&ex);
    return !ex&&original_stat_equals(state,"get_Regen",2,raw)&&original_stat_equals(state,"get_RegenInit",2,raw);
}
static void run(){
    if(!progress_ready||access(DATA_ROOT "unit-regen-audit.request",F_OK))return;
    unlink(DATA_ROOT "unit-regen-audit.request");auto before=progress;
    FILE* f=fopen(DATA_ROOT "unit-regen-audit.jsonl","w");if(!f)return;
    bool ok=research_batch_audit::parser_checks();int tested=0,predebit=0,cumulative=0,inits=0;Plan plans[2];
    for(const char* source:{"CONFEDERATION","RESISTANCE"}){int faction=!strcmp(source,"CONFEDERATION")?1:2;
        void* rows=levels(source);int n=rows?*(int*)((char*)rows+0x18):0;
        for(int i=0;i<n;++i){void* row=list_item(rows,i),*up=local_hero::get(row,0x48);char text[4096];research_effects::UnitStats d;int unknown=0;
            if(!up||strcmp(class_name(object_class(up)),"UpgradeUnit")||!local_research_runtime::ascii(local_hero::get(row,0x18),text,sizeof(text))||
               research_effects::unit_stats_parse(text,d,unknown)!=research_effects::UnitParse::ok||!d.regen)continue;
            Effect e;const char* reason=nullptr;bool resolved=resolve(row,e)&&shape(e.owner,e,reason);Roots roots;
            uint64_t hash=resolved?weapon_parameters_audit::hash(e.owner):0;void* copy=resolved?copy_type(e.owner,e,roots):nullptr;
            bool pass=resolved&&values(copy,e.owner,e)&&plans[faction-1].add(e)&&hash&&hash==weapon_parameters_audit::hash(e.owner);
            if(faction==local_faction::active){pass=pass&&purchase_supported(row);++predebit;}
            ok=ok&&pass;++tested;
            fprintf(f,"{\"phase\":\"row\",\"faction\":%d,\"id\":%d,\"owner\":%d,\"regen\":%d,\"pass\":%s}\n",faction,*(int*)((char*)row+0x10),e.owner?*(int*)((char*)e.owner+0x10):-1,d.regen,pass?"true":"false");
        }}
    for(int faction=0;faction<2;++faction){auto& plan=plans[faction];
        for(int i=0;i<plan.count;++i){auto e=plan.families[i];Roots roots;uint64_t hash=weapon_parameters_audit::hash(e.owner);
            void* a=copy_type(e.owner,e,roots),*b=copy_type(e.owner,e,roots);bool pass=values(a,e.owner,e)&&values(b,e.owner,e);
            auto bad=e;bad.delta.regen=INT16_MAX+1;pass=pass&&!copy_type(e.owner,bad,roots);
            void* states=local_hero::get(a,0x18);for(int j=0;pass&&states&&j<*(int*)((char*)states+0x18);++j){pass=initialized_state(list_item(states,j),roots);++inits;}
            pass=pass&&hash==weapon_parameters_audit::hash(e.owner);ok=ok&&pass;++cumulative;
            fprintf(f,"{\"phase\":\"family\",\"faction\":%d,\"owner\":%d,\"regen_total\":%d,\"pass\":%s}\n",faction+1,*(int*)((char*)e.owner+0x10),e.delta.regen,pass?"true":"false");
        }
        Roots roots;void* config=campaign_seeds::decode(faction?DATA_ROOT "offline-seeds/fmc2m.ggs":DATA_ROOT "offline-seeds/fmc1m.ggs",roots);
        void* battle=local_hero::get(config,0x10),*sides=local_hero::get(battle,0x178),*side=local_dictionary_item(sides,1),*enemy=local_dictionary_item(sides,2);
        void* baseline=local_hero::get(side,0xf0),*other=local_hero::get(enemy,0xf0);int matched=0;
        auto own_hash=hero_battle_projection_audit::type_hash(baseline),enemy_hash=hero_battle_projection_audit::type_hash(other);
        bool pass=own_hash&&enemy_hash&&apply(side,baseline,plan,matched)&&matched==plan.count&&research_batch_audit::producer_links(side);
        for(int repeat=0;pass&&repeat<2;++repeat){
            for(int i=0;i<plan.count;++i){auto& e=plan.families[i];void* source=nullptr,*projected=nullptr;
                uintptr_t handle=0;void* all=local_dictionary_values(baseline,find_class("com.geargames.aow.entities","UnitType"),handle);if(handle)roots.take(handle);
                for(size_t j=0;all&&j<array_length(all);++j){void* t=((void**)((char*)all+32))[j];if(identity(t,e.owner))source=t;}
                projected=source?local_dictionary_item(local_hero::get(side,0xf0),*(int*)((char*)source+0x10)):nullptr;
                pass=pass&&source&&values(projected,source,e);
            }
            pass=pass&&apply(side,baseline,plan,matched)&&matched==plan.count;
        }
        pass=pass&&own_hash==hero_battle_projection_audit::type_hash(baseline)&&enemy_hash==hero_battle_projection_audit::type_hash(other);
        ok=ok&&pass;fprintf(f,"{\"phase\":\"decoded_battle\",\"faction\":%d,\"matched\":%d,\"pass\":%s}\n",faction+1,matched,pass?"true":"false");
    }
    bool isolated=!memcmp(&before,&progress,sizeof(progress));
    fprintf(f,"{\"summary\":true,\"pass\":%s,\"rows\":%d,\"families\":%d,\"state_initializers\":%d,\"active_faction_predebit\":%d,\"player_state_unchanged\":%s,\"player_save_writes\":0,\"battle_runs\":0}\n",ok&&isolated&&tested==486&&cumulative==38?"true":"false",tested,cumulative,inits,predebit,isolated?"true":"false");
    fclose(f);chmod(DATA_ROOT "unit-regen-audit.jsonl",0644);
}
}
