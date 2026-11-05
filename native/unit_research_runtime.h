// Private player unit dictionary, prepared before the fresh battle is started.
#pragma once
namespace local_research_runtime {
// Native bot recruitment reads BuildingLevelType.Barracks, while player
// commands can resolve Side.UnitTypes directly. Keep both reference paths on
// the same PRIVATE upgraded prototypes. Never rewrite canonical level lists.
static bool relink_production_units(void* side) {
    Roots roots;
    void* units=*(void**)((char*)side+0xf0);
    void* dictionary=*(void**)((char*)side+0xf8);
    uintptr_t handle=0;
    void* values=local_dictionary_values(dictionary,find_class("com.geargames.aow.entities","BuildingType"),handle);
    if(!units||!values||!roots.take(handle)) return false;
    void* replacement=object_new(object_class(dictionary));
    if(!roots.keep(replacement)) return false;
    void* exception=nullptr; invoke_named(replacement,".ctor",0,nullptr,&exception);
    if(exception) return false;
    int changed_types=0,changed_levels=0,changed_references=0;
    for(size_t i=0;i<array_length(values);++i) {
        void* type=((void**)((char*)values+32))[i]; if(!type) return false;
        Roots type_roots;
        void* levels=*(void**)((char*)type+0x18);
        int n=levels?*(int*)((char*)levels+0x18):0;
        if(n<0||n>32) return false;
        bool type_changed=false;
        for(int l=0;l<n;++l) {
            void* level=list_item(levels,l); if(!level) return false;
            void* list=*(void**)((char*)level+0x20);
            int count=list?*(int*)((char*)list+0x18):0;
            if(count<0||count>256) return false;
            for(int k=0;k<count;++k) {
                void* u=list_item(list,k); if(!u) return false;
                void* mapped=local_dictionary_item(units,*(int*)((char*)u+0x10));
                if(!mapped) {LOG("Producer link missing unit=%d",*(int*)((char*)u+0x10));return false;}
                if(mapped!=u) type_changed=true;
            }
        }
        void* published=type;
        if(type_changed) {
            published=clone(type,type_roots);
            void* new_levels=object_new(object_class(levels));
            if(!published||!type_roots.keep(new_levels)) return false;
            invoke_named(new_levels,".ctor",0,nullptr,&exception);
            for(int l=0;!exception&&l<n;++l) {
                Roots level_roots;
                void* level=list_item(levels,l);
                void* list=*(void**)((char*)level+0x20);
                int count=list?*(int*)((char*)list+0x18):0;
                bool changed=false;
                for(int k=0;k<count;++k) {
                    void* u=list_item(list,k);
                    if(local_dictionary_item(units,*(int*)((char*)u+0x10))!=u) changed=true;
                }
                void* new_level=level;
                if(changed) {
                    new_level=clone(level,level_roots);
                    void* new_list=object_new(object_class(list));
                    if(!new_level||!level_roots.keep(new_list)) return false;
                    invoke_named(new_list,".ctor",0,nullptr,&exception);
                    for(int k=0;!exception&&k<count;++k) {
                        void* u=list_item(list,k);
                        void* mapped=local_dictionary_item(units,*(int*)((char*)u+0x10));
                        if(mapped!=u) ++changed_references;
                        void* args[]={mapped};invoke_named(new_list,"Add",1,args,&exception);
                    }
                    if(exception) return false;
                    field_set(new_level,class_field_from_name(object_class(new_level),"barracks"),new_list);
                    ++changed_levels;
                }
                void* args[]={new_level};invoke_named(new_levels,"Add",1,args,&exception);
            }
            if(exception) return false;
            field_set(published,class_field_from_name(object_class(published),"levels"),new_levels);
            ++changed_types;
        }
        int id=*(int*)((char*)type+0x10);
        void* args[]={&id,published};invoke_named(replacement,"Add",2,args,&exception);
        if(exception) return false;
    }
    if(changed_types) field_set(side,class_field_from_name(object_class(side),"$Gg"),replacement);
    LOG("Private producer links side=%d types=%d levels=%d unit_refs=%d",*(int*)((char*)side+0x10),changed_types,changed_levels,changed_references);
    return true;
}
static void* rifle(void* side) {
    if (!side) return nullptr;
    Roots roots; uintptr_t handle = 0;
    void* values = local_dictionary_values(*(void**)((char*)side + 0xf0),
        find_class("com.geargames.aow.entities", "UnitType"), handle);
    if (!values || !roots.take(handle)) return nullptr;
    void* match = nullptr;
    for (size_t i = 0; i < array_length(values); ++i) {
        void* type = ((void**)((char*)values + 32))[i]; char model[256];
        if (!type || !ascii(*(void**)((char*)type + 0x60), model, sizeof(model))) continue;
        if (strcmp(model, "f2/f2_inf_light")) continue;
        void* faction = *(void**)((char*)type + 0x20);
        if (!faction || *(int*)((char*)faction + 0x10) != 2) continue;
        if (match) return nullptr;
        match = type;
    }
    return match;
}
static bool gather_unit_effects(research_effects::RifleDelta& total, void*& baseline) {
    research_effects::RifleDelta ignored;
    if (!unit_level_delta(local_upgrade_level(10000), ignored, baseline)) return false;
    research_effects::RifleDelta sum;
    for (uint32_t i = 0; i < progress.upgrade_count; ++i) {
        void* level = local_upgrade_level(progress.upgrade_ids[i]);
        void* upgrade = level ? *(void**)((char*)level + 0x48) : nullptr;
        if (!upgrade) return false;
        if (strcmp(class_name(object_class(upgrade)), "UpgradeUnit")) continue;
        research_effects::RifleDelta delta, next; void* owner = nullptr;
        if (!unit_level_delta(level, delta, owner) || owner != baseline) return false;
        if (!research_effects::rifle_add(sum, delta, next)) return false;
        sum = next;
    }
    total = sum; return true;
}
static bool empty_player_units(void* battle, void* side) {
    void* units = *(void**)((char*)side + 0x140);
    if (units && array_length(units)) return false;
    void* alliances = *(void**)((char*)battle + 0x170);
    for (size_t i = 0; alliances && i < array_length(alliances); ++i) {
        void* alliance = ((void**)((char*)alliances + 32))[i];
        for (int offset : {0x78, 0x80}) {
            void* list = alliance ? *(void**)((char*)alliance + offset) : nullptr;
            int n = list ? *(int*)((char*)list + 0x18) : 0;
            for (int j = 0; j < n; ++j) {
                void* unit = list_item(list, j);
                if (unit && *(void**)((char*)unit + 0x18) == side) return false;
            }
        }
    }
    return true;
}
static bool rifle_stats(void* type, research_effects::RifleDelta& stats) {
    void* states = type ? *(void**)((char*)type + 0x18) : nullptr;
    if (!states || *(int*)((char*)states + 0x18) != 1) return false;
    void* state = list_item(states, 0);
    void* weapons = state ? *(void**)((char*)state + 0x18) : nullptr;
    if (!weapons || *(int*)((char*)weapons + 0x18) != 2) return false;
    research_effects::RifleDelta out;
    out.hp = *(int*)((char*)type + 0x34);
    out.armor = *(int16_t*)((char*)state + 0x22);
    out.speed = *(int8_t*)((char*)state + 0x58);
    out.sight = *(int*)((char*)state + 0x4c);
    for (int w=0;w<2;++w) {
        void* weapon = list_item(weapons,w);
        if(!weapon || *(int16_t*)((char*)weapon+0x24)!=(w?20:10)) return false;
        for(int t=0;t<3;++t) out.damage[w][t]=*(int*)((char*)weapon+0x28+t*4);
    }
    stats=out; return true;
}
static void* new_list(void* source, Roots& roots) {
    if (!source) return nullptr;
    void* list=object_new(object_class(source));
    if(!roots.keep(list)) return nullptr;
    void* exception=nullptr; invoke_named(list,".ctor",0,nullptr,&exception);
    return exception?nullptr:list;
}
static bool apply_unit_effects(void* battle, int player, const research_effects::RifleDelta& delta, void* baseline) {
    void* side = local_dictionary_item(battle ? *(void**)((char*)battle + 0x178) : nullptr, player);
    void* original = rifle(side);
    if (!original || !baseline || !empty_player_units(battle, side)) return false;
    research_effects::RifleDelta base, source, next;
    if (!rifle_stats(baseline,base) || !rifle_stats(original,source) ||
        !research_effects::rifle_add(base,delta,next) || next.hp<=0 || next.hp>INT16_MAX ||
        next.armor>INT16_MAX || next.speed<=0 || next.speed>INT8_MAX) return false;
    Roots roots; void* copy = clone(original, roots);
    if (!copy) return false;
    *(int16_t*)((char*)copy + 0x32) = (int16_t)next.hp;
    *(int*)((char*)copy + 0x34) = next.hp;
    void* source_states=*(void**)((char*)original+0x18);
    void* source_state=list_item(source_states,0);
    void* states=new_list(source_states,roots);
    void* state=clone(source_state,roots);
    void* source_weapons=*(void**)((char*)source_state+0x18);
    void* weapons=new_list(source_weapons,roots);
    if(!states || !state || !weapons) return false;
    void* exception=nullptr;
    for(int w=0;w<2;++w) {
        void* weapon=clone(list_item(source_weapons,w),roots);
        if(!weapon) return false;
        for(int t=0;t<3;++t) *(int*)((char*)weapon+0x28+t*4)=next.damage[w][t];
        void* args[]={weapon}; invoke_named(weapons,"Add",1,args,&exception);
    }
    if(exception) return false;
    *(int16_t*)((char*)state+0x22)=(int16_t)next.armor;
    *(int8_t*)((char*)state+0x58)=(int8_t)next.speed;
    *(int*)((char*)state+0x4c)=next.sight;
    field_set(state,class_field_from_name(object_class(state),"weapons"),weapons);
    void* state_args[]={state}; invoke_named(states,"Add",1,state_args,&exception);
    if(exception) return false;
    field_set(copy,class_field_from_name(object_class(copy),"states"),states);
    void* dictionary = *(void**)((char*)side + 0xf0); uintptr_t handle = 0;
    void* values = local_dictionary_values(dictionary,
        find_class("com.geargames.aow.entities", "UnitType"), handle);
    if (!values || !roots.take(handle)) return false;
    void* replacement = object_new(object_class(dictionary));
    if (!roots.keep(replacement)) return false;
    exception = nullptr;
    invoke_named(replacement, ".ctor", 0, nullptr, &exception);
    for (size_t i = 0; !exception && i < array_length(values); ++i) {
        void* item = ((void**)((char*)values + 32))[i]; if (!item) return false;
        int id = *(int*)((char*)item + 0x10);
        if (local_dictionary_item(dictionary, id) != item) return false;
        void* args[] = {&id, item == original ? copy : item};
        invoke_named(replacement, "Add", 2, args, &exception);
    }
    if (exception) return false;
    field_set(side, class_field_from_name(object_class(side), "$gg"), replacement);
    if(!relink_production_units(side)) return false;
    LOG("Local rifle research player=%d type=%d HP=%d armor=%d speed=%d sight=%d damage=%d/%d/%d;%d/%d/%d",
        player, *(int*)((char*)copy + 0x10), next.hp,next.armor,next.speed,next.sight,
        next.damage[0][0],next.damage[0][1],next.damage[0][2],next.damage[1][0],next.damage[1][1],next.damage[1][2]);
    return true;
}
static bool apply_unit_health(void* battle,int player,int hp,void* baseline) {
    research_effects::RifleDelta delta; delta.hp=hp;
    return apply_unit_effects(battle,player,delta,baseline);
}
static bool apply_saved_unit_health(void* battle, int player) {
    research_effects::RifleDelta delta; void* baseline = nullptr;
    if (!gather_unit_effects(delta, baseline)) return false;
    return !research_effects::rifle_any(delta) || apply_unit_effects(battle, player, delta, baseline);
}
} // namespace local_research_runtime

static uint64_t unit_dictionary_health_hash(void* side) {
    if (!side) return 0;
    local_research_runtime::Roots roots; uintptr_t handle = 0;
    void* values = local_dictionary_values(*(void**)((char*)side + 0xf0),
        find_class("com.geargames.aow.entities", "UnitType"), handle);
    if (!values || !roots.take(handle)) return UINT64_MAX;
    uint64_t hash = 1469598103934665603ull;
    for (size_t i = 0; i < array_length(values); ++i) {
        void* type = ((void**)((char*)values + 32))[i]; if (!type) return UINT64_MAX;
        for (int offset : {0x10, 0x34}) hash = (hash ^ *(uint32_t*)((char*)type + offset)) * 1099511628211ull;
        hash = (hash ^ *(uint16_t*)((char*)type + 0x32)) * 1099511628211ull;
        void* states=*(void**)((char*)type+0x18);
        int n=states?*(int*)((char*)states+0x18):0;
        for(int j=0;j<n;++j) {
            void* state=list_item(states,j); if(!state) return UINT64_MAX;
            for(int value : {(int)*(int16_t*)((char*)state+0x22), (int)*(int8_t*)((char*)state+0x58), *(int*)((char*)state+0x4c)})
                hash=(hash^(uint32_t)value)*1099511628211ull;
            void* weapons=*(void**)((char*)state+0x18);
            int m=weapons?*(int*)((char*)weapons+0x18):0;
            for(int k=0;k<m;++k) {
                void* weapon=list_item(weapons,k); if(!weapon) return UINT64_MAX;
                for(int offset:{0x28,0x2c,0x30}) hash=(hash^*(uint32_t*)((char*)weapon+offset))*1099511628211ull;
            }
        }
    }
    return hash;
}
static bool original_stat_equals(void* object,const char* method,int width,int expected) {
    void* exception=nullptr; void* box=invoke_named(object,method,0,nullptr,&exception);
    if(exception || !box) return false;
    int value=width==1?*(int8_t*)((char*)box+16):width==2?*(int16_t*)((char*)box+16):*(int*)((char*)box+16);
    return value==expected;
}
static bool rifle_original_getters_equal(void* type,const research_effects::RifleDelta& expected) {
    void* state=type?list_item(*(void**)((char*)type+0x18),0):nullptr;
    if(!type || !state || !original_stat_equals(type,"get_LifeInt",4,expected.hp) ||
        !original_stat_equals(type,"get_Life",2,expected.hp) ||
        !original_stat_equals(state,"get_Armor",2,expected.armor) ||
        !original_stat_equals(state,"get_Speed",1,expected.speed) ||
        !original_stat_equals(state,"get_Sight",4,expected.sight)) return false;
    const char* names[]={"get_DamageLight","get_DamageMedium","get_DamageHeavy"};
    void* weapons=*(void**)((char*)state+0x18);
    for(int w=0;w<2;++w) for(int t=0;t<3;++t)
        if(!original_stat_equals(list_item(weapons,w),names[t],4,expected.damage[w][t])) return false;
    return true;
}
static void verify_local_rifle_batch() {
    if(access(DATA_ROOT "research-effects-verify.enabled",F_OK)) return;
    FILE* f=fopen(DATA_ROOT "art3offline-rifle-batch-check.txt","w"); if(!f) return;
    chmod(DATA_ROOT "art3offline-rifle-batch-check.txt",0644);
    research_effects::RifleDelta first; void* canonical=nullptr;
    bool ok=local_research_runtime::unit_level_delta(local_upgrade_level(10000),first,canonical);
    research_effects::RifleDelta base;
    ok=ok && local_research_runtime::rifle_stats(canonical,base);
    void* dictionary=saved("RESISTANCE");
    void* field=dictionary?class_field_from_name(object_class(dictionary),"upgradeLevels"):nullptr;
    void* levels=field?*(void**)((char*)dictionary+field_offset(field)):nullptr;
    int count=levels?*(int*)((char*)levels+0x18):0;
    fprintf(f,"Independent in-memory fixtures; no purchases, save writes or battle runs.\n");
    for(int chapter:{200,201}) {
        void* message=read_scenario_message(chapter==200?DATA_ROOT "offline-scenarios/resistance-001.ggs":DATA_ROOT "offline-scenarios/resistance-002.ggs",
            5040,chapter==200?"@rifle-batch-200":"@rifle-batch-201");
        void* battle=message?*(void**)((char*)message+0x20):nullptr;
        void* sides=battle?*(void**)((char*)battle+0x178):nullptr;
        void* player=local_dictionary_item(sides,1);
        uint64_t hashes[4]; void* dicts[4]={};
        for(int s=0;s<4;++s) {void* side=local_dictionary_item(sides,s); hashes[s]=unit_dictionary_health_hash(side); dicts[s]=side?*(void**)((char*)side+0xf0):nullptr;}
        local_research_runtime::Roots roots;
        bool chapter_ok=ok && player && roots.keep(dicts[1]);
        int candidates=0,passed=0,rejected=0;
        research_effects::RifleDelta sum;
        for(int i=0;i<count && chapter_ok;++i) {
            void* level=list_item(levels,i); void* upgrade=level?*(void**)((char*)level+0x48):nullptr;
            if(!upgrade || strcmp(class_name(object_class(upgrade)),"UpgradeUnit") || *(void**)((char*)upgrade+0x48)!=canonical) continue;
            ++candidates;
            research_effects::RifleDelta delta,expected,next; void* owner=nullptr;
            if(!local_research_runtime::unit_level_delta(level,delta,owner)) {++rejected;continue;}
            bool applied=research_effects::rifle_add(base,delta,expected) &&
                local_research_runtime::apply_unit_effects(battle,1,delta,canonical);
            void* type=local_research_runtime::rifle(player);
            bool exact=applied && rifle_original_getters_equal(type,expected);
            bool repeat=exact && local_research_runtime::apply_unit_effects(battle,1,delta,canonical) &&
                rifle_original_getters_equal(local_research_runtime::rifle(player),expected);
            bool isolated=rifle_original_getters_equal(canonical,base);
            for(int s:{0,2,3}) {
                void* side=local_dictionary_item(sides,s);
                isolated=isolated && hashes[s]!=UINT64_MAX && hashes[s]==unit_dictionary_health_hash(side) &&
                    dicts[s]==(side?*(void**)((char*)side+0xf0):nullptr);
            }
            fprintf(f,"chapter=%d level=%d original_getters_exact=%d idempotent=%d other_seats_and_canonical_unchanged=%d\n",
                chapter,*(int*)((char*)level+0x10),exact,repeat,isolated);
            chapter_ok=exact && repeat && isolated && research_effects::rifle_add(sum,delta,next);
            if(chapter_ok) {sum=next;++passed;}
        }
        research_effects::RifleDelta expected;
        bool cumulative=chapter_ok && research_effects::rifle_add(base,sum,expected) &&
            local_research_runtime::apply_unit_effects(battle,1,sum,canonical) &&
            rifle_original_getters_equal(local_research_runtime::rifle(player),expected);
        void* before=player?*(void**)((char*)player+0xf0):nullptr;
        research_effects::RifleDelta invalid; invalid.armor=INT_MAX;
        bool refused=player && !local_research_runtime::apply_unit_effects(battle,1,invalid,canonical) && before==*(void**)((char*)player+0xf0);
        fprintf(f,"SUMMARY chapter=%d candidates=%d supported_passed=%d unsupported_rejected=%d cumulative=%d overflow_no_publish=%d\n",
            chapter,candidates,passed,rejected,cumulative,refused);
        ok=ok && chapter_ok && candidates==105 && passed==64 && rejected==41 && cumulative && refused;
    }
    fprintf(f,"PASS=%d\n",ok); fclose(f);
    LOG("Rifle research family batch fixture passed=%d",ok);
}
// One focused export per requested family, using already-restored local data.
static void export_rifle_research_catalog() {
    if (access(DATA_ROOT "unit-research-catalog.enabled", F_OK)) return;
    FILE* f = fopen(DATA_ROOT "art3offline-rifle-research-catalog.txt", "w");
    if (!f) return;
    void* dictionary = saved("RESISTANCE");
    void* field = dictionary ? class_field_from_name(object_class(dictionary), "upgradeLevels") : nullptr;
    void* levels = field ? *(void**)((char*)dictionary + field_offset(field)) : nullptr;
    int n = levels ? *(int*)((char*)levels + 0x18) : 0, exported = 0;
    void* canonical = nullptr;
    for (int i = 0; i < n; ++i) {
        void* level = list_item(levels, i);
        void* upgrade = level ? *(void**)((char*)level + 0x48) : nullptr;
        if (!upgrade || strcmp(class_name(object_class(upgrade)), "UpgradeUnit")) continue;
        void* owner = *(void**)((char*)upgrade + 0x48);
        if (!owner || *(int*)((char*)owner + 0x10) != 100) continue;
        canonical = owner; char desc[4096], name[256];
        if (!local_research_runtime::ascii(*(void**)((char*)level + 0x18), desc, sizeof(desc)) ||
            !local_research_runtime::ascii(*(void**)((char*)upgrade + 0x28), name, sizeof(name))) continue;
        // A single physical line preserves separators as a space for indexing.
        for (char* p = desc; *p; ++p) if (*p == '\n' || *p == '\r') *p = ' ';
        fprintf(f, "LEVEL id=%d upgrade=%d slot=%d order=%d cost=%d xp=%d name=%s desc=%s\n",
            *(int*)((char*)level + 0x10), *(int*)((char*)upgrade + 0x10),
            *((int8_t*)upgrade + 0x21), *((int8_t*)upgrade + 0x20),
            *(int*)((char*)level + 0x20), *(int*)((char*)level + 0x28), name, desc);
        ++exported;
    }
    auto dump_type = [&](const char* label, void* type) {
        fprintf(f, "%s\n", label); visited_count = 0; dump_object(f, type, 0, 1);
        void* states = type ? *(void**)((char*)type + 0x18) : nullptr;
        int count = states ? *(int*)((char*)states + 0x18) : 0;
        for (int i = 0; i < count && i < 16; ++i) {
            fprintf(f, "STATE_INDEX %d\n", i); visited_count = 0; dump_object(f, list_item(states, i), 0, 3);
            void* state = list_item(states, i);
            void* weapons = state ? *(void**)((char*)state + 0x18) : nullptr;
            int weapon_count = weapons ? *(int*)((char*)weapons + 0x18) : 0;
            for (int j = 0; j < weapon_count && j < 8; ++j) {
                fprintf(f, "WEAPON_INDEX %d\n", j); visited_count = 0; dump_object(f, list_item(weapons, j), 0, 1);
            }
        }
    };
    dump_type("CANONICAL_RIFLE", canonical);
    for (int chapter : {200, 201}) {
        void* message = read_scenario_message(chapter == 200 ?
            DATA_ROOT "offline-scenarios/resistance-001.ggs" : DATA_ROOT "offline-scenarios/resistance-002.ggs",
            5040, chapter == 200 ? "@rifle-catalog-200" : "@rifle-catalog-201");
        void* battle = message ? *(void**)((char*)message + 0x20) : nullptr;
        void* sides = battle ? *(void**)((char*)battle + 0x178) : nullptr;
        fprintf(f, "SCENARIO %d\n", chapter);
        dump_type("PLAYER_RIFLE", local_research_runtime::rifle(local_dictionary_item(sides, 1)));
    }
    fprintf(f, "EXPORTED=%d; no purchase, save or battle run\n", exported);
    fclose(f); chmod(DATA_ROOT "art3offline-rifle-research-catalog.txt", 0644);
}
static void verify_local_unit_research() {
    if (access(DATA_ROOT "research-effects-verify.enabled", F_OK)) return;
    FILE* file = fopen(DATA_ROOT "art3offline-unit-research-check.txt", "w");
    if (!file) return;
    chmod(DATA_ROOT "art3offline-unit-research-check.txt", 0644);
    research_effects::RifleDelta first; void* baseline = nullptr;
    bool ok = local_research_runtime::unit_level_delta(local_upgrade_level(10000), first, baseline);
    int delta=first.hp;
    fprintf(file, "fixture_only upgrade=10000 delta=%d canonicalHP=%d; no purchase or save change\n",
        delta, baseline ? *(int*)((char*)baseline + 0x34) : -1);
    for (int chapter : {200, 201}) {
        void* message = read_scenario_message(chapter == 200 ?
            DATA_ROOT "offline-scenarios/resistance-001.ggs" : DATA_ROOT "offline-scenarios/resistance-002.ggs",
            5040, "@unit-research-fixture");
        void* battle = message ? *(void**)((char*)message + 0x20) : nullptr;
        void* sides = battle ? *(void**)((char*)battle + 0x178) : nullptr;
        void* player = local_dictionary_item(sides, 1);
        void* unit = local_research_runtime::rifle(player);
        int before = unit ? *(int*)((char*)unit + 0x34) : -1;
        uint64_t hashes[4]; void* dictionaries[4] = {};
        for (int i = 0; i < 4; ++i) {
            void* side = local_dictionary_item(sides, i);
            hashes[i] = unit_dictionary_health_hash(side);
            dictionaries[i] = side ? *(void**)((char*)side + 0xf0) : nullptr;
        }
        bool applied = ok && local_research_runtime::apply_unit_health(battle, 1, delta, baseline);
        void* after_unit = local_research_runtime::rifle(player);
        int after = after_unit ? *(int*)((char*)after_unit + 0x34) : -1;
        bool exact = after_unit && after == 854 && *(int16_t*)((char*)after_unit + 0x32) == 854;
        bool isolated = baseline && *(int*)((char*)baseline + 0x34) == 840;
        for (int i : {0, 2, 3}) {
            void* side = local_dictionary_item(sides, i);
            isolated = isolated && hashes[i] != UINT64_MAX && hashes[i] == unit_dictionary_health_hash(side) &&
                dictionaries[i] == (side ? *(void**)((char*)side + 0xf0) : nullptr);
        }
        uint64_t after_hash = unit_dictionary_health_hash(player);
        bool again = applied && local_research_runtime::apply_unit_health(battle, 1, delta, baseline);
        bool idempotent = again && after_hash == unit_dictionary_health_hash(player);
        void* published = player ? *(void**)((char*)player + 0xf0) : nullptr;
        bool refused = player && !local_research_runtime::apply_unit_health(battle, 1, INT16_MAX, baseline) &&
            published == *(void**)((char*)player + 0xf0);
        fprintf(file, "chapter=%d beforeHP=%d afterHP=%d applied=%d exact=%d other_seats_and_canonical_unchanged=%d idempotent=%d overflow_refused_without_publish=%d\n",
            chapter, before, after, applied, exact, isolated, idempotent, refused);
        ok = ok && applied && exact && isolated && idempotent && refused;
    }
    fprintf(file, "PASS=%d\n", ok); fclose(file);
    LOG("Local unit research fixture verification passed=%d", ok);
}
static void audit_local_unit_health() {
    if (!local_run_active || access(DATA_ROOT "research-live-audit.enabled", F_OK)) return;
    static uint64_t audited_run;
    if (audited_run == local_run) return;
    void* own = static_object("com.geargames.aow.scenes.battle", "ClientBattleContext", "m_ownSide");
    void* units = own ? *(void**)((char*)own + 0x48) : nullptr;
    int count = units ? *(int*)((char*)units + 0x18) : 0;
    for (int i = 0; i < count; ++i) {
        void* unit = list_item(units, i);
        void* type = unit ? *(void**)((char*)unit + 0x78) : nullptr; char model[256];
        if (!type || !local_research_runtime::ascii(*(void**)((char*)type + 0x60), model, sizeof(model)) ||
            strcmp(model, "f2/f2_inf_light") || *(void**)((char*)unit + 0x70) != own) continue;
        int max_hp = reinterpret_cast<int(*)(void*,void*)>(image_base + 0x80f433c)(unit, nullptr);
        int health = *(int*)((char*)unit + 0x118);
        FILE* file = fopen(DATA_ROOT "art3offline-unit-research-live.txt", "w");
        if (file) {
            fprintf(file, "run=%llu chapter=%d type=%d original_engine_max_health=%d current_health=%d prototypeHP=%d\n",
                (unsigned long long)local_run, local_mission_id, *(int*)((char*)type + 0x10),
                max_hp, health, *(int*)((char*)type + 0x34));
            fclose(file); chmod(DATA_ROOT "art3offline-unit-research-live.txt", 0644);
        }
        LOG("Research live rifle run=%llu originalMaxHealth=%d currentHealth=%d", (unsigned long long)local_run, max_hp, health);
        audited_run = local_run; return;
    }
}
