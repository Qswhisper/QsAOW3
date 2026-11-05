// Full retained catalogue and original-object fixtures; no purchases or games.
#pragma once
namespace research_batch_audit {
using namespace generic_unit_research;
static void json_string(FILE* f,void* value) {
    fputc('"',f);
    int n=value?*(int*)((char*)value+16):0;auto p=value?(uint16_t*)((char*)value+20):nullptr;
    for(int i=0;i<n&&i<16384;++i) {
        unsigned c=p[i];if(c=='"'||c=='\\')fprintf(f,"\\%c",c);
        else if(c>=32&&c<127)fputc(c,f);else fprintf(f,"\\u%04x",c);
    }
    fputc('"',f);
}
static int owner_faction(void* owner) {void* f=owner?*(void**)((char*)owner+0x20):nullptr;return f?*(int*)((char*)f+0x10):0;}
static uint64_t type_hash(void* type) {
    uint64_t h=1469598103934665603ull;
    auto add=[&](int v){h=(h^(uint32_t)v)*1099511628211ull;};
    add(*(int*)((char*)type+0x34));add(*(int16_t*)((char*)type+0x32));
    void* states=*(void**)((char*)type+0x18);int n=states?*(int*)((char*)states+0x18):0;
    for(int i=0;i<n;++i) {
        void* s=list_item(states,i);if(!s)return 0;
        add(*(int16_t*)((char*)s+0x22));add(*(int8_t*)((char*)s+0x58));add(*(int*)((char*)s+0x4c));
        add(*(int16_t*)((char*)s+0x72));add(*(int16_t*)((char*)s+0x74));
        void* weapons=*(void**)((char*)s+0x18);int nw=weapons?*(int*)((char*)weapons+0x18):0;
        for(int w=0;w<nw;++w){void* weapon=list_item(weapons,w);if(!weapon)return 0;for(int t=0;t<3;++t)add(*(int*)((char*)weapon+0x28+t*4));}
    }
    return h;
}
static bool getters(void* copy,void* base,const Effect& e) {
    int hp=*(int*)((char*)base+0x34)+e.delta.hp;
    int short_hp=e.delta.hp?hp:*(int16_t*)((char*)base+0x32);
    if(!original_stat_equals(copy,"get_LifeInt",4,hp)||!original_stat_equals(copy,"get_Life",2,short_hp))return false;
    void* states=*(void**)((char*)base+0x18);void* actual=*(void**)((char*)copy+0x18);
    int n=states?*(int*)((char*)states+0x18):0;
    if((actual?*(int*)((char*)actual+0x18):0)!=n)return false;
    int mode_armor[2];if(!state_armor_deltas(base,e,mode_armor))return false;
    for(int i=0;i<n;++i) {
        void* s=list_item(states,i);void* c=list_item(actual,i);
        if(!original_stat_equals(c,"get_Armor",2,*(int16_t*)((char*)s+0x22)+e.delta.armor+(e.state_armor.group?mode_armor[i]:0))||
           !original_stat_equals(c,"get_Speed",1,*(int8_t*)((char*)s+0x58)+e.delta.speed)||
           !original_stat_equals(c,"get_Sight",4,*(int*)((char*)s+0x4c)+e.delta.sight)||
           !original_stat_equals(c,"get_Regen",2,*(int16_t*)((char*)s+0x72)+e.delta.regen))return false;
        void* sw=*(void**)((char*)s+0x18);void* cw=*(void**)((char*)c+0x18);
        int count=sw?*(int*)((char*)sw+0x18):0;
        if((cw?*(int*)((char*)cw+0x18):0)!=count)return false;
        const char* names[]={"get_DamageLight","get_DamageMedium","get_DamageHeavy"};
        for(int w=0;w<count;++w) {
            void* original=list_item(sw,w);void* actual=list_item(cw,w);int target=target_index(original,e);
            if(!original||!actual||(!e.target_count&&original!=actual))return false;
            for(int t=0;t<3;++t)if(!original_stat_equals(actual,names[t],4,
                *(int*)((char*)original+0x28+t*4)+(target>=0?e.delta.damage[target][t]:0)))return false;
            if(e.target_count&&target<0&&original!=actual)return false;
            for(int previous=0;previous<=i;++previous) {
                void* ps=*(void**)((char*)list_item(states,previous)+0x18);
                void* pc=*(void**)((char*)list_item(*(void**)((char*)copy+0x18),previous)+0x18);
                int pn=ps?*(int*)((char*)ps+0x18):0;
                for(int q=0;q<(previous==i?w:pn);++q)
                    if((original==list_item(ps,q))!=(actual==list_item(pc,q)))return false;
            }
        }
    }
    return true;
}
static bool producer_links(void* side) {
    Roots roots;uintptr_t handle=0;
    void* values=local_dictionary_values(*(void**)((char*)side+0xf8),find_class("com.geargames.aow.entities","BuildingType"),handle);
    if(!values||!roots.take(handle))return false;
    for(size_t i=0;i<array_length(values);++i) {
        void* type=((void**)((char*)values+32))[i];void* levels=*(void**)((char*)type+0x18);
        int n=levels?*(int*)((char*)levels+0x18):0;
        for(int j=0;j<n;++j) {
            void* level=list_item(levels,j);void* list=*(void**)((char*)level+0x20);int count=list?*(int*)((char*)list+0x18):0;
            for(int k=0;k<count;++k){void* u=list_item(list,k);if(!u||local_dictionary_item(*(void**)((char*)side+0xf0),*(int*)((char*)u+0x10))!=u)return false;}
        }
    }
    return true;
}
static bool parser_checks() {
    using namespace research_effects;
    UnitStats d;int unknown=0;
    if(unit_stats_parse("{#20000<17>}?{#20001<2>} {#20014<1>}\n{#20034<3>}",d,unknown)!=UnitParse::ok||d.hp!=17||d.armor!=2||d.speed!=1||d.sight!=3)return false;
    for(const char* bad:{"","{","{#","{#20000","{#20000<0>}","{#20000<-1>}","{#20000<1.5>}","{#20000<2147483648>}","{#20000<2>} ","{#20000<1>} {#20000<2>}","{#20000<1>} {#20099<2>}"}) {
        UnitStats before;before.hp=77;
        if(unit_stats_parse(bad,before,unknown)==UnitParse::ok||before.hp!=77)return false;
    }
    UnitStats x,y,out;x.hp=INT_MAX;y.hp=1;out.hp=77;
    if(rifle_add(x,y,out)||out.hp!=77)return false;
    int damage[3]={};
    if(!weapon_damage_parse("{#20004<2>}\n{#20005<3>}?{#20006<4>}",damage)||damage[0]!=2||damage[1]!=3||damage[2]!=4)return false;
    for(const char* bad:{"","{","{#20004<0>}","{#20004<2147483648>}","{#20004<1>} {#20004<2>}","{#20004<1>} {#20008<2>}","{#20004<1.5>}","{#20004<1>} "}) {
        int unchanged[3]={77,88,99};if(weapon_damage_parse(bad,unchanged)||unchanged[0]!=77||unchanged[1]!=88||unchanged[2]!=99)return false;
    }
    return true;
}
static void run() {
    if(access(DATA_ROOT "research-batch.enabled",F_OK))return;
    FILE* f=fopen(DATA_ROOT "art3offline-research-batch.jsonl","w");if(!f)return;
    chmod(DATA_ROOT "art3offline-research-batch.jsonl",0644);
    fputs("{\"record\":\"policy\",\"schema\":5,\"purchase_calls\":0,\"battle_runs\":0,\"effect_source\":\"retained numeric description tokens; UpgradeLevel has no separate numeric effect fields\",\"active_profile\":\"RESISTANCE\"}\n",f);
    int total=0,effect_count=0,objects=0,object_failures=0,legacy=0,legacy_compatible=0;bool ok=parser_checks();
    int preflight_cases=0,preflight_failures=0;bool identity_guard=false;bool target_guards=false;
    fprintf(f,"{\"record\":\"parser_check\",\"passed\":%s,\"whole_level_rejection\":true}\n",ok?"true":"false");
    Plan samples,cumulative;int sample_ids[128]={},damage_ids[128]={},damage_count=0;
    for(const char* source:{"CONFEDERATION","RESISTANCE","BATTLE"}) {
        void* list=levels(source);int n=list?*(int*)((char*)list+0x18):0;
        fprintf(f,"{\"record\":\"source\",\"source\":\"%s\",\"present\":%s,\"levels\":%d}\n",source,saved(source)?"true":"false",n);
        if(n<0||n>20000){ok=false;break;}
        for(int i=0;i<n;++i) {
            void* level=list_item(list,i);void* up=level?*(void**)((char*)level+0x48):nullptr;
            if(!level||!up){ok=false;continue;}++total;
            const char* kind=class_name(object_class(up));
            Effect e;bool unit=!strcmp(kind,"UpgradeUnit"),building=!strcmp(kind,"UpgradeBuilding");
            void* owner=(unit||building)?*(void**)((char*)up+0x48):nullptr;
            bool accepted=unit&&resolve(level,e);const char* reason=e.reason;
            if(unit&&!strcmp(source,"RESISTANCE")) {
                UnitStats old;void* old_owner=nullptr;
                if(local_research_runtime::unit_level_delta(level,old,old_owner)) {
                    bool same=accepted&&old_owner==e.owner&&research_effects::rifle_equal(old,e.delta);
                    if(same)++legacy_compatible;else ok=false;
                }
            }
            if(accepted){accepted=shape(e.owner,e,reason);if(accepted)reason=e.reason;}
            if(building) {
                research_effects::Delta d;char text[4096];
                accepted=owner&&owner_faction(owner)==2&&*((int8_t*)owner+0x30)==1&&
                    local_research_runtime::ascii(*(void**)((char*)level+0x18),text,sizeof(text))&&research_effects::parse(text,d);
                reason=accepted?"legacy_hq_stats":"building_effect_handler_missing";
            }
            if(!unit&&!building)reason="upgrade_class_unimplemented";
            int cards=*(int*)((char*)level+0x54);
            if(unit&&!strcmp(source,"RESISTANCE")) {
                bool allowed=purchase_supported(level),expected=accepted&&!cards;
                ++preflight_cases;if(allowed!=expected){++preflight_failures;ok=false;}
                fprintf(f,"{\"record\":\"preflight_check\",\"id\":%d,\"handler_allows\":%s,\"expected\":%s,\"passed\":%s,\"debit_calls\":0}\n",*(int*)((char*)level+0x10),allowed?"true":"false",expected?"true":"false",allowed==expected?"true":"false");
                if(accepted&&!identity_guard) {
                    Roots shadow_roots;
                    void* shadow=local_research_runtime::clone(level,shadow_roots);
                    void* shadow_up=local_research_runtime::clone(up,shadow_roots);
                    void* shadow_owner=local_research_runtime::clone(owner,shadow_roots);
                    if(shadow&&shadow_up&&shadow_owner) {
                        *(int*)((char*)shadow_owner+0x10)=*(int*)((char*)owner+0x10)+1;
                        field_set(shadow_up,class_field_from_name(object_class(shadow_up),"owner"),shadow_owner);
                        field_set(shadow,class_field_from_name(object_class(shadow),"owner"),shadow_up);
                        identity_guard=!purchase_supported(shadow);
                    }
                }
            }
            fprintf(f,"{\"record\":\"level\",\"source\":\"%s\",\"id\":%d,\"upgrade\":%d,\"kind\":\"%s\",\"owner_id\":%d,\"faction\":%d,\"slot\":%d,\"order\":%d,\"cost\":%d,\"xp\":%d,\"card_amount\":%d,\"rank_id\":%d,\"effect_supported\":%s,\"non_card_effect_supported\":%s,\"reason\":\"%s\",\"model\":",
                source,*(int*)((char*)level+0x10),*(int*)((char*)up+0x10),kind,owner?*(int*)((char*)owner+0x10):-1,owner_faction(owner),
                *((int8_t*)up+0x21),*((int8_t*)up+0x20),*(int*)((char*)level+0x20),*(int*)((char*)level+0x28),cards,
                *(void**)((char*)level+0x40)?*(int*)((char*)*(void**)((char*)level+0x40)+0x10):-1,
                accepted?"true":"false",accepted&&!cards?"true":"false",reason);
            json_string(f,unit&&owner?*(void**)((char*)owner+0x60):nullptr);
            fputs(",\"upgrade_name\":",f);json_string(f,*(void**)((char*)up+0x28));
            fputs(",\"upgrade_description\":",f);json_string(f,*(void**)((char*)up+0x30));
            fputs(",\"upgrade_icon\":",f);json_string(f,*(void**)((char*)up+0x38));
            fputs(",\"description\":",f);json_string(f,*(void**)((char*)level+0x18));
            fputs(",\"prerequisites\":[",f);void* req=*(void**)((char*)level+0x30);size_t nr=req?array_length(req):0;
            if(nr>512){ok=false;nr=0;}
            for(size_t j=0;j<nr;++j){void* r=((void**)((char*)req+32))[j];fprintf(f,"%s%d",j?",":"",r?*(int*)((char*)r+0x10):-1);}
            fputs("]}\n",f);
            if(!accepted)continue;++effect_count;
            if(!unit)continue;
            Roots roots;uint64_t before=type_hash(e.owner);void* copy=copy_type(e.owner,e,roots);
            bool checked=copy&&getters(copy,e.owner,e)&&before==type_hash(e.owner);++objects;if(!checked){++object_failures;ok=false;}
            fprintf(f,"{\"record\":\"object_check\",\"source\":\"%s\",\"id\":%d,\"owner_id\":%d,\"faction\":%d,\"passed\":%s,\"canonical_unchanged\":%s}\n",source,*(int*)((char*)level+0x10),*(int*)((char*)e.owner+0x10),owner_faction(e.owner),checked?"true":"false",before==type_hash(e.owner)?"true":"false");
            if(e.target_count) {
                fprintf(f,"{\"record\":\"weapon_target_check\",\"source\":\"%s\",\"id\":%d,\"owner_id\":%d,\"passed\":%s,\"all_weapon_getters\":true,\"targets\":[",source,*(int*)((char*)level+0x10),*(int*)((char*)e.owner+0x10),checked?"true":"false");
                for(int t=0;t<e.target_count;++t)fprintf(f,"%s{\"name\":%d,\"type\":%d,\"references\":%d,\"damage\":[%d,%d,%d]}",t?",":"",e.targets[t].name,e.targets[t].type,e.targets[t].refs,e.delta.damage[t][0],e.delta.damage[t][1],e.delta.damage[t][2]);
                fputs("]}\n",f);
                if(!target_guards) {
                    Effect wrong=e;wrong.targets[0].name=INT16_MAX;const char* guard_reason=nullptr;Roots guard_roots;
                    bool wrong_name=!shape(e.owner,wrong,guard_reason)&&!copy_type(e.owner,wrong,guard_roots);
                    wrong=e;++wrong.targets[0].refs;bool wrong_refs=!shape(e.owner,wrong,guard_reason);
                    wrong=e;for(int t=0;t<3;++t)wrong.delta.damage[0][t]=INT_MAX;
                    bool overflow=!shape(e.owner,wrong,guard_reason)&&!copy_type(e.owner,wrong,guard_roots);
                    target_guards=wrong_name&&wrong_refs&&overflow&&before==type_hash(e.owner);
                }
            }
            if(!strcmp(source,"RESISTANCE")) {
                if(e.legacy_damage)++legacy;
                if(e.target_count&&!e.legacy_damage&&damage_count<128) {
                    bool found=false;for(int j=0;j<damage_count;++j) {
                        void* old=local_upgrade_level(damage_ids[j]);Effect old_effect;
                        if(resolve(old,old_effect)&&old_effect.owner==e.owner&&old_effect.target_count==e.target_count&&same_key(old_effect.targets[0],e.targets[0]))found=true;
                    }
                    if(!found)damage_ids[damage_count++]=*(int*)((char*)level+0x10);
                }
                if(!cumulative.add(e))ok=false;
                if(e.delta.hp) {
                    bool found=false;for(int j=0;j<samples.count;++j)if(samples.families[j].owner==e.owner)found=true;
                    if(!found&&samples.count<128){sample_ids[samples.count]=*(int*)((char*)level+0x10);if(!samples.add(e))ok=false;}
                }
            }
        }
    }
    for(int i=0;i<cumulative.count;++i) {
        const Effect& e=cumulative.families[i];const char* reason=nullptr;bool in_range=shape(e.owner,e,reason);Roots roots;
        uint64_t before=type_hash(e.owner);void* copy=copy_type(e.owner,e,roots);
        bool checked=in_range?(copy&&getters(copy,e.owner,e)):copy==nullptr;
        checked=checked&&before==type_hash(e.owner);ok=ok&&checked;
        fprintf(f,"{\"record\":\"cumulative_check\",\"owner_id\":%d,\"in_range\":%s,\"passed\":%s}\n",*(int*)((char*)e.owner+0x10),in_range?"true":"false",checked?"true":"false");
    }
    void* message=read_scenario_message(DATA_ROOT "offline-scenarios/resistance-002.ggs",5040,"@generic-research-fixture");
    void* battle=message?*(void**)((char*)message+0x20):nullptr;
    void* dictionary=battle?*(void**)((char*)battle+0x178):nullptr;void* side=local_dictionary_item(dictionary,1);
    bool integration=false,idempotent=false,atomic=false,links=false,isolated=false;int matched=0,verified=0;
    if(side) {
        Roots roots;void* base=*(void**)((char*)side+0xf0);roots.keep(base);
        uint64_t original=unit_dictionary_health_hash(side),other[2]={unit_dictionary_health_hash(local_dictionary_item(dictionary,2)),unit_dictionary_health_hash(local_dictionary_item(dictionary,3))};
        integration=apply(side,base,samples,matched);links=integration&&producer_links(side);
        if(integration) {
            uintptr_t h=0;void* values=local_dictionary_values(base,find_class("com.geargames.aow.entities","UnitType"),h);roots.take(h);
            for(size_t j=0;values&&j<array_length(values);++j) {
                void* type=((void**)((char*)values+32))[j];
                for(int i=0;i<samples.count;++i)if(identity(type,samples.families[i].owner)) {
                    void* actual=local_dictionary_item(*(void**)((char*)side+0xf0),*(int*)((char*)type+0x10));bool good=getters(actual,type,samples.families[i]);
                    if(good)++verified;ok=ok&&good;
                    fprintf(f,"{\"record\":\"scenario_family\",\"owner_id\":%d,\"level\":%d,\"runtime_type\":%d,\"passed\":%s}\n",*(int*)((char*)samples.families[i].owner+0x10),sample_ids[i],*(int*)((char*)type+0x10),good?"true":"false");
                }
            }
            uint64_t after=unit_dictionary_health_hash(side);int again=0;
            idempotent=apply(side,base,samples,again)&&again==matched&&after==unit_dictionary_health_hash(side)&&producer_links(side);
            Plan bad=samples;
            // Invalidate every family, so at least one matched target fails.
            for(int i=0;i<bad.count;++i)bad.families[i].delta.hp=INT_MAX;
            void* units=*(void**)((char*)side+0xf0);void* buildings=*(void**)((char*)side+0xf8);int ignored=0;
            atomic=!apply(side,base,bad,ignored)&&units==*(void**)((char*)side+0xf0)&&buildings==*(void**)((char*)side+0xf8)&&after==unit_dictionary_health_hash(side);
            void* shadow=local_research_runtime::clone(side,roots);
            if(shadow)field_set(shadow,class_field_from_name(object_class(shadow),"$gg"),base);
            isolated=shadow&&original==unit_dictionary_health_hash(shadow)&&other[0]==unit_dictionary_health_hash(local_dictionary_item(dictionary,2))&&other[1]==unit_dictionary_health_hash(local_dictionary_item(dictionary,3));
        }
    }
    ok=ok&&integration&&matched>=2&&verified==matched&&idempotent&&atomic&&links&&isolated;
    fprintf(f,"{\"record\":\"scenario_check\",\"families_requested\":%d,\"families_present\":%d,\"families_verified\":%d,\"applied\":%s,\"idempotent\":%s,\"atomic_refusal\":%s,\"producer_links\":%s,\"baseline_and_other_sides_unchanged\":%s}\n",samples.count,matched,verified,integration?"true":"false",idempotent?"true":"false",atomic?"true":"false",links?"true":"false",isolated?"true":"false");
    void* saved_message=read_scenario_message(DATA_ROOT "offline-scenarios/resistance-002.ggs",5040,"@generic-saved-plan-fixture");
    void* saved_battle=saved_message?*(void**)((char*)saved_message+0x20):nullptr;
    bool saved_ok=saved_battle&&apply_local_battle_research(saved_battle);ok=ok&&saved_ok;
    fprintf(f,"{\"record\":\"saved_plan_check\",\"passed\":%s,\"saved_upgrades\":%u}\n",saved_ok?"true":"false",progress.upgrade_count);
    // A multi-family saved profile must pass the complete HQ+unit initializer.
    // This fixture mutates only the in-memory copy and restores it before exit;
    // it does not call save, debit, completion or allocate a campaign run ID.
    auto backup=progress;int added=0;
    auto add_id=[&](int id){for(uint32_t j=0;j<progress.upgrade_count;++j)if(progress.upgrade_ids[j]==id)return;
        if(progress.upgrade_count<8192){progress.upgrade_ids[progress.upgrade_count++]=id;++added;}};
    for(int i=0;i<samples.count;++i)add_id(sample_ids[i]);
    for(int i=0;i<damage_count;++i)add_id(damage_ids[i]);
    Plan saved_plan;bool collected=collect(saved_plan);
    void* synthetic_message=read_scenario_message(DATA_ROOT "offline-scenarios/resistance-002.ggs",5040,"@multi-family-saved-research-fixture");
    void* synthetic=synthetic_message?*(void**)((char*)synthetic_message+0x20):nullptr;
    void* synthetic_side=local_dictionary_item(synthetic?*(void**)((char*)synthetic+0x178):nullptr,1);
    Roots synthetic_roots;void* synthetic_baseline=synthetic_side?*(void**)((char*)synthetic_side+0xf0):nullptr;
    bool rooted=synthetic_roots.keep(synthetic_baseline);
    bool multi_ok=collected&&synthetic&&rooted&&apply_local_battle_research(synthetic);
    bool multi_stats=multi_ok&&producer_links(synthetic_side);
    if(multi_stats)for(int i=0;i<saved_plan.count;++i) {
        const Effect& e=saved_plan.families[i];bool found=false;Roots roots;uintptr_t handle=0;
        void* values=local_dictionary_values(*(void**)((char*)synthetic_side+0xf0),find_class("com.geargames.aow.entities","UnitType"),handle);roots.take(handle);
        for(size_t j=0;values&&j<array_length(values);++j){void* type=((void**)((char*)values+32))[j];
            if(identity(type,e.owner)){void* base=local_dictionary_item(synthetic_baseline,*(int*)((char*)type+0x10));found=base&&getters(type,base,e);break;}}
        multi_stats=multi_stats&&found;
    }
    progress=backup;bool restored=!memcmp(&progress,&backup,sizeof(progress));
    bool multi_passed=multi_ok&&multi_stats&&restored&&added>0&&damage_count>0;ok=ok&&multi_passed;
    int damage_families=0;
    for(int i=0;i<damage_count;++i){Effect current;bool seen=false;if(!resolve(local_upgrade_level(damage_ids[i]),current)){ok=false;continue;}
        for(int j=0;j<i;++j){Effect previous;if(resolve(local_upgrade_level(damage_ids[j]),previous)&&previous.owner==current.owner)seen=true;}
        if(!seen)++damage_families;}
    fprintf(f,"{\"record\":\"multi_family_saved_check\",\"passed\":%s,\"added_fixture_levels\":%d,\"families\":%d,\"damage_families\":%d,\"damage_targets\":%d,\"memory_restored\":%s,\"disk_writes\":0}\n",multi_passed?"true":"false",added,saved_plan.count,damage_families,damage_count,restored?"true":"false");
    fprintf(f,"{\"record\":\"weapon_target_guards\",\"passed\":%s,\"unknown_name_refused\":true,\"wrong_reference_count_refused\":true,\"overflow_refused\":true}\n",target_guards?"true":"false");
    ok=ok&&target_guards&&legacy_compatible==64&&identity_guard&&preflight_cases>0&&!preflight_failures;
    fprintf(f,"{\"record\":\"summary\",\"levels\":%d,\"effect_supported\":%d,\"original_object_cases\":%d,\"object_failures\":%d,\"legacy_rifle_cases\":%d,\"legacy_levels_compatible\":%d,\"preflight_cases\":%d,\"preflight_failures\":%d,\"wrong_owner_refused\":%s,\"passed\":%s,\"purchase_calls\":0,\"battle_runs\":0}\n",total,effect_count,objects,object_failures,legacy,legacy_compatible,preflight_cases,preflight_failures,identity_guard?"true":"false",ok?"true":"false");
    fclose(f);LOG("Generic research batch passed=%d levels=%d object_cases=%d families_in_scenario=%d no_purchase=1 no_battle=1",ok,total,objects,matched);
}
}
