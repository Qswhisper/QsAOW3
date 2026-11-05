// Detached original building/weapon prototypes and runtime references, no player grants.
#pragma once
namespace building_weapon_audit {
using namespace building_weapon_research;
struct Restore {
    local_progress::StateBuffer saved;int faction=local_faction::active;
    Restore(){if(saved.data)*saved.data=progress;}
    ~Restore(){if(saved.data)progress=*saved.data;local_faction::active=faction;}
};
static uint64_t hash(void* type){
    uint64_t h=1469598103934665603ull;void* levels=get(type,0x18);
    for(int i=0;i<count(levels);++i){void* level=list_item(levels,i);if(!level)return 0;
        for(int k=0x28;k<0xa8;++k)h=(h^((uint8_t*)level)[k])*1099511628211ull;
        void* weapons=get(level,0x18);for(int j=0;j<count(weapons);++j){void* w=list_item(weapons,j);if(!w)return 0;
            for(int k=0x10;k<0x110;++k)h=(h^((uint8_t*)w)[k])*1099511628211ull;}}
    return h;
}
static bool raw(void* copy,void* source,const Effect& e){
    if(!copy||copy==source||!building_research::identity(copy,source))return false;
    void* old=get(source,0x18),*current=get(copy,0x18),*canonical=get(e.owner,0x18);
    if(old==current||count(old)!=count(current))return false;
    for(int i=0;i<count(old);++i){void* a=list_item(old,i),*b=list_item(current,i);
        if(!a||!b||a==b)return false;uint8_t state[0x80];memcpy(state,(char*)b+0x28,sizeof(state));
        if(e.sight){int expected=*(int*)((char*)list_item(canonical,i)+0x40)+e.sight;
            if(!original_stat_equals(b,"get_Sight",4,expected)||!original_stat_equals(b,"get_SightInit",4,expected))return false;
            memcpy(state+0x40-0x28,(char*)a+0x40,8);}
        if(memcmp((char*)a+0x28,state,sizeof(state)))return false;
        void* x=get(a,0x18),*y=get(b,0x18),*z=get(list_item(canonical,i),0x18),*ex=nullptr;
        void* list=invoke_named(b,"get_Weapons",0,nullptr,&ex);if(ex||list!=y||(e.targets?x==y:x!=y)||count(x)!=count(y))return false;
        for(int j=0;j<count(x);++j){void* original=list_item(x,j),*w=list_item(y,j);int t=target(original,e);
            if(t<0){if(w!=original)return false;continue;}
            WeaponParameters expected;if(w==original||!values(list_item(z,j),e.delta[t],expected))return false;
            auto actual=read(w);if(memcmp(&actual,&expected,sizeof(actual)))return false;
            const char* names[]={"get_DamageLight","get_DamageMedium","get_DamageHeavy"};
            for(int k=0;k<3;++k)if(!original_stat_equals(w,names[k],4,expected.damage[k]))return false;
            if(!original_stat_equals(w,"get_Distance",4,expected.range)||!original_stat_equals(w,"get_ExplosionRadius",4,expected.explosion)||
                !original_stat_equals(w,"get_RoundLen",2,expected.reload)||!original_stat_equals(w,"get_ShotStart",2,expected.shot_start)||!original_stat_equals(w,"get_AccuracyStatic",2,expected.accuracy[0])||
                !original_stat_equals(w,"get_AccuracyWalk",2,expected.accuracy[1])||!original_stat_equals(w,"get_AccuracyDynamic",2,expected.accuracy[2]))return false;
            uint8_t now[0x100];memcpy(now,(char*)w+0x10,sizeof(now));
            for(int offset:{0x28,0x2c,0x30,0x4c,0x7c})memcpy(now+offset-0x10,(char*)original+offset,4);
            for(int offset:{0x60,0x66,0x8e,0x92,0x90})memcpy(now+offset-0x10,(char*)original+offset,2);
            if(memcmp(now,(char*)original+0x10,sizeof(now)))return false;
            for(int pi=0;pi<=i;++pi){void* px=get(list_item(old,pi),0x18),*py=get(list_item(current,pi),0x18);
                for(int pj=0;pj<(pi==i?j:count(px));++pj)if((original==list_item(px,pj))!=(w==list_item(py,pj)))return false;}
        }}return true;
}
static bool initialize(void* copy,const Effect& e,int& initializers){
    void* seen[160]={};int n=0;Roots roots;
    for(int i=0;i<count(get(copy,0x18));++i){void* list=get(list_item(get(copy,0x18),i),0x18);
        for(int j=0;j<count(list);++j){void* w=list_item(list,j);if(target(w,e)<0)continue;
            bool known=false;for(int k=0;k<n;++k)if(seen[k]==w)known=true;if(known)continue;if(n==160)return false;seen[n++]=w;
            int distance=*(int*)((char*)w+0x4c),padded=distance>1000?distance+50:distance*105/100;
            int radius=*(int*)((char*)w+0x7c),reload=*(int16_t*)((char*)w+0x66);void* ex=nullptr;
            invoke_named(w,"init",0,nullptr,&ex);
            if(ex||!*(bool*)((char*)w+0x14)||*(int*)((char*)w+0x4c)!=padded*padded||*(int*)((char*)w+0x50)!=padded||
                *(int*)((char*)w+0x54)!=padded||*(int*)((char*)w+0x7c)!=radius||*(int*)((char*)w+0x80)!=radius)return false;
            void* runtime=object_new(find_class("com.geargames.aow.entities","Weapon"));if(!roots.keep(runtime))return false;
            field_set(runtime,class_field_from_name(object_class(runtime),"we_type"),w);
            if(!original_stat_equals(runtime,"get_RoundLen",4,reload))return false;++initializers;
        }}return n>0;
}
static bool fixture(FILE* out,int faction,const Plan& plan,int& entities,int& links){
    preplaced_research_audit::Fixture x;bool ok=x.init(faction==1?"fmc1m":"fmc2m");Roots roots;uintptr_t handle=0;
    void* old=ok?get(x.side,0xf8):nullptr,*prepared=ok?local_research_runtime::clone(x.side,roots):nullptr;
    void* types=old?local_dictionary_values(old,find_class("com.geargames.aow.entities","BuildingType"),handle):nullptr;
    ok=ok&&prepared&&types&&roots.take(handle);int slot=0;
    for(int i=0;ok&&i<plan.size;++i){void* type=nullptr;for(size_t j=0;j<array_length(types);++j){void* item=((void**)((char*)types+32))[j];
            if(building_research::identity(item,plan.families[i].owner)){if(type){ok=false;break;}type=item;}}
        if(!type){ok=false;break;}
        for(int state=0;ok&&state<count(get(type,0x18));++state)for(int damage:{0,7,-1})ok=ok&&x.spawn(type,state,damage,true,slot++);}
    int matched=0;bool applied=ok&&apply(prepared,old,plan,matched);preplaced_research::Migration migration;
    bool staged=applied&&x.unchanged()&&migration.prepare(x.battle,x.side,prepared)&&x.unchanged();
    if(staged){field_set(x.side,class_field_from_name(object_class(x.side),"$Gg"),get(prepared,0xf8));migration.commit();}
    int weapon_links=0;bool migrated=staged&&x.verify(weapon_links);ok=ok&&applied&&staged&&migrated&&matched==5&&x.count==15&&weapon_links==18;
    entities+=x.count;links+=weapon_links;
    fprintf(out,"{\"phase\":\"preplaced_original_buildings\",\"faction\":%d,\"matched\":%d,\"entities\":%d,\"weapon_links\":%d,\"staged_unchanged\":%s,\"pass\":%s}\n",faction,matched,x.count,weapon_links,staged?"true":"false",ok?"true":"false");return ok;
}
static bool saved_integration(FILE* out,int faction,int& cold_count){
    local_progress::StateBuffer state,loaded;if(!state.data||!loaded.data)return false;*state.data=local_progress::seed(100000000,0,0,10000);
    void* rows=generic_unit_research::levels(faction==1?"CONFEDERATION":"RESISTANCE");int owners[10]={},n=0,first=0;
    bool ok=true;for(int i=0;i<count(rows);++i){void* row=list_item(rows,i);Effect e;if(!resolve(row,e)||e.sight||!shape(e.owner,e))continue;
        int owner=*(int*)((char*)e.owner+0x10);bool known=false;for(int k=0;k<n;++k)if(owners[k]==owner)known=true;if(known)continue;
        if(n==10){ok=false;break;}owners[n++]=owner;int id=*(int*)((char*)row+0x10);if(!first)first=id;
        ok=ok&&local_progress::buy_upgrade(*state.data,id,*(int*)((char*)row+0x20),*(int*)((char*)row+0x28));}
    ok=ok&&n==5&&local_progress::buy_upgrade(*state.data,faction==1?3075:13075,400,1);progress=*state.data;
    // A preplaced Fixture deliberately reserves1024 unit-array slots. It is not
    // a fresh-input fixture even when all entries are null. Use a real retained
    // fresh route, without clearing arrays/entities or relaxing the guard.
    const LocalScenario* route=nullptr;for(const auto& candidate:local_scenarios)
        if(candidate.faction==faction&&candidate.rebase_growth){route=&candidate;break;}
    Roots input_roots;void* input=route?read_scenario_message(route->file,5040,"@building-weapon-saved-integration"):nullptr;
    void* battle=get(input,0x20),*side=battle?local_dictionary_item(get(battle,0x178),1):nullptr;
    bool fresh=side&&local_research_runtime::empty_player_units(battle,side)&&local_research_runtime::empty_player_buildings(battle,side);
    ok=ok&&fresh&&apply_local_battle_research(battle,1,true);
    Plan plan;ok=ok&&collect(plan)&&plan.size==5;
    Roots roots;uintptr_t handle=0;void* values=ok?local_dictionary_values(get(side,0xf8),find_class("com.geargames.aow.entities","BuildingType"),handle):nullptr;
    ok=ok&&values&&roots.take(handle);int checked=0;
    for(int i=0;ok&&i<plan.size;++i)for(size_t j=0;j<array_length(values);++j){void* type=((void**)((char*)values+32))[j];if(!building_research::identity(type,plan.families[i].owner))continue;
        // Compare prepared fields to canonical+saved deltas; preserve scenario identities.
        for(int l=0;l<count(get(type,0x18));++l){void* list=get(list_item(get(type,0x18),l),0x18),*base=get(list_item(get(plan.families[i].owner,0x18),l),0x18);
            for(int w=0;w<count(list);++w){void* weapon=list_item(list,w);int t=target(weapon,plan.families[i]);if(t<0)continue;
                WeaponParameters expected;if(!building_weapon_research::values(list_item(base,w),plan.families[i].delta[t],expected)){ok=false;break;}
                auto actual=read(weapon);ok=ok&&!memcmp(&actual,&expected,sizeof(actual));++checked;}}
    }
    ok=ok&&checked==5&&*(int8_t*)((char*)side+0x1a9)==21;
    char path[512];snprintf(path,sizeof(path),DATA_ROOT "building-weapon-private-%d-f%d",getpid(),faction);
    bool cold=ok&&mkdir(path,0700)==0&&local_progress::write(path,*state.data)&&local_progress::load(path,*loaded.data)==1&&
        !memcmp(state.data,loaded.data,sizeof(progress));int wallet=loaded.data->credits;
    cold=cold&&!local_progress::buy_upgrade(*loaded.data,first,1,1)&&loaded.data->credits==wallet;if(cold)++cold_count;
    ok=ok&&cold;fprintf(out,"{\"phase\":\"saved_outer_preparation\",\"faction\":%d,\"chapter\":%d,\"fresh_original_input\":%s,\"building_families\":%d,\"weapons\":%d,\"hq_capacity\":21,\"private_cold_receipt\":%s,\"pass\":%s}\n",faction,route?route->chapter:-1,fresh?"true":"false",n,checked,cold?"true":"false",ok?"true":"false");return ok;
}
static __attribute__((noinline)) void run(){
    if(!progress_ready||access(DATA_ROOT "building-weapon-audit.request",F_OK))return;unlink(DATA_ROOT "building-weapon-audit.request");
    Restore restore;if(!restore.saved.data)return;FILE* out=fopen(DATA_ROOT "building-weapon-audit.jsonl","w");if(!out)return;chmod(DATA_ROOT "building-weapon-audit.jsonl",0644);
    bool ok=true;int rows_count=0,initializers=0,preflight=0,families=0,entities=0,links=0,cold=0;
    for(int faction:{1,2}){local_faction::active=faction;progress=local_progress::seed(100000000,0,0,10000);Plan plan;
        void* rows=generic_unit_research::levels(faction==1?"CONFEDERATION":"RESISTANCE");
        for(int i=0;i<count(rows);++i){void* row=list_item(rows,i);Effect e;if(!resolve(row,e)||e.sight)continue;
            uint64_t before=hash(e.owner);Roots roots;bool pass=shape(e.owner,e)&&purchase_supported(row)&&local_research_supported(row);
            if(pass)++preflight;void* copy=pass?copy_type(e.owner,e,roots):nullptr;
            pass=pass&&raw(copy,e.owner,e)&&initialize(copy,e,initializers)&&hash(e.owner)==before&&plan.add(e);ok=ok&&pass;++rows_count;
            fprintf(out,"{\"phase\":\"original_weapon_row\",\"faction\":%d,\"id\":%d,\"owner\":%d,\"slot\":%d,\"weapon\":%d,\"type\":%d,\"pass\":%s}\n",faction,*(int*)((char*)row+0x10),*(int*)((char*)e.owner+0x10),*(int8_t*)((char*)get(row,0x48)+0x21),e.keys[0].name,e.keys[0].type,pass?"true":"false");
        }
        for(int i=0;i<plan.size;++i){const auto& e=plan.families[i];Roots roots;uint64_t before=hash(e.owner);
            void* a=copy_type(e.owner,e,roots),*b=a?copy_type(a,e,roots):nullptr;
            bool pass=raw(a,e.owner,e)&&b&&hash(a)==hash(b)&&hash(e.owner)==before;
            auto bad=e;bad.delta[0].reload=INT16_MAX;pass=pass&&!copy_type(e.owner,bad,roots)&&hash(e.owner)==before;
            ok=ok&&pass;++families;fprintf(out,"{\"phase\":\"cumulative_family\",\"faction\":%d,\"owner\":%d,\"targets\":%d,\"pass\":%s}\n",faction,*(int*)((char*)e.owner+0x10),e.targets,pass?"true":"false");}
        ok=fixture(out,faction,plan,entities,links)&&ok;ok=saved_integration(out,faction,cold)&&ok;
    }
    progress=*restore.saved.data;local_faction::active=restore.faction;bool unchanged=!memcmp(&progress,restore.saved.data,sizeof(progress));
    ok=ok&&unchanged&&rows_count==291&&preflight==291&&families==10&&entities==30&&links==36&&cold==2;
    fprintf(out,"{\"summary\":true,\"pass\":%s,\"original_rows\":%d,\"preflight\":%d,\"native_initializers\":%d,\"families\":%d,\"preplaced_entities\":%d,\"weapon_links\":%d,\"private_cold_receipts\":%d,\"player_state_unchanged\":%s,\"player_save_writes\":0,\"battle_runs\":0}\n",ok?"true":"false",rows_count,preflight,initializers,families,entities,links,cold,unchanged?"true":"false");fclose(out);
}
}
