// Build the local loading context from canonical content and the actual battle
// lobby seats for the captured single-player scenarios. Script-owned allied
// armies are not lobby players. No GoToBattle session or server is needed.
#pragma once
namespace local_campaign {
using local_research_runtime::Roots;
static bool failure(int line) { LOG("Local campaign context rejected at campaign_runtime.h:%d", line); return false; }
static bool failed_exception(void* exception, const char* phase) {
    if (!exception) return false;
    LOG("Local campaign exception phase=%s class=%s", phase, class_name(object_class(exception)));
    FILE* f = fopen(DATA_ROOT "art3offline-scenario-error.txt", "w");
    if (f) {
        fprintf(f, "phase=%s\n", phase); visited_count = 0;
        dump_object(f, exception, 0, 2); fclose(f);
        chmod(DATA_ROOT "art3offline-scenario-error.txt", 0644);
    }
    return true;
}
static void* field_list(void* owner_class, const char* name, Roots& roots) {
    void* field = owner_class ? class_field_from_name(owner_class, name) : nullptr;
    void* klass = field ? class_from_type(field_type(field)) : nullptr;
    void* list = klass ? object_new(klass) : nullptr;
    if (!roots.keep(list)) return nullptr;
    void* exception = nullptr;
    invoke_named(list, ".ctor", 0, nullptr, &exception);
    return exception ? nullptr : list;
}
static bool append(void* list, void* value) {
    if (!list || !value) return failure(__LINE__);
    void* exception = nullptr; void* args[] = {value};
    invoke_named(list, "Add", 1, args, &exception);
    return !exception;
}
static bool initialize(void* battle, void* mission, int own_side, bool cooperative=false) {
    if (!battle || !mission || *(int*)((char*)battle + 0x28) != *(int*)((char*)mission + 0x10)) return failure(__LINE__);
    void* context = find_class("com.geargames.aow.scenes.battle", "ClientBattleContext");
    // The current local career profile was restored before entering the hall.
    // Do not depend on a previous battle/GoToBattle description to find it.
    void* user=saved("UserConfiguration");
    void* profile=user?*(void**)((char*)user+0x28):nullptr;
    if(!profile){LOG("Local campaign missing restored career profile");return failure(__LINE__);}
    Roots roots;
    void* description_class = find_class("com.geargames.aow.client.entities", "ClientBattleDescription");
    void* alliance_class = find_class("com.geargames.aow.client.entities", "ClientBattleAllianceDescription");
    void* player_class = find_class("com.geargames.aow.client.entities", "ClientBattlePlayer");
    void* alliances = field_list(description_class, "m_alliances", roots);
    void* headers = field_list(description_class, "m_headers", roots);
    if (!alliances || !headers || !append(headers, string_new("{#695}")) ||
        !append(headers, *(void**)((char*)mission + 0x38))) return failure(__LINE__);
    void* source = *(void**)((char*)battle + 0x170);
    size_t count = source ? array_length(source) : 0;
    if (!count || count > 8) return failure(__LINE__);
    int own_count = 0, enemy_count = 0, seat_count = 0;
    int own_alliance = -1, enemy_alliance = -1;
    void* exception = nullptr;
    for (size_t i = 0; i < count; ++i) {
        void* source_alliance = ((void**)((char*)source + 32))[i];
        void* sides = source_alliance ? *(void**)((char*)source_alliance + 0x18) : nullptr;
        int n = sides ? *(int*)((char*)sides + 0x18) : 0;
        if (n < 0 || n > 16) return failure(__LINE__);
        void* players = field_list(alliance_class, "m_players", roots);
        if (!players) return failure(__LINE__);
        for (int j = 0; j < n; ++j) {
            void* side = list_item(sides, j);
            if (!side) return failure(__LINE__);
            int id = *(int*)((char*)side + 0x10), league = 0;
            // The fresh solo captures use lobby seats 1/2. Extra sides such as
            // chapter-02 ally 3 belong to GAI; including them here creates a
            // partial human/AI team that the original profile/color code rejects.
            bool team_match=local_match::playing||offline_world_runtime::batch_fixture;
            if (cooperative?(id!=1&&id!=2&&id!=3):(team_match?(id<1||id>local_duel::seat_count()):(id != own_side && id != 2))) continue;
            if (local_dictionary_item(*(void**)((char*)battle + 0x178), id) != side) return failure(__LINE__);
            void* display_profile=id==own_side?profile:nullptr;
            if(offline_world_runtime::batch_fixture&&id==own_side){
                display_profile=local_research_runtime::clone(profile,roots);if(!display_profile)return failure(__LINE__);
                field_set(display_profile,class_field_from_name(object_class(display_profile),"fraction"),*(void**)((char*)side+0xd8));
            }
            if((team_match||cooperative)&&id!=own_side){
                // The original PvE helper fills only an all-AI alliance. A
                // human+AI alliance must supply its ally's PublicProfile too:
                // GetColorSides dereferences every player's Fraction.
                display_profile=object_new(find_class("com.geargames.aow.entities","PublicProfile"));
                if(!roots.keep(display_profile))return failure(__LINE__);
                invoke_named(display_profile,".ctor",0,nullptr,&exception);
                if(exception)return failure(__LINE__);
                void* public_class=find_class("com.geargames.aow.entities","PublicProfile");
                field_set(display_profile,class_field_from_name(public_class,"fraction"),*(void**)((char*)side+0xd8));
                field_set(display_profile,class_field_from_name(public_class,"rank"),*(void**)((char*)profile+0x60));
                field_set(display_profile,class_field_from_name(public_class,"name"),string_new(id%2?"AI 队友":"AI 对手"));
            }
            void* player = object_new(player_class);
            if (!roots.keep(player)) return failure(__LINE__);
            void* args[] = {&id, &league, display_profile};
            invoke_named(player, ".ctor", 3, args, &exception);
            if (exception || !append(players, player)) return failure(__LINE__);
            int alliance_id = *(int*)((char*)source_alliance + 0x10);
            if (id == own_side) { ++own_count; own_alliance = alliance_id; }
            else { ++enemy_count; if(id==2)enemy_alliance = alliance_id; }
            ++seat_count;
        }
        if (!*(int*)((char*)players + 0x18)) continue;
        int id = *(int*)((char*)source_alliance + 0x10);
        void* alliance = object_new(alliance_class);
        if (!roots.keep(alliance)) return failure(__LINE__);
        void* args[] = {&id, players};
        invoke_named(alliance, ".ctor", 2, args, &exception);
        if (exception || !append(alliances, alliance)) return failure(__LINE__);
    }
    bool team_match=local_match::playing||offline_world_runtime::batch_fixture;
    int expected=cooperative?3:(team_match?local_duel::seat_count():2);
    if (own_count != 1 || enemy_count != expected-1 || own_alliance == enemy_alliance || seat_count!=expected) return failure(__LINE__);
    void* extension = find_class("com.geargames.aow.ugui.extension", "ExtensionClientBattleDescription");
    void* ai_args[] = {alliances};
    runtime_invoke(class_method(extension, "InitiateAllianceAIInfo", 1), nullptr, ai_args, &exception);
    if (failed_exception(exception, "AI profile")) return failure(__LINE__);
    if((local_match::playing||cooperative)&&!offline_world_runtime::decorate_alliances(alliances))return failure(__LINE__);
    if(local_match::playing){char strength[160];const auto* other=offline_world_runtime::actor(2);
        snprintf(strength,sizeof(strength),"%s · 实力 %d · 对手 %s（%s）",local_match_store::formal(local_match::state)?"正式指挥官对抗":"训练军演",offline_world_runtime::session.player_rating,other?other->name:"指挥官",offline_world::tier_name(other?other->tier:1));
        if(!append(headers,string_new(strength)))return failure(__LINE__);}
    void* description = object_new(description_class);
    if (!roots.keep(description)) return failure(__LINE__);
    int difficulty = 0, duration = 0; bool replay = false, exact = false;
    void* args[] = {&difficulty, headers, &replay, mission, &exact, alliances, &duration};
    invoke_named(description, ".ctor", 7, args, &exception);
    if (failed_exception(exception, "client description")) return failure(__LINE__);
    // Ordinary serverless PvE is supported by the original engine. Keep its
    // explicit logon-scene return policy, but do not masquerade every chapter
    // as first-run onboarding or store it into FirstMissionConfiguration.
    void* first_utils=find_class("com.geargames.aow.scenes.logon","FirstMissionUtils");
    const void* reset=first_utils?class_method(first_utils,"ResetFirstMissionState",0):nullptr;
    if(!reset)return failure(__LINE__);
    runtime_invoke(reset,nullptr,nullptr,&exception);
    if(failed_exception(exception,"reset first-run state"))return failure(__LINE__);
    bool first = false, logout = true;
    void* initialize_args[] = {description, &first, &logout};
    runtime_invoke(class_method(context, "InitializeWithoutServerPvE", 3), nullptr, initialize_args, &exception);
    if (failed_exception(exception, "context initialization")) return failure(__LINE__);
    void* statistics = nullptr;
    static_get(class_field_from_name(context, "m_statistics"), &statistics);
    void* time_class = find_class("com.geargames.common.utils", "TimeUtils");
    void* now = runtime_invoke(class_method(time_class, "GetMilliTicks", 0), nullptr, nullptr, &exception);
    if (!statistics || !now || exception) return failure(__LINE__);
    *(int64_t*)((char*)statistics + 0x10) = *(int64_t*)((char*)now + 0x10);
    void* helper = find_class("com.geargames.aow.client.utils", "BattleInitializeHelper");
    void* map_args[] = {mission};
    runtime_invoke(class_method(helper, "SetBattleMapInfo", 1), nullptr, map_args, &exception);
    if (exception) return failure(__LINE__);
    LOG("Local clean campaign context mission=%d alliances=%zu seats=%d own=%d first_mission=0 bootstrap_first_mission=0", *(int*)((char*)mission + 0x10), count, seat_count, own_side);
    return true;
}
}
