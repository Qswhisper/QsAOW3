// One opt-in batch on detached original models and decoded original battles.
#pragma once
namespace weapon_dispersion_audit {
using namespace generic_unit_research;
static bool ballistic(const Effect& e){
    void* states=local_hero::get(e.owner,0x18);
    for(int i=0;states&&i<*(int*)((char*)states+0x18);++i){void* weapons=local_hero::get(list_item(states,i),0x18);
        for(int j=0;weapons&&j<*(int*)((char*)weapons+0x18);++j){void* w=list_item(weapons,j);
            if(target_index(w,e)>=0&&research_effects::dispersion_weapon(*(int16_t*)((char*)w+0x24),*(int8_t*)((char*)w+0x94)))return true;}}
    return false;
}
static void run(){
    if(!progress_ready||access(DATA_ROOT "weapon-dispersion-audit.request",F_OK))return;
    unlink(DATA_ROOT "weapon-dispersion-audit.request");local_progress::StateBuffer before;if(!before.data)return;*before.data=progress;
    FILE* f=fopen(DATA_ROOT "weapon-dispersion-audit.jsonl","w");if(!f)return;chmod(DATA_ROOT "weapon-dispersion-audit.jsonl",0644);
    Plan* plans=new(std::nothrow) Plan[2];if(!plans){fclose(f);return;}
    bool ok=true;int candidates=0,accepted=0,refused=0,ballistic_rows=0,composite_rows=0,inits=0,getters=0,predebit=0,families=0,decoded=0;
    for(const char* key:{"CONFEDERATION","RESISTANCE"}){int faction=!strcmp(key,"CONFEDERATION")?1:2;void* rows=levels(key);int n=rows?*(int*)((char*)rows+0x18):0;
        for(int i=0;i<n;++i){void* row=list_item(rows,i),*up=local_hero::get(row,0x48);char text[4096];research_effects::WeaponParameters p;UnitStats scalar;
            if(!up||strcmp(class_name(object_class(up)),"UpgradeUnit")||!local_research_runtime::ascii(local_hero::get(row,0x18),text,sizeof(text)))continue;
            bool composite=research_effects::composite_unit_weapon_parse(text,scalar,p);
            if((!composite&&!research_effects::weapon_parameters_parse(text,p))||(!p.accuracy[0]&&!p.accuracy[1]&&!p.accuracy[2]))continue;
            ++candidates;Effect e;const char* reason=nullptr;
            if(!resolve(row,e)||!shape(e.owner,e,reason)){++refused;fprintf(f,"{\"phase\":\"refused\",\"faction\":%d,\"id\":%d,\"reason\":\"%s\"}\n",faction,*(int*)((char*)row+0x10),reason?reason:e.reason);continue;}
            Roots roots;uint64_t hash=weapon_parameters_audit::hash(e.owner);void* copy=copy_type(e.owner,e,roots);
            bool pass=copy&&weapon_accuracy_audit::values(copy,e.owner,e,getters)&&weapon_parameters_audit::native_init(copy,e,inits)&&weapon_accuracy_audit::values(copy,e.owner,e,getters,true)&&hash==weapon_parameters_audit::hash(e.owner);
            bool is_ballistic=ballistic(e);if(is_ballistic){pass=pass&&plans[faction-1].add(e);++ballistic_rows;composite_rows+=composite;}
            if(faction==local_faction::active){pass=pass&&purchase_supported(row);++predebit;}ok&=pass;++accepted;
            fprintf(f,"{\"phase\":\"original_row\",\"faction\":%d,\"id\":%d,\"owner\":%d,\"ballistic\":%s,\"composite\":%s,\"pass\":%s,\"canonical_unchanged\":%s}\n",faction,*(int*)((char*)row+0x10),*(int*)((char*)e.owner+0x10),is_ballistic?"true":"false",composite?"true":"false",pass?"true":"false",hash==weapon_parameters_audit::hash(e.owner)?"true":"false");
        }
    }
    for(int faction=0;faction<2;++faction){auto& plan=plans[faction];
        for(int i=0;i<plan.count;++i){auto& e=plan.families[i];Roots roots;uint64_t hash=weapon_parameters_audit::hash(e.owner);
            void* a=copy_type(e.owner,e,roots),*b=copy_type(e.owner,e,roots);
            bool pass=a&&b&&weapon_accuracy_audit::values(a,e.owner,e,getters)&&weapon_accuracy_audit::values(b,e.owner,e,getters)&&hash==weapon_parameters_audit::hash(e.owner);
            auto invalid=e;invalid.accuracy[0][0]=101;pass=pass&&!copy_type(e.owner,invalid,roots);ok&=pass;++families;
            fprintf(f,"{\"phase\":\"family_repeat_and_guard\",\"faction\":%d,\"owner\":%d,\"pass\":%s}\n",faction+1,*(int*)((char*)e.owner+0x10),pass?"true":"false");
        }
        Roots roots;void* config=campaign_seeds::decode(faction?DATA_ROOT "offline-seeds/fmc2m.ggs":DATA_ROOT "offline-seeds/fmc1m.ggs",roots);
        void* battle=local_hero::get(config,0x10),*sides=local_hero::get(battle,0x178),*side=local_dictionary_item(sides,1),*enemy=local_dictionary_item(sides,2);
        void* baseline=local_hero::get(side,0xf0),*other=local_hero::get(enemy,0xf0);int matched=0;
        auto own_hash=hero_battle_projection_audit::type_hash(baseline),enemy_hash=hero_battle_projection_audit::type_hash(other);
        bool pass=own_hash&&enemy_hash&&plan.count&&apply(side,baseline,plan,matched)&&matched==plan.count&&research_batch_audit::producer_links(side);
        for(int repeat=0;pass&&repeat<2;++repeat){
            uintptr_t handle=0;void* all=local_dictionary_values(baseline,find_class("com.geargames.aow.entities","UnitType"),handle);if(handle)roots.take(handle);
            for(int i=0;pass&&i<plan.count;++i){auto& e=plan.families[i];void* source=nullptr;
                for(size_t j=0;all&&j<array_length(all);++j){void* t=((void**)((char*)all+32))[j];if(identity(t,e.owner))source=t;}
                void* projected=source?local_dictionary_item(local_hero::get(side,0xf0),*(int*)((char*)source+0x10)):nullptr;
                pass=source&&projected&&weapon_accuracy_audit::values(projected,source,e,getters);
                if(pass&&repeat==1)pass=weapon_parameters_audit::native_init(projected,e,inits)&&weapon_accuracy_audit::values(projected,source,e,getters,true);
            }
            if(repeat==0)pass=pass&&apply(side,baseline,plan,matched)&&matched==plan.count;
        }
        pass=pass&&own_hash==hero_battle_projection_audit::type_hash(baseline)&&enemy_hash==hero_battle_projection_audit::type_hash(other);
        ok&=pass;++decoded;fprintf(f,"{\"phase\":\"decoded_battle_projection_and_native_init\",\"faction\":%d,\"matched\":%d,\"pass\":%s}\n",faction+1,matched,pass?"true":"false");
    }
    bool unchanged=!memcmp(before.data,&progress,sizeof(progress));delete[] plans;
    fprintf(f,"{\"summary\":true,\"pass\":%s,\"candidates\":%d,\"accepted\":%d,\"refused\":%d,\"ballistic_rows\":%d,\"ballistic_composite_rows\":%d,\"ballistic_families\":%d,\"native_initializers\":%d,\"original_formula_checks\":%d,\"active_faction_predebit\":%d,\"decoded_factions\":%d,\"player_state_unchanged\":%s,\"player_save_writes\":0,\"battle_runs\":0}\n",ok&&ballistic_rows>0&&decoded==2&&unchanged?"true":"false",candidates,accepted,refused,ballistic_rows,composite_rows,families,inits,getters,predebit,decoded,unchanged?"true":"false");fclose(f);
}
}
