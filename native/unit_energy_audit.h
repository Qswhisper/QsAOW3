#pragma once
#include <utility>
namespace unit_energy_audit {
using namespace generic_unit_research;
static void* get(void* p,int o){return p?*(void**)((char*)p+o):nullptr;}
static bool labelled(const char* p){return strstr(p,"{#20020<")||strstr(p,"{#20021<");}
static uint64_t hash(void* type){uint64_t h=weapon_parameters_audit::hash(type);void* states=get(type,0x18);
    for(int i=0;states&&i<*(int*)((char*)states+0x18);++i){void* s=list_item(states,i);if(!s)return 0;
        for(int k=0x10;k<0xa8;++k)h=(h^((uint8_t*)s)[k])*1099511628211ull;}return h;}
static bool values(void* copy,void* source,const Effect& e,int& getters,FILE* out=nullptr,int faction=0){
    if(!copy||!research_batch_audit::getters(copy,source,e))return false;void* a=get(source,0x18),*b=get(copy,0x18);
    for(int i=0;i<*(int*)((char*)a+0x18);++i){void* old=list_item(a,i),*s=list_item(b,i);research_effects::UnitEnergy expected;
        if(!research_effects::unit_energy_values(*(int16_t*)((char*)old+0x88),*(int16_t*)((char*)old+0x8c),*(int8_t*)((char*)old+0x77),e.energy,expected)||
           !original_stat_equals(s,"get_EnMax",2,expected.capacity)||!original_stat_equals(s,"get_EnRegen",2,expected.recovery)||
           !original_stat_equals(s,"get_EnRegenInit",2,expected.recovery)||!original_stat_equals(s,"get_JumpDist",1,expected.jump))return false;getters+=4;
        uint8_t bytes[0x90];memcpy(bytes,(char*)s+0x18,sizeof(bytes));for(int o:{0x88,0x8c,0x8e})memcpy(bytes+o-0x18,(char*)old+o,2);
        bytes[0x77-0x18]=((uint8_t*)old)[0x77];if(memcmp(bytes,(char*)old+0x18,sizeof(bytes)))return false;
        if(out)fprintf(out,"{\"phase\":\"original_energy_state\",\"faction\":%d,\"owner\":%d,\"state\":%d,\"capacity_before\":%d,\"capacity_after\":%d,\"recovery_before\":%d,\"recovery_after\":%d,\"jump_before\":%d,\"jump_after\":%d}\n",faction,*(int*)((char*)e.owner+0x10),i,*(int16_t*)((char*)old+0x88),expected.capacity,*(int16_t*)((char*)old+0x8c),expected.recovery,*(int8_t*)((char*)old+0x77),expected.jump);
    }return true;
}
static bool initialize(void* type,int& native){
    void* states=get(type,0x18);for(int i=0;i<*(int*)((char*)states+0x18);++i){Roots roots;void* s=list_item(states,i),*klass=object_class(s),*ex=nullptr;
        // Detach every primitive array that this original initializer can use.
        for(auto pair:{std::pair<int,int>{0x28,4},{0x30,4},{0x38,4},{0x40,1},{0x68,1}}){void* old=get(s,pair.first);if(!old)continue;
            size_t n=array_length(old);if(n>256)return false;void* copy=array_new(class_element(object_class(old)),n);if(!roots.keep(copy))return false;
            memcpy((char*)copy+32,(char*)old+32,n*pair.second);const char* name=pair.first==0x28?"undergo":pair.first==0x30?"undergo_original":pair.first==0x38?"undergo_original_init":pair.first==0x40?"damage_priority":"opens";
            field_set(s,class_field_from_name(klass,name),copy);}
        void* old=get(s,0x18),*ws=local_research_runtime::new_list(old,roots);if(!ws)return false;
        for(int w=0;old&&w<*(int*)((char*)old+0x18);++w){Roots one;void* copy=local_research_runtime::clone(list_item(old,w),one);if(!copy)return false;
            void* args[]={copy};invoke_named(ws,"Add",1,args,&ex);if(ex)return false;}
        field_set(s,class_field_from_name(klass,"weapons"),ws);
        int capacity=*(int16_t*)((char*)s+0x88),recovery=*(int16_t*)((char*)s+0x8c),jump=*(int8_t*)((char*)s+0x77);
        invoke_named(s,"init",0,nullptr,&ex);if(ex||!original_stat_equals(s,"get_EnMax",2,capacity)||
            !original_stat_equals(s,"get_EnRegen",2,recovery)||!original_stat_equals(s,"get_EnRegenInit",2,recovery)||!original_stat_equals(s,"get_JumpDist",1,jump))return false;++native;
    }return true;
}
static bool preplaced(int faction,const Plan& plan,int& entities){
    preplaced_research_audit::Fixture fixture;if(!fixture.init(faction==1?"fmc1m":"fmc2m"))return false;Roots roots;uintptr_t handle=0;
    void* dictionary=get(fixture.side,0xf0),*types=local_dictionary_values(dictionary,find_class("com.geargames.aow.entities","UnitType"),handle);if(!types||!roots.take(handle))return false;
    int slot=0;for(int i=0;i<plan.count;++i){void* type=nullptr;
        for(size_t j=0;j<array_length(types);++j){void* item=((void**)((char*)types+32))[j];if(identity(item,plan.families[i].owner))type=item;}
        if(!type)return false;void* states=get(type,0x18);
        for(int j=0;j<*(int*)((char*)states+0x18);++j){if(!fixture.spawn(type,j,0,false,slot++))return false;}}
    void* runtime=get(fixture.side,0x140);for(int i=0;i<slot;++i){void* unit=((void**)((char*)runtime+32))[i];if(!unit)return false;*(int16_t*)((char*)unit+0xb2)=13;}
    if(!apply_local_battle_research(fixture.battle,1))return false;
    for(int i=0;i<slot;++i){void* unit=((void**)((char*)runtime+32))[i];if(!original_stat_equals(unit,"get_Energy",2,13))return false;
        void* type=get(unit,0x40);int state=*(int8_t*)((char*)unit+0x6b);void* s=list_item(get(type,0x18),state);bool found=false;
        for(int j=0;j<plan.count;++j)if(identity(type,plan.families[j].owner)){
            void* old=list_item(get(plan.families[j].owner,0x18),state);research_effects::UnitEnergy expected;
            if(!research_effects::unit_energy_values(*(int16_t*)((char*)old+0x88),*(int16_t*)((char*)old+0x8c),*(int8_t*)((char*)old+0x77),plan.families[j].energy,expected)||
                !original_stat_equals(s,"get_EnMax",2,expected.capacity)||!original_stat_equals(s,"get_EnRegen",2,expected.recovery))return false;found=true;}
        if(!found)return false;++entities;}return true;
}
static __attribute__((noinline)) void run(){
    if(!progress_ready||access(DATA_ROOT "unit-energy-audit.request",F_OK))return;unlink(DATA_ROOT "unit-energy-audit.request");
    building_weapon_audit::Restore restore;if(!restore.saved.data)return;FILE* f=fopen(DATA_ROOT "unit-energy-audit.jsonl","w");if(!f)return;chmod(DATA_ROOT "unit-energy-audit.jsonl",0644);
    bool ok=true;int candidates=0,accepted=0,refused=0,predebit=0,getters=0,native=0,families=0,entities=0;
    auto* plans=new Plan[2];
    for(int faction=1;faction<=2;++faction){local_faction::active=faction;progress=local_progress::seed(100000000,0,0,10000);
        void* rows=levels(faction==1?"CONFEDERATION":"RESISTANCE");
        for(int i=0;rows&&i<*(int*)((char*)rows+0x18);++i){void* row=list_item(rows,i),*up=get(row,0x48);char text[4096];
            if(!up||strcmp(class_name(object_class(up)),"UpgradeUnit")||!local_research_runtime::ascii(get(row,0x18),text,sizeof(text))||!labelled(text))continue;++candidates;
            Effect e;const char* why=nullptr;int id=*(int*)((char*)row+0x10);
            if(!resolve(row,e)||!shape(e.owner,e,why)){++refused;fprintf(f,"{\"phase\":\"refused\",\"faction\":%d,\"id\":%d,\"reason\":\"%s\"}\n",faction,id,why?why:e.reason);continue;}
            Roots roots;uint64_t before=hash(e.owner);void* copy=copy_type(e.owner,e,roots);
            bool pass=values(copy,e.owner,e,getters)&&purchase_supported(row)&&local_research_supported(row)&&plans[faction-1].add(e);if(pass)++predebit;
            pass=pass&&local_progress::buy_upgrade(progress,id,*(int*)((char*)row+0x20),*(int*)((char*)row+0x28))&&hash(e.owner)==before;ok&=pass;++accepted;
            fprintf(f,"{\"phase\":\"original_row\",\"faction\":%d,\"id\":%d,\"owner\":%d,\"kind\":\"unit\",\"pass\":%s}\n",faction,id,*(int*)((char*)e.owner+0x10),pass?"true":"false");}
        for(int i=0;i<plans[faction-1].count;++i){const auto& e=plans[faction-1].families[i];Roots roots;uint64_t before=hash(e.owner);
            void* a=copy_type(e.owner,e,roots),*b=copy_type(e.owner,e,roots);bool pass=values(a,e.owner,e,getters,f,faction)&&values(b,e.owner,e,getters)&&initialize(a,native);
            auto bad=e;bad.energy.capacity=INT16_MAX;pass=pass&&!copy_type(e.owner,bad,roots)&&hash(e.owner)==before;ok&=pass;++families;
            // A recovery-only row cannot enable an entirely disabled profile.
            auto capacity_only=e;capacity_only.energy={1,0,0};void* disabled=copy_type(e.owner,capacity_only,roots);
            if(disabled){void* states=get(disabled,0x18);for(int k=0;k<*(int*)((char*)states+0x18);++k){void* state=list_item(states,k);*(int16_t*)((char*)state+0x8c)=0;*(int16_t*)((char*)state+0x8e)=0;}}
            auto recovery_only=e;recovery_only.energy={0,1,0};pass=pass&&disabled&&!copy_type(disabled,recovery_only,roots)&&hash(e.owner)==before;ok&=pass;
            fprintf(f,"{\"phase\":\"cumulative\",\"faction\":%d,\"owner\":%d,\"pass\":%s}\n",faction,*(int*)((char*)e.owner+0x10),pass?"true":"false");}
        bool migrated=preplaced(faction,plans[faction-1],entities);ok&=migrated;
        fprintf(f,"{\"phase\":\"preplaced\",\"faction\":%d,\"spent_energy_preserved\":%s}\n",faction,migrated?"true":"false");
    }
    delete[] plans;progress=*restore.saved.data;local_faction::active=restore.faction;research_view::invalidate();bool unchanged=!memcmp(restore.saved.data,&progress,sizeof(progress));
    fprintf(f,"{\"summary\":true,\"pass\":%s,\"candidates\":%d,\"accepted\":%d,\"refused\":%d,\"active_faction_predebit\":%d,\"original_getters\":%d,\"cumulative_families\":%d,\"native_state_initializers\":%d,\"preplaced_units\":%d,\"player_state_unchanged\":%s,\"player_save_writes\":0,\"battle_runs\":0}\n",ok&&accepted==34&&families==3&&native>0&&entities>0&&unchanged?"true":"false",candidates,accepted,refused,predebit,getters,families,native,entities,unchanged?"true":"false");fclose(f);
}
}
