// Original-engine startup fixtures, never a campaign run or save transaction.
#pragma once
namespace preplaced_research_audit {
using namespace preplaced_research;
struct Sample {void* entity=nullptr;void* old_type=nullptr;void* old_state=nullptr;bool building=false;int life=0,damage=0;unsigned char bytes[0x300]={};};
struct Fixture {
    Roots roots;Sample* samples=(Sample*)calloc(4096,sizeof(Sample));int count=0;
    void* battle=nullptr;void* side=nullptr;void* unit_array=nullptr;void* building_array=nullptr;
    void (*barrier)(void*,void**,void*)=nullptr;
    ~Fixture(){free(samples);}
    bool array_put(void* array,int index,void* value){if(!barrier||!array||index<0||(size_t)index>=array_length(array))return false;
        barrier(array,((void**)((char*)array+32))+index,value);return true;}
    void* field_list(void* klass,const char* name,Roots& r){void* f=class_field_from_name(klass,name);void* c=f?class_from_type(field_type(f)):nullptr;
        void* list=c?object_new(c):nullptr;if(!r.keep(list))return nullptr;void* ex=nullptr;invoke_named(list,".ctor",0,nullptr,&ex);return ex?nullptr:list;}
    bool add(void* list,void* value){void* ex=nullptr;void* args[]={value};invoke_named(list,"Add",1,args,&ex);return !ex;}
    bool init(const char* name){char path[512];snprintf(path,sizeof(path),DATA_ROOT "offline-seeds/%s.ggs",name);
        void* config=campaign_seeds::decode(path,roots);battle=pointer(config,0x10);side=local_dictionary_item(pointer(battle,0x178),1);
        barrier=(decltype(barrier))dlsym(il2cpp,"il2cpp_gc_wbarrier_set_field");
        if(!side||!samples||!barrier)return false;
        unit_array=array_new(find_class("com.geargames.aow.entities","Unit"),1024);
        building_array=array_new(find_class("com.geargames.aow.entities","Building"),32);
        if(!roots.keep(unit_array)||!roots.keep(building_array))return false;
        field_set(side,class_field_from_name(object_class(side),"$lg"),unit_array);
        field_set(side,class_field_from_name(object_class(side),"$Lg"),building_array);
        return pointer(side,0x140)==unit_array&&pointer(side,0x148)==building_array;
    }
    bool spawn(void* type,int state,int damage,bool building,int slot){
        if(count==4096)return false;Roots r;
        void* klass=find_class("com.geargames.aow.entities",building?"Building":"Unit");void* e=object_new(klass);
        if(!r.keep(e))return false;
        field_set(e,class_field_from_name(find_class("com.geargames.aow.entities","Dynamic"),"side"),side);
        *(int*)((char*)e+0x10)=1000+count;*(int8_t*)((char*)e+0x20)=1;
        field_set(e,class_field_from_name(klass,building?"bu_type":"un_type"),type);
        void* level=list_item(pointer(type,0x18),state);if(!level)return false;
        int max=*(int*)((char*)(building?level:type)+0x34);int life=damage<0?0:max-damage;if(life<0)return false;
        if(building){*(int8_t*)((char*)e+0x60)=state;*(int16_t*)((char*)e+0x58)=life;*(int*)((char*)e+0x5c)=life;
            *(int16_t*)((char*)e+0x40)=12;*(int16_t*)((char*)e+0x42)=14;*(int*)((char*)e+0xc0)=77;}
        else {*(int8_t*)((char*)e+0x6b)=state;*(int16_t*)((char*)e+0x182)=life;*(int*)((char*)e+0x184)=life;
            *(int8_t*)((char*)e+0xba)=*(int8_t*)((char*)level+0x58);
            *(int*)((char*)e+0xbc)=*(int*)((char*)level+0x4c)+3;
            *(int16_t*)((char*)e+0x48)=12;*(int16_t*)((char*)e+0x4a)=14;*(int16_t*)((char*)e+0x70)=22;*(int8_t*)((char*)e+0x6a)=1;}
        void* runtime=field_list(klass,"weapons",r);if(!runtime)return false;
        void* source=pointer(level,0x18);int n=list_size(source);if(n<0||n>64)return false;
        for(int i=0;i<n;++i){Roots wr;void* w=object_new(find_class("com.geargames.aow.entities","Weapon"));if(!wr.keep(w))return false;
            field_set(w,class_field_from_name(object_class(w),"we_type"),list_item(source,i));
            *(int16_t*)((char*)w+0x1c)=19;*(int8_t*)((char*)w+0x35)=2;*(int8_t*)((char*)w+0x36)=i;
            if(!add(runtime,w))return false;}
        field_set(e,class_field_from_name(klass,"weapons"),runtime);
        if(!array_put(building?building_array:unit_array,slot,e))return false;
        Sample& sample=samples[count++];sample.entity=e;sample.old_type=type;sample.old_state=level;sample.building=building;sample.life=life;sample.damage=damage;
        memcpy(sample.bytes,e,building?0x168:0x2e8);return true;
    }
    bool unchanged(){for(int i=0;i<count;++i)if(memcmp(samples[i].bytes,samples[i].entity,samples[i].building?0x168:0x2e8))return false;return true;}
    bool verify(int& weapon_count){
        bool ok=true;weapon_count=0;
        for(int i=0;i<count;++i){auto& s=samples[i];void* e=s.entity;
            int type_offset=s.building?0x38:0x40;void* type=pointer(e,type_offset);
            void* dict=pointer(side,s.building?0xf8:0xf0);
            int state=*(int8_t*)((char*)e+(s.building?0x60:0x6b));void* level=list_item(pointer(type,0x18),state);
            int oldmax=*(int*)((char*)(s.building?s.old_state:s.old_type)+0x34),max=*(int*)((char*)(s.building?level:type)+0x34);
            int expected=s.life>0?s.life+max-oldmax:s.life;
            int offset=s.building?0x5c:0x184,short_offset=s.building?0x58:0x182;
            ok=ok&&local_dictionary_item(dict,*(int*)((char*)type+0x10))==type&&
                original_stat_equals(e,"get_LifeInt",4,expected);
            // Compare all bytes except the explicit migration fields. This
            // covers position, state, destinations, queue timing and identity.
            unsigned char now[0x300];memcpy(now,e,s.building?0x168:0x2e8);
            memcpy(now+type_offset,s.bytes+type_offset,8);memcpy(now+offset,s.bytes+offset,4);memcpy(now+short_offset,s.bytes+short_offset,2);
            if(s.building){for(int offset:{0xa0,0xa8,0x150})memcpy(now+offset,s.bytes+offset,8);}
            if(!s.building){memcpy(now+0xba,s.bytes+0xba,1);memcpy(now+0xbc,s.bytes+0xbc,4);
                int speed=s.bytes[0xba]+*(int8_t*)((char*)level+0x58)-*(int8_t*)((char*)s.old_state+0x58);
                int sight=*(int*)(s.bytes+0xbc)+*(int*)((char*)level+0x4c)-*(int*)((char*)s.old_state+0x4c);
                ok=ok&&*(int8_t*)((char*)e+0xba)==speed&&*(int*)((char*)e+0xbc)==sight;}
            ok=ok&&!memcmp(now,s.bytes,s.building?0x168:0x2e8)&&*(int16_t*)((char*)e+short_offset)==expected;
            void* runtime=pointer(e,0x28);void* types=pointer(level,0x18);
            for(int j=0;j<list_size(runtime);++j){void* w=list_item(runtime,j);++weapon_count;
                ok=ok&&pointer(w,0x10)==list_item(types,j)&&*(int16_t*)((char*)w+0x1c)==19&&*(int8_t*)((char*)w+0x35)==2;}
        }
        return ok;
    }
};
static void run(){
    if(access(DATA_ROOT "preplaced-research-audit.enabled",F_OK))return;
    FILE* f=fopen(DATA_ROOT "art3offline-preplaced-research.jsonl","w");if(!f)return;chmod(DATA_ROOT "art3offline-preplaced-research.jsonl",0644);
    bool ok=true;int total=0;
    for(const char* name:{"fmc1m","fmc2m"}){
        Fixture x;bool good=x.init(name);generic_unit_research::Plan plan;int slot=0,states=0;
        uint64_t baseline_hash=good?unit_dictionary_health_hash(x.side):0;
        Roots r;uintptr_t h=0;void* values=good?local_dictionary_values(pointer(x.side,0xf0),find_class("com.geargames.aow.entities","UnitType"),h):nullptr;
        good=good&&values&&r.take(h);int n=values?(int)array_length(values):0;
        for(int i=0;good&&i<n;++i){void* type=((void**)((char*)values+32))[i];generic_unit_research::Effect e;e.owner=type;e.delta.hp=17;e.delta.armor=2;e.delta.sight=3;
            void* list=pointer(type,0x18);int count=list_size(list);if(count<1||count>32){good=false;break;}
            bool mobile=true;for(int j=0;j<count;++j)mobile=mobile&&*(int8_t*)((char*)list_item(list,j)+0x58)>0;
            e.delta.speed=mobile?1:0;
            void* weapons=pointer(list_item(list,0),0x18);if(list_size(weapons)){
                void* w=list_item(weapons,0);e.target_count=(*(int16_t*)((char*)w+0x20)>0&&*(int16_t*)((char*)w+0x24)>0)?1:0;e.targets[0]={*(int16_t*)((char*)w+0x20),*(int16_t*)((char*)w+0x24),0};
                for(int j=0;j<count;++j){void* ws=pointer(list_item(list,j),0x18);for(int k=0;k<list_size(ws);++k)
                    if(e.target_count&&generic_unit_research::weapon_matches(list_item(ws,k),e.targets[0]))++e.targets[0].refs;}
                if(e.target_count)e.delta.damage[0][0]=1;}
            if(*(int*)((char*)type+0x34)<=0||*(int*)((char*)type+0x34)>INT16_MAX-17)e.delta.hp=0;
            for(int j=0;j<count;++j){void* st=list_item(list,j);
                if(*(int16_t*)((char*)st+0x22)<0||*(int16_t*)((char*)st+0x22)>INT16_MAX-2)e.delta.armor=0;
                if(*(int*)((char*)st+0x4c)<0||*(int*)((char*)st+0x4c)>INT_MAX-3)e.delta.sight=0;}
            good=plan.add(e);
            for(int state=0;good&&state<count;++state){++states;for(int damage:{0,7,-1})good=good&&x.spawn(type,state,damage,false,slot++);}
        }
        void* hq=good?local_research_runtime::headquarters(x.side):nullptr;int bslot=0;
        for(int level=0;good&&level<5;++level)for(int damage:{0,7,-1})good=good&&x.spawn(hq,level,damage,true,bslot++);
        // Deduplicate an entity present in the side array AND alliance list.
        void* alliances=pointer(x.battle,0x170);void* alliance=alliances?((void**)((char*)alliances+32))[0]:nullptr;
        if(good){void* list=x.field_list(object_class(alliance),"$pE",r);good=list&&x.add(list,x.samples[0].entity);
            if(good)field_set(alliance,class_field_from_name(object_class(alliance),"$pE"),list);}
        void* prepared=good?local_research_runtime::clone(x.side,r):nullptr;
        research_effects::Delta delta;for(int i=0;i<5;++i)delta.hp[i]=17;delta.income_base=3;delta.income_level=1;
        int matched=0;bool hq_ok=good&&prepared&&local_research_runtime::apply(x.battle,1,delta,hq,prepared);
        bool types_ok=hq_ok;
        void* replacements=types_ok?object_new(object_class(pointer(x.side,0xf0))):nullptr;
        void* ex=nullptr;if(!r.keep(replacements))types_ok=false;
        if(types_ok)invoke_named(replacements,".ctor",0,nullptr,&ex);types_ok=types_ok&&!ex;
        // The migration matrix addresses each original ID explicitly. It is
        // independent of research-family matching, which intentionally rejects
        // ambiguous same-model variants in the purchase/effect handler.
        for(int i=0;types_ok&&i<n;++i){Roots tr;void* type=((void**)((char*)values+32))[i];
            void* copy=generic_unit_research::copy_type(type,plan.families[i],tr);
            if(!copy){const char* reason=nullptr;generic_unit_research::shape(type,plan.families[i],reason);
                LOG("Preplaced matrix copy rejected id=%d reason=%s",*(int*)((char*)type+0x10),reason?reason:"unknown");types_ok=false;break;}
            int id=*(int*)((char*)type+0x10);void* args[]={&id,copy};invoke_named(replacements,"Add",2,args,&ex);types_ok=!ex;++matched;}
        if(types_ok){field_set(prepared,class_field_from_name(object_class(prepared),"$gg"),replacements);types_ok=local_research_runtime::relink_production_units(prepared);}
        good=good&&hq_ok&&types_ok;LOG("Preplaced matrix source=%s built=%d HQ=%d types=%d matched=%d/%d",name,x.count,hq_ok,types_ok,matched,n);
        Migration migration;good=good&&migration.prepare(x.battle,x.side,prepared)&&x.unchanged();
        bool atomic=false;int checked=0;
        if(good){
            // Fail at the LAST entity after all earlier patches were staged.
            void* last=x.samples[x.count-1].entity;int old=*(int*)((char*)last+0x5c);*(int*)((char*)last+0x5c)=INT_MAX;
            Migration bad;atomic=!bad.prepare(x.battle,x.side,prepared);*(int*)((char*)last+0x5c)=old;atomic=atomic&&x.unchanged();
            void* source=local_research_runtime::clone(x.side,r);field_set(source,class_field_from_name(object_class(source),"$gg"),pointer(x.side,0xf0));
            field_set(x.side,class_field_from_name(object_class(x.side),"$gg"),pointer(prepared,0xf0));
            field_set(x.side,class_field_from_name(object_class(x.side),"$Gg"),pointer(prepared,0xf8));migration.commit();
            good=x.verify(checked)&&baseline_hash==unit_dictionary_health_hash(source)&&research_batch_audit::producer_links(x.side);
        }
        good=good&&atomic&&matched==n&&migration.units==slot&&migration.buildings==15;ok=ok&&good;total+=x.count;
        fprintf(f,"{\"record\":\"faction_matrix\",\"source\":\"%s\",\"families\":%d,\"states\":%d,\"units\":%d,\"buildings\":%d,\"runtime_weapons\":%d,\"late_refusal_atomic\":%s,\"passed\":%s}\n",name,n,states,slot,bslot,checked,atomic?"true":"false",good?"true":"false");
    }
    // Exercise the exact production wrapper with the current saved research.
    for(const char* name:{"fmc2m","fmc2r"}){
        Fixture x;bool good=x.init(name);void* rifle=good?local_research_runtime::rifle(x.side):nullptr;void* hq=good?local_research_runtime::headquarters(x.side):nullptr;
        for(int i=0;good&&i<3;++i)good=x.spawn(rifle,0,i==2?-1:i*7,false,i)&&x.spawn(hq,i,i==2?-1:i*7,true,i);
        void* units=pointer(x.side,0xf0);void* buildings=pointer(x.side,0xf8);auto before=local_research_runtime::metrics(x.side);
        void* last=good?x.samples[x.count-1].entity:nullptr;bool atomic=false,shared_guard=false,queue=false;int weapons=0;
        if(good){
            int old=*(int*)((char*)last+0x5c);*(int*)((char*)last+0x5c)=INT_MAX;
            atomic=!apply_local_battle_research(x.battle)&&units==pointer(x.side,0xf0)&&buildings==pointer(x.side,0xf8)&&local_research_runtime::equal(before,local_research_runtime::metrics(x.side));
            *(int*)((char*)last+0x5c)=old;atomic=atomic&&x.unchanged();
            // Existing training references and queue entries must resolve to
            // the same private UnitTypes as future original native recruitment.
            Roots qr;void* b=x.samples[1].entity;void* q=x.field_list(object_class(b),"queue",qr);
            void* qe=object_new(find_class("com.geargames.aow.entities","QueueElement"));good=q&&qr.keep(qe);
            if(good){field_set(qe,class_field_from_name(object_class(qe),"un_type"),rifle);good=x.add(q,qe);
                field_set(b,class_field_from_name(object_class(b),"queue"),q);
                field_set(b,class_field_from_name(object_class(b),"curr_train"),rifle);
                memcpy(x.samples[1].bytes,b,0x168);}
            // A weapon accidentally shared with another side must refuse the
            // complete transaction, including the otherwise valid HQ changes.
            void* enemy=local_dictionary_item(pointer(x.battle,0x178),2);void* enemy_units=array_new(find_class("com.geargames.aow.entities","Unit"),1);
            void* foreign=object_new(find_class("com.geargames.aow.entities","Unit"));good=good&&qr.keep(enemy_units)&&qr.keep(foreign);
            void* original=pointer(enemy,0x140);
            if(good){field_set(foreign,class_field_from_name(find_class("com.geargames.aow.entities","Dynamic"),"side"),enemy);
                field_set(foreign,class_field_from_name(object_class(foreign),"weapons"),pointer(x.samples[0].entity,0x28));x.array_put(enemy_units,0,foreign);
                field_set(enemy,class_field_from_name(object_class(enemy),"$lg"),enemy_units);
                // Add a saved rifle weapon level in memory to ensure runtime
                // weapon patching is needed, then restore the profile exactly.
                auto backup=progress;bool added=false;
                void* levels=generic_unit_research::levels("RESISTANCE");
                for(int i=0;i<list_size(levels);++i){void* level=list_item(levels,i);generic_unit_research::Effect e;
                    if(generic_unit_research::resolve(level,e)&&e.owner&&e.target_count&&generic_unit_research::identity(e.owner,rifle)){
                        int id=*(int*)((char*)level+0x10);if(!local_progress::has_upgrade(progress,id)){progress.upgrade_ids[progress.upgrade_count++]=id;added=true;break;}}}
                shared_guard=added&&!apply_local_battle_research(x.battle)&&units==pointer(x.side,0xf0)&&buildings==pointer(x.side,0xf8)&&x.unchanged();
                progress=backup;field_set(enemy,class_field_from_name(object_class(enemy),"$lg"),original);}
            good=good&&apply_local_battle_research(x.battle)&&x.verify(weapons)&&research_batch_audit::producer_links(x.side);
            void* expected=local_dictionary_item(pointer(x.side,0xf0),*(int*)((char*)rifle+0x10));
            queue=pointer(b,0xa0)==expected&&pointer(qe,0x10)==expected;
        }
        good=good&&atomic&&shared_guard&&queue;ok=ok&&good;total+=x.count;
        fprintf(f,"{\"record\":\"saved_wrapper\",\"source\":\"%s\",\"entities\":%d,\"late_refusal_atomic\":%s,\"foreign_weapon_alias_refused\":%s,\"queue_links\":%s,\"passed\":%s}\n",name,x.count,atomic?"true":"false",shared_guard?"true":"false",queue?"true":"false",good?"true":"false");
    }
    fprintf(f,"{\"record\":\"summary\",\"passed\":%s,\"entities\":%d,\"purchase_calls\":0,\"battle_runs\":0,\"save_writes\":0}\n",ok?"true":"false",total);fclose(f);LOG("Preplaced research startup audit passed=%d entities=%d",ok,total);
}
}
