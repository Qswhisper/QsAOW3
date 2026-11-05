// Opt-in economy/production contract checks. No purchases, runs or save writes.
#pragma once
namespace building_economy_audit {
using namespace building_research;
static bool fresh(const Effect& e){return e.delta.flat_power||e.delta.power_mask||e.delta.training;}
static uint64_t units_hash(void* side){
    Roots r;uintptr_t h=0;void* values=local_dictionary_values(*(void**)((char*)side+0xf0),find_class("com.geargames.aow.entities","UnitType"),h);
    if(!values||!r.take(h))return 0;uint64_t result=1469598103934665603ull;
    for(size_t i=0;i<array_length(values);++i){void* u=((void**)((char*)values+32))[i];
        for(int j=0x18;j<0xd8;++j)result=(result^*((uint8_t*)u+j))*1099511628211ull;}
    return result;
}
static bool expected(void* u,const Plan& plan,int& time,void** original=nullptr){
    void* base=nullptr;int percent=0;
    for(int i=0;i<plan.size;++i){const auto& e=plan.families[i];if(!e.delta.training)continue;
        void* levels=*(void**)((char*)e.owner+0x18);
        for(int j=0;j<count(levels);++j){void* units=*(void**)((char*)list_item(levels,j)+0x20);
            for(int k=0;k<count(units);++k){void* candidate=list_item(units,k);
                if(generic_unit_research::identity(u,candidate)){
                    if(base&&(base!=candidate||percent!=e.delta.training))return false;base=candidate;percent=e.delta.training;}}}}
    if(original)*original=base;
    return base&&research_effects::training_time(*(int*)((char*)base+0x38),percent,time);
}
static void run(){
    if(access(DATA_ROOT "building-economy-audit.enabled",F_OK))return;
    FILE* out=fopen(DATA_ROOT "art3offline-building-economy.jsonl","w");if(!out)return;
    chmod(DATA_ROOT "art3offline-building-economy.jsonl",0644);
    bool passed=true;int accepted=0,power=0,training=0,matched_total=0;
    for(const char* source:{"CONFEDERATION","RESISTANCE"}){
        Plan sums;void* levels=generic_unit_research::levels(source);int families=0;
        for(int i=0;i<count(levels);++i){void* level=list_item(levels,i);Effect e;
            if(!resolve(level,e)||!fresh(e))continue;
            bool good=shape(e.owner,e);if(good){Roots r;auto hash=building_research_audit::hash(e.owner);
                good=building_research_audit::getters(copy_type(e.owner,e,r),e.owner,e)&&hash==building_research_audit::hash(e.owner)&&sums.add(e);}
            ++accepted;if(e.delta.training)++training;else ++power;passed=passed&&good;
            fprintf(out,"{\"record\":\"level\",\"source\":\"%s\",\"id\":%d,\"training\":%d,\"power_mask\":%u,\"flat_power\":%s,\"passed\":%s}\n",
                source,*(int*)((char*)level+0x10),e.delta.training,e.delta.power_mask,e.delta.flat_power?"true":"false",good?"true":"false");}
        preplaced_research_audit::Fixture x;Roots roots;bool good=x.init(source[0]=='C'?"fmc1m":"fmc2m");
        void* side=good?local_research_runtime::clone(x.side,roots):nullptr;int matched=0,changed=0;
        uint64_t before=good?units_hash(x.side):0;
        good=good&&side&&apply(side,*(void**)((char*)x.side+0xf8),sums,matched)&&apply_training(side,sums,changed);
        matched_total+=changed;uintptr_t h=0;
        void* values=good?local_dictionary_values(*(void**)((char*)side+0xf0),find_class("com.geargames.aow.entities","UnitType"),h):nullptr;
        good=good&&values&&roots.take(h);int checked=0;
        for(size_t i=0;good&&i<array_length(values);++i){void* u=((void**)((char*)values+32))[i];int time;void* original=nullptr;
            if(!expected(u,sums,time,&original))continue;++checked;
            good=original_stat_equals(u,"get_TimeTrain",4,time)&&original_stat_equals(u,"get_TimeTrainInit",4,time);
            Roots one;void* control=local_research_runtime::clone(u,one);if(!control){good=false;break;}
            *(int*)((char*)control+0x38)=*(int*)((char*)original+0x38);
            auto calculate=(int(*)(void*,void*,int,void*))(image_base+0x46e5c80);
            int native_before=calculate(side,control,0,nullptr),native_after=calculate(side,u,0,nullptr);
            good=good&&native_before>native_after&&native_after>0;
            fprintf(out,"{\"record\":\"training_time\",\"source\":\"%s\",\"unit\":%d,\"canonical\":%d,\"researched\":%d,\"engine_before\":%d,\"engine_after\":%d,\"passed\":%s}\n",
                source,*(int*)((char*)u+0x10),*(int*)((char*)original+0x38),time,native_before,native_after,good?"true":"false");}
        // Re-apply from the same canonical plan: time never compounds, and the
        // producer lists still point at the side's own upgraded prototypes.
        int again=0;good=good&&changed>0&&checked==changed&&apply_training(side,sums,again)&&again==changed&&before==units_hash(x.side);
        preplaced_research::Migration migration;
        good=good&&migration.prepare(x.battle,x.side,side)&&x.unchanged();
        passed=passed&&good;families=sums.size;
        fprintf(out,"{\"record\":\"source\",\"source\":\"%s\",\"families\":%d,\"matched_units\":%d,\"getters\":%d,\"canonical_unchanged\":%s,\"passed\":%s}\n",
            source,families,changed,checked,before==units_hash(x.side)?"true":"false",good?"true":"false");
    }
    fprintf(out,"{\"record\":\"summary\",\"passed\":%s,\"levels\":%d,\"power\":%d,\"training\":%d,\"matched_units\":%d,\"purchase_calls\":0,\"save_writes\":0,\"battle_runs\":0}\n",
        passed?"true":"false",accepted,power,training,matched_total);
    fclose(out);LOG("Building economy audit passed=%d levels=%d power=%d training=%d units=%d",passed,accepted,power,training,matched_total);
}
}
