// Original calcSight and whole mixed rows; detached original objects only.
#pragma once
namespace building_sight_audit {
using namespace building_weapon_research;
static bool calculated(void* type,const Effect& e,int& cases){
    Roots roots;void* klass=find_class("com.geargames.aow.entities","Building");
    void* building=object_new(klass),*boost=array_new(find_class("System","Int16"),4);
    if(!roots.keep(building)||!roots.keep(boost))return false;
    field_set(building,class_field_from_name(klass,"bu_type"),type);
    void* field=class_field_from_name(klass,"boostEffects");if(!field||field_offset(field)!=0x130)return false;
    for(int i=0;i<count(get(type,0x18));++i){void* level=list_item(get(type,0x18),i),*base=list_item(get(e.owner,0x18),i);
        int expected=*(int*)((char*)base+0x40)+e.sight;
        *(int8_t*)((char*)building+0x60)=i;
        if(!original_stat_equals(level,"get_Sight",4,expected)||!original_stat_equals(level,"get_SightInit",4,expected))return false;
        field_set(building,field,nullptr);
        if(!original_stat_equals(building,"calcSight",4,expected))return false;++cases;
        ((int16_t*)((char*)boost+32))[3]=7;field_set(building,field,boost);
        if(!original_stat_equals(building,"calcSight",4,expected+7))return false;++cases;
    }return true;
}
static bool migrate(FILE* out,int faction,const Plan& plan,int& entities,int& runtime_refs){
    preplaced_research_audit::Fixture x;bool ok=x.init(faction==1?"fmc1m":"fmc2m");Roots roots;uintptr_t handle=0;
    void* old=ok?get(x.side,0xf8):nullptr,*prepared=ok?local_research_runtime::clone(x.side,roots):nullptr;
    void* types=old?local_dictionary_values(old,find_class("com.geargames.aow.entities","BuildingType"),handle):nullptr;
    ok=ok&&prepared&&types&&roots.take(handle);int slot=0;
    for(int i=0;ok&&i<plan.size;++i){void* type=nullptr;
        for(size_t j=0;j<array_length(types);++j){void* item=((void**)((char*)types+32))[j];if(!building_research::identity(item,plan.families[i].owner))continue;if(type){ok=false;break;}type=item;}
        if(!type){ok=false;break;}
        for(int level=0;ok&&level<count(get(type,0x18));++level)for(int damage:{0,7,-1})ok=ok&&x.spawn(type,level,damage,true,slot++);
    }
    int matched=0;bool applied=ok&&apply(prepared,old,plan,matched);preplaced_research::Migration migration;
    bool staged=applied&&x.unchanged()&&migration.prepare(x.battle,x.side,prepared)&&x.unchanged();
    if(staged){field_set(x.side,class_field_from_name(object_class(x.side),"$Gg"),get(prepared,0xf8));migration.commit();}
    int references=0;bool verified=staged&&x.verify(references);
    for(int i=0;verified&&i<x.count;++i){void* b=x.samples[i].entity,*type=get(b,0x38);
        void* level=list_item(get(type,0x18),*(int8_t*)((char*)b+0x60));verified=original_stat_equals(b,"calcSight",4,*(int*)((char*)level+0x40));}
    ok=ok&&applied&&staged&&verified&&matched==plan.size;entities+=x.count;runtime_refs+=references;
    fprintf(out,"{\"phase\":\"preplaced_sight\",\"faction\":%d,\"families\":%d,\"entities\":%d,\"runtime_weapon_references\":%d,\"changed_weapon_links\":%d,\"staged_unchanged\":%s,\"pass\":%s}\n",faction,matched,x.count,references,migration.weapon_links,staged?"true":"false",ok?"true":"false");return ok;
}
static bool saved(FILE* out,int faction,int& cold_count){
    local_progress::StateBuffer state,loaded;if(!state.data||!loaded.data)return false;*state.data=local_progress::seed(100000000,0,0,10000);progress=*state.data;
    void* rows=generic_unit_research::levels(faction==1?"CONFEDERATION":"RESISTANCE");int n=0,first=0,old_owners[5]={},old_count=0;bool ok=true;
    for(int i=0;i<count(rows);++i){void* row=list_item(rows,i);Effect e;if(!resolve(row,e))continue;
        if(!e.sight){int owner=*(int*)((char*)e.owner+0x10);bool known=false;for(int k=0;k<old_count;++k)if(old_owners[k]==owner)known=true;
            if(known)continue;if(old_count==5){ok=false;break;}old_owners[old_count++]=owner;
        }else ++n;
        int id=*(int*)((char*)row+0x10);if(e.sight&&!first)first=id;
        ok=ok&&local_research_supported(row)&&local_progress::buy_upgrade(*state.data,id,*(int*)((char*)row+0x20),*(int*)((char*)row+0x28));progress=*state.data;
    }
    ok=ok&&old_count==5&&n>0&&local_progress::buy_upgrade(*state.data,faction==1?3075:13075,400,1);progress=*state.data;
    const LocalScenario* route=nullptr;for(const auto& r:local_scenarios)if(r.faction==faction&&r.rebase_growth){route=&r;break;}
    void* input=route?read_scenario_message(route->file,5040,"@building-sight-saved"):nullptr,*battle=get(input,0x20);
    void* side=battle?local_dictionary_item(get(battle,0x178),1):nullptr;
    ok=ok&&side&&apply_local_battle_research(battle,1,true);Plan plan;ok=ok&&collect(plan)&&plan.size==7;
    Roots roots;uintptr_t handle=0;void* types=ok?local_dictionary_values(get(side,0xf8),find_class("com.geargames.aow.entities","BuildingType"),handle):nullptr;
    ok=ok&&types&&roots.take(handle);int checked=0;
    for(int i=0;ok&&i<plan.size;++i){const auto& e=plan.families[i];if(!e.sight)continue;int found=0;
        for(size_t j=0;j<array_length(types);++j){void* type=((void**)((char*)types+32))[j];if(!building_research::identity(type,e.owner))continue;++found;
            for(int l=0;l<count(get(type,0x18));++l){void* actual=list_item(get(type,0x18),l),*base=list_item(get(e.owner,0x18),l);int value=*(int*)((char*)base+0x40)+e.sight;
                ok=ok&&original_stat_equals(actual,"get_Sight",4,value)&&original_stat_equals(actual,"get_SightInit",4,value);++checked;}}
        ok=ok&&found==1;
    }
    ok=ok&&*(int8_t*)((char*)side+0x1a9)==21;
    char path[512];snprintf(path,sizeof(path),DATA_ROOT "building-sight-private-%d-f%d",getpid(),faction);
    bool cold=ok&&mkdir(path,0700)==0&&local_progress::write(path,*state.data)&&local_progress::load(path,*loaded.data)==1&&!memcmp(state.data,loaded.data,sizeof(progress));
    int credits=loaded.data->credits;cold=cold&&!local_progress::buy_upgrade(*loaded.data,first,1,1)&&loaded.data->credits==credits;if(cold)++cold_count;
    ok=ok&&cold;fprintf(out,"{\"phase\":\"saved_sight\",\"faction\":%d,\"new_rows\":%d,\"old_weapon_families\":%d,\"combined_families\":%d,\"sight_levels\":%d,\"hq_capacity\":21,\"private_cold_receipt\":%s,\"pass\":%s}\n",faction,n,old_count,plan.size,checked,cold?"true":"false",ok?"true":"false");return ok;
}
static __attribute__((noinline)) void run(){
    if(!progress_ready||access(DATA_ROOT "building-sight-audit.request",F_OK))return;unlink(DATA_ROOT "building-sight-audit.request");
    building_weapon_audit::Restore restore;if(!restore.saved.data)return;FILE* out=fopen(DATA_ROOT "building-sight-audit.jsonl","w");if(!out)return;chmod(DATA_ROOT "building-sight-audit.jsonl",0644);
    bool ok=true;int rows_count=0,preflight=0,row_calcs=0,cumulative_calcs=0,families=0,mixed=0,initializers=0,entities=0,links=0,cold=0;
    for(int faction:{1,2}){local_faction::active=faction;progress=local_progress::seed(100000000,0,0,10000);Plan plan;
        void* rows=generic_unit_research::levels(faction==1?"CONFEDERATION":"RESISTANCE");
        for(int i=0;i<count(rows);++i){void* row=list_item(rows,i);Effect e;if(!resolve(row,e)||!e.sight)continue;
            Roots roots;uint64_t before=building_weapon_audit::hash(e.owner);bool allowed=local_research_supported(row);if(allowed)++preflight;
            void* copy=allowed?copy_type(e.owner,e,roots):nullptr;
            bool raw=copy&&building_weapon_audit::raw(copy,e.owner,e),calc=raw&&calculated(copy,e,row_calcs);
            bool init=calc&&(!e.targets||building_weapon_audit::initialize(copy,e,initializers));
            bool pass=allowed&&raw&&calc&&init&&building_weapon_audit::hash(e.owner)==before&&plan.add(e);ok=ok&&pass;++rows_count;if(e.targets)++mixed;
            fprintf(out,"{\"phase\":\"original_sight_row\",\"faction\":%d,\"id\":%d,\"owner\":%d,\"sight\":%d,\"mixed\":%s,\"raw\":%s,\"calc\":%s,\"init\":%s,\"preflight\":%s,\"pass\":%s}\n",faction,*(int*)((char*)row+0x10),*(int*)((char*)e.owner+0x10),e.sight,e.targets?"true":"false",raw?"true":"false",calc?"true":"false",init?"true":"false",allowed?"true":"false",pass?"true":"false");
        }
        for(int i=0;i<plan.size;++i){const auto& e=plan.families[i];Roots roots;auto before=building_weapon_audit::hash(e.owner);
            void* a=copy_type(e.owner,e,roots),*b=a?copy_type(a,e,roots):nullptr;
            bool pass=building_weapon_audit::raw(a,e.owner,e)&&calculated(a,e,cumulative_calcs)&&b&&building_weapon_audit::hash(a)==building_weapon_audit::hash(b);
            auto bad=e;bad.sight=INT_MAX;pass=pass&&!copy_type(e.owner,bad,roots)&&before==building_weapon_audit::hash(e.owner);ok=ok&&pass;++families;
            fprintf(out,"{\"phase\":\"cumulative_sight\",\"faction\":%d,\"owner\":%d,\"sight\":%d,\"pass\":%s}\n",faction,*(int*)((char*)e.owner+0x10),e.sight,pass?"true":"false");}
        ok=migrate(out,faction,plan,entities,links)&&ok;ok=saved(out,faction,cold)&&ok;
    }
    progress=*restore.saved.data;local_faction::active=restore.faction;bool unchanged=!memcmp(&progress,restore.saved.data,sizeof(progress));
    ok=ok&&unchanged&&rows_count==39&&mixed==10&&preflight==39&&row_calcs==222&&cumulative_calcs==36&&families==10&&initializers==10&&entities==54&&links==21&&cold==2;
    fprintf(out,"{\"summary\":true,\"pass\":%s,\"new_original_rows\":%d,\"mixed_rows\":%d,\"preflight\":%d,\"row_calc_sight_cases\":%d,\"cumulative_calc_sight_cases\":%d,\"families\":%d,\"native_weapon_init\":%d,\"preplaced_entities\":%d,\"runtime_weapon_references\":%d,\"private_cold_receipts\":%d,\"player_state_unchanged\":%s,\"player_save_writes\":0,\"battle_runs\":0}\n",ok?"true":"false",rows_count,mixed,preflight,row_calcs,cumulative_calcs,families,initializers,entities,links,cold,unchanged?"true":"false");fclose(out);
}
}
