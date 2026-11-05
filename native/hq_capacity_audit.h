// Detached original cap calculator and private growth receipts, never player grants.
#pragma once
namespace hq_capacity_audit {
using local_research_runtime::Roots;
struct Restore {
    local_progress::StateBuffer saved;int faction=local_faction::active;
    Restore(){if(saved.data)*saved.data=progress;}
    ~Restore(){if(saved.data)progress=*saved.data;local_faction::active=faction;}
};
static bool native_limits(FILE* f,int faction,const research_effects::Delta& expected,int& cases){
    preplaced_research_audit::Fixture x;bool ok=x.init(faction==1?"fmc1m":"fmc2m");
    Roots roots;void* old_fraction=ok?*(void**)((char*)x.side+0xd8):nullptr;
    uint8_t fraction_bytes[0x110]={};if(old_fraction)memcpy(fraction_bytes,old_fraction,sizeof(fraction_bytes));
    void* enemy=ok?local_dictionary_item(*(void**)((char*)x.battle+0x178),2):nullptr;
    uint8_t enemy_bytes[0x464]={};if(enemy)memcpy(enemy_bytes,enemy,sizeof(enemy_bytes));
    ok=ok&&old_fraction&&enemy&&apply_local_battle_research(x.battle,1);
    void* hq=ok?local_research_runtime::headquarters(x.side):nullptr;
    void* baseline=nullptr;research_effects::Delta sum;
    ok=ok&&hq&&local_research_runtime::gather(sum,baseline)&&sum.units_base==expected.units_base&&sum.units_level==expected.units_level;
    void* canonical=baseline?*(void**)((char*)baseline+0x20):nullptr;int b=0,i=0;
    ok=ok&&canonical&&research_effects::capacity_values(*(int8_t*)((char*)canonical+0x38),*(int8_t*)((char*)canonical+0x39),sum,b,i)&&
        original_stat_equals(x.side,"$iW",1,b)&&original_stat_equals(x.side,"$jW",1,i);
    // Original input, private HQ, original per-base cap array and no-op CommandAdd.
    // This executes the real Hf recalculator; it does not write the cap result.
    void* list=ok?x.field_list(object_class(x.side),"$yG",roots):nullptr;
    void* caps=ok?array_new(find_class("System","Int32"),8):nullptr;
    void* command=ok?object_new(find_class("com.geargames.aow.entities.battle","CommandAdd")):nullptr,*ex=nullptr;
    if(!list||!roots.keep(caps)||!roots.keep(command))ok=false;
    if(ok){invoke_named(command,".ctor",0,nullptr,&ex);ok=!ex&&x.spawn(hq,0,0,true,0);}
    void* building=ok?x.samples[0].entity:nullptr;
    if(ok){*(int8_t*)((char*)building+0x47)=1;*(int8_t*)((char*)building+0x21)=1;
        field_set(x.side,class_field_from_name(object_class(x.side),"$yG"),list);
        field_set(x.side,class_field_from_name(object_class(x.side),"$Yh"),caps);
        field_set(x.battle,class_field_from_name(object_class(x.battle),"$EB"),command);
        *(int*)((char*)x.side+0x32c)=0;ok=x.add(list,building);}
    const void* calculate=class_method(find_class("com.geargames.aow.entities.battle","SideAct"),"$Hf",2);
    for(int tier=0;ok&&tier<5;++tier){
        *(int8_t*)((char*)building+0x60)=tier;void* args[]={x.battle,x.side};ex=nullptr;
        if(calculate)runtime_invoke(calculate,nullptr,args,&ex);
        int actual=*(int*)((char*)x.side+0x34c),wanted=b+tier*i;
        bool good=calculate&&!ex&&actual==wanted&&original_stat_equals(x.side,"$kW",4,wanted);
        fprintf(f,"{\"phase\":\"native_limit\",\"faction\":%d,\"base_delta\":%d,\"increment_delta\":%d,\"tier\":%d,\"base\":%d,\"increment\":%d,\"actual\":%d,\"expected\":%d,\"pass\":%s,\"exception\":\"%s\"}\n",
            faction,sum.units_base,sum.units_level,tier,b,i,actual,wanted,good?"true":"false",ex?class_name(object_class(ex)):"none");
        ok=good;if(good)++cases;
    }
    bool isolated=old_fraction&&!memcmp(old_fraction,fraction_bytes,sizeof(fraction_bytes))&&enemy&&!memcmp(enemy,enemy_bytes,sizeof(enemy_bytes));
    return ok&&isolated;
}
static void run(){
    if(!progress_ready||access(DATA_ROOT "hq-capacity-audit.request",F_OK))return;unlink(DATA_ROOT "hq-capacity-audit.request");
    Restore restore;local_progress::StateBuffer fixture,cold;if(!restore.saved.data||!fixture.data||!cold.data)return;
    FILE* f=fopen(DATA_ROOT "hq-capacity-audit.jsonl","w");if(!f)return;chmod(DATA_ROOT "hq-capacity-audit.jsonl",0644);
    bool ok=true;int tested=0,native=0,cold_count=0,preflight=0;
    for(int faction:{1,2}){
        local_faction::active=faction;*fixture.data=local_progress::seed(100000000,0,0,10000);progress=*fixture.data;
        void* rows=generic_unit_research::levels(faction==1?"CONFEDERATION":"RESISTANCE");
        int n=rows?*(int*)((char*)rows+0x18):0,selected=0;research_effects::Delta total;
        for(int j=0;ok&&j<n;++j){void* row=list_item(rows,j),*owner=nullptr;research_effects::Delta d,next;
            if(!local_research_runtime::level_delta(row,d,owner)||(!d.units_base&&!d.units_level))continue;
            int id=*(int*)((char*)row+0x10),cost=*(int*)((char*)row+0x20),xp=*(int*)((char*)row+0x28);
            bool allowed=local_research_supported(row);if(allowed)++preflight;
            int before=fixture.data->credits;
            bool bought=allowed&&local_progress::buy_upgrade(*fixture.data,id,cost,xp)&&fixture.data->credits==before-cost&&
                research_effects::add(total,d,next);
            if(bought){total=next;progress=*fixture.data;}
            fprintf(f,"{\"phase\":\"private_purchase\",\"faction\":%d,\"id\":%d,\"cost\":%d,\"base_delta\":%d,\"increment_delta\":%d,\"pass\":%s}\n",faction,id,cost,total.units_base,total.units_level,bought?"true":"false");
            ok=bought;++tested;++selected;
            if(selected==1)ok=ok&&native_limits(f,faction,total,native);
        }
        ok=ok&&selected==12&&total.units_base==20&&total.units_level==10&&native_limits(f,faction,total,native);
        char path[512];snprintf(path,sizeof(path),DATA_ROOT "hq-capacity-private-f%d",faction);
        bool directory=mkdir(path,0700)==0;bool saved=directory&&local_progress::write(path,*fixture.data)&&local_progress::load(path,*cold.data)==1&&
            !memcmp(fixture.data,cold.data,sizeof(progress));
        int wallet=cold.data->credits;int first=faction==1?3075:13075;
        bool duplicate=saved&&!local_progress::buy_upgrade(*cold.data,first,400,1)&&cold.data->credits==wallet;
        if(duplicate)++cold_count;ok=ok&&duplicate;
    }
    progress=*restore.saved.data;local_faction::active=restore.faction;
    bool unchanged=!memcmp(&progress,restore.saved.data,sizeof(progress));ok=ok&&unchanged&&tested==24&&native==20&&preflight==24&&cold_count==2;
    fprintf(f,"{\"summary\":true,\"pass\":%s,\"original_rows\":%d,\"preflight_cases\":%d,\"original_native_limits\":%d,\"private_cold_receipts\":%d,\"player_state_unchanged\":%s,\"player_save_writes\":0,\"actual_player_purchases\":0,\"battle_runs\":0,\"growth_version\":%u,\"growth_size\":%zu}\n",
        ok?"true":"false",tested,preflight,native,cold_count,unchanged?"true":"false",local_progress::kVersion,sizeof(progress));fclose(f);
}
}
