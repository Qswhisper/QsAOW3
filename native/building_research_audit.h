// Opt-in original-object checks for the new building mechanisms only.
#pragma once
namespace building_research_audit {
using namespace building_research;
static bool getters(void* copy,void* source,const Effect& e) {
    if(!copy||copy==source||!identity(copy,source))return false;
    void* old=*(void**)((char*)source+0x18);void* actual=*(void**)((char*)copy+0x18);
    void* baseline=*(void**)((char*)e.owner+0x18);
    if(count(old)!=count(actual)||old==actual)return false;
    for(int i=0;i<count(old);++i){void* s=list_item(old,i);void* c=list_item(actual,i);void* b=list_item(baseline,i);
        int hp=(e.delta.flat_hp||e.delta.hp_mask)?*(int*)((char*)b+0x34)+e.delta.hp[i]:*(int*)((char*)s+0x34);
        int armor=e.delta.armor?*(int16_t*)((char*)b+0x38)+e.delta.armor:*(int16_t*)((char*)s+0x38);
        if(!c||c==s||!original_stat_equals(c,"get_LifeInt",4,hp)||!original_stat_equals(c,"get_Life",2,hp)||
           !original_stat_equals(c,"get_Armor",2,armor)||*(void**)((char*)s+0x18)!=*(void**)((char*)c+0x18)||
           *(void**)((char*)s+0x20)!=*(void**)((char*)c+0x20)||
           memcmp((char*)s+0x3a,(char*)c+0x3a,0x26)||memcmp((char*)s+0x64,(char*)c+0x64,0x44))return false;
        for(int offset:{0x60,0x62}){int value=e.delta.power[i]?*(int16_t*)((char*)b+0x60)-e.delta.power[i]:*(int16_t*)((char*)s+offset);
            if(*(int16_t*)((char*)c+offset)!=value)return false;}}
    return true;
}
static uint64_t hash(void* type) {
    uint64_t h=1469598103934665603ull;void* levels=type?*(void**)((char*)type+0x18):nullptr;
    for(int i=0;i<count(levels);++i){void* level=list_item(levels,i);if(!level)return 0;
        for(int j=0x28;j<0xa8;++j)h=(h^*((uint8_t*)level+j))*1099511628211ull;}
    return h;
}
static bool parser_checks() {
    BuildingStats a,b,c;bool ok=research_effects::building_parse("{#20000<192>}",a)&&
        research_effects::building_parse("{#20001<4>}",b)&&research_effects::building_add(a,b,c)&&c.hp[0]==192&&c.armor==4;
    for(const char* bad:{"","{","{#","{#20000<1","{#20000<1>","{#20000<0>}","{#20000<-1>}",
                        "{#20000<2147483648>}","{#20000<1>}\n","{#20000<1>}{#20001<2>}",
                        "{#20075<1>}","{#20000<1>}?{#20075<1>}","{#20001<1>}?{#20001<2>}",
                        "{#20000<1>}?{#20016<2>}"}) {
        auto before=c;ok=ok&&!research_effects::building_parse(bad,c)&&research_effects::building_equal(before,c);}
    return ok;
}
static void run() {
    if(access(DATA_ROOT "building-research-audit.enabled",F_OK))return;
    FILE* out=fopen(DATA_ROOT "art3offline-building-research.jsonl","w");if(!out)return;
    chmod(DATA_ROOT "art3offline-building-research.jsonl",0644);
    bool ok=parser_checks();int accepted=0,objects=0,failures=0,building_levels=0,newly_supported=0;
    for(const char* source:{"CONFEDERATION","RESISTANCE"}) {
        void* levels=generic_unit_research::levels(source);int n=count(levels);Plan sums;int successful=0;
        for(int i=0;i<n;++i){void* level=list_item(levels,i);void* upgrade=level?*(void**)((char*)level+0x48):nullptr;
            if(!upgrade||strcmp(class_name(object_class(upgrade)),"UpgradeBuilding"))continue;++building_levels;
            Effect e;bool supported=resolve(level,e)&&shape(e.owner,e);
            research_effects::Delta old_delta;void* old_owner=nullptr;
            bool old_supported=local_research_runtime::level_delta(level,old_delta,old_owner);
            bool exact=true,unchanged=true;
            if(supported){++accepted;++objects;++successful;if(!old_supported)++newly_supported;Roots r;uint64_t before=hash(e.owner);
                void* copy=copy_type(e.owner,e,r);exact=getters(copy,e.owner,e);unchanged=before==hash(e.owner);
                ok=ok&&exact&&unchanged;if(!exact||!unchanged)++failures;
                if(!sums.add(e))ok=false;}
            fprintf(out,"{\"record\":\"level\",\"source\":\"%s\",\"id\":%d,\"owner\":%d,\"supported\":%s,\"getters\":%s,\"canonical_unchanged\":%s,\"description\":",
                source,*(int*)((char*)level+0x10),e.owner?*(int*)((char*)e.owner+0x10):-1,supported?"true":"false",exact?"true":"false",unchanged?"true":"false");
            research_batch_audit::json_string(out,*(void**)((char*)level+0x18));fputs("}\n",out);
        }
        int cumulative=0,range_refused=0;
        for(int i=0;i<sums.size;++i){const auto& e=sums.families[i];Roots r;
            if(!shape(e.owner,e)){++range_refused;continue;}
            bool exact=getters(copy_type(e.owner,e,r),e.owner,e);ok=ok&&exact;if(exact)++cumulative;}
        fprintf(out,"{\"record\":\"source\",\"source\":\"%s\",\"supported\":%d,\"families\":%d,\"cumulative_families\":%d,\"cumulative_range_refused\":%d}\n",source,successful,sums.size,cumulative,range_refused);
    }
    int entity_count=0;
    for(const char* name:{"fmc1m","fmc2m"}) {
        preplaced_research_audit::Fixture x;bool good=x.init(name);Plan plan;Roots r;uintptr_t handle=0;
        // This fixture uses real original types and synthetic injured/dead
        // entities, not a campaign result or a persisted research purchase.
        x.building_array=array_new(find_class("com.geargames.aow.entities","Building"),256);
        good=good&&x.roots.keep(x.building_array);
        if(good)field_set(x.side,class_field_from_name(object_class(x.side),"$Lg"),x.building_array);
        int faction=name[3]-'0';void* levels=generic_unit_research::levels(faction==1?"CONFEDERATION":"RESISTANCE");
        void* dict=x.side?*(void**)((char*)x.side+0xf8):nullptr;
        void* types=good?local_dictionary_values(dict,find_class("com.geargames.aow.entities","BuildingType"),handle):nullptr;
        good=good&&types&&r.take(handle);int slot=0,families=0;
        for(size_t j=0;good&&j<array_length(types);++j){void* type=((void**)((char*)types+32))[j];Effect combined;bool found_hp=false,found_armor=false;
            for(int i=0;i<count(levels);++i){Effect e;if(!resolve(list_item(levels,i),e)||!identity(type,e.owner)||!shape(type,e))continue;
                // One representative level per distinct mechanism/family.
                if((e.delta.flat_hp||e.delta.hp_mask)&&!found_hp){if(!plan.add(e)){good=false;break;}found_hp=true;}
                if(e.delta.armor&&!found_armor){if(!plan.add(e)){good=false;break;}found_armor=true;}}
            if(!found_hp&&!found_armor)continue;++families;
            for(int state=0;state<count(*(void**)((char*)type+0x18));++state)
                for(int damage:{0,7,-1})if(!x.spawn(type,state,damage,true,slot++)){good=false;break;}
        }
        void* prepared=good?local_research_runtime::clone(x.side,r):nullptr;int matched=0;
        good=good&&prepared&&apply(prepared,dict,plan,matched);
        bool staged=x.unchanged();preplaced_research::Migration migration;
        good=good&&staged&&migration.prepare(x.battle,x.side,prepared);
        if(good){field_set(x.side,class_field_from_name(object_class(x.side),"$Gg"),*(void**)((char*)prepared+0xf8));migration.commit();}
        int weapons=0;bool migrated=good&&x.verify(weapons);
        // Apply the same immutable dictionary a second time: no accumulation.
        bool repeated=good&&apply(prepared,dict,plan,matched);
        if(repeated)for(size_t j=0;j<array_length(types);++j){void* type=((void**)((char*)types+32))[j];
            int id=*(int*)((char*)type+0x10);repeated=repeated&&hash(local_dictionary_item(*(void**)((char*)prepared+0xf8),id))==hash(local_dictionary_item(*(void**)((char*)x.side+0xf8),id));}
        good=good&&migrated&&repeated;ok=ok&&good;entity_count+=x.count;
        fprintf(out,"{\"record\":\"migration\",\"source\":\"%s\",\"families\":%d,\"entities\":%d,\"staged_unchanged\":%s,\"injury_death_references\":%s,\"repeat_from_baseline\":%s,\"passed\":%s}\n",name,families,x.count,staged?"true":"false",migrated?"true":"false",repeated?"true":"false",good?"true":"false");
    }
    fprintf(out,"{\"record\":\"summary\",\"passed\":%s,\"building_levels\":%d,\"supported\":%d,\"newly_supported\":%d,\"original_objects\":%d,\"failures\":%d,\"entities\":%d,\"purchase_calls\":0,\"save_writes\":0,\"battle_runs\":0}\n",ok?"true":"false",building_levels,accepted,newly_supported,objects,failures,entity_count);
    fclose(out);LOG("Building research audit passed=%d supported=%d objects=%d failures=%d entities=%d",ok,accepted,objects,failures,entity_count);
}
}
