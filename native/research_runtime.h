// Included after the offline IL2CPP helpers and dictionary value reader.
#pragma once
#include "rifle_research_effects.h"
namespace local_research_runtime {
struct Roots {
    uintptr_t handles[32] = {}; int count = 0;
    ~Roots() { while (count) gchandle_free(handles[--count]); }
    bool keep(void* value) {
        if (!value || count == 32) return false;
        handles[count++] = gchandle_new(value, true); return true;
    }
    bool take(uintptr_t handle) {
        if (!handle || count == 32) return false;
        handles[count++] = handle; return true;
    }
};
static bool ascii(void* value, char* buffer, size_t capacity) {
    if (!value) return false;
    int n = *(int*)((char*)value + 16);
    if (n < 0 || (size_t)n >= capacity) return false;
    auto text = (uint16_t*)((char*)value + 20);
    for (int i = 0; i < n; ++i) { if (text[i] > 127) return false; buffer[i] = (char)text[i]; }
    buffer[n] = 0; return true;
}
static bool unit_level_delta(void* level, research_effects::RifleDelta& delta, void*& owner) {
    void* upgrade = level ? *(void**)((char*)level + 0x48) : nullptr;
    if (!upgrade || strcmp(class_name(object_class(upgrade)), "UpgradeUnit")) return false;
    owner = *(void**)((char*)upgrade + 0x48);
    void* faction = owner ? *(void**)((char*)owner + 0x20) : nullptr;
    if (!owner || *(int*)((char*)owner + 0x10) != 100 || !faction ||
        *(int*)((char*)faction + 0x10) != 2) return false;
    char description[4096];
    return ascii(*(void**)((char*)level + 0x18), description, sizeof(description)) &&
           research_effects::rifle_parse(description, *((int8_t*)upgrade + 0x21), delta);
}
static bool level_delta(void* level, research_effects::Delta& delta, void*& owner) {
    if (!level) return false;
    void* upgrade = *(void**)((char*)level + 0x48);
    if (!upgrade || strcmp(class_name(object_class(upgrade)), "UpgradeBuilding")) return false;
    owner = *(void**)((char*)upgrade + 0x48);
    void* faction = owner ? *(void**)((char*)owner + 0x20) : nullptr;
    // Only the active faction's headquarters has this verified building
    // effect handler; other building families remain explicitly unsupported.
    if (!owner || *(int8_t*)((char*)owner + 0x30) != 1 || !faction ||
        *(int*)((char*)faction + 0x10) != local_faction::active) return false;
    char description[4096];
    void* desc = *(void**)((char*)level + 0x18);
    bool parsed = ascii(desc, description, sizeof(description)) && research_effects::parse(description, delta);
    if(parsed&&(delta.units_base||delta.units_level)&&
       (*(int8_t*)((char*)upgrade+0x21)!=4||*(int*)((char*)owner+0x10)!=(local_faction::active==1?0:100)))return false;
    if (!parsed && desc) {
        auto chars = (uint16_t*)((char*)desc + 20);
        int length = *(int*)((char*)desc + 16);
        for (int i = 0; i < length && i < 256; ++i)
            if (chars[i] < 32 || chars[i] > 126) LOG("Research description id=%d separator[%d]=U+%04x", *(int*)((char*)level + 0x10), i, chars[i]);
        LOG("Research description not supported id=%d length=%d", *(int*)((char*)level + 0x10), length);
    }
    return parsed;
}
static bool gather(research_effects::Delta& sum, void*& owner,void* extra=nullptr) {
    void* config=saved(local_faction::key());void* field=config?class_field_from_name(object_class(config),"upgradeLevels"):nullptr;
    void* levels=field?*(void**)((char*)config+field_offset(field)):nullptr;int count=levels?*(int*)((char*)levels+0x18):0;
    owner=nullptr;
    for(int i=0;i<count;++i){void* level=list_item(levels,i);void* upgrade=level?*(void**)((char*)level+0x48):nullptr;
        if(!upgrade||strcmp(class_name(object_class(upgrade)),"UpgradeBuilding"))continue;
        void* candidate=*(void**)((char*)upgrade+0x48);void* faction=candidate?*(void**)((char*)candidate+0x20):nullptr;
        if(candidate&&faction&&*(int8_t*)((char*)candidate+0x30)==1&&*(int*)((char*)faction+0x10)==local_faction::active){owner=candidate;break;}}
    if(!owner)return false;
    bool included=false;
    for (uint32_t i = 0; i < progress.upgrade_count; ++i) {
        if(extra&&progress.upgrade_ids[i]==*(int*)((char*)extra+0x10))included=true;
        void* saved_level=local_upgrade_level(progress.upgrade_ids[i]);
        void* upgrade=saved_level?*(void**)((char*)saved_level+0x48):nullptr;
        // All unit families are validated/applied by generic_unit_research.
        // Do not reject another unit's saved research in the HQ-only gather.
        if(upgrade&&!strcmp(class_name(object_class(upgrade)),"UpgradeUnit"))continue;
        research_effects::Delta delta, next; void* candidate = nullptr;
        if (!level_delta(saved_level, delta, candidate)) {
            // Other building mechanisms are validated/applied as one plan by
            // building_research. Unknown saved effects still refuse there.
            if(upgrade&&!strcmp(class_name(object_class(upgrade)),"UpgradeBuilding"))continue;
            return false;
        }
        if (
            *(int*)((char*)candidate + 0x10) != *(int*)((char*)owner + 0x10) ||
            !research_effects::add(sum, delta, next)) return false;
        sum = next;
    }
    if(extra&&!included){research_effects::Delta d,next;void* candidate=nullptr;
        if(!level_delta(extra,d,candidate)||candidate!=owner||!research_effects::add(sum,d,next))return false;sum=next;}
    return true;
}
static void* clone(void* source, Roots& roots) {
    void* exception = nullptr;
    const void* method = class_method(find_class("System", "Object"), "MemberwiseClone", 0);
    void* copy = source && method ? runtime_invoke(method, source, nullptr, &exception) : nullptr;
    return !exception && roots.keep(copy) ? copy : nullptr;
}
static void* headquarters(void* side) {
    if (!side) return nullptr;
    void* faction = *(void**)((char*)side + 0xd8);
    if (!faction) return nullptr;
    int faction_id = *(int*)((char*)faction + 0x10);
    Roots roots; uintptr_t handle = 0;
    void* values = local_dictionary_values(*(void**)((char*)side + 0xf8),
        find_class("com.geargames.aow.entities", "BuildingType"), handle);
    if (!values || !roots.take(handle)) return nullptr;
    void* match = nullptr;
    for (size_t i = 0; i < array_length(values); ++i) {
        void* item = ((void**)((char*)values + 32))[i];
        void* fraction = item ? *(void**)((char*)item + 0x20) : nullptr;
        if (item && *(int8_t*)((char*)item + 0x30) == 1 && fraction &&
            *(int*)((char*)fraction + 0x10) == faction_id) {
            if (match) return nullptr;
            match = item;
        }
    }
    return match;
}
struct Metrics {
    bool valid = false; int hp[5] = {}, base = 0, increment = 0,units_base=0,units_level=0;
};
static Metrics metrics(void* side) {
    Metrics value; void* hq = headquarters(side);
    void* levels = hq ? *(void**)((char*)hq + 0x18) : nullptr;
    if (!levels || *(int*)((char*)levels + 0x18) != 5) return value;
    for (int i = 0; i < 5; ++i) {
        void* level = list_item(levels, i); if (!level) return value;
        value.hp[i] = *(int*)((char*)level + 0x34);
    }
    value.base = *(int*)((char*)side + 0x17c);
    value.increment = *(int*)((char*)side + 0x180);
    value.units_base=*(int8_t*)((char*)side+0x1a9);value.units_level=*(int8_t*)((char*)side+0x1aa);
    value.valid = true; return value;
}
static bool equal(const Metrics& a, const Metrics& b) {
    if (a.valid != b.valid || a.base != b.base || a.increment != b.increment||a.units_base!=b.units_base||a.units_level!=b.units_level) return false;
    for (int i = 0; i < 5; ++i) if (a.hp[i] != b.hp[i]) return false;
    return true;
}
static bool empty_player_buildings(void* battle, void* side) {
    // Current two fresh tutorial snapshots spawn buildings through GAI type IDs.
    // Do not silently give stale current HP to a future pre-populated HQ.
    void* alliances = *(void**)((char*)battle + 0x170);
    for (size_t i = 0; alliances && i < array_length(alliances); ++i) {
        void* alliance = ((void**)((char*)alliances + 32))[i];
        for (int offset : {0x88, 0xa0}) {
            void* list = alliance ? *(void**)((char*)alliance + offset) : nullptr;
            int count = list ? *(int*)((char*)list + 0x18) : 0;
            for (int j = 0; j < count; ++j) {
                void* building = list_item(list, j);
                if (building && *(void**)((char*)building + 0x18) == side) return false;
            }
        }
    }
    return true;
}
static bool apply(void* battle, int player, const research_effects::Delta& delta, void* baseline, void* prepared_side = nullptr) {
    LOG("Research prepare phase=lookup");
    void* sides = battle ? *(void**)((char*)battle + 0x178) : nullptr;
    void* side = prepared_side ? prepared_side : local_dictionary_item(sides, player);
    void* hq = headquarters(side);
    if (!hq || !baseline || (!prepared_side && !empty_player_buildings(battle, side))) return false;
    char model[256], original_model[256];
    if (!ascii(*(void**)((char*)hq + 0x38), model, sizeof(model)) ||
        !ascii(*(void**)((char*)baseline + 0x38), original_model, sizeof(original_model)) ||
        strcmp(model, original_model)) return false;
    void* source_levels = *(void**)((char*)hq + 0x18);
    void* base_levels = *(void**)((char*)baseline + 0x18);
    void* base_fraction = *(void**)((char*)baseline + 0x20);
    if (!source_levels || !base_levels || !base_fraction ||
        *(int*)((char*)source_levels + 0x18) != 5 || *(int*)((char*)base_levels + 0x18) != 5) return false;
    int hp[5];
    for (int i = 0; i < 5; ++i) {
        void* base_level = list_item(base_levels, i);
        if (!base_level || !list_item(source_levels, i)) return false;
        int64_t next = (int64_t)*(int*)((char*)base_level + 0x34) + delta.hp[i];
        if (next <= 0 || next > INT16_MAX) return false;
        hp[i] = (int)next;
    }
    int64_t income = (int64_t)*(int*)((char*)base_fraction + 0x28) + delta.income_base;
    int64_t increment = (int64_t)*(int*)((char*)base_fraction + 0x2c) + delta.income_level;
    if (income < 0 || increment < 0 || income > INT_MAX || increment > INT_MAX) return false;
    int units_base=0,units_level=0;
    if(!research_effects::capacity_values(*(int8_t*)((char*)base_fraction+0x38),*(int8_t*)((char*)base_fraction+0x39),delta,units_base,units_level))return false;
    Roots roots;
    LOG("Research prepare phase=clone");
    void* copy = clone(hq, roots);
    void* levels = object_new(object_class(source_levels));
    if (!copy || !roots.keep(levels)) return false;
    void* exception = nullptr;
    invoke_named(levels, ".ctor", 0, nullptr, &exception);
    for (int i = 0; i < 5 && !exception; ++i) {
        void* level = clone(list_item(source_levels, i), roots);
        if (!level) return false;
        *(int16_t*)((char*)level + 0x30) = (int16_t)hp[i];
        *(int*)((char*)level + 0x34) = hp[i];
        void* args[] = {level}; invoke_named(levels, "Add", 1, args, &exception);
    }
    if (exception) return false;
    LOG("Research prepare phase=dictionary");
    field_set(copy, class_field_from_name(object_class(copy), "levels"), levels);
    void* dictionary = *(void**)((char*)side + 0xf8);
    uintptr_t values_handle = 0;
    void* values = local_dictionary_values(dictionary,
        find_class("com.geargames.aow.entities", "BuildingType"), values_handle);
    if (!values || !roots.take(values_handle)) return false;
    void* replacements = object_new(object_class(dictionary));
    if (!roots.keep(replacements)) return false;
    invoke_named(replacements, ".ctor", 0, nullptr, &exception);
    for (size_t i = 0; i < array_length(values) && !exception; ++i) {
        void* item = ((void**)((char*)values + 32))[i];
        if (!item) return false;
        int key = *(int*)((char*)item + 0x10);
        if (local_dictionary_item(dictionary, key) != item) { LOG("Research dictionary key mismatch id=%d", key); return false; }
        void* args[] = {&key, item == hq ? copy : item};
        invoke_named(replacements, "Add", 2, args, &exception);
    }
    if (exception) { LOG("Research dictionary exception=%s", class_name(object_class(exception))); return false; }
    LOG("Research prepare phase=commit");
    // Only now publish the prepared player dictionary and its per-side income.
    // Shared faction objects, allies, enemies, and canonical baselines stay intact.
    field_set(side, class_field_from_name(object_class(side), "$Gg"), replacements);
    *(int*)((char*)side + 0x17c) = (int)income;
    *(int*)((char*)side + 0x180) = (int)increment;
    *(int8_t*)((char*)side+0x1a9)=units_base;*(int8_t*)((char*)side+0x1aa)=units_level;
    return true;
}
} // namespace local_research_runtime

#include "unit_research_runtime.h"
#include "mine_research_runtime.h"
#include "mine_growth_runtime.h"
#include "support_ability_runtime.h"
#include "generic_unit_research.h"
#include "building_research_runtime.h"
#include "building_weapon_research.h"
#include "preplaced_research_runtime.h"
#include "skill_combat_runtime.h"
#include "battle_growth_baseline.h"

static bool local_research_supported(void* level) {
    void* upgrade=level?*(void**)((char*)level+0x48):nullptr;
    if(mine_research::candidate(level))return mine_research::purchase_supported(level);
    if(mine_growth::candidate(level))return mine_growth::purchase_supported(level);
    if(support_ability::candidate(level))return support_ability::purchase_supported(level);
    if(upgrade&&!strcmp(class_name(object_class(upgrade)),"UpgradeUnit"))
        return generic_unit_research::purchase_supported(level);
    research_effects::Delta live, archived; void* owner = nullptr; void* stored_owner = nullptr;
    if (!local_research_runtime::level_delta(level, live, owner)) return building_research::purchase_supported(level)||building_weapon_research::purchase_supported(level);
    int id = *(int*)((char*)level + 0x10);
    if (!local_research_runtime::level_delta(local_upgrade_level(id), archived, stored_owner)) return false;
    for (int i = 0; i < 5; ++i) if (live.hp[i] != archived.hp[i]) return false;
    if(live.income_base!=archived.income_base||live.income_level!=archived.income_level||live.units_base!=archived.units_base||live.units_level!=archived.units_level)return false;
    if(live.units_base||live.units_level){research_effects::Delta sum;void* baseline=nullptr;int b=0,i=0;
        if(!local_research_runtime::gather(sum,baseline,local_upgrade_level(id)))return false;
        void* fraction=*(void**)((char*)baseline+0x20);
        return fraction&&research_effects::capacity_values(*(int8_t*)((char*)fraction+0x38),*(int8_t*)((char*)fraction+0x39),sum,b,i);}
    return true;
}

namespace local_mod_effects {static bool prepare_side(void*,const local_progress::State&);}
static bool apply_local_battle_research(void* battle, int player = 1, bool rebase_growth = false) {
    research_effects::Delta delta; void* owner = nullptr;
    auto side = local_dictionary_item(battle ? *(void**)((char*)battle + 0x178) : nullptr, player);
    local_research_runtime::Roots roots;
    void* prepared=local_research_runtime::clone(side,roots);
    if(!prepared)return false;
    // This is an unpublished side shell. Entity patches below still validate
    // against the original side/dictionaries, including alias protection.
    bool ok=(!rebase_growth||battle_growth_baseline::prepare(prepared,battle,side))&&local_research_runtime::gather(delta,owner)&&
        local_research_runtime::apply(battle,player,delta,owner,prepared)&&
        building_research::prepare_saved(prepared)&&
        generic_unit_research::prepare_saved(prepared)&&
        building_weapon_research::prepare_saved(prepared)&&
        mine_research::prepare_saved(prepared)&&
        mine_growth::prepare_saved(prepared)&&
        support_ability::prepare_saved(prepared)&&
        local_mod_effects::prepare_side(prepared,progress)&&
        (!local_skill::prepare_combat||local_skill::prepare_combat(prepared,progress));
    preplaced_research::Migration migration;
    migration.allow_mod_penalties=progress.modification_equipment_count>0;
    ok=ok&&migration.prepare(battle,side,prepared);
    auto stats=local_research_runtime::metrics(prepared);
    if(!ok||!stats.valid)return false;
    // Everything that can refuse/allocate is complete. Publish on the loading
    // thread before ToStartBattle, keeping all entity and list identities.
    field_set(side,class_field_from_name(object_class(side),"$gg"),*(void**)((char*)prepared+0xf0));
    field_set(side,class_field_from_name(object_class(side),"$Gg"),*(void**)((char*)prepared+0xf8));
    generic_unit_research::publish_hero_parameter(side,prepared);
    field_set(side,class_field_from_name(object_class(side),"$Eg"),*(void**)((char*)prepared+0xd8));
    field_set(side,class_field_from_name(object_class(side),"$hg"),*(void**)((char*)prepared+0x100));
    *(int*)((char*)side+0x17c)=stats.base;*(int*)((char*)side+0x180)=stats.increment;
    *(int8_t*)((char*)side+0x1a9)=stats.units_base;*(int8_t*)((char*)side+0x1aa)=stats.units_level;
    field_set(side,class_field_from_name(object_class(side),"$IH"),*(void**)((char*)prepared+0x3e8));
    *(int16_t*)((char*)side+0x218)=*(int16_t*)((char*)prepared+0x218);
    migration.commit();
    LOG("Local HQ command capacity prepared=1 side=%d base=%d increment=%d native_limits=1",player,stats.units_base,stats.units_level);
    LOG("Local combat research applied=1 levels=%u HQ=%d,%d,%d,%d,%d income=%d/%d preplaced=%d/%d weapon_links=%d queue_links=%d",
        progress.upgrade_count,stats.hp[0],stats.hp[1],stats.hp[2],stats.hp[3],stats.hp[4],stats.base,stats.increment,
        migration.units,migration.buildings,migration.weapon_links,migration.queue_links);
    return true;
}

static void verify_local_battle_research() {
    if (access(DATA_ROOT "research-effects-verify.enabled", F_OK)) return;
    FILE* file = fopen(DATA_ROOT "art3offline-research-combat-check.txt", "w");
    if (!file) return;
    chmod(DATA_ROOT "art3offline-research-combat-check.txt", 0644);
    research_effects::Delta delta; void* owner = nullptr;
    bool ok = local_research_runtime::gather(delta, owner);
    LOG("Research fixture gathered=%d owner=%p delta=%d/%d/%d", ok, owner, delta.hp[0], delta.income_base, delta.income_level);
    for (int chapter : {200, 201}) {
        void* message = read_scenario_message(chapter == 200 ?
            DATA_ROOT "offline-scenarios/resistance-001.ggs" : DATA_ROOT "offline-scenarios/resistance-002.ggs",
            5040, "@research-effects-fixture");
        void* battle = message ? *(void**)((char*)message + 0x20) : nullptr;
        void* sides = battle ? *(void**)((char*)battle + 0x178) : nullptr;
        auto before = local_research_runtime::metrics(local_dictionary_item(sides, 1));
        auto enemy = local_research_runtime::metrics(local_dictionary_item(sides, 2));
        auto ally = local_research_runtime::metrics(local_dictionary_item(sides, 3));
        bool applied = ok && local_research_runtime::apply(battle, 1, delta, owner);
        auto after = local_research_runtime::metrics(local_dictionary_item(sides, 1));
        bool isolated = local_research_runtime::equal(enemy, local_research_runtime::metrics(local_dictionary_item(sides, 2))) &&
                        local_research_runtime::equal(ally, local_research_runtime::metrics(local_dictionary_item(sides, 3)));
        bool again = applied && local_research_runtime::apply(battle, 1, delta, owner);
        bool idempotent = again && local_research_runtime::equal(after,
            local_research_runtime::metrics(local_dictionary_item(sides, 1)));
        bool exact = before.valid && after.valid && after.base == 61 && after.increment == 11;
        const int expected[] = {7878, 10107, 12378, 14643, 18035};
        for (int i = 0; i < 5; ++i) exact = exact && after.hp[i] == expected[i];
        fprintf(file, "chapter=%d applied=%d exact_current_two_research=%d other_seats_unchanged=%d repeated_apply_unchanged=%d\n",
            chapter, applied, exact, isolated, idempotent);
        fprintf(file, "HQ_before=%d,%d,%d,%d,%d HQ_after=%d,%d,%d,%d,%d income=%d/%d->%d/%d\n",
            before.hp[0], before.hp[1], before.hp[2], before.hp[3], before.hp[4],
            after.hp[0], after.hp[1], after.hp[2], after.hp[3], after.hp[4],
            before.base, before.increment, after.base, after.increment);
        ok = ok && applied && exact && isolated && idempotent;
    }
    fprintf(file, "PASS=%d; no save transaction or battle run started\n", ok);
    fclose(file);
    LOG("Local combat research fixture verification passed=%d", ok);
}
