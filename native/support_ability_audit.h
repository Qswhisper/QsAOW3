#pragma once
namespace support_ability_audit {
using namespace support_ability;
static uint64_t hash(void* f){uint64_t h=1469598103934665603ull;if(!f)return 0;
    for(int i=0x10;i<0x50;++i)h=(h^((uint8_t*)f)[i])*1099511628211ull;return h;}
static bool same_other(void* a,void* b){if(!a||!b)return false;uint8_t bytes[0x40];memcpy(bytes,(char*)a+0x10,sizeof(bytes));
    memcpy(bytes+0x34,(char*)b+0x44,8);return !memcmp(bytes,(char*)b+0x10,sizeof(bytes));}
static bool values(void* side,void* source,const Plan& p,int& getters){
    SupportAura expected;void* canonical=base(p.faction),*f=get(side,0xd8);if(!canonical||!f||!research_effects::support_aura_values(
        *(int*)((char*)canonical+0x44),*(int*)((char*)canonical+0x48),p.delta,expected)||!same_other(f,get(source,0xd8))||
        !original_stat_equals(f,"get_ShieldFogRadius",4,expected.radius)||!original_stat_equals(f,"get_ShieldFogBonus",4,expected.bonus)||
        !original_stat_equals(f,"get_ShieldFogActEnergy",2,*(int16_t*)((char*)get(source,0xd8)+0x4c)))return false;getters+=3;
    for(int kind=0;kind<2;++kind){Roots roots;uintptr_t handle=0;int offset=kind?0xf8:0xf0;
        void* old=get(source,offset),*list=local_dictionary_values(old,find_class("com.geargames.aow.entities",kind?"BuildingType":"UnitType"),handle);
        if(!list||!roots.take(handle))return false;
        for(size_t i=0;i<array_length(list);++i){void* type=((void**)((char*)list+32))[i],*before=get(type,0x20);
            void* copy=local_dictionary_item(get(side,offset),*(int*)((char*)type+0x10)),*after=get(copy,0x20);if(!copy)return false;
            if(before&&*(int*)((char*)before+0x10)==p.faction){
                if(copy==type||after==before||!same_other(after,before)||
                    !original_stat_equals(after,"get_ShieldFogRadius",4,p.delta.radius?expected.radius:*(int*)((char*)before+0x44))||
                    !original_stat_equals(after,"get_ShieldFogBonus",4,p.delta.bonus?expected.bonus:*(int*)((char*)before+0x48)))return false;getters+=2;
            }else if(copy!=type||after!=before)return false;
        }
    }return research_batch_audit::producer_links(side);
}
static bool outer(int faction,const Plan& aura,const generic_unit_research::Effect& timer,int& entities,int& getters){
    preplaced_research_audit::Fixture fixture;if(!fixture.init(faction==1?"fmc1m":"fmc2m"))return false;Roots roots;uintptr_t handle=0;
    void* dict=get(fixture.side,0xf0),*types=local_dictionary_values(dict,find_class("com.geargames.aow.entities","UnitType"),handle);
    if(!types||!roots.take(handle))return false;void* owner_type=nullptr;
    for(size_t i=0;i<array_length(types);++i){void* type=((void**)((char*)types+32))[i];if(generic_unit_research::identity(type,timer.owner))owner_type=type;}
    if(!owner_type)return false;void* source=local_research_runtime::clone(fixture.side,roots);if(!source)return false;
    void* enemy=local_dictionary_item(get(fixture.battle,0x178),2),*foreign=get(enemy,0xd8);uint64_t before=hash(foreign);
    for(int i=0;i<2;++i)if(!fixture.spawn(owner_type,i,9,false,i))return false;
    {if(!apply_local_battle_research(fixture.battle,1)||!values(fixture.side,source,aura,getters))return false;
        for(int i=0;i<fixture.count;++i){void* type=get(fixture.samples[i].entity,0x40);
            if(!original_stat_equals(type,"get_TickToSpec",1,*(int8_t*)((char*)owner_type+0x4d)-timer.transition.to)||
               !original_stat_equals(type,"get_TickFromSpec",1,*(int8_t*)((char*)owner_type+0x4e)-timer.transition.from))return false;}
        int weapons=0;if(!fixture.verify(weapons)||hash(foreign)!=before||get(enemy,0xd8)!=foreign)return false;
    }entities+=fixture.count;return true;
}
static __attribute__((noinline)) void run(){
    if(!progress_ready||access(DATA_ROOT "support-ability-audit.request",F_OK))return;unlink(DATA_ROOT "support-ability-audit.request");
    building_weapon_audit::Restore restore;if(!restore.saved.data)return;FILE* out=fopen(DATA_ROOT "support-ability-audit.jsonl","w");if(!out)return;chmod(DATA_ROOT "support-ability-audit.jsonl",0644);
    bool ok=true;int candidates=0,accepted=0,predebit=0,prefixes=0,getters=0,ui=0,entities=0,timers=0,combined=0;
    for(int faction=1;faction<=2;++faction){local_faction::active=faction;progress=local_progress::seed(100000000,0,0,10000);research_view::invalidate();
        bool ready=research_view::ready();Roots roots;void* source=ready?research_view::cache.side:nullptr;bool good=source&&roots.keep(source);
        uint64_t canonical_hash=hash(base(faction));Plan sum;sum.faction=faction;generic_unit_research::Effect timer;
        void* rows=generic_unit_research::levels(faction==1?"CONFEDERATION":"RESISTANCE");int first=0;
        for(int i=0;rows&&i<*(int*)((char*)rows+0x18);++i){void* row=list_item(rows,i);char text[4096];research_effects::UnitTransition d;int profile=0;
            bool aura=candidate(row);bool transition=local_research_runtime::ascii(get(row,0x18),text,sizeof(text))&&research_effects::unit_transition_parse(text,d,&profile)&&profile==faction;
            if(!aura&&!transition)continue;++candidates;int id=*(int*)((char*)row+0x10);if(!first)first=id;
            bool pass=good&&local_research_supported(row);if(pass)++predebit;
            if(aura){Effect e;pass=pass&&resolve(row,e)&&research_effects::support_aura_add(sum.delta,e.delta,sum.delta);
                Roots one;void* a=source?local_research_runtime::clone(source,one):nullptr;void* b=nullptr;
                pass=pass&&a&&prepare(a,sum)&&values(a,source,sum,getters);if(pass)b=local_research_runtime::clone(a,one);
                pass=pass&&b&&prepare(b,sum)&&values(b,source,sum,getters);if(pass)++prefixes;
            }else{generic_unit_research::Effect e;const char* why=nullptr;Roots one;
                pass=pass&&generic_unit_research::resolve(row,e)&&generic_unit_research::shape(e.owner,e,why);
                if(!timer.owner)timer=e;else pass=pass&&research_effects::unit_transition_add(timer.transition,e.transition,timer.transition);
                void* copy=pass?generic_unit_research::copy_type(timer.owner,timer,one):nullptr;
                pass=pass&&copy&&unit_transition_audit::values(copy,timer.owner,timer);if(pass)++timers;
            }
            int wallet=progress.credits,cost=*(int*)((char*)row+0x20),xp=*(int*)((char*)row+0x28);
            pass=pass&&local_progress::buy_upgrade(progress,id,cost,xp)&&progress.credits==wallet-cost&&
                !local_progress::buy_upgrade(progress,id,cost,xp)&&progress.credits==wallet-cost&&canonical_hash==hash(base(faction));
            if(pass)++accepted;ok&=pass;fprintf(out,"{\"phase\":\"original_row\",\"faction\":%d,\"id\":%d,\"kind\":\"%s\",\"pass\":%s}\n",faction,id,aura?"aura":"timers",pass?"true":"false");
        }
        Plan collected;auto* plan=new generic_unit_research::Plan;bool saved=collect(collected)&&collected.faction==faction&&!memcmp(&collected.delta,&sum.delta,sizeof(sum.delta))&&generic_unit_research::collect(*plan)&&plan->count==1;
        delete plan;ok&=saved;
        research_view::invalidate();void* original=local_career::lookup("BATTLE","unitTypes",faction==1?17:117);
        void* projected=research_view::projected(original,false),*shown=research_view::fraction(get(original,0x20),original,projected);
        bool mapped=projected&&projected!=original&&shown==get(research_view::cache.side,0xd8)&&
            values(research_view::cache.side,source,sum,getters)&&original_stat_equals(projected,"get_TickToSpec",1,12)&&original_stat_equals(projected,"get_TickFromSpec",1,12);
        if(mapped)++ui;ok&=mapped;
        Roots one;void* bad=local_research_runtime::clone(local_upgrade_level(first),one),*desc=string_new("{#20022<0,33>} {#20013<1>}");
        bool atomic=bad&&one.keep(desc);if(atomic){field_set(bad,class_field_from_name(object_class(bad),"desc"),desc);atomic=!local_research_supported(bad);}ok&=atomic;
        auto invalid=timer;if(timer.owner)invalid.transition.to=*(int8_t*)((char*)timer.owner+0x4d);
        bool timer_guard=timer.owner&&!generic_unit_research::copy_type(timer.owner,invalid,one);ok&=timer_guard;
        // The complete loader runs once per decoded battle. Two independent
        // decodes test fresh loads; aura-only preparation above explicitly
        // tests repeat projection on a retained source, without stacking.
        bool migrated=outer(faction,sum,timer,entities,getters)&&outer(faction,sum,timer,entities,getters);ok&=migrated&&canonical_hash==hash(base(faction));
        void* a=local_research_runtime::clone(source,one);auto* state=new local_progress::State(progress);int unit=faction==1?17:117,mod=faction==1?171:1171;
        bool equipped=local_progress::grant_stock(*state,17,mod,7)&&local_progress::equip_modification(*state,unit,mod);
        bool mod_ok=a&&prepare(a,sum)&&equipped&&local_mod_effects::prepare_side(a,*state);
        if(mod_ok){void* f=get(a,0xd8);int before=*(int*)((char*)base(faction)+0x44)+sum.delta.radius;
            mod_ok=*(int*)((char*)f+0x44)>0&&(faction==1?*(int*)((char*)f+0x44)>before:*(int*)((char*)f+0x44)<before)&&
                *(int*)((char*)f+0x48)==*(int*)((char*)base(faction)+0x48)+sum.delta.bonus;
            void* type=local_dictionary_item(get(a,0xf0),unit),*s=get(type,0x18);
            mod_ok=mod_ok&&s&&*(int16_t*)((char*)list_item(s,0)+0x8c)>0&&*(int16_t*)((char*)list_item(s,1)+0x8c)==0;
            // All private own-faction aliases, including building producers,
            // observe the same radius action and retain unrelated fields.
            for(int kind=0;kind<2&&mod_ok;++kind){Roots check;uintptr_t handle=0;void* types=local_dictionary_values(get(a,kind?0xf8:0xf0),find_class("com.geargames.aow.entities",kind?"BuildingType":"UnitType"),handle);
                mod_ok=types&&check.take(handle);for(size_t j=0;mod_ok&&j<array_length(types);++j){void* p=((void**)((char*)types+32))[j],*fraction=get(p,0x20);
                    if(fraction&&*(int*)((char*)fraction+0x10)==faction)mod_ok=original_stat_equals(fraction,"get_ShieldFogRadius",4,*(int*)((char*)f+0x44));}}
            mod_ok=mod_ok&&research_batch_audit::producer_links(a);}
        delete state;if(mod_ok)++combined;ok&=mod_ok;
        fprintf(out,"{\"phase\":\"cumulative\",\"faction\":%d,\"radius_delta\":%d,\"bonus_delta\":%d,\"to_ticks_reduction\":%d,\"from_ticks_reduction\":%d,\"saved_plan\":%s,\"original_ui\":%s,\"two_fresh_preplaced_loads\":%s,\"modification_combined\":%s,\"atomic_guards\":%s,\"pass\":%s}\n",faction,sum.delta.radius,sum.delta.bonus,timer.transition.to,timer.transition.from,saved?"true":"false",mapped?"true":"false",migrated?"true":"false",mod_ok?"true":"false",atomic&&timer_guard?"true":"false",saved&&mapped&&migrated&&mod_ok&&atomic&&timer_guard&&canonical_hash==hash(base(faction))?"true":"false");
    }
    progress=*restore.saved.data;local_faction::active=restore.faction;research_view::invalidate();bool unchanged=!memcmp(&progress,restore.saved.data,sizeof(progress));
    fprintf(out,"{\"summary\":true,\"pass\":%s,\"candidates\":%d,\"accepted\":%d,\"active_faction_predebit\":%d,\"aura_prefixes\":%d,\"timer_prefixes\":%d,\"original_getters\":%d,\"original_ui_owners\":%d,\"preplaced_units\":%d,\"support_modifications_combined\":%d,\"player_state_unchanged\":%s,\"player_save_writes\":0,\"battle_runs\":0}\n",ok&&accepted==30&&prefixes==18&&timers==12&&ui==2&&entities==8&&combined==2&&unchanged?"true":"false",candidates,accepted,predebit,prefixes,timers,getters,ui,entities,combined,unchanged?"true":"false");fclose(out);
}
}
