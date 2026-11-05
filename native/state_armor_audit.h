// Detached original prototypes, real native state lookup and startup migration.
#pragma once
namespace state_armor_audit {
using namespace generic_unit_research;
static void* get(void* p,int off){return p?*(void**)((char*)p+off):nullptr;}
static int count(void* p){return p?*(int*)((char*)p+0x18):0;}
static void* family(void* dictionary,void* owner,Roots& roots){
    uintptr_t handle=0;void* rows=local_dictionary_values(dictionary,find_class("com.geargames.aow.entities","UnitType"),handle);
    if(!rows||!roots.take(handle))return nullptr;void* match=nullptr;
    for(size_t i=0;i<array_length(rows);++i){void* type=((void**)((char*)rows+32))[i];if(identity(type,owner)){if(match)return nullptr;match=type;}}
    return match;
}
static bool values(void* copy,void* source,const Effect& e){
    if(!copy||!research_batch_audit::getters(copy,source,e))return false;
    void* before=get(source,0x18),*after=get(copy,0x18);
    for(int i=0;i<count(before);++i){void* a=list_item(before,i),*b=list_item(after,i);unsigned char bytes[0xa8];
        memcpy(bytes,b,sizeof(bytes));memcpy(bytes+0x22,(char*)a+0x22,2);
        if(memcmp(bytes+0x10,(char*)a+0x10,sizeof(bytes)-0x10)||get(a,0x18)!=get(b,0x18))return false;}
    return get(copy,0x98)==get(source,0x98);
}
static void run(){
    if(!progress_ready||access(DATA_ROOT "state-armor-audit.request",F_OK))return;unlink(DATA_ROOT "state-armor-audit.request");
    local_progress::StateBuffer saved;if(!saved.data)return;*saved.data=progress;
    FILE* f=fopen(DATA_ROOT "state-armor-audit.jsonl","w");if(!f)return;chmod(DATA_ROOT "state-armor-audit.jsonl",0644);
    Plan* plans=new(std::nothrow) Plan[2];if(!plans){fclose(f);return;}
    bool ok=true;int total=0,predebit=0,families=0,entities=0;
    for(const char* source:{"CONFEDERATION","RESISTANCE"}){int faction=!strcmp(source,"CONFEDERATION")?1:2;void* rows=levels(source);
        for(int i=0;i<count(rows);++i){void* row=list_item(rows,i),*up=get(row,0x48);char text[4096];research_effects::StateArmor parsed;
            if(!up||strcmp(class_name(object_class(up)),"UpgradeUnit")||!local_research_runtime::ascii(get(row,0x18),text,sizeof(text))||!research_effects::state_armor_parse(text,parsed))continue;
            Effect e;const char* reason=nullptr;Roots roots;bool pass=resolve(row,e)&&shape(e.owner,e,reason);
            uint64_t hash=pass?weapon_parameters_audit::hash(e.owner):0;void* copy=pass?copy_type(e.owner,e,roots):nullptr;
            pass=pass&&values(copy,e.owner,e)&&hash==weapon_parameters_audit::hash(e.owner)&&plans[faction-1].add(e);
            if(faction==local_faction::active){pass=pass&&purchase_supported(row);++predebit;}
            ok&=pass;++total;fprintf(f,"{\"phase\":\"original_row\",\"faction\":%d,\"id\":%d,\"owner\":%d,\"group\":%d,\"pass\":%s}\n",faction,*(int*)((char*)row+0x10),e.owner?*(int*)((char*)e.owner+0x10):-1,parsed.group,pass?"true":"false");
        }}
    for(int faction=1;faction<=2;++faction){Plan& plan=plans[faction-1];
        for(int i=0;i<plan.count;++i){auto& e=plan.families[i];Roots roots;uint64_t hash=weapon_parameters_audit::hash(e.owner);
            void* a=copy_type(e.owner,e,roots),*b=copy_type(e.owner,e,roots);bool pass=values(a,e.owner,e)&&values(b,e.owner,e);
            auto mixed=e;mixed.delta.armor=3;void* combined=copy_type(e.owner,mixed,roots);pass=pass&&values(combined,e.owner,mixed);
            auto bad=e;bad.state_armor.value[0]=INT16_MAX;pass=pass&&!copy_type(e.owner,bad,roots);
            bad=e;bad.state_armor.group=e.state_armor.group==4?1:e.state_armor.group+1;pass=pass&&!copy_type(e.owner,bad,roots);
            // Reordering the source state list must follow state IDs, retaining
            // the original first/second list order rather than swapping effects.
            void* reversed=local_research_runtime::clone(e.owner,roots),*old=get(e.owner,0x18),*list=local_research_runtime::new_list(old,roots);void* ex=nullptr;
            if(list)for(int j=1;j>=0;--j){void* args[]={list_item(old,j)};invoke_named(list,"Add",1,args,&ex);}
            if(reversed&&list&&!ex)field_set(reversed,class_field_from_name(object_class(reversed),"states"),list);
            else pass=false;
            pass=pass&&values(copy_type(reversed,e,roots),reversed,e)&&hash==weapon_parameters_audit::hash(e.owner);
            if(pass){void* changed=local_research_runtime::clone(list_item(list,0),roots);
                if(!changed)pass=false;
                else{*(int*)((char*)changed+0x10)=INT_MIN;int index=0;void* args[]={&index,changed};invoke_named(list,"set_Item",2,args,&ex);
                    pass=!ex&&values(copy_type(reversed,e,roots),reversed,e);
                    *(int*)((char*)changed+0x80)^=0x40000000;
                    pass=pass&&!copy_type(reversed,e,roots)&&hash==weapon_parameters_audit::hash(e.owner);}}
            ok&=pass;++families;fprintf(f,"{\"phase\":\"family_cumulative\",\"faction\":%d,\"owner\":%d,\"pass\":%s,\"repeated_baseline_and_order_independent\":%s}\n",faction,*(int*)((char*)e.owner+0x10),pass?"true":"false",pass?"true":"false");
        }
        preplaced_research_audit::Fixture fixture;bool pass=fixture.init(faction==1?"fmc1m":"fmc2m");Roots roots;
        void* baseline=pass?get(fixture.side,0xf0):nullptr,*enemy=pass?local_dictionary_item(get(fixture.battle,0x178),2):nullptr;
        void* enemy_dictionary=get(enemy,0xf0);uint64_t source_hash=pass?unit_dictionary_health_hash(fixture.side):0,enemy_hash=enemy?unit_dictionary_health_hash(enemy):0;
        int slot=0;
        for(int i=0;pass&&i<plan.count;++i){void* type=family(baseline,plan.families[i].owner,roots);
            pass=type&&identity(type,plan.families[i].owner);
            int deltas[2]={};bool shape_ok=pass&&state_armor_deltas(type,plan.families[i],deltas);pass=pass&&shape_ok;
            void* states=get(type,0x18);fprintf(f,"{\"phase\":\"decoded_family\",\"faction\":%d,\"owner\":%d,\"battle_id\":%d,\"state_ids\":[%d,%d],\"deltas\":[%d,%d],\"pass\":%s}\n",faction,*(int*)((char*)plan.families[i].owner+0x10),type?*(int*)((char*)type+0x10):-1,states?*(int*)((char*)list_item(states,0)+0x10):-1,states?*(int*)((char*)list_item(states,1)+0x10):-1,deltas[0],deltas[1],shape_ok?"true":"false");
            for(int state=0;pass&&state<2;++state)for(int damage:{0,7,-1}){pass=fixture.spawn(type,state,damage,false,slot++);if(!pass)break;}}
        void* prepared=pass?local_research_runtime::clone(fixture.side,roots):nullptr;int matched=0;
        pass=pass&&prepared&&apply(prepared,baseline,plan,matched)&&matched==plan.count&&fixture.unchanged();
        void* first_dictionary=pass?get(prepared,0xf0):nullptr;
        pass=pass&&apply(prepared,baseline,plan,matched);
        for(int i=0;pass&&i<plan.count;++i){void* original=family(baseline,plan.families[i].owner,roots);int id=original?*(int*)((char*)original+0x10):-1;
            pass=values(local_dictionary_item(first_dictionary,id),original,plan.families[i])&&values(local_dictionary_item(get(prepared,0xf0),id),original,plan.families[i]);}
        bool atomic=false;preplaced_research::Migration migration;
        if(pass){auto bad=plan.families[0];bad.state_armor.group=99;Plan invalid;invalid.families[0]=bad;invalid.count=1;
            void* previous=get(prepared,0xf0);int ignored=0;atomic=!apply(prepared,baseline,invalid,ignored)&&get(prepared,0xf0)==previous&&fixture.unchanged();
            pass=atomic&&migration.prepare(fixture.battle,fixture.side,prepared)&&fixture.unchanged();}
        int weapons=0,lookups=0;
        if(pass){field_set(fixture.side,class_field_from_name(object_class(fixture.side),"$gg"),get(prepared,0xf0));
            field_set(fixture.side,class_field_from_name(object_class(fixture.side),"$Gg"),get(prepared,0xf8));migration.commit();
            pass=fixture.verify(weapons)&&research_batch_audit::producer_links(fixture.side);
            for(int i=0;pass&&i<fixture.count;++i){void* e=fixture.samples[i].entity,*type=get(e,0x40);int state=*(int8_t*)((char*)e+0x6b);void* ex=nullptr;
                void* actual=invoke_named(e,"calcState",0,nullptr,&ex);pass=!ex&&actual==list_item(get(type,0x18),state);++lookups;}
            // Hash the immutable original dictionary and untouched enemy seat.
            void* original_side=local_research_runtime::clone(fixture.side,roots);
            if(original_side)field_set(original_side,class_field_from_name(object_class(original_side),"$gg"),baseline);
            pass=pass&&original_side&&source_hash==unit_dictionary_health_hash(original_side)&&enemy_dictionary==get(enemy,0xf0)&&enemy_hash==unit_dictionary_health_hash(enemy);}
        ok&=pass;entities+=fixture.count;fprintf(f,"{\"phase\":\"decoded_side_migration\",\"faction\":%d,\"families\":%d,\"entities\":%d,\"native_state_lookups\":%d,\"atomic_refusal\":%s,\"pass\":%s}\n",faction,matched,fixture.count,lookups,atomic?"true":"false",pass?"true":"false");
    }
    bool unchanged=!memcmp(saved.data,&progress,sizeof(progress));delete[] plans;
    fprintf(f,"{\"summary\":true,\"pass\":%s,\"rows\":%d,\"families\":%d,\"predebit\":%d,\"migrated_entities\":%d,\"player_state_unchanged\":%s,\"save_writes\":0,\"battle_runs\":0}\n",ok&&total==115&&families==7&&unchanged?"true":"false",total,families,predebit,entities,unchanged?"true":"false");fclose(f);
}
}
