// Content-only recorder. Included after the IL2CPP API bindings; never edits
// the incoming object passed to the live online game.
#pragma once
#include <unordered_set>
#include <vector>
#include <ctype.h>
#include "capture_start_policy.h"

namespace content_capture {
constexpr const char* root = DATA_ROOT "content-captures-v1/";
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
struct Context {
    int mission = -1, map = -1, faction = -1, difficulty = -1, side = -1;
    int format = -1, mission_type = -1, players = -1;
    char gai[256] = {};
};
static Context context;
static unsigned sequence;
static bool probe_done;

static bool ensure_directory() {
    // On a first launch Unity may create external files storage after our hook
    // initializes. Retry at export time; never report a successful empty write.
    mkdir(DATA_ROOT, 0777);
    mkdir(root, 0777);
    struct stat status;
    if (stat(root, &status) || !S_ISDIR(status.st_mode)) return false;
    chmod(root, 0777);
    return true;
}

static bool contains_ci(const char* text, const char* needle) {
    for (; *text; ++text) {
        const char *a = text, *b = needle;
        while (*a && *b && tolower((unsigned char)*a) == tolower((unsigned char)*b)) { ++a; ++b; }
        if (!*b) return true;
    }
    return false;
}
static bool private_type(void* klass) {
    for (; klass; klass = class_parent(klass)) {
        const char* n = class_name(klass);
        for (const char* blocked : {"Profile", "PublicProfile", "PublicClan", "ClientProfile", "Player",
             "PlayerIdentifier", "MessageMeta", "BattleStatistics", "BattleProfiler", "VoiceChatInfo"})
            if (!strcmp(n, blocked)) return true;
    }
    return false;
}
static bool private_field(const char* name) {
    return contains_ci(name, "session") || contains_ci(name, "token") || contains_ci(name, "password") ||
        contains_ci(name, "email") || contains_ci(name, "accountId") || contains_ci(name, "profileId") ||
        contains_ci(name, "deviceId") || contains_ci(name, "auth");
}
struct Scrubber {
    std::unordered_set<void*> seen;
    unsigned removed = 0;
    bool fields(void* klass, char* data, int adjustment, unsigned depth) {
        if (depth > 160) return false;
        for (; klass; klass = class_parent(klass)) {
            void* iter = nullptr;
            while (void* field = class_fields(klass, &iter)) {
                if (field_flags(field) & (0x10 | 0x40)) continue;
                int offset = field_offset(field);
                if (offset < 16) continue;
                char* value = data + offset + adjustment;
                const void* type = field_type(field);
                int code = type_type(type);
                bool reference = code == 14 || code == 18 || code == 20 || code == 28 || code == 29;
                void* value_class = (code == 17 || code == 21) ? class_from_type(type) : nullptr;
                if (code == 21 && value_class && !class_valuetype(value_class)) reference = true;
                if (private_field(field_name(field))) {
                    if (reference) { if (*(void**)value) { *(void**)value = nullptr; ++removed; } }
                    else if (code == 8 || code == 9) { *(uint32_t*)value = 0; ++removed; }
                    else if (code == 10 || code == 11) { *(uint64_t*)value = 0; ++removed; }
                } else if (reference) {
                    if (!object((void**)value, depth + 1)) return false;
                } else if (value_class && class_valuetype(value_class)) {
                    if (!fields(value_class, value, -16, depth + 1)) return false;
                }
            }
        }
        return true;
    }
    bool object(void** slot, unsigned depth = 0) {
        void* value = *slot;
        if (!value) return true;
        if (depth > 160 || seen.size() > 200000) return false;
        void* klass = object_class(value);
        if (private_type(klass)) { *slot = nullptr; ++removed; return true; }
        if (!seen.insert(value).second) return true;
        const char* name = class_name(klass);
        if (!strcmp(name, "String")) {
            int count = *(int*)((char*)value + 16);
            if (count < 0 || count > 1048576) return false;
            auto chars = (uint16_t*)((char*)value + 20);
            int hex_run = 0;
            for (int i = 0; i < count; ++i) {
                unsigned c = chars[i];
                hex_run = c < 128 && isxdigit(c) ? hex_run + 1 : 0;
                // Session identifiers are not expected in scenario text. Refuse
                // the capture rather than exporting a token-looking string.
                if (hex_run >= 32 || c == '@') return false;
            }
            return true;
        }
        size_t n = strlen(name);
        if (n > 2 && name[n - 1] == ']') {
            size_t count = array_length(value);
            void* element = class_element(klass);
            if (!element || count > 1048576) return false;
            if (!class_valuetype(element)) {
                auto items = (void**)((char*)value + 32);
                for (size_t i = 0; i < count; ++i) if (!object(&items[i], depth + 1)) return false;
            } else {
                uint32_t align = 0;
                int stride = class_value_size(element, &align);
                if (stride <= 0) return false;
                for (size_t i = 0; i < count; ++i)
                    if (!fields(element, (char*)value + 32 + i * stride, -16, depth + 1)) return false;
            }
            return true;
        }
        return fields(klass, (char*)value, 0, depth);
    }
};
static void* encode(void* message) {
    int initial = 4096, maximum = 64*1024*1024;
    void* args[] = {message, &initial, &maximum};
    void* exception = nullptr;
    void* helper = find_class("com.geargames.common.serialization", "SerializationHelpers");
    void* buffer = runtime_invoke(class_method(helper, "SerializeMessage", 3), nullptr, args, &exception);
    return exception ? nullptr : buffer;
}
static void* decode(void* buffer) {
    int type = 5040;
    void* args[] = {buffer, &type}; void* exception = nullptr;
    void* helper = find_class("com.geargames.common.serialization", "SerializationHelpers");
    void* result = runtime_invoke(class_method(helper, "DeserializeMessage", 2), nullptr, args, &exception);
    return exception ? nullptr : result;
}
static std::vector<uint8_t> bytes(void* buffer) {
    if (!buffer) return {};
    void* array = *(void**)((char*)buffer + 0x10);
    int count = *(int*)((char*)buffer + 0x1c);
    if (!array || count <= 0 || count > 64*1024*1024 || (size_t)count > array_length(array)) return {};
    return std::vector<uint8_t>((uint8_t*)array + 32, (uint8_t*)array + 32 + count);
}
static void* decode_bytes(const std::vector<uint8_t>& input) {
    if (input.empty()) return nullptr;
    void* array = array_new(find_class("System", "Byte"), input.size());
    memcpy((char*)array + 32, input.data(), input.size());
    void* buffer = object_new(find_class("com.geargames.common.utils", "ClientByteBuffer"));
#ifdef ART3_CAPTURE_DYNAMIC_BUFFER_CTOR
    // Official-client attachment resolves metadata instead of using an ARM64 RVA.
    int length = (int)input.size(); void* args[] = {array, &length};
    void* exception = nullptr;
    runtime_invoke(class_method(object_class(buffer), ".ctor", 2), buffer, args, &exception);
    if (exception) return nullptr;
#else
    reinterpret_cast<void(*)(void*,void*,void*)>(image_base + 0x7a02ee8)(buffer, array, nullptr);
#endif
    return decode(buffer);
}
static bool export_start(void* incoming, bool probe = false) {
    if (!ensure_directory()) { LOG("Content capture refused: output directory unavailable"); return false; }
    // Clone via the game's serializer first. No field in the live online
    // message is cleared, replaced, or delivered to the server by this recorder.
    auto original = bytes(encode(incoming));
    void* clone = decode_bytes(original);
    if (!clone) return false;
    uintptr_t handle = gchandle_new(clone, true);
    void* battle = *(void**)((char*)clone + 0x20);
    if (!battle) { gchandle_free(handle); return false; }
    if (probe) {
        void* profile = object_new(find_class("com.geargames.aow.entities", "Profile"));
        void* candidate = profile;
        Scrubber profile_check;
        if (!profile || !profile_check.object(&candidate) || candidate || profile_check.removed != 1) {
            gchandle_free(handle); return false;
        }
        void* stats = object_new(find_class("com.geargames.aow.entities.battle.descriptions", "BattleStatistics"));
        invoke_named(stats, ".ctor", 0, nullptr, nullptr);
        // A synthetic session marker exists only on this isolated clone.
        field_set(stats, class_field_from_name(object_class(stats), "$CN"), string_new("PRIVATE_SESSION_0123456789abcdef0123456789abcdef"));
        field_set(battle, class_field_from_name(object_class(battle), "$VA"), stats);
    }
    Scrubber scrub;
    bool ok = scrub.object(&clone);
    *(int*)((char*)clone + 0x10) = -1;
    *(int*)((char*)clone + 0x14) = -1;
    *(int*)((char*)battle + 0x10) = 1; // anonymous local battle identity
    *(int*)((char*)battle + 0x40) = 1;
    auto output = ok ? bytes(encode(clone)) : std::vector<uint8_t>();
    // Verify that serialization and scrubbing left the live input byte-exact.
    ok = ok && !output.empty() && bytes(encode(incoming)) == original;
    if (ok) {
        unsigned id=0;
        char path[640];
        int binary_fd=capture_start_policy::reserve(root,sequence,id,path,sizeof(path));
        FILE* file=binary_fd>=0?fdopen(binary_fd,"wb"):nullptr;
        if(!file&&binary_fd>=0)close(binary_fd);
        ok = file && fwrite(output.data(), 1, output.size(), file) == output.size();
        if (file) { if(fflush(file)||fsync(fileno(file)))ok=false;if (fclose(file)) ok = false; chmod(path, 0644); }
        if (ok) {
            snprintf(path, sizeof(path), "%s%06u_StartBattleNotification.json", root, id);
            int metadata_fd=open(path,O_WRONLY|O_CREAT|O_EXCL,0644);
            file=metadata_fd>=0?fdopen(metadata_fd,"w"):nullptr;
            if(!file&&metadata_fd>=0)close(metadata_fd);
            if (!file) ok = false;
            else {
                int recorded_mission = *(int*)((char*)battle + 0x28);
                bool known = context.mission == recorded_mission;
                if(fprintf(file, "{\"schema\":\"battle-content-v1\",\"message_type\":5040,\"size\":%zu,"
                    "\"mission_id\":%d,\"map_id\":%d,\"fraction_id\":%d,\"format\":%d,\"mission_type\":%d,"
                    "\"difficulty\":%d,\"side_id\":%d,\"gai\":\"%s\",\"removed_references\":%u,"
                    "\"live_message_unchanged\":true,\"capture_time_attributes\":true,\"probe\":%s}\n",
                    output.size(), recorded_mission, known ? context.map : -1,
                    known ? context.faction : -1, known ? context.format : -1,
                    known ? context.mission_type : -1, known ? context.difficulty : -1, context.side,
                    known ? context.gai : "", scrub.removed, probe ? "true" : "false")<=0)ok=false;
                if(fflush(file)||fsync(fileno(file)))ok=false;
                if (fclose(file)) ok = false;
                chmod(path, 0644);
            }
        }
        LOG("Content capture saved=%d sequence=%u mission=%d map=%d removed=%u unchanged=1 probe=%d",
            ok, id, *(int*)((char*)battle + 0x28), context.map, scrub.removed, probe);
    } else LOG("Content capture refused: clone, privacy guard, or input-integrity check failed");
    gchandle_free(handle);
    return ok;
}
static void update_context(void* message, const char* name) {
    if (!strcmp(name, "GoToBattleNotification")) {
        context = {};
        void* desc = *(void**)((char*)message + 0x30);
        void* mission = desc ? *(void**)((char*)desc + 0x18) : nullptr;
        if (!mission) return;
        context.mission = *(int*)((char*)mission + 0x10);
        context.map = *(int*)((char*)mission + 0x14);
        context.faction = *(int*)((char*)mission + 0x30);
        context.mission_type = *(int*)((char*)mission + 0x24);
        context.format = *(int*)((char*)mission + 0x20);
        context.players = *(int*)((char*)mission + 0x70);
        context.difficulty = *(int*)((char*)desc + 0x38);
        void* gai = *(void**)((char*)mission + 0x28);
        int count = gai ? *(int*)((char*)gai + 16) : 0;
        if (count > 0 && count < 256) {
            auto chars = (uint16_t*)((char*)gai + 20);
            bool safe = true;
            for (int i = 0; i < count; ++i) {
                unsigned c = chars[i];
                if (!(c < 128 && (isalnum(c) || c == '_' || c == '-' || c == '.'))) safe = false;
                context.gai[i] = c < 128 ? c : '_';
            }
            if (!safe) context.gai[0] = 0;
        }
    } else if (!strcmp(name, "LoginToBattleAnswer")) {
        context.side = *(int*)((char*)message + 0x30);
    }
}
static void maybe_probe() {
    if (probe_done || access(DATA_ROOT "content-probe.enabled", F_OK)) return;
    probe_done = true;
    FILE* f = fopen(DATA_ROOT "content-probe-input.ggs", "rb");
    if (!f) { LOG("Content privacy probe: input unavailable"); return; }
    fseek(f, 0, SEEK_END); long length = ftell(f); rewind(f);
    std::vector<uint8_t> input(length > 0 && length < 16*1024*1024 ? length : 0);
    bool read = !input.empty() && fread(input.data(), 1, input.size(), f) == input.size(); fclose(f);
    void* incoming = read ? decode_bytes(input) : nullptr;
    uintptr_t handle = incoming ? gchandle_new(incoming, true) : 0;
    bool ok = incoming && export_start(incoming, true);
    if (handle) gchandle_free(handle);
    LOG("Content privacy probe passed=%d", ok);
}
static void dispatch(void* message, const char* name) {
    struct Guard {
        Guard() { pthread_mutex_lock(&lock); }
        ~Guard() { pthread_mutex_unlock(&lock); }
    } guard;
    try {
        update_context(message, name);
        if (!strcmp(name, "StartBattleNotification")) {
            void* battle = *(void**)((char*)message + 0x20);
            auto kind=battle?capture_start_policy::fresh(context.mission,*(int*)((char*)battle+0x28),
                *(int*)((char*)message+0x30),context.mission_type,context.format,context.players,context.side):capture_start_policy::None;
            if(kind!=capture_start_policy::None){LOG("Content capture eligible mode=%s",kind==capture_start_policy::Cooperative?"cooperative":"campaign");export_start(message);}
            else LOG("Content capture skipped: only fresh confirmed campaign/cooperative starts are enabled");
        }
        if (!strcmp(name, "UserConfiguration")) maybe_probe();
    } catch (...) {
        // Recorder failures are visible and never replace the game's dispatch.
        LOG("Content capture refused: recorder allocation/runtime exception");
    }
}
static void initialize() {
    if (!ensure_directory()) LOG("Content capture storage not ready; will retry when exporting");
    if (DIR* dir = opendir(root)) {
        while (dirent* e = readdir(dir)) {
            unsigned id; char separator;
            if (sscanf(e->d_name, "%u%c", &id, &separator) == 2 && separator == '_' && id < 1000000 && sequence <= id)
                sequence = id + 1;
        }
        closedir(dir);
    }
    LOG("Content-only recorder: start-battle snapshots; account/configuration/login payloads excluded");
}
} // namespace content_capture
