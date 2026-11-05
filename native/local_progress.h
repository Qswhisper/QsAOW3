// Versioned, checksummed local campaign state. No Unity pointers or guest IDs.
#pragma once
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stddef.h>
#include <limits.h>
#include <sys/stat.h>
#include <new>
#include "supply_types.h"
#include "obsolete_reward_policy.h"
#include "shop_box_types.h"
#include "contract_box_types.h"
#include "competition_track_types.h"

namespace local_progress {
constexpr uint32_t kVersion = 16, kCapacity = 256, kUpgradeCapacity = 8192, kLegacySize = 8256, kV2Size = 41024, kV5Size = 45128, kV6Size=47184, kV7Size=51288, kV8Size=52336,kV9Size=52344,kV10Size=54064,kV11Size=54064,kV12Size=54584,kV13Size=54592,kV14Size=56328,kV15Size=58072;
constexpr uint32_t kSkillCapacity=512;
constexpr uint32_t kQuestCapacity=512;
constexpr uint32_t kCareerCapacity = 256, kStockCapacity = 256;
struct Stock { int32_t type, id, amount; };
struct CareerReward { int32_t type, id, amount; };
struct QuestReceipt { int32_t id,state; }; // Original state 8=read, 32=claimed.
struct ContractReceipt {int32_t id;uint32_t claims;uint64_t baseline;};
struct ModificationEquipment {int32_t unit,modification;};
struct Mission {
    int32_t id, wins, losses, stars, xp, credits;
    uint32_t objective_count, objectives;
};
struct State {
    uint32_t magic, version, size, checksum;
    uint64_t generation, next_run, settled_run;
    int32_t credits, gold, experience, stars;
    uint32_t count, upgrade_count;
    Mission missions[kCapacity];
    int32_t upgrade_ids[kUpgradeCapacity];
    uint32_t career_count, stock_count;
    int32_t career_ids[kCareerCapacity];
    Stock stocks[kStockCapacity];
    uint32_t skill_count;
    int32_t skill_nodes[kSkillCapacity];
    uint32_t v6_padding; // Preserve the old tail bytes before appending v7 data.
    uint32_t quest_count;
    QuestReceipt quests[kQuestCapacity];
    int32_t quest_warpoints;
    uint32_t contract_count;
    int32_t contract_points;
    uint64_t contract_pvp_cursor,contract_pvp_wins;
    ContractReceipt contracts[64];
    uint64_t warpoint_pvp_cursor;
    supply_types::Ledger supply;
    uint32_t modification_equipment_count,modification_reserved;
    ModificationEquipment modification_equipment[64];
    uint64_t cooperative_credit_cursor;
    shop_box_policy::Ledger shop;
    contract_box_types::Ledger contract_boxes;
    competition_track_types::Ledger competition_tracks;
};
static_assert(sizeof(Mission) == 32 && offsetof(State, career_count) == kV2Size && offsetof(State,skill_count)==kV5Size && offsetof(State,quest_count)==kV6Size && offsetof(State,contract_count)==kV7Size && offsetof(State,warpoint_pvp_cursor)==kV8Size && offsetof(State,supply)==kV9Size && offsetof(State,modification_equipment_count)==kV11Size && offsetof(State,cooperative_credit_cursor)==kV12Size && offsetof(State,shop)==kV13Size && offsetof(State,contract_boxes)==kV14Size && offsetof(State,competition_tracks)==kV15Size && sizeof(State) == 60280, "Save ABI changed");
// Unity's loading thread has a small stack. Save slots exceed 53 KiB; nested
// loader/API frames must not place several complete slots on that stack.
struct StateBuffer {
    State* data=new(std::nothrow) State{};
    ~StateBuffer(){delete data;}
    StateBuffer()=default;StateBuffer(const StateBuffer&)=delete;StateBuffer& operator=(const StateBuffer&)=delete;
};
inline uint32_t checksum_bytes(const State& state, size_t size) {
    uint32_t h = 2166136261u;
    for (size_t i = 0; i < size; ++i)
        h = (h ^ (i >= 12 && i < 16 ? 0 : ((const uint8_t*)&state)[i])) * 16777619u;
    return h;
}
inline uint32_t checksum(const State& state) { return checksum_bytes(state, sizeof(state)); }
inline bool valid(const State& s) {
    if (s.magic != 0x33545241 || s.version != kVersion || s.size != sizeof(s) ||
        s.count > kCapacity || s.upgrade_count > kUpgradeCapacity || s.checksum != checksum(s) || s.credits < 0 ||
        s.gold < 0 || s.experience < 0 || s.stars < 0 || s.settled_run > s.next_run ||
        s.career_count > kCareerCapacity || s.stock_count > kStockCapacity || s.skill_count>kSkillCapacity || s.quest_count>kQuestCapacity || s.quest_warpoints<0 || s.contract_count>64 || s.contract_points<0 || !supply_types::valid(s.supply)||s.modification_equipment_count>64||s.modification_reserved||!shop_box_policy::valid(s.shop)||!contract_box_types::valid(s.contract_boxes,s.contract_points)||!competition_track_types::valid(s.competition_tracks)) return false;
    for (uint32_t i = 0; i < s.count; ++i) {
        const auto& m = s.missions[i];
        if (m.id < 0 || m.wins < 0 || m.losses < 0 || m.stars < 0 || m.stars > 3 ||
            m.xp < 0 || m.xp > 32767 || m.credits < 0 || m.objective_count > 32) return false;
        for (uint32_t j = 0; j < i; ++j) if (s.missions[j].id == m.id) return false;
    }
    for (uint32_t i = 0; i < s.upgrade_count; ++i) {
        if (s.upgrade_ids[i] < 0) return false;
        for (uint32_t j = 0; j < i; ++j) if (s.upgrade_ids[j] == s.upgrade_ids[i]) return false;
    }
    for (uint32_t i = 0; i < s.career_count; ++i) {
        if (s.career_ids[i] < 0) return false;
        for (uint32_t j = 0; j < i; ++j) if (s.career_ids[i] == s.career_ids[j]) return false;
    }
    for (uint32_t i = 0; i < s.stock_count; ++i) {
        const auto& v = s.stocks[i];
        if (!inventory_types::stock(v.type) || v.id < 0 || v.amount <= 0) return false;
        for (uint32_t j = 0; j < i; ++j) if (s.stocks[j].type == v.type && s.stocks[j].id == v.id) return false;
    }
    for(uint32_t i=0;i<s.modification_equipment_count;++i){const auto& e=s.modification_equipment[i];if(e.unit<0||e.modification<=0)return false;bool owned=false;
        for(uint32_t j=0;j<s.stock_count;++j)if(s.stocks[j].type==17&&s.stocks[j].id==e.modification)owned=true;
        if(!owned)return false;for(uint32_t j=0;j<i;++j)if(s.modification_equipment[j].unit==e.unit)return false;}
    for(uint32_t i=0;i<s.skill_count;++i){if(s.skill_nodes[i]<=0)return false;
        for(uint32_t j=0;j<i;++j)if(s.skill_nodes[j]==s.skill_nodes[i])return false;}
    for(uint32_t i=0;i<s.quest_count;++i){if(s.quests[i].id<0||(s.quests[i].state!=8&&s.quests[i].state!=32))return false;
        for(uint32_t j=0;j<i;++j)if(s.quests[j].id==s.quests[i].id)return false;}
    for(uint32_t i=0;i<s.contract_count;++i){if(s.contracts[i].id<0)return false;
        for(uint32_t j=0;j<i;++j)if(s.contracts[j].id==s.contracts[i].id)return false;}
    return true;
}
inline State seed(int credits, int gold, int xp, int stars) {
    State s{}; s.magic = 0x33545241; s.version = kVersion; s.size = sizeof(s);
    s.credits = credits; s.gold = gold; s.experience = xp; s.stars = stars;
    s.checksum = checksum(s); return s;
}
inline bool read(const char* path, State& s) {
    FILE* f = fopen(path, "rb"); if (!f) return false;
    StateBuffer storage;if(!storage.data){fclose(f);return false;}State& candidate=*storage.data;
    bool ok = fread(&candidate, 1, 16, f) == 16;
    bool legacy = candidate.version == 1 && candidate.size == kLegacySize;
    bool v2 = candidate.version == 2 && candidate.size == kV2Size;
    bool v3 = candidate.version == 3 && candidate.size == kV5Size;
    bool v4 = candidate.version == 4 && candidate.size == kV5Size;
    bool v5 = candidate.version == 5 && candidate.size == kV5Size;
    bool v6 = candidate.version == 6 && candidate.size == kV6Size;
    bool v7 = candidate.version == 7 && candidate.size == kV7Size;
    bool v8 = candidate.version == 8 && candidate.size == kV8Size;
    bool v9 = candidate.version == 9 && candidate.size == kV9Size;
    bool v10 = candidate.version == 10 && candidate.size == kV10Size;
    bool v11 = candidate.version == 11 && candidate.size == kV11Size;
    bool v12 = candidate.version == 12 && candidate.size == kV12Size;
    bool v13 = candidate.version == 13 && candidate.size == kV13Size;
    bool v14 = candidate.version == 14 && candidate.size == kV14Size;
    bool v15 = candidate.version == 15 && candidate.size == kV15Size;
    size_t size = legacy ? kLegacySize : v2 ? kV2Size : (v3||v4||v5)?kV5Size:v6?kV6Size:v7?kV7Size:v8?kV8Size:v9?kV9Size:(v10||v11)?kV11Size:v12?kV12Size:v13?kV13Size:v14?kV14Size:v15?kV15Size:sizeof(candidate);
    ok = ok && (legacy || v2 || v3 || v4 || v5 || v6 || v7 || v8 || v9 || v10 || v11 || v12 || v13 || v14 || v15 || (candidate.version == kVersion && candidate.size == sizeof(candidate)));
    ok = ok && fread((char*)&candidate + 16, 1, size - 16, f) == size - 16 && fgetc(f) == EOF;
    fclose(f);
    if (ok && (legacy || v2 || v3 || v4 || v5 || v6 || v7 || v8 || v9 || v10 || v11 || v12 || v13 || v14 || v15)) {
        if (candidate.checksum != checksum_bytes(candidate, size) || (legacy && candidate.upgrade_count)) return false;
        if(v3){if(candidate.stock_count>kStockCapacity)return false;for(uint32_t i=0;i<candidate.stock_count;++i)if(candidate.stocks[i].type!=4)return false;}
        if(v4){if(candidate.stock_count>kStockCapacity)return false;for(uint32_t i=0;i<candidate.stock_count;++i)if(candidate.stocks[i].type!=4&&candidate.stocks[i].type!=9)return false;}
        if(candidate.stock_count>kStockCapacity)return false;
        if(!v12&&!v13&&!v14&&!v15)for(uint32_t i=0;i<candidate.stock_count;++i)if(v11?!inventory_types::pre_mod_stock(candidate.stocks[i].type):!inventory_types::legacy_stock(candidate.stocks[i].type))return false;
        if(v10||v11){if(candidate.supply.grant_count>128)return false;for(uint32_t i=0;i<candidate.supply.grant_count;++i){int type=candidate.supply.grants[i].type;if(type>2&&(v11?!inventory_types::pre_mod_stock(type):!inventory_types::legacy_stock(type)))return false;}}
        candidate.version = kVersion; candidate.size = sizeof(candidate); candidate.checksum = checksum(candidate);
    }
    if (!ok || !valid(candidate)) return false;
    s = candidate; return true;
}
// Two independent slots: writing the older slot never overwrites the newest
// valid generation. A failed write/rename leaves the previous state readable.
inline int load(const char* root, State& s) {
    char path[640];StateBuffer first,second;if(!first.data||!second.data)return -1;State& a=*first.data;State& b=*second.data;
    snprintf(path, sizeof(path), "%s/local-progress.0", root); bool va = read(path, a);
    snprintf(path, sizeof(path), "%s/local-progress.1", root); bool vb = read(path, b);
    if (!va && !vb) {
        snprintf(path, sizeof(path), "%s/local-progress.0", root); bool exists = access(path, F_OK) == 0;
        snprintf(path, sizeof(path), "%s/local-progress.1", root); exists |= access(path, F_OK) == 0;
        return exists ? -1 : 0;
    }
    s = va && (!vb || a.generation >= b.generation) ? a : b;
    return 1;
}
inline bool write(const char* root, State& s) {
    if(s.generation==UINT64_MAX)return false;
    StateBuffer storage;if(!storage.data)return false;State& next=*storage.data;next=s;++next.generation;next.checksum = checksum(next);
    if (!valid(next)) return false;
    char path[640], temp[660];
    snprintf(path, sizeof(path), "%s/local-progress.%u", root, unsigned(next.generation & 1));
    snprintf(temp, sizeof(temp), "%s.tmp", path);
    FILE* f = fopen(temp, "wb"); if (!f) return false;
    bool ok = fchmod(fileno(f), 0644) == 0 && fwrite(&next, 1, sizeof(next), f) == sizeof(next);
    if (fflush(f) || fsync(fileno(f))) ok = false;
    if (fclose(f)) ok = false;
    if (!ok || rename(temp, path)) return false;
    int dir = open(root, O_RDONLY | O_DIRECTORY);
    if (dir < 0) return false;
    ok = fsync(dir) == 0; close(dir);
    if (!ok) return false;
    s = next; return true;
}
inline Mission* mission(State& s, int id) {
    for (uint32_t i = 0; i < s.count; ++i) if (s.missions[i].id == id) return &s.missions[i];
    if (s.count == kCapacity) return nullptr;
    auto* m = &s.missions[s.count++]; *m = {}; m->id = id; return m;
}
inline int add(int a, int b) { return int64_t(a) + b > INT_MAX ? INT_MAX : a + b; }
inline bool has_upgrade(const State& s, int id) {
    for (uint32_t i = 0; i < s.upgrade_count; ++i) if (s.upgrade_ids[i] == id) return true;
    return false;
}
inline bool has_career(const State& s, int id) {
    for (uint32_t i = 0; i < s.career_count; ++i) if (s.career_ids[i] == id) return true;
    return false;
}
inline int quest_state(const State& s,int id){for(uint32_t i=0;i<s.quest_count;++i)if(s.quests[i].id==id)return s.quests[i].state;return 0;}
inline bool credit_pvp_warpoints(State& s,uint64_t run,int amount,int career_stars=0,int credits=0){
    if(!run||run<=s.warpoint_pvp_cursor||amount<0||career_stars<0||credits<0||s.credits<0||s.stars<0||s.quest_warpoints<0||int64_t(s.quest_warpoints)+amount>INT_MAX||int64_t(s.stars)+career_stars>INT_MAX||int64_t(s.credits)+credits>INT_MAX)return false;
    StateBuffer storage;if(!storage.data)return false;auto& next=*storage.data;next=s;next.quest_warpoints+=amount;next.stars+=career_stars;next.credits+=credits;next.warpoint_pvp_cursor=run;s=next;return true;
}
inline bool has_quest(const State& s,int id){return quest_state(s,id)==32;}
inline bool read_quest(State& s,int id){if(id<0||quest_state(s,id)||s.quest_count==kQuestCapacity)return false;s.quests[s.quest_count++]={id,8};return true;}
inline bool claim_quest(State& s,int id,int credits,int gold,int xp,int warpoints){
    if(id<0||credits<0||gold<0||xp<0||warpoints<0||has_quest(s,id)||(!quest_state(s,id)&&s.quest_count==kQuestCapacity))return false;
    if(int64_t(s.credits)+credits>INT_MAX||int64_t(s.gold)+gold>INT_MAX||int64_t(s.experience)+xp>INT_MAX||int64_t(s.quest_warpoints)+warpoints>INT_MAX)return false;
    State next=s;next.credits+=credits;next.gold+=gold;next.experience+=xp;next.quest_warpoints+=warpoints;
    bool found=false;for(uint32_t i=0;i<next.quest_count;++i)if(next.quests[i].id==id){next.quests[i].state=32;found=true;}
    if(!found)next.quests[next.quest_count++]={id,32};s=next;return true;
}
inline int stock_amount(const State& s, int type, int id) {
    for (uint32_t i=0;i<s.stock_count;++i)
        if(s.stocks[i].type==type && s.stocks[i].id==id) return s.stocks[i].amount;
    return 0;
}
inline uint32_t stock_types(const State& s,int type){
    uint32_t count=0;for(uint32_t i=0;i<s.stock_count;++i)if(s.stocks[i].type==type)++count;return count;
}
inline bool consume_stock(State& s, int type, int id, int amount) {
    if(type==17 || !inventory_types::stock(type) || id<0 || amount<=0) return false;
    for(uint32_t i=0;i<s.stock_count;++i) {
        auto& item=s.stocks[i];
        if(item.type!=type || item.id!=id) continue;
        if(item.amount<amount) return false;
        item.amount-=amount;
        if(!item.amount) {
            for(uint32_t j=i+1;j<s.stock_count;++j) s.stocks[j-1]=s.stocks[j];
            s.stocks[--s.stock_count]={};
        }
        return true;
    }
    return false;
}
inline bool grant_stock(State& s,int type,int id,int amount){
    if(!inventory_types::stock(type)||id<0||amount<=0)return false;
    for(uint32_t i=0;i<s.stock_count;++i)if(s.stocks[i].type==type&&s.stocks[i].id==id){
        if(int64_t(s.stocks[i].amount)+amount>INT_MAX)return false;s.stocks[i].amount+=amount;return true;
    }
    if(s.stock_count==kStockCapacity)return false;s.stocks[s.stock_count++]={type,id,amount};return true;
}
inline int equipped_modification(const State& s,int unit){
    for(uint32_t i=0;i<s.modification_equipment_count;++i)if(s.modification_equipment[i].unit==unit)return s.modification_equipment[i].modification;return 0;
}
// Runtime supplies canonical definition/faction/unit/gate/effect checks. This
// transaction preserves permanent ownership and equips at most one per unit.
inline bool equip_modification(State& s,int unit,int modification){
    if(unit<0||modification<0||(modification&&!stock_amount(s,17,modification))||equipped_modification(s,unit)==modification)return false;
    auto next=s;
    for(uint32_t i=0;i<next.modification_equipment_count;++i)if(next.modification_equipment[i].unit==unit){
        if(modification)next.modification_equipment[i].modification=modification;
        else{for(uint32_t j=i+1;j<next.modification_equipment_count;++j)next.modification_equipment[j-1]=next.modification_equipment[j];next.modification_equipment[--next.modification_equipment_count]={};}
        s=next;return true;}
    if(!modification||next.modification_equipment_count==64)return false;next.modification_equipment[next.modification_equipment_count++]={unit,modification};s=next;return true;
}
inline bool apply_rewards(State& s,const CareerReward* rewards,uint32_t count){
    if(count>128||(!rewards&&count))return false;
    StateBuffer storage;if(!storage.data)return false;State& next=*storage.data;next=s;
    for (uint32_t i=0; i<count; ++i) {
        const auto& r=rewards[i];
        if (r.id < 0 || r.amount <= 0) return false;
        if (r.type==0 || r.type==1) {
            int& value=r.type==0?next.credits:next.gold;
            if (int64_t(value)+r.amount>INT_MAX) return false;
            value+=r.amount;
        } else if(r.type==2 && r.id==0){
            if(int64_t(next.supply.tokens)+r.amount>INT_MAX)return false;
            next.supply.tokens+=r.amount;
        } else if(r.type==6 && (r.id==1||r.id==3)){
            // cancellation of this obsolete premium-days gift. The
            // complete original milestone is its once-only acknowledgement;
            // no VIP state, multiplier, inventory or compensation is granted.
        } else if(r.type==7){
            if(!obsolete_reward_policy::speedup(r.type,r.id))return false;
        } else if (inventory_types::stock(r.type)) {
            Stock* item=nullptr;
            for(uint32_t j=0;j<next.stock_count;++j) if(next.stocks[j].type==r.type&&next.stocks[j].id==r.id) item=&next.stocks[j];
            if (!item) {
                if(next.stock_count==kStockCapacity) return false;
                item=&next.stocks[next.stock_count++]; *item={r.type,r.id,0};
            }
            if(int64_t(item->amount)+r.amount>INT_MAX) return false;
            item->amount+=r.amount;
        } else if ((r.type==18 || r.type==19 || r.type==20 || r.type==21 || r.type==22 || r.type==23) && r.amount==1) {
            // Canonical definitions are checked by the runtime before commit;
            // structural unlocks are reconstructed from these stone receipts.
        } else return false;
    }
    s=next;return true;
}
inline bool claim_career(State& s,int id,int required_stars,const CareerReward* rewards,uint32_t count){
    if(id<0||required_stars<0||s.stars<required_stars||has_career(s,id)||s.career_count==kCareerCapacity||!rewards||!count||count>128)return false;
    StateBuffer storage;if(!storage.data)return false;State& next=*storage.data;next=s;
    if(!apply_rewards(next,rewards,count))return false;
    next.career_ids[next.career_count++]=id;s=next;return true;
}
inline bool buy_upgrade(State& s, int id, int cost, int xp) {
    if (id < 0 || cost < 0 || xp < 0 || s.credits < cost || s.upgrade_count == kUpgradeCapacity || has_upgrade(s, id)) return false;
    s.credits -= cost; s.experience = add(s.experience, xp);
    s.upgrade_ids[s.upgrade_count++] = id; return true;
}
// Wallet, card debit and upgrade receipt live in the same checksummed slot.
// All canonical rank/dependency/sequence checks are supplied by the runtime.
inline bool buy_card_upgrade(State& s,int id,int cost,int xp,int card,int amount){
    if(card<0||amount<=0)return false;State next=s;
    if(!consume_stock(next,9,card,amount)||!buy_upgrade(next,id,cost,xp))return false;
    s=next;return true;
}
inline bool has_skill_node(const State& s,int id){for(uint32_t i=0;i<s.skill_count;++i)if(s.skill_nodes[i]==id)return true;return false;}
// : obsolete free-box interval/card12 and uranium
// regeneration/card13 keep their ordered, manual, zero-cost tree receipts.
inline bool waived_skill_card(int card){return card==11||card==12||card==13||card==22;}
inline bool buy_skill_node(State& s,int id,int cost,int card,int amount){
    bool waived=waived_skill_card(card);
    if(id<=0||cost<0||card<=0||(waived?(cost!=0||amount!=0):amount<=0)||s.credits<cost||s.skill_count==kSkillCapacity||has_skill_node(s,id))return false;
    State next=s;if(!waived&&!consume_stock(next,8,card,amount))return false;
    next.credits-=cost;next.skill_nodes[next.skill_count++]=id;s=next;return true;
}
// Win rewards stay repeatable; experience and career stars use best-result deltas.
// The caller supplies stars/XP computed from the real result, never a forced win.
inline bool settle(State& s, uint64_t run, int id, bool win, int stars, int xp,
                   int credits, uint32_t count, uint32_t objectives) {
    if (!run || run != s.next_run || run <= s.settled_run || id < 0 ||
        stars < 0 || stars > 3 || xp < 0 || xp > 32767 || credits < 0 || count > 32) return false;
    Mission* m = mission(s, id); if (!m) return false;
    s.settled_run = run;
    if (win) {
        m->wins = add(m->wins, 1);
        if (stars > m->stars) { s.stars = add(s.stars, stars - m->stars); m->stars = stars; }
        if (xp > m->xp) { s.experience = add(s.experience, xp - m->xp); m->xp = xp; }
        s.credits = add(s.credits, credits); m->credits = add(m->credits, credits);
        m->objective_count = count; m->objectives |= objectives;
    } else m->losses = add(m->losses, 1);
    return true;
}
} // namespace local_progress
