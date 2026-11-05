// Detached original fields, mode timers and battle projection; no player writes.
#pragma once
namespace unit_transition_audit {
using namespace generic_unit_research;
static bool values(void* copy,void* base,const Effect& e){
    if(!copy||!research_batch_audit::getters(copy,base,e))return false;
    int to=*(int8_t*)((char*)base+0x4d)-e.transition.to,from=*(int8_t*)((char*)base+0x4e)-e.transition.from;
    if(!original_stat_equals(copy,"get_TickToSpec",1,to)||!original_stat_equals(copy,"get_TickFromSpec",1,from))return false;
    unsigned char bytes[0xd8];memcpy(bytes,copy,sizeof(bytes));
    // This paired timer effect changes no other original type/state/weapon.
    bytes[0x4d]=((unsigned char*)base)[0x4d];bytes[0x4e]=((unsigned char*)base)[0x4e];
    return !memcmp(bytes+0x10,(char*)base+0x10,sizeof(bytes)-0x10);
}
static bool native_timer(void* type,int faction,int expected,int& actual,const char*& error){
    preplaced_research_audit::Fixture fixture;
    if(!fixture.init(faction==1?"fmc1m":"fmc2m")||!fixture.spawn(type,1,0,false,1000)){error="fixture_init";return false;}
    void* unit=fixture.samples[0].entity,*exception=nullptr;
    void* tick=invoke_named(fixture.battle,"$Qo",0,nullptr,&exception);if(!tick||exception){error="original_battle_tick";return false;}
    int current=*(int*)((char*)tick+16);
    // Original UnitAct.cG reads Unit.UnType.TickFromSpec and sets SiegeTick
    // to current Battle.Qo + the signed timer (483643c/4836200 dataflow).
    const void* method=class_method(find_class("com.geargames.aow.entities.battle","UnitAct"),"$cG",2);
    if(!method){error="original_mode_return_method";return false;}
    void* args[]={fixture.battle,unit};runtime_invoke(method,nullptr,args,&exception);
    if(exception){error=class_name(object_class(exception));return false;}
    void* deadline=invoke_named(unit,"get_SiegeTick",0,nullptr,&exception);
    if(!deadline||exception){error="original_siege_tick";return false;}
    actual=*(int*)((char*)deadline+16)-current;
    return actual==expected;
}
static void run(){
    if(!progress_ready||access(DATA_ROOT "unit-transition-audit.request",F_OK))return;unlink(DATA_ROOT "unit-transition-audit.request");
    bool commands=!access(DATA_ROOT "unit-transition-commands-audit.request",F_OK);
    if(commands)unlink(DATA_ROOT "unit-transition-commands-audit.request");
    local_progress::StateBuffer before;if(!before.data)return;*before.data=progress;
    FILE* f=fopen(DATA_ROOT "unit-transition-audit.jsonl","w");if(!f)return;chmod(DATA_ROOT "unit-transition-audit.jsonl",0644);
    Plan* plans=new(std::nothrow) Plan[2];if(!plans){fclose(f);return;}
    bool ok=true,command_ok=true;int tested=0,predebit=0,families=0,native=0,decoded=0;
    for(const char* key:{"CONFEDERATION","RESISTANCE"}){int faction=!strcmp(key,"CONFEDERATION")?1:2;void* rows=levels(key);int n=rows?*(int*)((char*)rows+0x18):0;
        for(int i=0;i<n;++i){void* row=list_item(rows,i),*up=local_hero::get(row,0x48);char text[4096];research_effects::UnitTransition transition;
            if(!up||strcmp(class_name(object_class(up)),"UpgradeUnit")||!local_research_runtime::ascii(local_hero::get(row,0x18),text,sizeof(text))||!research_effects::unit_transition_parse(text,transition))continue;
            Effect e;const char* reason=nullptr;bool pass=resolve(row,e)&&shape(e.owner,e,reason);Roots roots;uint64_t hash=pass?weapon_parameters_audit::hash(e.owner):0;
            void* copy=pass?copy_type(e.owner,e,roots):nullptr;pass=pass&&values(copy,e.owner,e)&&hash==weapon_parameters_audit::hash(e.owner)&&plans[faction-1].add(e);
            if(faction==local_faction::active){pass=pass&&purchase_supported(row);++predebit;}ok&=pass;++tested;
            fprintf(f,"{\"phase\":\"original_row\",\"faction\":%d,\"id\":%d,\"owner\":%d,\"to_ticks\":%d,\"from_ticks\":%d,\"pass\":%s}\n",faction,*(int*)((char*)row+0x10),e.owner?*(int*)((char*)e.owner+0x10):-1,transition.to,transition.from,pass?"true":"false");
        }
    }
    for(int faction=0;faction<2;++faction){auto& plan=plans[faction];
        for(int i=0;i<plan.count;++i){auto& e=plan.families[i];Roots roots;uint64_t hash=weapon_parameters_audit::hash(e.owner);
            void* a=copy_type(e.owner,e,roots),*b=copy_type(e.owner,e,roots);bool pass=values(a,e.owner,e)&&values(b,e.owner,e);
            auto bad=e;bad.transition.to=*(int8_t*)((char*)e.owner+0x4d);pass=pass&&!copy_type(e.owner,bad,roots);
            if(commands&&a){int original=*(int8_t*)((char*)e.owner+0x4e),adapted=*(int8_t*)((char*)a+0x4e);
                int raw_original=-999,raw_projected=-999;
                const char* old_error="none",*new_error="none";
                bool original_ok=native_timer(e.owner,faction+1,original,raw_original,old_error),projected_ok=native_timer(a,faction+1,adapted,raw_projected,new_error);
                bool timer=original_ok&&projected_ok;command_ok=command_ok&&timer;native+=2;
                fprintf(f,"{\"phase\":\"native_transition_counter\",\"faction\":%d,\"owner\":%d,\"state\":1,\"original_ticks\":%d,\"projected_ticks\":%d,\"actual_original\":%d,\"actual_projected\":%d,\"original_error\":\"%s\",\"projected_error\":\"%s\",\"pass\":%s}\n",faction+1,*(int*)((char*)e.owner+0x10),original,adapted,raw_original,raw_projected,old_error,new_error,timer?"true":"false");
            }
            pass=pass&&hash==weapon_parameters_audit::hash(e.owner);ok&=pass;++families;
            fprintf(f,"{\"phase\":\"cumulative_repeat_guard\",\"faction\":%d,\"owner\":%d,\"pass\":%s}\n",faction+1,*(int*)((char*)e.owner+0x10),pass?"true":"false");
        }
        Roots roots;void* config=campaign_seeds::decode(faction?DATA_ROOT "offline-seeds/fmc2m.ggs":DATA_ROOT "offline-seeds/fmc1m.ggs",roots);
        void* battle=local_hero::get(config,0x10),*side=local_dictionary_item(local_hero::get(battle,0x178),1),*enemy=local_dictionary_item(local_hero::get(battle,0x178),2);
        void* baseline=local_hero::get(side,0xf0),*enemy_types=local_hero::get(enemy,0xf0);int matched=0;
        uint64_t hash=hero_battle_projection_audit::type_hash(baseline),other=hero_battle_projection_audit::type_hash(enemy_types);
        bool pass=hash&&other&&plan.count==2&&apply(side,baseline,plan,matched)&&matched==2&&research_batch_audit::producer_links(side);
        for(int repeat=0;repeat<2&&pass;++repeat){uintptr_t handle=0;void* all=local_dictionary_values(baseline,find_class("com.geargames.aow.entities","UnitType"),handle);if(handle)roots.take(handle);
            for(int i=0;i<plan.count&&pass;++i){auto& e=plan.families[i];void* source=nullptr;
                for(size_t j=0;all&&j<array_length(all);++j){void* u=((void**)((char*)all+32))[j];if(identity(u,e.owner))source=u;}
                void* projected=source?local_dictionary_item(local_hero::get(side,0xf0),*(int*)((char*)source+0x10)):nullptr;pass=source&&values(projected,source,e);}
            pass=pass&&apply(side,baseline,plan,matched)&&matched==2;}
        pass=pass&&hash==hero_battle_projection_audit::type_hash(baseline)&&other==hero_battle_projection_audit::type_hash(enemy_types);ok&=pass;++decoded;
        fprintf(f,"{\"phase\":\"decoded_battle\",\"faction\":%d,\"matched\":%d,\"pass\":%s}\n",faction+1,matched,pass?"true":"false");
    }
    bool unchanged=!memcmp(before.data,&progress,sizeof(progress));delete[] plans;
    fprintf(f,"{\"summary\":true,\"pass\":%s,\"rows\":%d,\"families\":%d,\"original_native_counter_samples\":%d,\"commands_requested\":%s,\"command_fixture_pass\":%s,\"active_faction_predebit\":%d,\"decoded_factions\":%d,\"player_state_unchanged\":%s,\"player_save_writes\":0,\"battle_runs\":0}\n",ok&&(!commands||command_ok)&&tested==24&&families==4&&decoded==2&&unchanged?"true":"false",tested,families,native,commands?"true":"false",commands?(command_ok?"true":"false"):"null",predebit,decoded,unchanged?"true":"false");fclose(f);
}
}
