// Development bootstrap for the exact, hash-checked ARM64 release build.
// The original Unity NativeLoader remains responsible for loading Unity.
#include <jni.h>
#include <android/log.h>
#include <dlfcn.h>
#include <pthread.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <initializer_list>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <android/asset_manager_jni.h>
#include "local_progress.h"
#include "competition_track_store.h"
#include "local_match_store.h"
#include "offline_world_store.h"
namespace offline_world_runtime {static bool publish_profile(void*);}
#include "local_history_view.h"
#include "local_match_maps.h"
#include "ai_team_policy.h"
#include "ai_base_policy.h"
#include "ai_guard_policy.h"
#include "research_effects.h"

#ifdef ART3_CAPTURE
#if defined(ART3_CAPTURE_LEGACY) || defined(ART3_CAPTURE_GUEST)
#define DATA_ROOT "/sdcard/Android/data/com.art3cap00.aow/files/"
#else
#define DATA_ROOT "/sdcard/Android/data/com.art3cnt00.aow/files/"
#endif
#else
#define DATA_ROOT "/sdcard/Android/data/com.art3rts00.aow/files/"
#endif

#include "offline_account_store.h"
#include "faction_store.h"

#define LOG(...) __android_log_print(ANDROID_LOG_INFO, "Art3Offline", __VA_ARGS__)

static void* il2cpp;
static uintptr_t image_base;
static JavaVM* java_vm;
static bool (*prepare_original)(void*);
static void replay_saved_messages(bool linked = false);
static void restore_saved_dictionaries();
static void* linked_read_manager;
static const char* report_path = DATA_ROOT "art3offline-runtime.txt";

static void* (*domain_get)();
static const void** (*domain_assemblies)(void*, size_t*);
static void* (*assembly_image)(const void*);
static void* (*class_from_name)(void*, const char*, const char*);
static void* (*object_class)(void*);
static const char* (*class_name)(void*);
static const char* (*class_namespace)(void*);
static void* (*class_parent)(void*);
static void* (*class_fields)(void*, void**);
static void* (*class_field_from_name)(void*, const char*);
static const char* (*field_name)(void*);
static const void* (*field_type)(void*);
static int (*field_offset)(void*);
static int (*field_flags)(void*);
static int (*type_type)(const void*);
static char* (*type_name)(const void*);
static void (*static_get)(void*, void*);
static void (*static_set)(void*, void*);
static void (*field_set)(void*, void*, void*);
static size_t (*array_length)(void*);
static void* (*class_element)(void*);
static bool (*class_valuetype)(void*);
static void* (*class_from_type)(const void*);
static void* (*class_from_system_type)(void*);
static void (*il_free)(void*);
static int (*class_value_size)(void*, uint32_t*);
static const void* (*class_method)(void*, const char*, int);
static void* (*runtime_invoke)(const void*, void*, void**, void**);
static void* (*object_new)(void*);
static void* (*array_new)(void*, size_t);
static void* (*string_new)(const char*);
static const void* (*class_type)(void*);
static void* (*type_object)(const void*);
// Unity 6000 uses pointer-sized Il2CppGCHandle values on ARM64.
static uintptr_t (*gchandle_new)(void*, bool);
static void (*gchandle_free)(uintptr_t);
struct SavedObject { char name[128]; void* object; uintptr_t handle; };
static SavedObject saved_objects[32];
static int saved_count;

static void save_object(const char* name, void* object) {
    if (!object) return;
    int slot = -1;
    for (int i = 0; i < saved_count; i++)
        if (!strcmp(saved_objects[i].name, name)) { slot = i; break; }
    if (slot < 0) {
        if (saved_count >= 32) { LOG("Saved object capacity reached: %s", name); return; }
        slot = saved_count++;
    }
    auto& entry = saved_objects[slot];
    uintptr_t previous = entry.handle;
    snprintf(entry.name, sizeof(entry.name), "%s", name);
    entry.object = object;
    entry.handle = gchandle_new(object, false);
    if (previous) gchandle_free(previous);
}

static void* saved(const char* name) {
    for (int i = saved_count - 1; i >= 0; i--)
        if (!strcmp(saved_objects[i].name, name)) return saved_objects[i].object;
    return nullptr;
}

template <typename T> static bool api(T& target, const char* name) {
    target = reinterpret_cast<T>(dlsym(il2cpp, name));
    if (!target) LOG("Missing API: %s", name);
    return target != nullptr;
}

static void* find_class(const char* ns, const char* name) {
    size_t count = 0;
    const void** assemblies = domain_assemblies(domain_get(), &count);
    for (size_t i = 0; i < count; i++) {
        void* klass = class_from_name(assembly_image(assemblies[i]), ns, name);
        if (klass) return klass;
    }
    return nullptr;
}

static void indent(FILE* f, int level) {
    for (int i = 0; i < level; i++) fputs("  ", f);
}

static void* visited[4096];
static size_t visited_count;

static void dump_object(FILE* f, void* object, int level, int depth) {
    if (!object) { fputs("null\n", f); return; }
    void* klass = object_class(object);
    const char* name = class_name(klass);
    fprintf(f, "%s.%s @%p", class_namespace(klass), name, object);
    if (!strcmp(name, "String")) {
        int length = *(int*)((char*)object + 16);
        fputs(" = ", f);
        auto chars = (uint16_t*)((char*)object + 20);
        for (int i = 0; i < length && i < 120; i++) fputc(chars[i] >= 32 && chars[i] < 127 ? chars[i] : '?', f);
        fputc('\n', f); return;
    }
    size_t n = strlen(name);
    if (n > 2 && name[n - 1] == ']') {
        size_t length = array_length(object);
        fprintf(f, " length=%zu\n", length);
        void* element = class_element(klass);
        if (depth > 0 && element && !class_valuetype(element)) {
            auto elements = (void**)((char*)object + 32);
            for (size_t i = 0; i < length && i < 32; i++) {
                indent(f, level + 1); fprintf(f, "[%zu] ", i);
                dump_object(f, elements[i], level + 1, depth - 1);
            }
        }
        return;
    }
    fputc('\n', f);
    if (depth <= 0 || visited_count >= 4096) return;
    for (size_t i = 0; i < visited_count; i++) if (visited[i] == object) return;
    visited[visited_count++] = object;
    for (void* parent = klass; parent; parent = class_parent(parent)) {
        void* iter = nullptr;
        while (void* field = class_fields(parent, &iter)) {
            if (field_flags(field) & (0x10 | 0x40)) continue;
            int offset = field_offset(field);
            if (offset < 16) continue;
            const void* type = field_type(field);
            int code = type_type(type);
            char* label = type_name(type);
            indent(f, level + 1);
            fprintf(f, "%s +0x%x [%s] = ", field_name(field), offset, label ? label : "?");
            if (label) il_free(label);
            char* data = (char*)object + offset;
            switch (code) {
                case 2: fprintf(f, "%s\n", *(bool*)data ? "true" : "false"); break;
                case 3: case 7: fprintf(f, "%u\n", *(uint16_t*)data); break;
                case 4: fprintf(f, "%d\n", *(int8_t*)data); break;
                case 5: fprintf(f, "%u\n", *(uint8_t*)data); break;
                case 6: fprintf(f, "%d\n", *(int16_t*)data); break;
                case 8: fprintf(f, "%d\n", *(int32_t*)data); break;
                case 9: fprintf(f, "%u\n", *(uint32_t*)data); break;
                case 10: fprintf(f, "%lld\n", (long long)*(int64_t*)data); break;
                case 11: fprintf(f, "%llu\n", (unsigned long long)*(uint64_t*)data); break;
                case 12: fprintf(f, "%g\n", *(float*)data); break;
                case 13: fprintf(f, "%g\n", *(double*)data); break;
                case 14: case 18: case 20: case 28: case 29:
                    dump_object(f, *(void**)data, level + 1, depth - 1); break;
                case 21:
                    if (class_valuetype(class_from_type(type))) fputs("generic value-type\n", f);
                    else dump_object(f, *(void**)data, level + 1, depth - 1);
                    break;
                default: fprintf(f, "value-type (0x%x)\n", code); break;
            }
        }
    }
}

static void dump_configuration() {
    void* klass = find_class("com.geargames.aow.scenes.battle", "ClientBattleContext");
    if (!klass) { LOG("ClientBattleContext not found"); return; }
    void* field = class_field_from_name(klass, "m_battleInfo");
    if (!field) return;
    void* configuration = nullptr;
    static_get(field, &configuration);
    FILE* file = fopen(report_path, "w");
    if (!file) { LOG("Cannot open runtime report"); return; }
    visited_count = 0;
    dump_object(file, configuration, 0, 8);
    const char* extra_names[] = {"m_description", "m_battleMapInfo"};
    for (const char* name : extra_names) {
        void* extra_field = class_field_from_name(klass, name);
        void* value = nullptr;
        if (extra_field) static_get(extra_field, &value);
        fprintf(file, "\n%s: ", name);
        visited_count = 0;
        dump_object(file, value, 0, 6);
    }
    fclose(file);
    // Record every scenario command, including commands beyond the bounded graph preview.
    void* battle = configuration ? *(void**)((char*)configuration + 0x10) : nullptr;
    void* gai = battle ? *(void**)((char*)battle + 0x140) : nullptr;
    void* commands = gai ? *(void**)((char*)gai + 0x30) : nullptr;
    if (commands) {
        file = fopen(DATA_ROOT "art3offline-commands.txt", "w");
        if (file) {
            int count = *(int*)((char*)commands + 0x18);
            void* array = *(void**)((char*)commands + 0x10);
            auto items = (void**)((char*)array + 32);
            for (int i = 0; i < count; i++) {
                fprintf(file, "\nCOMMAND %d: ", i);
                visited_count = 0;
                dump_object(file, items[i], 0, 3);
            }
            fclose(file);
        }
    }
    // Export the per-side unit/building definitions from Dictionary<int, object>.
    if (battle) {
        file = fopen(DATA_ROOT "art3offline-definitions.txt", "w");
        if (file) {
            void* sides = *(void**)((char*)battle + 0x178);
            void* entries = sides ? *(void**)((char*)sides + 0x18) : nullptr;
            int side_count = sides ? *(int*)((char*)sides + 0x20) : 0;
            if (entries) {
                void* entry_type = class_element(object_class(entries));
                uint32_t alignment = 0;
                int stride = class_value_size(entry_type, &alignment);
                void* value_field = class_field_from_name(entry_type, "value");
                int value_offset = value_field ? field_offset(value_field) - 16 : -1;
                for (int i = 0; i < side_count && value_offset >= 0; i++) {
                    auto entry = (char*)entries + 32 + i * stride;
                    void* side = *(void**)(entry + value_offset);
                    if (!side || *(int*)entry < 0) continue;
                    fprintf(file, "\nSIDE %d\n", *(int*)((char*)side + 16));
                    const int dict_offsets[] = {0xf0, 0xf8};
                    for (int dict_offset : dict_offsets) {
                        void* dict = *(void**)((char*)side + dict_offset);
                        void* records = dict ? *(void**)((char*)dict + 0x18) : nullptr;
                        int count = dict ? *(int*)((char*)dict + 0x20) : 0;
                        if (!records) continue;
                        void* element = class_element(object_class(records));
                        int record_size = class_value_size(element, &alignment);
                        void* vf = class_field_from_name(element, "value");
                        int vo = vf ? field_offset(vf) - 16 : -1;
                        for (int j = 0; j < count && vo >= 0; j++) {
                            auto record = (char*)records + 32 + j * record_size;
                            if (*(int*)record < 0) continue;
                            visited_count = 0;
                            dump_object(file, *(void**)(record + vo), 0, 3);
                        }
                    }
                }
            }
            fclose(file);
        }
    }
    LOG("Runtime configuration recorded: %s", report_path);
}

static bool prepare_hook(void* method) {
    bool result = prepare_original(method);
    LOG("Prepare local battle result=%d", result);
    if (result) {
        dump_configuration(); replay_saved_messages(); restore_saved_dictionaries();
    }
    return result;
}

static void* invoke_named(void* instance, const char* name, int count, void** arguments, void** exception) {
    if (!instance) return nullptr;
    const void* method = class_method(object_class(instance), name, count);
    return method ? runtime_invoke(method, instance, arguments, exception) : nullptr;
}

static void export_dictionary_records(void* dictionary) {
    const char* names[] = {"missionDescriptions", "mapDescriptions"};
    for (const char* name : names) {
        void* field = class_field_from_name(object_class(dictionary), name);
        void* list = field ? *(void**)((char*)dictionary + field_offset(field)) : nullptr;
        int count = list ? *(int*)((char*)list + 0x18) : 0;
        void* items = list ? *(void**)((char*)list + 0x10) : nullptr;
        if (!items || count < 0 || count > 10000 || (size_t)count > array_length(items)) continue;
        char path[512];
        snprintf(path, sizeof(path), DATA_ROOT "records-%s.txt", name);
        FILE* file = fopen(path, "w");
        if (!file) continue;
        fprintf(file, "RECORD_COUNT %d\n", count);
        for (int i = 0; i < count; i++) {
            fprintf(file, "RECORD %d\n", i);
            visited_count = 0;
            dump_object(file, *(void**)((char*)items + 32 + i*8), 0, 1);
        }
        fclose(file); chmod(path, 0644);
    }
}

static void restore_saved_dictionaries() {
    struct Definition { const char* file; const char* type; } definitions[] = {
        {"BATTLE", "com.geargames.aow.messages.authorization.BattleBaseDictionary"},
        {"SHARED", "com.geargames.aow.messages.authorization.SharedBaseDictionary"},
        {"CONFEDERATION", "com.geargames.aow.messages.authorization.BaseDictionary"},
        {"RESISTANCE", "com.geargames.aow.messages.authorization.BaseDictionary"},
        {"MissionDictionary", "com.geargames.aow.dictionaries.MissionDictionary"},
        {"LOCALE", "com.geargames.aow.messages.LocalizationDictionary"}
    };
    void* manager_class = find_class("com.geargames.common.serialization", "ThreadSafeSerializationManager");
    void* helper_class = find_class("com.geargames.common.serialization", "SerializationHelpers");
    void* buffer_class = find_class("com.geargames.common.utils", "ClientByteBuffer");
    void* byte_class = find_class("System", "Byte");
    if (!manager_class || !helper_class || !buffer_class || !byte_class) return;
    const void* create = class_method(manager_class, "CreateOrdinal", 0);
    const void* serialize = class_method(manager_class, "SerializeObject", 3);
    FILE* report = fopen(DATA_ROOT "art3offline-dictionaries.txt", "w");
    if (!report || !create || !serialize) { if (report) fclose(report); return; }
    // Dictionary files contain references to entities defined in earlier dictionaries.
    // Keep the entity cache across BATTLE -> SHARED -> faction dictionaries, with an
    // independent serialization cache for the exact round-trip comparison.
    void* setup_exception = nullptr;
    void* manager = runtime_invoke(create, nullptr, nullptr, &setup_exception);
    void* write_manager = runtime_invoke(create, nullptr, nullptr, &setup_exception);
    void* context = manager ? *(void**)((char*)manager + 0x10) : nullptr;
    void* write_context = write_manager ? *(void**)((char*)write_manager + 0x10) : nullptr;
    if (setup_exception || !context || !write_context) { fclose(report); return; }
    linked_read_manager = manager;
    save_object("@dictionary-reader", manager);
    for (const Definition& definition : definitions) {
        char filename[512];
        snprintf(filename, sizeof(filename), DATA_ROOT "offline-dictionaries/%s.ggs", definition.file);
        FILE* file = fopen(filename, "rb");
        if (!file) { fprintf(report, "MISSING %s\n", definition.file); continue; }
        fseek(file, 0, SEEK_END); long length = ftell(file); rewind(file);
        if (length < 1 || length > 64 * 1024 * 1024) { fclose(file); continue; }
        void* bytes = array_new(byte_class, length);
        size_t read = bytes ? fread((char*)bytes + 32, 1, length, file) : 0;
        fclose(file);
        if (read != (size_t)length) continue;
        void* buffer = object_new(buffer_class);
        auto ctor = reinterpret_cast<void(*)(void*, void*, void*)>(image_base + 0x7a02ee8);
        ctor(buffer, bytes, nullptr);
        void* exception = nullptr;
        void* identity = *(void**)((char*)manager + 0x20);
        invoke_named(identity, "reset", 0, nullptr, &exception);
        void* deserializers = invoke_named(context, "get_ObjectDeserializers", 0, nullptr, &exception);
        void* key = string_new(definition.type);
        void* get_arguments[] = {key};
        void* deserializer = invoke_named(deserializers, "get_Item", 1, get_arguments, &exception);
        void* logger = invoke_named(context, "get_InLogger", 0, nullptr, &exception);
        void* arguments[] = {buffer, context, logger};
        void* dictionary = exception ? nullptr : invoke_named(deserializer, "deserialize", 3, arguments, &exception);
        if (!dictionary || exception) {
            fprintf(report, "FAIL %s: %s\n", definition.file,
                    exception ? class_name(object_class(exception)) : "deserializer unavailable");
            if (exception) { visited_count = 0; dump_object(report, exception, 1, 2); }
            LOG("Dictionary restore failed %s", definition.file); continue;
        }
        void* restored_buffer = object_new(buffer_class);
        auto capacity_ctor = reinterpret_cast<void(*)(void*, int, void*)>(image_base + 0x7a02df0);
        capacity_ctor(restored_buffer, (int)length + 2097152, nullptr);
        void* out_logger = invoke_named(write_context, "get_OutLogger", 0, nullptr, &exception);
        void* serialize_arguments[] = {dictionary, restored_buffer, out_logger};
        runtime_invoke(serialize, write_manager, serialize_arguments, &exception);
        bool equal = false;
        if (!exception && restored_buffer) {
            void* restored_bytes = *(void**)((char*)restored_buffer + 0x10);
            int restored_length = *(int*)((char*)restored_buffer + 0x1c);
            equal = restored_bytes && restored_length == length &&
                    !memcmp((char*)bytes + 32, (char*)restored_bytes + 32, length);
        }
        fprintf(report, "%s %s class=%s bytes=%ld exact_round_trip=%s\n", equal ? "PASS" : "FAIL",
                definition.file, class_name(object_class(dictionary)), length, equal ? "true" : "false");
        LOG("Dictionary restore %s exact=%d", definition.file, equal);
        if (equal) save_object(definition.file, dictionary);
        if (equal && !strcmp(definition.file, "MissionDictionary")) export_dictionary_records(dictionary);
        const char* lists[] = {"unitTypes", "buildingTypes", "upgradeUnits", "upgradeBuildings", "upgradeLevels",
                               "profileRanks", "levels", "battleTemplates", "missions", "maps", "buildOrders", "botSettings"};
        for (const char* list_name : lists) {
            void* field = class_field_from_name(object_class(dictionary), list_name);
            if (!field) continue;
            void* list = *(void**)((char*)dictionary + field_offset(field));
            fprintf(report, "  %s count=%d\n", list_name, list ? *(int*)((char*)list + 0x18) : 0);
        }
        snprintf(filename, sizeof(filename), DATA_ROOT "dictionary-%s.txt", definition.file);
        file = fopen(filename, "w");
        if (file) { visited_count = 0; dump_object(file, dictionary, 0, 5); fclose(file); chmod(filename, 0644); }
    }
    fclose(report);
    chmod(DATA_ROOT "art3offline-dictionaries.txt", 0644);
}

static void replay_saved_messages(bool linked) {
    DIR* directory = opendir(DATA_ROOT "offline-replay");
    if (!directory) return;
    void* helper = find_class("com.geargames.common.serialization", "SerializationHelpers");
    const void* deserialize = helper ? class_method(helper, "DeserializeMessage", 2) : nullptr;
    const void* serialize = helper ? class_method(helper, "SerializeMessage", 3) : nullptr;
    void* byte_class = find_class("System", "Byte");
    void* buffer_class = find_class("com.geargames.common.utils", "ClientByteBuffer");
    FILE* report = fopen(linked ? DATA_ROOT "art3offline-linked-replay.txt" : DATA_ROOT "art3offline-replay.txt", "w");
    if (report) fchmod(fileno(report), 0644);
    if (!deserialize || !serialize || !byte_class || !buffer_class || !report) {
        LOG("Replay prerequisites unavailable");
        if (report) fclose(report);
        closedir(directory); return;
    }
    while (dirent* entry = readdir(directory)) {
        size_t name_length = strlen(entry->d_name);
        if (name_length < 9 || strcmp(entry->d_name + name_length - 9, ".meta.txt")) continue;
        char filename[512], expected_class[128] = {};
        snprintf(filename, sizeof(filename), DATA_ROOT "offline-replay/%s", entry->d_name);
        FILE* file = fopen(filename, "r");
        int message_type = -1, expected_length = 0;
        if (!file) continue;
        int parsed = fscanf(file, "{\"message_type\":%d,\"class\":\"%127[^\"]\",\"size\":%d}",
                            &message_type, expected_class, &expected_length);
        fclose(file);
        if (parsed != 3 || expected_length < 1 || expected_length > 64 * 1024 * 1024) continue;
        char base_name[256];
        snprintf(base_name, sizeof(base_name), "%s", entry->d_name);
        base_name[name_length - 9] = '\0';
        snprintf(filename, sizeof(filename), DATA_ROOT "offline-replay/%s.ggs", base_name);
        file = fopen(filename, "rb");
        if (!file) continue;
        fseek(file, 0, SEEK_END); long length = ftell(file); rewind(file);
        if (length != expected_length) { fclose(file); continue; }
        void* bytes = array_new(byte_class, length);
        if (!bytes || fread((char*)bytes + 32, 1, length, file) != (size_t)length) {
            fclose(file); continue;
        }
        fclose(file);
        void* buffer = object_new(buffer_class);
        auto ctor = reinterpret_cast<void(*)(void*, void*, void*)>(image_base + 0x7a02ee8);
        ctor(buffer, bytes, nullptr);
        void* exception = nullptr;
        void* arguments[] = {buffer, &message_type};
        void* message = nullptr;
        if (linked && linked_read_manager) {
            void* context = *(void**)((char*)linked_read_manager + 0x10);
            void* identity = *(void**)((char*)linked_read_manager + 0x20);
            invoke_named(identity, "reset", 0, nullptr, &exception);
            void* logger = invoke_named(context, "get_InLogger", 0, nullptr, &exception);
            void* linked_arguments[] = {&message_type, buffer, logger};
            message = invoke_named(linked_read_manager, "DeserializeMessage", 3, linked_arguments, &exception);
        } else message = runtime_invoke(deserialize, nullptr, arguments, &exception);
        if (exception || !message) {
            fprintf(report, "FAIL %s: deserialization %s\n", expected_class,
                    exception ? class_name(object_class(exception)) : "returned null");
            if (exception) { visited_count = 0; dump_object(report, exception, 0, 2); }
            LOG("Offline replay linked=%d failed %s", linked, expected_class);
            continue;
        }
        const char* actual_class = class_name(object_class(message));
        save_object(actual_class, message);
        int initial = 4096, maximum = 64 * 1024 * 1024;
        void* serialize_args[] = {message, &initial, &maximum};
        exception = nullptr;
        void* restored_buffer = runtime_invoke(serialize, nullptr, serialize_args, &exception);
        bool equal = false;
        if (!exception && restored_buffer) {
            void* restored_bytes = *(void**)((char*)restored_buffer + 0x10);
            int restored_length = *(int*)((char*)restored_buffer + 0x1c);
            equal = restored_bytes && restored_length == length &&
                    !memcmp((char*)bytes + 32, (char*)restored_bytes + 32, length);
        }
        fprintf(report, "%s type=%d class=%s bytes=%ld exact_round_trip=%s\n",
                !strcmp(actual_class, expected_class) && equal ? "PASS" : "FAIL",
                message_type, actual_class, length, equal ? "true" : "false");
        LOG("Offline replay %s type=%d exact=%d", actual_class, message_type, equal);
    }
    fclose(report);
    closedir(directory);
}

static void absolute_jump(void* from, void* to) {
    uint32_t instructions[] = {0x58000051, 0xd61f0220}; // ldr x17, #8; br x17
    memcpy(from, instructions, 8);
    memcpy((char*)from + 8, &to, 8);
}

static void* install_hook(uintptr_t offset, const uint32_t* expected, void* replacement) {
    auto target = (uint8_t*)(image_base + offset);
    if (memcmp(target, expected, 16)) { LOG("Unsupported hook signature at %lx", (unsigned long)offset); return nullptr; }
    void* trampoline = mmap(nullptr, 4096, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (trampoline == MAP_FAILED) return nullptr;
    size_t emitted = 0;
    for (int i = 0; i < 4; i++) {
        uint32_t word = ((uint32_t*)target)[i];
        if ((word & 0x9f000000) == 0x90000000) {
            // Relocate an ADRP without truncating the distance to the new page.
            int64_t immediate = ((word >> 29) & 3) | (((word >> 5) & 0x7ffff) << 2);
            if (immediate & 0x100000) immediate -= 0x200000;
            uintptr_t page_address = (((uintptr_t)target + 4*i) & ~uintptr_t(4095)) + immediate*4096;
            uint32_t literal[] = {uint32_t(0x58000040 | (word & 31)), 0x14000003};
            memcpy((char*)trampoline + emitted, literal, 8);
            memcpy((char*)trampoline + emitted + 8, &page_address, 8);
            emitted += 16;
        } else {
            memcpy((char*)trampoline + emitted, &word, 4); emitted += 4;
        }
    }
    absolute_jump((char*)trampoline + emitted, target + 16);
    __builtin___clear_cache((char*)trampoline, (char*)trampoline + emitted + 16);
    if (mprotect(trampoline, 4096, PROT_READ | PROT_EXEC)) return nullptr;
    size_t page_size = (size_t)sysconf(_SC_PAGESIZE);
    auto page = (void*)((uintptr_t)target & ~(page_size - 1));
    if (mprotect(page, page_size, PROT_READ | PROT_WRITE | PROT_EXEC)) return nullptr;
    absolute_jump(target, replacement);
    __builtin___clear_cache((char*)target, (char*)target + 16);
    mprotect(page, page_size, PROT_READ | PROT_EXEC);
    return trampoline;
}

#ifndef ART3_CAPTURE
static bool install_packaged_data() {
    static bool attempted;
    static bool installed;
    if (attempted) return installed;
    attempted = true;
    JNIEnv* env = nullptr;
    if (!java_vm || java_vm->GetEnv((void**)&env, JNI_VERSION_1_6) != JNI_OK) return false;
    jclass player = env->FindClass("com/unity3d/player/UnityPlayer");
    if (!player) { env->ExceptionClear(); return false; }
    jfieldID activity_field = env->GetStaticFieldID(player, "currentActivity", "Landroid/app/Activity;");
    jobject activity = activity_field ? env->GetStaticObjectField(player, activity_field) : nullptr;
    if (!activity) { env->ExceptionClear(); return false; }
    jclass activity_class = env->GetObjectClass(activity);
    jmethodID get_external = env->GetMethodID(activity_class, "getExternalFilesDir", "(Ljava/lang/String;)Ljava/io/File;");
    jobject external = get_external ? env->CallObjectMethod(activity, get_external, nullptr) : nullptr;
    if (external) env->DeleteLocalRef(external);
    if (env->ExceptionCheck()) { env->ExceptionClear(); return false; }
    chmod(DATA_ROOT, 0777);
    jmethodID get_assets = env->GetMethodID(activity_class, "getAssets", "()Landroid/content/res/AssetManager;");
    jobject java_assets = get_assets ? env->CallObjectMethod(activity, get_assets) : nullptr;
    AAssetManager* assets = java_assets ? AAssetManager_fromJava(env, java_assets) : nullptr;
    if (!assets) { env->ExceptionClear(); return false; }
    AAsset* manifest = AAssetManager_open(assets, "offline/install.list", AASSET_MODE_BUFFER);
    if (!manifest) return false;
    size_t length = (size_t)AAsset_getLength64(manifest);
    char* lines = (char*)malloc(length + 1);
    if (!lines) { AAsset_close(manifest); return false; }
    memcpy(lines, AAsset_getBuffer(manifest), length); lines[length] = '\0';
    AAsset_close(manifest);
    int copied = 0, checked = 0;
    bool ok = true;
    char* state = nullptr;
    for (char* line = strtok_r(lines, "\r\n", &state); line; line = strtok_r(nullptr, "\r\n", &state)) {
        long long expected = 0;
        char relative[512], path[1024], asset_name[1024];
        if (sscanf(line, "%lld\t%511[^\n]", &expected, relative) != 2 || expected < 0 ||
            strstr(relative, "..") || relative[0] == '/') { ok = false; break; }
        snprintf(path, sizeof(path), DATA_ROOT "%s", relative);
        struct stat info{};
        checked++;
        // Refresh this small index on every APK update/cold install pass;
        // same-sized route edits must not leave a stale catalogue on disk.
        if (strcmp(relative,"offline-campaign/catalog.bin") && strcmp(relative,"offline-cooperative/catalog.bin") && !stat(path, &info) && info.st_size == expected) continue;
        for (char* slash = path + strlen(DATA_ROOT); (slash = strchr(slash, '/')); slash++) {
            *slash = '\0'; mkdir(path, 0777); chmod(path, 0777); *slash = '/';
        }
        snprintf(asset_name, sizeof(asset_name), "offline/%s", relative);
        AAsset* asset = AAssetManager_open(assets, asset_name, AASSET_MODE_STREAMING);
        if (!asset || AAsset_getLength64(asset) != expected) {
            LOG("Packaged asset unavailable: %s expected=%lld", asset_name, expected);
            if (asset) AAsset_close(asset); ok = false; break;
        }
        char temporary[1100]; snprintf(temporary, sizeof(temporary), "%s.part", path);
        FILE* file = fopen(temporary, "wb");
        if (!file) { AAsset_close(asset); ok = false; break; }
        char buffer[65536]; long long written = 0;
        while (int amount = AAsset_read(asset, buffer, sizeof(buffer))) {
            if (amount < 0 || fwrite(buffer, 1, amount, file) != (size_t)amount) { ok = false; break; }
            written += amount;
        }
        fclose(file); AAsset_close(asset);
        if (!ok || written != expected || rename(temporary, path)) { ok = false; break; }
        chmod(path, 0644); copied++;
    }
    free(lines);
    env->DeleteLocalRef(java_assets); env->DeleteLocalRef(activity_class);
    env->DeleteLocalRef(activity); env->DeleteLocalRef(player);
    installed = ok;
    LOG("Packaged data checked=%d copied=%d success=%d", checked, copied, ok);
#ifdef ART3_MAIN
    if (ok) { FILE* flag = fopen(DATA_ROOT "offline-main.enabled", "w"); if (flag) fclose(flag); }
#endif
    return installed;
}

static void (*logon_update_original)(void*, void*);
static bool main_attempted;
static void* logon_manager;
static void* offline_network;
static local_progress::State progress;
static bool progress_ready, progress_checked, local_run_active;
static uint64_t local_run;
static int local_mission_id = 200;
#include "campaign_catalog.h"
using LocalScenario=campaign_catalog::Entry;
static campaign_catalog::Catalog local_scenarios;
static const LocalScenario* local_scenario(int chapter) {
    for (const auto& scenario : local_scenarios) if (scenario.chapter == chapter && scenario.faction==local_faction::active) return &scenario;
    return nullptr;
}
static bool local_scenario_unlocked(const LocalScenario& scenario) {
    if(scenario.faction!=local_faction::active)return false;
    if (scenario.previous < 0) return true;
    for (unsigned i = 0; i < progress.count; ++i)
        if (progress.missions[i].id == scenario.previous && progress.missions[i].wins > 0) return true;
    return false;
}

static void* static_object(const char* ns, const char* type, const char* name) {
    void* klass = find_class(ns, type);
    void* field = klass ? class_field_from_name(klass, name) : nullptr;
    void* value = nullptr; if (field) static_get(field, &value); return value;
}
static void* list_item(void* list, int i) {
    if (!list || i < 0 || i >= *(int*)((char*)list + 0x18)) return nullptr;
    void* items = *(void**)((char*)list + 0x10);
    return items && size_t(i) < array_length(items) ? ((void**)((char*)items + 32))[i] : nullptr;
}
static void* local_mission_description(int id) {
    void* dictionary = saved("MissionDictionary");
    void* field = dictionary ? class_field_from_name(object_class(dictionary), "missionDescriptions") : nullptr;
    void* list = field ? *(void**)((char*)dictionary + field_offset(field)) : nullptr;
    int count = list ? *(int*)((char*)list + 0x18) : 0;
    for (int i = 0; i < count; ++i) {
        void* item = list_item(list, i);
        if (item && *(int*)((char*)item + 0x10) == id) return item;
    }
    return nullptr;
}
static void* local_upgrade_level(int id) {
    void* dictionary = saved(local_faction::key());
    void* field = dictionary ? class_field_from_name(object_class(dictionary), "upgradeLevels") : nullptr;
    void* list = field ? *(void**)((char*)dictionary + field_offset(field)) : nullptr;
    int count = list ? *(int*)((char*)list + 0x18) : 0;
    for (int i = 0; i < count; ++i) {
        void* level = list_item(list, i);
        if (level && *(int*)((char*)level + 0x10) == id) return level;
    }
    return nullptr;
}
#include "profile_statistics_runtime.h"
#include "faction_runtime.h"
#include "campaign_presentation.h"

static bool restore_local_research(void* user) {
    void* config = *(void**)((char*)user + 0x100);
    void* completed = config ? *(void**)((char*)config + 0x40) : nullptr;
    if (!completed) return progress.upgrade_count == 0;
    for (uint32_t i = 0; i < progress.upgrade_count; ++i) {
        int id = progress.upgrade_ids[i]; bool found = false;
        int count = *(int*)((char*)completed + 0x18);
        for (int j = 0; j < count; ++j) {
            void* level = list_item(completed, j);
            if (level && *(int*)((char*)level + 0x10) == id) { found = true; break; }
        }
        if (found) continue;
        void* level = local_upgrade_level(id);
        if (!level) { LOG("Missing saved research level=%d", id); return false; }
        void* exception = nullptr; void* args[] = {level};
        invoke_named(completed, "Add", 1, args, &exception);
        if (exception) return false;
    }
    LOG("Local research restored upgrades=%u", progress.upgrade_count);
    return true;
}
static bool restore_local_unit_unlocks(void* user) {
    void* config = *(void**)((char*)user + 0x100);
    void* available = config ? *(void**)((char*)config + 0x48) : nullptr;
    void* base = saved(local_faction::key());
    void* battle = saved("BATTLE");
    void* stones_field = base ? class_field_from_name(object_class(base), "careerStones") : nullptr;
    void* units_field = battle ? class_field_from_name(object_class(battle), "unitTypes") : nullptr;
    void* stones = stones_field ? *(void**)((char*)base + field_offset(stones_field)) : nullptr;
    void* units = units_field ? *(void**)((char*)battle + field_offset(units_field)) : nullptr;
    if (!available || !stones || !units) return false;
    int stone_count = *(int*)((char*)stones + 0x18);
    int unit_count = *(int*)((char*)units + 0x18);
    int unlocked = 0;
    for (int i = 0; i < stone_count; ++i) {
        void* stone = list_item(stones, i);
        if (!stone || *(int*)((char*)stone + 0x20) > progress.stars ||
            !local_progress::has_career(progress, *(int*)((char*)stone + 0x10))) continue;
        void* rewards = *(void**)((char*)stone + 0x18);
        int count = rewards ? *(int*)((char*)rewards + 0x18) : 0;
        for (int j = 0; j < count; ++j) {
            void* reward = list_item(rewards, j);
            // RewardItem.TYPE_UNIT_PROTOTYPE. Other milestone rewards need
            // their own persisted claim transaction; never grant them blindly.
            if (!reward || *(int*)((char*)reward + 0x14) != 21) continue;
            int id = *(int*)((char*)reward + 0x18);
            void* unit = nullptr;
            for (int k = 0; k < unit_count; ++k) {
                void* item = list_item(units, k);
                if (item && *(int*)((char*)item + 0x10) == id) { unit = item; break; }
            }
            if (!unit) { LOG("Career unit definition missing id=%d", id); return false; }
            void* exception = nullptr; void* args[] = {unit};
            invoke_named(available, "Add", 1, args, &exception);
            if (exception) return false;
            ++unlocked;
            LOG("Local earned unit id=%d requiredStars=%d currentStars=%d", id,
                *(int*)((char*)stone + 0x20), progress.stars);
        }
    }
    LOG("Local earned unit unlocks restored=%d", unlocked);
    return true;
}
#include "career_runtime.h"
#include "career_box_runtime.h"
#include "hero_runtime.h"
#include "skill_runtime.h"
#include "reward_skill_runtime.h"
#include "skill_hero_capacity_runtime.h"
#include "quest_runtime.h"
#include "contract_runtime.h"
#include "warpoint_runtime.h"
#include "career_star_runtime.h"
static bool apply_local_progress();
static bool register_shop_start(shop_box_policy::Mode mode,uint64_t run){
    if(!progress_ready)return false;local_progress::StateBuffer storage;if(!storage.data)return false;
    auto& next=*storage.data;next=progress;
    if(!shop_box_policy::begin(next.shop,mode,run))return false;
    if(mode==shop_box_policy::Cooperative&&local_faction::active==2&&local_skill::rank(progress)>=competition_track_content::rank&&
       !competition_track_policy::begin(next.competition_tracks,competition_track_content::competition,competition_track_content::definition,run))return false;
    if(!local_progress::write(local_faction::root(),next))return false;
    progress=next;return true;
}
namespace local_modifications {static bool prepare_profile(void*);static bool publish();static void install();}
#include "item_stock_runtime.h"
#include "supply_runtime.h"
namespace local_boost_conversion {static void* set_shop_handler(void(*)(void*,int,void*));}
#include "shop_box_runtime.h"
#include "contract_box_runtime.h"
#include "effective_box_runtime.h"
#include "competition_track_runtime.h"
#include "daily_box_content_audit.h"

static bool apply_local_progress() {
    if(!local_scenarios.load(DATA_ROOT)){LOG("Local campaign catalogue unavailable/corrupt; preserving progress");return false;}
    LOG("Local campaign catalogue ready entries=%d",local_scenarios.count);
    void* user = saved("UserConfiguration");
    void* profile = user ? *(void**)((char*)user + 0x28) : nullptr;
    if (!profile) return false;
    if (!progress_checked) {
        int status = local_progress::load(local_faction::root(), progress);
        progress_checked = true;
        if (status < 0) { LOG("Local progress unreadable; preserving files and refusing seed overwrite"); return false; }
        if (!status) {
            if(offline_accounts::fresh_account()&&local_faction::active==2&&
               (*(int*)((char*)profile+0x30)||*(int*)((char*)profile+0x68))){LOG("Fresh account refused non-starter template; no save overwrite");return false;}
            progress = local_faction::active==1 ? local_progress::seed(0,0,0,0) :
                local_progress::seed(*(int*)((char*)profile + 0xd0), *(int*)((char*)user + 0x20),
                *(int*)((char*)profile + 0x30), *(int*)((char*)profile + 0x68));
            if (!local_progress::write(local_faction::root(), progress)) { LOG("Local seed save failed"); return false; }
        }
        progress_ready = true;
        // These files contain only local gameplay state and can be backed up by
        // the development ADB collector, like the existing configuration reports.
        char slot[640];for(int i:{0,1}){snprintf(slot,sizeof(slot),"%s/local-progress.%d",local_faction::root(),i);chmod(slot,0644);}
    }
    if (!progress_ready || !local_warpoints::sync()) return false;
    if (!local_career::apply(user) || !local_hero::restore(user)) return false;
    local_hero::audit();
    local_career::mixed_audit();
    *(int*)((char*)profile + 0xd0) = progress.credits;
    *(int*)((char*)profile + 0x30) = progress.experience;
    *(int*)((char*)profile + 0x68) = progress.stars;
    int account_gold=0;if(!local_faction::account_gold(progress,account_gold))return false;
    *(int*)((char*)user + 0x20) = account_gold;
    void* list = *(void**)((char*)user + 0x38);
    if (!list) return false;
    auto available = progress;
    for (const auto& scenario : local_scenarios) {
        bool present = false;
        for (uint32_t i = 0; i < available.count; ++i)
            if (available.missions[i].id == scenario.chapter) present = true;
        if (!present && local_scenario_unlocked(scenario) && !access(scenario.file, F_OK) && available.count < 256) {
            available.missions[available.count] = {};
            available.missions[available.count++].id = scenario.chapter;
        }
    }
    for (uint32_t i = 0; i < available.count; ++i) {
        const auto& record = available.missions[i];
        void* result = nullptr;
        int count = *(int*)((char*)list + 0x18);
        for (int j = 0; j < count; ++j) {
            void* item = list_item(list, j);
            void* desc = item ? *(void**)((char*)item + 0x10) : nullptr;
            if (desc && *(int*)((char*)desc + 0x10) == record.id) { result = item; break; }
        }
        void* description = local_presented_mission(record.id);
        if (!description) { LOG("Missing saved mission description=%d", record.id); return false; }
        void* exception = nullptr;
        if (!result) {
            result = object_new(find_class("com.geargames.aow.entities", "MissionResult"));
            if (!result) return false;
            invoke_named(result, ".ctor", 0, nullptr, &exception);
            field_set(result, class_field_from_name(object_class(result), "description"), description);
            void* args[] = {result}; invoke_named(list, "Add", 1, args, &exception);
            if (exception) return false;
        } else field_set(result, class_field_from_name(object_class(result), "description"), description);
        *(int8_t*)((char*)result + 0x18) = record.stars;
        *(int16_t*)((char*)result + 0x1a) = record.xp;
        *(int16_t*)((char*)result + 0x22) = record.wins > 32767 ? 32767 : record.wins;
        // The save keeps primary then secondary objectives. This original UI
        // accepts secondary objectives only, matching Description.AimStr/AimXp.
        void* aim_text = *(void**)((char*)description + 0x68);
        void* aim_xp = *(void**)((char*)description + 0x60);
        size_t secondary_count = aim_text ? array_length(aim_text) : 0;
        if (!aim_text || !aim_xp || secondary_count != array_length(aim_xp) || secondary_count > 32 ||
            (record.wins && record.objective_count < secondary_count)) {
            LOG("Local mission objective presentation mismatch chapter=%d", record.id); return false;
        }
        void* objectives = array_new(find_class("System", "Boolean"), secondary_count);
        if (!objectives) return false;
        uint32_t secondary_start = record.wins ? record.objective_count - secondary_count : 0;
        for (uint32_t j = 0; j < secondary_count; ++j)
            *((uint8_t*)objectives + 32 + j) = record.wins && ((record.objectives >> (secondary_start + j)) & 1);
        field_set(result, class_field_from_name(object_class(result), "objectives"), objectives);
    }
    LOG("Local progress applied generation=%llu missions=%u credits=%d xp=%d stars=%d",
        (unsigned long long)progress.generation, progress.count, progress.credits, progress.experience, progress.stars);
    return restore_local_research(user) && restore_local_unit_unlocks(user) && local_quests::prepare_profile(user) && local_contracts::prepare_profile(user) && local_items::prepare(user,progress) && local_modifications::prepare_profile(user) && local_supply::prepare_profile(user);
}

static bool local_research_supported(void* level);
static void (*local_research_view_refresh)();
static void (*upgrade_action_original)(void*, void*);
static void local_upgrade_action(void* self, void* method) {
    if (!offline_network) { upgrade_action_original(self, method); return; }
    if (!progress_ready) return;
    void* exception = nullptr;
    void* state = invoke_named(self, "get_CanStartState", 0, nullptr, &exception);
    void* value = invoke_named(state, "get_Value", 0, nullptr, &exception);
    // Retain the original requirements, rank, resource, slot and matching checks.
    if (exception || !value || *(int*)((char*)value + 16) != 1) { LOG("Local upgrade rejected by original availability"); return; }
    void* slot = *(void**)((char*)self + 0x20);
    void* next_level = invoke_named(slot, "get_NextUpgradeLevel", 0, nullptr, &exception);
    void* level = invoke_named(next_level, "get_Value", 0, nullptr, &exception);
    void* costs = invoke_named(slot, "get_Costs", 0, nullptr, &exception);
    void* discounted = invoke_named(costs, "get_CostDiscounted", 0, nullptr, &exception);
    void* price = invoke_named(discounted, "get_Value", 0, nullptr, &exception);
    void* service = *(void**)((char*)self + 0x10);
    void* research = service ? *(void**)((char*)service + 0x30) : nullptr;
    void* waiter = service ? *(void**)((char*)service + 0x18) : nullptr;
    void* network = waiter ? *(void**)((char*)waiter + 0x18) : nullptr;
    void* dispatcher = network ? *(void**)((char*)network + 0x88) : nullptr;
    void* profile = static_object("com.geargames.aow.config", "ClientContext", "m_activeProfile");
    if (exception || !level || !price || !research || !profile || !dispatcher ||
        !class_method(object_class(research), "CompleteUpgrade", 1) ||
        !class_method(object_class(profile), "set_Credits", 1)) { LOG("Local upgrade dependencies unavailable"); return; }
    int id = *(int*)((char*)level + 0x10);
    if (!local_research_supported(level)) {
        LOG("Local upgrade rejected without debit: combat effect not implemented or source delta mismatch id=%d", id);
        return;
    }
    int cost = *(int*)((char*)price + 16);
    int xp = *(int*)((char*)level + 0x28);
    int cards=*(int*)((char*)level+0x54),hero=0,opening_cards=0,hero_grade=0;
    local_hero::Roots hero_roots;local_hero::Publication hero_publication;
    if(cards&&(!local_hero::class_price(level,local_faction::active,hero,opening_cards,hero_grade)||cards!=opening_cards)){
        LOG("Local hero class research refused without debit id=%d",id);return;}
    auto next = progress;
    bool ok = cards?local_progress::buy_card_upgrade(next,id,cost,xp,hero,cards):local_progress::buy_upgrade(next,id,cost,xp);
    if(ok&&cards)ok=local_hero::prepare_publication(next,hero,hero_roots,hero_publication);
    ok=ok&&local_progress::write(local_faction::root(), next);
    if (!ok) { LOG("Local research save/debit refused id=%d cost=%d", id, cost); return; }
    progress = next;
    if(cards&&!local_hero::publish(hero_publication)){LOG("Local hero acquired saved=1 publication_failed=1 hero=%d; cold restore required",hero);return;}
    // Publish through the original local dispatcher: CreditsResourceService
    // updates both ClientProfile and its reactive resource/UI balance.
    void* balance = object_new(find_class("com.geargames.aow.messages", "SetProfileCredits"));
    void* credit_args[] = {&progress.credits};
    invoke_named(balance, ".ctor", 1, credit_args, &exception);
    void* balance_args[] = {balance};
    if (!exception) invoke_named(dispatcher, "Dispatch", 1, balance_args, &exception);
    void* profile_entity = *(void**)((char*)profile + 0x10);
    if (profile_entity) *(int*)((char*)profile_entity + 0x30) = progress.experience;
    void* args[] = {level};
    if (!exception) invoke_named(research, "CompleteUpgrade", 1, args, &exception);
    LOG("Local research saved=1 applied=%d id=%d cost=%d xp=%d credits=%d upgrades=%u",
        !exception, id, cost, xp, progress.credits, progress.upgrade_count);
    if(!exception&&local_research_view_refresh)local_research_view_refresh();
    if(!exception)local_quests::refresh();
    if(!exception)local_contracts::refresh();
}

namespace local_match {
static bool playing, pending_result, presenting, save_failed;
static uint64_t run;
static local_match_store::State state;
static void finished(bool win,int tick,int last_tick);
static void frame();
static bool publish();
static void install();
}
namespace local_coop {
static bool playing,pending_result,presenting;
static local_match_store::State state;
static uint64_t battle_points;
static uint32_t last_award,last_pumpkins;
static void step(void* battle);
static void frame();
static void finished(bool win,int tick,int last_tick);
static bool publish();
static bool start();
static void install();
}
static bool start_local_match(int selected_chapter=-1,int selected_opponent=0,int selected_mode=0,bool formal=false);
static void (*logic_stop_original)(void*, bool, int, int, void*);
namespace local_duel {
static bool active;
static void record_order(void* battle,void* command,int before,int after,int code);
static int seat_count();
static void step(void* battle);
static bool allow_script();
static void finished(bool win,int finish_tick,int last_tick);
}
static void (*army_tab_original)(void*, void*);
static int (*building_max_health_original)(void*, void*);
static void (*battle_upgrade_check_original)(void*, void*);
static void audit_local_unit_health();
namespace local_inventory { static void replay_input(); }
namespace local_campaign_results { static void diagnostic_frame(); }
#include "interaction_trace.h"
static int local_building_max_health(void* self, void* method) {
    int result = building_max_health_original(self, method);
    static uint64_t audited_run;
    if (local_run_active && audited_run != local_run &&
        !access(DATA_ROOT "research-live-audit.enabled", F_OK)) {
        void* own = static_object("com.geargames.aow.scenes.battle", "ClientBattleContext", "m_ownSide");
        void* type = self ? *(void**)((char*)self + 0x98) : nullptr;
        if (type && *(int8_t*)((char*)type + 0x30) == 1 &&
            *(void**)((char*)self + 0x88) == own) {
            void* side = own ? *(void**)((char*)own + 0x28) : nullptr;
            int health = *(int*)((char*)self + 0x118);
            int base = side ? *(int*)((char*)side + 0x17c) : -1;
            int increment = side ? *(int*)((char*)side + 0x180) : -1;
            FILE* file = fopen(DATA_ROOT "art3offline-research-live-hq.txt", "w");
            if (file) {
                fprintf(file, "run=%llu chapter=%d original_engine_max_health=%d current_health=%d income=%d/%d\n",
                    (unsigned long long)local_run, local_mission_id, result, health, base, increment);
                fclose(file); chmod(DATA_ROOT "art3offline-research-live-hq.txt", 0644);
            }
            LOG("Research live HQ run=%llu originalMaxHealth=%d currentHealth=%d income=%d/%d",
                (unsigned long long)local_run, result, health, base, increment);
            audited_run = local_run;
        }
    }
    return result;
}
static void research_battle_upgrade_check(void* self, void* method) {
    battle_upgrade_check_original(self, method);
    interaction_trace::frame(self);
    local_inventory::replay_input();
    local_campaign_results::diagnostic_frame();
    local_match::frame();
    local_coop::frame();
    audit_local_unit_health();
    if (!local_run_active || access(DATA_ROOT "research-live-audit.enabled", F_OK)) return;
    static uint64_t checked_run;
    if (checked_run == local_run) return;
    void* own = static_object("com.geargames.aow.scenes.battle", "ClientBattleContext", "m_ownSide");
    void* list = own ? *(void**)((char*)own + 0xe0) : nullptr;
    void* hq = list_item(list, 0);
    if (!hq) return;
    local_building_max_health(hq, nullptr);
    checked_run = local_run;
}
static void army_tab_hook(void* self, void* method) {
    static void* last_repository;
    void* repository = *(void**)((char*)self + 0x30);
    if (repository && repository != last_repository) {
        last_repository = repository;
        FILE* f = fopen(DATA_ROOT "art3offline-research.txt", "w");
        if (f) {
            visited_count = 0; dump_object(f, repository, 0, 4);
            fclose(f); chmod(DATA_ROOT "art3offline-research.txt", 0644);
        }
    }
    army_tab_original(self, method);
}
static void apply_local_mission_info(void* self) {
    if (!offline_network || !progress_ready || !self) return;
    void* result = *(void**)((char*)self + 0x20);
    void* description = result ? *(void**)((char*)result + 0x10) : nullptr;
    const auto* scenario = description ? local_scenario(*(int*)((char*)description + 0x10)) : nullptr;
    void* actual = scenario ? local_mission_description(scenario->recorded) : nullptr;
    if (!actual) return;
    int low = *(int*)((char*)actual + 0x54), high = *(int*)((char*)actual + 0x58);
    if (low < 0 || high < low) { LOG("Local reward preview refused invalid range"); return; }
    void* amount = *(void**)((char*)self + 0x60);
    void* maximum = *(void**)((char*)self + 0x68);
    void* hint = *(void**)((char*)self + 0x78);
    if (!amount || !maximum || !hint || !class_method(object_class(amount), "set_text", 1) ||
        !class_method(object_class(maximum), "set_text", 1) || !class_method(object_class(hint), "SetReplacer", 1)) {
        LOG("Local reward preview labels unavailable"); return;
    }
    int wins = 0, earned = 0;
    for (uint32_t i = 0; i < progress.count; ++i) if (progress.missions[i].id == scenario->chapter) {
        wins = progress.missions[i].wins; earned = progress.missions[i].credits;
    }
    char range[64], detail[768];
    if (low == high) snprintf(range, sizeof(range), "%d", low);
    else snprintf(range, sizeof(range), "%d~%d", low, high);
    // The local policy pays every real win; the original online Next/TotalCredits
    // fields describe a different reward schedule and stay untouched.
    snprintf(detail, sizeof(detail),
        "本地战役每次胜利按星级奖励资源：\n1星 %d，2星 %d，3星 %d。\n失败不发奖；重玩可获得资源，经验和生涯星数只补最佳成绩差额。\n本关已胜利 %d 次，累计获得 %d 资源。",
        low, low + (high - low) / 2, high, wins, earned);
    void* exception = nullptr;
    void* value = string_new(range); void* args[] = {value};
    invoke_named(amount, "set_text", 1, args, &exception);
    if (exception) { LOG("Local reward preview amount failed"); return; }
    value = string_new(""); args[0] = value;
    invoke_named(maximum, "set_text", 1, args, &exception);
    if (exception) { LOG("Local reward preview maximum failed"); return; }
    value = string_new(detail); args[0] = value;
    invoke_named(hint, "SetReplacer", 1, args, &exception);
    if (exception) { LOG("Local reward preview hint failed"); return; }
    // The online star animation derives partial stars from Xp2/Xp3. Local
    // stars are real persisted results and must not use those zero thresholds.
    void* stars = *(void**)((char*)self + 0x58);
    void* stars_hint = *(void**)((char*)self + 0x50);
    const void* fill = class_method(find_class("", "UIBasicSprite"), "set_fillAmount", 1);
    int best_stars = *(int8_t*)((char*)result + 0x18);
    if (!stars || array_length(stars) != 3 || !fill || !stars_hint || best_stars < 0 || best_stars > 3) {
        LOG("Local mission star presentation unavailable"); return;
    }
    for (int i = 0; i < 3; ++i) {
        void* sprite = ((void**)((char*)stars + 32))[i];
        if (!sprite) { LOG("Local mission star sprite missing"); return; }
        float amount = i < best_stars ? 1.f : 0.f;
        void* fill_args[] = {&amount}; runtime_invoke(fill, sprite, fill_args, &exception);
        if (exception) { LOG("Local mission star fill failed"); return; }
        bool shadowed = i >= best_stars; void* shadow_args[] = {sprite, &shadowed};
        invoke_named(self, "MakeStarShadowed", 2, shadow_args, &exception);
        if (exception) { LOG("Local mission star shadow failed"); return; }
    }
    snprintf(detail, sizeof(detail),
        "本关最佳成绩：%d星。\n胜利获得1星，完成全部次要目标再得1星，在目标时间内完成再得1星。\n重玩只补最佳成绩的经验和生涯星数差额。", best_stars);
    value = string_new(detail); args[0] = value;
    invoke_named(stars_hint, "SetReplacer", 1, args, &exception);
    LOG("Local reward preview chapter=%d range=%s wins=%d earned=%d applied=%d",
        scenario->chapter, range, wins, earned, !exception);
}
static void (*mission_info_original)(void*, void*, void*, void*, void*);
#include "campaign_result_runtime.h"
static void local_mission_info(void* self, void* mission, void* container, void* number, void* method) {
    mission_info_original(self, mission, container, number, method);
    apply_local_mission_info(self);
}
static void local_logic_stop(void* self, bool win, int finish_tick, int last_tick, void* method) {
    if(local_coop::playing) local_coop::finished(win,finish_tick,last_tick);
    else if(local_match::playing) local_match::finished(win,finish_tick,last_tick);
    else if(local_duel::active) local_duel::finished(win,finish_tick,last_tick);
    if (local_run_active && progress_ready) {
        const auto* scenario = local_scenario(local_mission_id);
        void* desc = local_mission_description(scenario ? scenario->recorded : local_mission_id);
        void* side = static_object("com.geargames.aow.scenes.battle", "ClientBattleContext", "m_ownSide");
        void* logic_side = side ? *(void**)((char*)side + 0x28) : nullptr;
        uint32_t objective_count = 0, completed = 0;
        bool secondary_complete = true;
        // The original logic side keeps primary and secondary BattleObjective lists.
        if (logic_side) for (int offset : {0x370, 0x380}) {
            void* list = *(void**)((char*)logic_side + offset);
            int count = list ? *(int*)((char*)list + 0x18) : 0;
            for (int j = 0; j < count && objective_count < 32; ++j, ++objective_count) {
                void* objective = list_item(list, j);
                bool done = objective && *(int8_t*)((char*)objective + 0x15) == 1;
                if (done) completed |= 1u << objective_count;
                if (offset == 0x380 && !done) secondary_complete = false;
            }
        }
        // Initial local scoring: clear, secondary objectives, fast completion.
        // Uses real scenario status/ticks; defeat never receives win rewards.
        int stars = win ? 1 + int(secondary_complete) : 0;
        int quick_tick = desc ? *(int*)((char*)desc + 0x74) : 0;
        if (win && secondary_complete && finish_tick > 0 && quick_tick > 0 && finish_tick <= quick_tick) ++stars;
        int max_xp = desc ? *(int*)((char*)desc + 0x48) : 0;
        int low = desc ? *(int*)((char*)desc + 0x54) : 0;
        int high = desc ? *(int*)((char*)desc + 0x58) : 0;
        int xp = win ? max_xp * stars / 3 : 0;
        int credits = win ? low + (high - low) * (stars - 1) / 2 : 0;
        auto next = progress;
        void* native_battle=static_object("com.geargames.aow.scenes.battle","ClientBattleContext","m_battle");
        int native_reason=native_battle?*(int*)((char*)native_battle+0xd8):0;bool free_changed=false;
        bool ok = desc && logic_side && local_progress::settle(next, local_run, local_mission_id, win, stars, xp,
            credits, objective_count, completed) && shop_box_policy::finish(next.shop,shop_box_policy::Campaign,local_run,native_reason==1,free_changed) && local_progress::write(local_faction::root(), next);
        if (ok) {
            local_campaign_results::capture(win,stars,xp,objective_count,completed,progress,next);
            progress = next; local_run_active = false;
        }
        LOG("Local settlement saved=%d run=%llu win=%d tick=%d stars=%d xp=%d credits=%d objectives=%u/%x",
            ok, (unsigned long long)local_run, win, finish_tick, stars, xp, credits, objective_count, completed);
    }
    logic_stop_original(self, win, finish_tick, last_tick, method);
}
#ifdef ART3_LOCAL_MENU
static pthread_mutex_t menu_action_lock = PTHREAD_MUTEX_INITIALIZER;
static char menu_action[256];

static void queue_menu_action(JNIEnv* env, jclass, jstring value) {
    if (!value) return;
    const char* action = env->GetStringUTFChars(value, nullptr);
    pthread_mutex_lock(&menu_action_lock);
    if (!menu_action[0] && action) snprintf(menu_action, sizeof(menu_action), "%s", action);
    pthread_mutex_unlock(&menu_action_lock);
    if (action) env->ReleaseStringUTFChars(value, action);
}

static bool menu_call(const char* method_name, const char* title = nullptr, const char* body = nullptr) {
    JNIEnv* env = nullptr;
    if (!java_vm || java_vm->GetEnv((void**)&env, JNI_VERSION_1_6) != JNI_OK) return false;
    jclass bridge = env->FindClass("com/art3rts/offline/OfflineMenuBridge");
    if (!bridge) { env->ExceptionClear(); LOG("Local menu Java bridge unavailable"); return false; }
    JNINativeMethod binding = {const_cast<char*>("queueAction"), const_cast<char*>("(Ljava/lang/String;)V"), (void*)queue_menu_action};
    if (env->RegisterNatives(bridge, &binding, 1)) {
        env->ExceptionClear(); env->DeleteLocalRef(bridge); return false;
    }
    if (!strcmp(method_name, "show")) {
        jclass player = env->FindClass("com/unity3d/player/UnityPlayer");
        jfieldID field = env->GetStaticFieldID(player, "currentActivity", "Landroid/app/Activity;");
        jobject activity = env->GetStaticObjectField(player, field);
        jmethodID show = env->GetStaticMethodID(bridge, "show", "(Landroid/app/Activity;)V");
        env->CallStaticVoidMethod(bridge, show, activity);
        env->DeleteLocalRef(activity); env->DeleteLocalRef(player);
    } else if (!strcmp(method_name, "message")) {
        jstring java_title = env->NewStringUTF(title ? title : "");
        jstring java_body = env->NewStringUTF(body ? body : "");
        jmethodID message = env->GetStaticMethodID(bridge, "message", "(Ljava/lang/String;Ljava/lang/String;)V");
        env->CallStaticVoidMethod(bridge, message, java_title, java_body);
        env->DeleteLocalRef(java_title); env->DeleteLocalRef(java_body);
    } else {
        jmethodID hide = env->GetStaticMethodID(bridge, "hide", "()V");
        env->CallStaticVoidMethod(bridge, hide);
    }
    bool success = !env->ExceptionCheck();
    if (!success) { env->ExceptionDescribe(); env->ExceptionClear(); }
    env->DeleteLocalRef(bridge);
    return success;
}

#endif

static void* read_scenario_message(const char* path, int type, const char* root) {
    FILE* file = fopen(path, "rb");
    if (!file) return nullptr;
    fseek(file, 0, SEEK_END); long length = ftell(file); rewind(file);
    if (length < 32 || length > 16*1024*1024) { fclose(file); return nullptr; }
    void* bytes = array_new(find_class("System", "Byte"), length);
    bool read = bytes && fread((char*)bytes + 32, 1, length, file) == (size_t)length;
    fclose(file);
    if (!read) return nullptr;
    void* buffer = object_new(find_class("com.geargames.common.utils", "ClientByteBuffer"));
    reinterpret_cast<void(*)(void*, void*, void*)>(image_base + 0x7a02ee8)(buffer, bytes, nullptr);
    void* helper = find_class("com.geargames.common.serialization", "SerializationHelpers");
    void* args[] = {buffer, &type};
    void* exception = nullptr;
    void* notification = runtime_invoke(class_method(helper, "DeserializeMessage", 2), nullptr, args, &exception);
    if (!notification || exception) { LOG("Local scenario decoding failed type=%d", type); return nullptr; }
    save_object(root, notification);
    return notification;
}

static void* local_dictionary_item(void* dictionary, int key) {
    if (!dictionary) return nullptr;
    void* value = nullptr; void* exception = nullptr;
    void* args[] = {&key, &value};
    void* found = invoke_named(dictionary, "TryGetValue", 2, args, &exception);
    return !exception && found && *(bool*)((char*)found + 16) ? value : nullptr;
}

static void* local_dictionary_values(void* dictionary, void* value_class, uintptr_t& handle) {
    if (!dictionary || !value_class) return nullptr;
    void* exception = nullptr;
    void* values = invoke_named(dictionary, "get_Values", 0, nullptr, &exception);
    void* boxed_count = values ? invoke_named(values, "get_Count", 0, nullptr, &exception) : nullptr;
    int count = boxed_count && !exception ? *(int*)((char*)boxed_count + 16) : -1;
    if (count < 0 || count > 8192) return nullptr;
    void* array = array_new(value_class, count);
    if (!array) return nullptr;
    handle = gchandle_new(array, true);
    int start = 0; void* args[] = {array, &start};
    invoke_named(values, "CopyTo", 2, args, &exception);
    if (exception) { gchandle_free(handle); handle = 0; return nullptr; }
    return array;
}

#include "research_runtime.h"
#include "hero_parameter_runtime.h"
#include "research_batch_audit.h"
#include "local_mobility_audit.h"
#include "growth_closure_audit.h"
#include "modification_effect_runtime.h"
#include "modification_runtime.h"
#include "research_view_runtime.h"
#include "campaign_seed_factory.h"
#include "ai_tech_dependency_audit.h"
#include "hero_battle_projection_audit.h"
#include "hero_class_effects_audit.h"
#include "skill_runtime_audit.h"
#include "skill_protection_audit.h"
#include "hero_seraphim_audit.h"
#include "hero_wasp_audit.h"
#include "hero_beholder_audit.h"
#include "hero_gatling_audit.h"
#include "hero_solaris_audit.h"
#include "hero_salamander_audit.h"
#include "hero_atlant_audit.h"
#include "hero_coil_audit.h"
#include "hero_battle_runtime.h"
#include "item_use_guards.h"
#include "preplaced_research_audit.h"
#include "hero_activation_audit.h"
#include "battery_native_audit.h"
#include "obsolete_speedup_audit.h"
#include "building_research_audit.h"
#include "building_economy_audit.h"
#include "hq_capacity_audit.h"
#include "building_weapon_audit.h"
#include "building_sight_audit.h"
#include "hero_retained_classes_audit.h"
#include "cooperative_input_audit.h"
#include "campaign_capture_audit.h"
#include "campaign_growth_audit.h"
#include "ai_spawn_audit.h"
#include "skill_waiver_audit.h"
#include "skill_chest_audit.h"
#include "skill_economy_audit.h"
#include "skill_flag_audit.h"
#include "skill_hero_protection_audit.h"
#include "skill_hero_capacity_audit.h"
#include "skill_phase3_audit.h"

static void research_effects_probe() {
    if (access(DATA_ROOT "research-effects-probe.enabled", F_OK)) return;
    FILE* file = fopen(DATA_ROOT "art3offline-research-effects.txt", "w");
    if (!file) return;
    chmod(DATA_ROOT "art3offline-research-effects.txt", 0644);
    int owner_id = -1;
    for (int id : {13000, 13060}) {
        void* level = local_upgrade_level(id);
        fprintf(file, "RESEARCH %d\n", id);
        visited_count = 0; dump_object(file, level, 0, 2);
        void* upgrade = level ? *(void**)((char*)level + 0x48) : nullptr;
        void* owner = upgrade ? *(void**)((char*)upgrade + 0x48) : nullptr;
        if (!owner) continue;
        owner_id = *(int*)((char*)owner + 0x10);
        fprintf(file, "BASE_OWNER %d\n", owner_id);
        visited_count = 0; dump_object(file, owner, 0, 1);
        void* levels = *(void**)((char*)owner + 0x18);
        int count = levels ? *(int*)((char*)levels + 0x18) : 0;
        for (int j = 0; j < count && j < 8; ++j) {
            fprintf(file, "BASE_BUILDING_LEVEL %d\n", j + 1);
            visited_count = 0; dump_object(file, list_item(levels, j), 0, 1);
        }
        fprintf(file, "BASE_FRACTION\n");
        visited_count = 0; dump_object(file, *(void**)((char*)owner + 0x20), 0, 1);
    }
    for (int chapter : {200, 201}) {
        void* message = read_scenario_message(chapter == 200 ?
            DATA_ROOT "offline-scenarios/resistance-001.ggs" : DATA_ROOT "offline-scenarios/resistance-002.ggs",
            5040, chapter == 200 ? "@research-probe-one" : "@research-probe-two");
        void* battle = message ? *(void**)((char*)message + 0x20) : nullptr;
        void* sides = battle ? *(void**)((char*)battle + 0x178) : nullptr;
        for (int side_id = 0; side_id < 4; ++side_id) {
            void* side = local_dictionary_item(sides, side_id);
            if (!side) continue;
            fprintf(file, "SCENARIO %d SIDE %d FRACTION\n", chapter, side_id);
            visited_count = 0; dump_object(file, *(void**)((char*)side + 0xd8), 0, 1);
            void* definitions = *(void**)((char*)side + 0xf8);
            if (side_id == 1) {
                fprintf(file, "SCENARIO %d PLAYER_SIDE_FIELDS\n", chapter);
                visited_count = 0; dump_object(file, side, 0, 1);
                fprintf(file, "SCENARIO %d BUILDING_DICTIONARY\n", chapter);
                visited_count = 0; dump_object(file, definitions, 0, 2);
                uintptr_t values_handle = 0;
                void* values = local_dictionary_values(definitions,
                    find_class("com.geargames.aow.entities", "BuildingType"), values_handle);
                for (size_t i = 0; values && i < array_length(values); ++i) {
                    void* type = ((void**)((char*)values + 32))[i];
                    if (!type || *(int8_t*)((char*)type + 0x30) != 1) continue;
                    fprintf(file, "SCENARIO %d HQ_DICTIONARY_VALUE\n", chapter);
                    visited_count = 0; dump_object(file, type, 0, 2);
                    void* levels = *(void**)((char*)type + 0x18);
                    int count = levels ? *(int*)((char*)levels + 0x18) : 0;
                    for (int j = 0; j < count; ++j) {
                        fprintf(file, "SCENARIO %d HQ_DICTIONARY_LEVEL %d\n", chapter, j + 1);
                        visited_count = 0; dump_object(file, list_item(levels, j), 0, 1);
                    }
                }
                if (values_handle) gchandle_free(values_handle);
            }
            void* owner = local_dictionary_item(definitions, owner_id);
            fprintf(file, "SCENARIO %d SIDE %d OWNER %d\n", chapter, side_id, owner_id);
            visited_count = 0; dump_object(file, owner, 0, 1);
            void* levels = owner ? *(void**)((char*)owner + 0x18) : nullptr;
            int count = levels ? *(int*)((char*)levels + 0x18) : 0;
            for (int j = 0; j < count && j < 8; ++j) {
                fprintf(file, "SCENARIO %d SIDE %d BUILDING_LEVEL %d\n", chapter, side_id, j + 1);
                visited_count = 0; dump_object(file, list_item(levels, j), 0, 1);
            }
            fprintf(file, "SCENARIO %d SIDE %d BUILDINGS\n", chapter, side_id);
            visited_count = 0; dump_object(file, *(void**)((char*)side + 0x148), 0, 2);
        }
    }
    fclose(file);
    LOG("Research effects probe exported definitions and archived scenario attributes; no battle run started");
}

#include "offline_world_runtime.h"
#include "campaign_runtime.h"
#include "bot_workspace_runtime.h"
#include "boost_conversion_runtime.h"
#include "boost_conversion_audit.h"
#include "skill_boost_duration_audit.h"
#include "skill_income_audit.h"
#include "weapon_parameters_audit.h"
#include "weapon_reload_audit.h"
#include "weapon_accuracy_audit.h"
#include "weapon_dispersion_audit.h"
#include "unit_transition_audit.h"
#include "composite_research_audit.h"
#include "weapon_groups_audit.h"
#include "weapon_aiming_audit.h"
#include "mine_times_audit.h"
#include "unit_energy_audit.h"
#include "mine_growth_audit.h"
#include "support_ability_audit.h"
#include "control_point_content_audit.h"
#include "state_armor_audit.h"
#include "effective_box_audit.h"
#include "unit_regen_audit.h"
#include "modification_effect_audit.h"
#include "modification_service_audit.h"
#include "modification_contract_audit.h"
#include "building_unlock_audit.h"
#include "skill_research_discount_audit.h"
#include "skill_hero_cooldown_audit.h"
#include "warpoint_audit.h"
#include "career_star_audit.h"
#include "reward_skill_audit.h"
#include "skill_hero_attack_audit.h"
#include "supply_audit.h"
#include "shop_box_audit.h"
#include "free_box_contract_audit.h"
#include "contract_box_audit.h"
#include "item_inventory_audit.h"
#include "hero_live_lab.h"
#include "ai_lab.h"
#include "duel_lab.h"
#include "control_point_layout_audit.h"
#include "ffa_seats_audit.h"
#include "ai_advanced_trace.h"
#include "inventory_runtime.h"
#include "activity_contract_runtime.h"
#include "battle_contract_runtime.h"
#include "local_history_journal.h"
#include "local_match_runtime.h"
#include "formal_credit_audit.h"
#include "local_resource_categories.h"
#include "cooperative_runtime.h"
#include "battle_contract_audit.h"
#include "fast_contract_audit.h"
#include "timed_building_contract_audit.h"
#include "boost_contract_audit.h"
#include "activity_contract_audit.h"
#include "cooperative_credit_audit.h"
#include "competition_track_audit.h"
#include "local_history_runtime.h"
#include "local_training_runtime.h"
#include "ai_replay_growth.h"

static bool start_local_campaign(int mission_id = 200, bool player_match = false, bool content_laboratory = false) {
    local_coop::presenting=false;
    if (!progress_ready) { LOG("Local campaign refused: progress unavailable"); return false; }
    const auto* scenario = local_scenario(mission_id);
    bool batch=offline_world_runtime::batch_fixture&&!player_match&&mission_id==offline_world_runtime::batch_job.scenario;
    if(batch){
        scenario=nullptr;const auto* r=local_match_maps::find(mission_id);
        for(const auto& source:local_scenarios)if(r&&source.chapter==mission_id&&source.faction==r->faction)scenario=&source;
    }
    if (!scenario || (!batch && !player_match && !content_laboratory && !ai_lab::naval_sample(mission_id) && !local_scenario_unlocked(*scenario))) {
        LOG("Local chapter unavailable or previous victory missing=%d", mission_id); return false;
    }
    if(!local_scenarios.verify(*scenario)){LOG("Local campaign source checksum refused chapter=%d",mission_id);return false;}
    if(scenario->rebase_growth&&(!local_resources::prepare()||!local_resources::ready())){
        LOG("Local campaign retained map category not ready chapter=%d",mission_id);return false;
    }
    if(!offline_world_runtime::configure_fixture(mission_id,player_match))return false;
    const bool ai_laboratory = ai_lab::begin(mission_id,player_match);
    if(hero_live_lab::active)local_duel::active=true;
    const bool seed_laboratory = mission_id==200&&!access(DATA_ROOT "campaign-seeds-lab.enabled",F_OK);
    const bool laboratory = ai_laboratory||seed_laboratory||hero_live_lab::active||content_laboratory;
    if (ai_laboratory && (!ai_lab::script_original || !ai_lab::tick_original || !ai_lab::automatic_original)) {
        LOG("AI lab refused: diagnostic hooks unavailable"); return false;
    }
    local_research_runtime::Roots source_roots;
    local_faction::Roots description_roots;
    void* source_config=scenario->embedded?campaign_seeds::decode(scenario->file,source_roots):nullptr;
    void* notification=scenario->embedded?nullptr:read_scenario_message(scenario->file,5040,"@local-first-scenario");
    if(scenario->embedded&&!campaign_seeds::inspect(source_config,batch?scenario->faction:local_faction::active,nullptr,"production"))return false;
    if(!scenario->embedded&&!notification)return false;
    void* battle_input=scenario->embedded?*(void**)((char*)source_config+0x10):*(void**)((char*)notification+0x20);
    int own_side=scenario->embedded?*(int*)((char*)source_config+0x18):1;
    int start_tick=scenario->embedded?0:*(int*)((char*)notification+0x30);
    if(!battle_input||own_side!=1||*(int*)((char*)battle_input+0x28)!=scenario->recorded)return false;
    void* exception = nullptr;
    void* battle_context = find_class("com.geargames.aow.scenes.battle", "ClientBattleContext");
    void* info_field = class_field_from_name(battle_context, "m_battleInfo");
    void* info = nullptr;
    {
        // The restored hall already owns the complete persons/boost/entity
        // dictionaries. Do not replace them with a reduced first-mission seed,
        // replay a starter profile, or initialize an unrelated battle first.
        void* mission_receiver = find_class("com.geargames.aow.scenes.logon.network", "CSMainMissionDictionaryReceiver");
        void* dictionary_args[] = {local_runtime_mission_dictionary()};
        if(!dictionary_args[0])return false;
        runtime_invoke(class_method(mission_receiver, "ProcessDictionary", 1), nullptr, dictionary_args, &exception);
        if (exception) { LOG("Local full mission dictionary publication failed"); return false; }
        // Chapter IDs remain stable in the career; play the actual captured
        // tutorial variant and its original map/script, never a renamed level.
        void* canonical = local_mission_description(scenario->recorded);
        void* chapter = local_mission_description(scenario->chapter);
        void* battle = battle_input;
        if (!canonical || !chapter || !battle || *(int*)((char*)battle + 0x28) != scenario->recorded ||
            *(int*)((char*)canonical + 0x14) != scenario->map ||
            (!scenario->recorded_presentation&&*(int*)((char*)chapter + 0x14) != scenario->map)) {
            LOG("Local scenario identity mismatch chapter=%d recorded=%d", scenario->chapter, scenario->recorded); return false;
        }
        if (local_duel::active) {
            const LocalScenario* donor_source=scenario;
            if(!player_match&&!access(DATA_ROOT "ai-tech-lab.enabled",F_OK)){
                donor_source=nullptr;
                for(const auto& source:local_scenarios)if(source.faction==1&&source.chapter==0&&source.embedded)donor_source=&source;
                if(!donor_source||!local_scenarios.verify(*donor_source)){LOG("Local technology fixture donor unavailable");return false;}
            }
            if(player_match&&local_match::state.opponent!=local_faction::active) {
                donor_source=nullptr;int chapter=local_match::state.opponent==1?0:200;
                for(const auto& source:local_scenarios)if(source.faction==local_match::state.opponent&&source.chapter==chapter&&source.embedded)donor_source=&source;
                if(!donor_source||!local_scenarios.verify(*donor_source)){LOG("Local AI opposing source unavailable; preserving careers");return false;}
            }
            if(batch&&offline_world_runtime::batch_job.opponent!=scenario->faction){
                donor_source=nullptr;int faction=offline_world_runtime::batch_job.opponent;
                for(const auto& source:local_scenarios)if(source.faction==faction&&source.chapter==(faction==1?0:200)&&source.embedded)donor_source=&source;
                if(!donor_source||!local_scenarios.verify(*donor_source))return false;
            }
            void* donor=donor_source->embedded?campaign_seeds::decode(donor_source->file,source_roots):read_scenario_message(donor_source->file,5040,"@local-duel-donor");
            void* donor_battle=donor?*(void**)((char*)donor+(donor_source->embedded?0x10:0x20)):nullptr;
            if(!donor_battle||!local_duel::prepare(battle,donor_battle)) {
                LOG("Local duel seed preparation rejected");return false;
            }
            int mode=batch?offline_world_runtime::batch_job.mode:player_match?local_match::state.mode:local_duel::control_points::private_mode;
            int extra_pairs=(player_match||batch)&&!local_battle_format::free_for_all(mode)?(ai_team_policy::seats(mode)/2-1):0;
            for(int pair=0;pair<extra_pairs;++pair){
                void* ally=scenario->embedded?campaign_seeds::decode(scenario->file,source_roots):read_scenario_message(scenario->file,5040,"@local-team-ally");
                void* ally_battle=ally?*(void**)((char*)ally+(scenario->embedded?0x10:0x20)):nullptr;
                void* enemy=donor_source->embedded?campaign_seeds::decode(donor_source->file,source_roots):read_scenario_message(donor_source->file,5040,"@local-team-enemy");
                void* enemy_battle=enemy?*(void**)((char*)enemy+(donor_source->embedded?0x10:0x20)):nullptr;
                if(!ally_battle||!enemy_battle||!local_duel::extend_teams(battle,ally_battle,enemy_battle)){LOG("Local team seed preparation rejected");return false;}
            }
            if(local_battle_format::free_for_all(mode))for(int id=3;id<=local_battle_format::seats(mode);++id){
                void* input=donor_source->embedded?campaign_seeds::decode(donor_source->file,source_roots):read_scenario_message(donor_source->file,5040,"@local-free-for-all");
                void* independent=input?*(void**)((char*)input+(donor_source->embedded?0x10:0x20)):nullptr;
                if(!independent||!local_duel::extend_free_for_all(battle,independent)){LOG("Local free-for-all seed preparation rejected");return false;}
            }
        }
        if(player_match) {
            canonical=local_faction::clone(canonical,description_roots);
            if(!canonical)return false;
            char title[96];snprintf(title,sizeof(title),"本地 AI %s",local_match::mode_name(local_match::state.mode));
            field_set(canonical,class_field_from_name(object_class(canonical),"name"),string_new(title));
            const char* objective=local_battle_format::free_for_all(local_match::state.mode)?"各自为战，消灭其他军队的全部建筑与部队获胜":local_match::state.mode==5?"持有战略点累计300分，或消灭敌队全部建筑与部队获胜":
                local_match::state.mode==6?"争夺战略点取得增益，消灭敌队全部建筑与部队获胜":"消灭敌队全部建筑与部队获胜";
            char rule[256];snprintf(rule,sizeof(rule),"map%d · %s",local_match::state.map,objective);
            field_set(canonical,class_field_from_name(object_class(canonical),"desc"),string_new(rule));
        }
        if (!local_campaign::initialize(battle, canonical, own_side)) {
            LOG("Local clean campaign context failed");
            return false;
        }
        info = object_new(find_class("com.geargames.aow.scenes.battle", "InitializationBattleInfo"));
        if (!source_roots.keep(info)) return false;
        invoke_named(info, ".ctor", 0, nullptr, &exception);
        if (exception) return false;
        static_set(info_field, info);
    }
    void* research_battle = battle_input;
    if((!laboratory||player_match||content_laboratory) && !local_inventory::prepare(battle_input,own_side)) {
        LOG("Local scenario refused: inventory could not be prepared");return false;
    }
    if (!ai_replay_growth::prepare(research_battle))return false;
    if (!offline_world_runtime::strategy_fixture&&!apply_local_battle_research(research_battle,1,scenario->rebase_growth)) {
        LOG("Local scenario refused: research attributes could not be prepared");
        return false;
    }
    if(hero_live_lab::active&&!hero_live_lab::project(research_battle))return false;
    void* battle = battle_input;
    void* battle_field = class_field_from_name(object_class(info), "Battle");
    field_set(info, battle_field, battle);
    *(int*)((char*)info + 0x20) = own_side;
    *(bool*)((char*)info + 0x24) = false;
    *(int*)((char*)info + 0x28) = start_tick;
    void* hero_configuration_fixture=nullptr;
    if(hero_live_lab::active&&!skill_hero_capacity_audit::fixture(hero_configuration_fixture))return false;
    if((!laboratory||player_match||hero_live_lab::active||content_laboratory)&&!local_hero_battle::prepare(info,battle,own_side,hero_configuration_fixture)){
        LOG("Local scenario refused: original hero battle input unavailable");return false;
    }
    if (!laboratory) {
        auto next = progress;
        ++next.next_run;
        if(!shop_box_policy::begin(next.shop,shop_box_policy::Campaign,next.next_run))return false;
        if (!local_progress::write(local_faction::root(), next)) { LOG("Cannot save battle run ID"); return false; }
        progress = next; local_run = progress.next_run;
    }
    local_run_active = !laboratory; local_mission_id = mission_id;
    local_history_view::clear();
    local_match::presenting=false;
    LOG("Starting local scenario chapter=%d recordedMission=%d tick=%d source=%s validation_only=%d",mission_id,
        *(int*)((char*)battle+0x28),start_tick,scenario->embedded?"embedded-original":"captured-original",seed_laboratory||content_laboratory);
#ifdef ART3_LOCAL_MENU
    menu_call("hide");
#endif
    void* manager = find_class("com.geargames.aow.scenes.logon.network", "CSMainLogonManager");
    runtime_invoke(class_method(manager, "ToStartBattle", 0), nullptr, nullptr, &exception);
    if (exception) {
        local_run_active = false;
#ifdef ART3_LOCAL_MENU
        menu_call("show");
#endif
        return false;
    }
    return true;
}


static bool start_local_match(int selected_chapter,int selected_opponent,int selected_mode,bool formal) {
    if(local_match::playing||local_run_active||!local_match_maps::first(local_faction::active)||!local_match::installed||
       !ai_lab::script_original||!ai_lab::tick_original||!ai_lab::automatic_original)return false;
    if(!local_match::restore()||!local_contracts::sync_state()||!local_warpoints::sync()||(formal&&!local_warpoints::initialize()))return false;
    const auto* route=local_match_maps::find(selected_chapter>=0?selected_chapter:local_match::selected);
    const auto* input=route?local_scenario(route->chapter):nullptr;
    if(!route||route->faction!=local_faction::active||!input||input->map!=route->map)return false;
    int opponent=selected_opponent?selected_opponent:local_match::selected_opponent;
    if(opponent!=1&&opponent!=2)return false;
    int mode=selected_mode?selected_mode:local_match::selected_mode;
    if(!control_point_policy::valid_mode(mode)||(formal&&mode>=5))return false;
    if(ai_team_policy::seats(mode)>route->max_seats)return false;
    // Preserve the actual last result before a new reservation replaces it.
    if(!local_history_journal::pvp(local_match::state))return false;
    auto next=local_match::state;next.scenario=route->chapter;next.map=route->map;next.opponent=opponent;next.mode=mode;
    if(!local_match_store::begin_pvp(next,formal)||!local_career_stars::freeze(next,formal)||!battle_contract_runtime::begin(next)||!local_match_store::freeze_rules(next)||
        !offline_world_runtime::freeze(next)||(formal&&!register_shop_start(shop_box_policy::Formal,next.next))||!local_match_store::write(local_faction::root(),next))return false;
    local_match::state=next;local_match::run=next.next;
    local_match::playing=true;local_match::save_failed=false;local_match::pending_result=false;
    local_match::runtime_failed=local_match::error_exit_requested=false;
    bool started=start_local_campaign(route->chapter,true);
    if(!started) {
        auto failed=local_match::state;
        if(local_match_store::settle(failed,failed.next,local_match_store::Error,0,0,-1,nullptr,nullptr)&&
           local_match_store::write(local_faction::root(),failed))local_match::state=failed;
        local_match::playing=local_duel::active=ai_lab::active=false;local_bot_runtime::reset(false);
    }
    LOG("Local AI player match started=%d run=%llu player_side=1 ai_side=2 map=%d scenario=%d opponent=%d mode=%d",started,(unsigned long long)local_match::run,route->map,route->chapter,opponent,mode);
    return started;
}

#ifdef ART3_LOCAL_MENU
static void process_menu_action() {
    char action[256] = {};
    pthread_mutex_lock(&menu_action_lock);
    memcpy(action, menu_action, sizeof(action)); menu_action[0] = '\0';
    pthread_mutex_unlock(&menu_action_lock);
    if (!action[0]) return;
    LOG("Local menu action: %s", action);
    if (!strcmp(action, "battle:campaign:200")) {
        if (!start_local_campaign())
            menu_call("message", "战役未能启动", "关卡初始化失败，请保留本地工程日志以便修复。");
    } else menu_call("message", "移植进度", "此入口尚未接通。当前实验包先验证战役与原战斗引擎，尚未完成全部单机玩法。");
}
#endif
static int local_connection_status(void* self, void*) {
    if (self == offline_network) return 0;
    void* socket = *(void**)((char*)self + 0x70);
    if (!socket) return 2;
    void* exception = nullptr;
    void* status = invoke_named(socket, "get_ConnectionStatus", 0, nullptr, &exception);
    return status && !exception ? *(int*)((char*)status + 16) : 3;
}
static float (*supply_progress_original)(void*, void*);
static void (*supply_icon_original)(void*, void*);
static void (*supply_change_original)(void*, void*);

static bool presenter_has_supply_boxes(void* self) {
    void* model = *(void**)((char*)self + 0x28);
    void* exception = nullptr;
    void* boxes = invoke_named(model, "get_SupplyBoxes", 0, nullptr, &exception);
    void* count = invoke_named(boxes, "get_Count", 0, nullptr, &exception);
    return count && !exception && *(int*)((char*)count + 16) > 0;
}

static void supply_balance_component(void* view){
    // The original empty-list guard skips UpdateIcon entirely, leaving prefab
    // text (13/200) visible. Reuse its real view and existing balance label.
    if(!view||strcmp(class_name(object_class(view)),"UGUISupplyButtonView")||!progress_ready)return;
    void* ex=nullptr;invoke_named(view,"SetNotAvailableState",0,nullptr,&ex);
    float zero=0.f;bool animate=false;void* value_args[]={&zero,&animate};
    if(!ex)invoke_named(view,"SetProgressValue",2,value_args,&ex);
    char text[40];snprintf(text,sizeof(text),"%d",progress.quest_warpoints);
    local_research_runtime::Roots roots;void* value=string_new(text);void* args[]={value};
    if(roots.keep(value)&&!ex)invoke_named(view,"SetProgressText",1,args,&ex);
    void* label=*(void**)((char*)view+0x78);void* actual=label&&!ex?invoke_named(label,"get_text",0,nullptr,&ex):nullptr;char readback[40];
    bool written=actual&&!ex&&local_research_runtime::ascii(actual,readback,sizeof(readback))&&!strcmp(text,readback);
    if(written){if(!local_warpoints::balance_view_written||local_warpoints::balance_view_amount!=progress.quest_warpoints)
        LOG("Local original supply balance label=%s claims_enabled=0",text);
        local_warpoints::balance_view_written=true;local_warpoints::balance_view_amount=progress.quest_warpoints;}
}

static void supply_balance_view(void* self){supply_balance_component(self?*(void**)((char*)self+0x18):nullptr);}
static void supply_balance_hall(){
    // A disabled supply mechanic need not open its presenter. Find the actual
    // loaded original view, excluding prefab/inactive resource objects.
    void* klass=find_class("Domains.UiManagement.MainScreen.Presentation","UGUISupplyButtonView");
    void* resources=find_class("UnityEngine","Resources");
    const void* method=resources?class_method(resources,"FindObjectsOfTypeAll",1):nullptr;
    void* type=klass?type_object(class_type(klass)):nullptr;void* args[]={type};void* ex=nullptr;
    void* views=method&&type?runtime_invoke(method,nullptr,args,&ex):nullptr;
    size_t count=views&&!ex?array_length(views):0;int active=0;
    if(count<=128)for(size_t i=0;i<count;++i){void* view=*(void**)((char*)views+32+i*8);
        void* enabled=invoke_named(view,"get_isActiveAndEnabled",0,nullptr,&ex);
        if(enabled&&!ex&&*(bool*)((char*)enabled+16)){++active;
            if(local_supply::unlocked(progress,local_faction::active)&&local_supply::live_available()){
                for(int offset:{0x48,0x90,0x98,0xa0,0xa8})local_supply::hide(*(void**)((char*)view+offset));
            }else supply_balance_component(view);}}
    LOG("Local original supply loaded views=%zu active=%d balance_published=%d",count,active,local_warpoints::balance_view_written);
}

static void supply_icon_hook(void* self, void* method) {
    if (presenter_has_supply_boxes(self)) supply_icon_original(self, method);else supply_balance_view(self);
}

static void supply_change_hook(void* self, void* method) {
    if (presenter_has_supply_boxes(self)) supply_change_original(self, method);else supply_balance_view(self);
}

static void local_ready_to_act(void* self, void*) {
    void* exception = nullptr;
    invoke_named(self, "ToCompleted", 0, nullptr, &exception);
    LOG("Local ready-to-act completed=%d", !exception);
}

#include "offline_entry_runtime.h"
static void (*network_state_start_original)(void*, void*);
static void network_state_start_hook(void* self, void* method) {
    void* exception = nullptr;
    void* network = invoke_named(self, "get_Network", 0, nullptr, &exception);
    if (!exception && network && network == offline_network) {
        // A local session has no socket or receive timeout to monitor.
        *(int*)((char*)self + 0x30) = 0;
        return;
    }
    network_state_start_original(self, method);
}

static void local_unlock_update(void* self, void*) {
    // OnStart has already installed the original window/history subscriptions.
    // Complete on the next frame so OnEnd closes the original waiting window.
    void* exception = nullptr;
    invoke_named(self, "ToCompleted", 0, nullptr, &exception);
    LOG("Local original hall unlocked=%d", !exception);
    if(!exception) {
        local_quests::refresh();
        local_contracts::refresh();
        local_items::publish();
        local_modifications::publish();
        local_supply::refresh();
        local_supply::publish_pending();
        local_supply::publish_pending(true);
        local_supply::publish_pending(false,true);
        local_competition_tracks::publish_pending();
        supply_balance_hall();
        // Explicit development-only original-content sample. No career unlock,
        // run reservation, reward or save; original UI has no path to this flag.
        local_quests::audit();
        local_contracts::audit();
        free_box_contract_audit::run();
        battle_contract_audit::run();
        fast_contract_audit::run();
        timed_building_contract_audit::run();
        boost_contract_audit::run();
        activity_contract_audit::run();
        ai_tech_dependency_audit::run();
        skill_research_discount_audit::run();
        skill_boost_duration_audit::run();
        skill_income_audit::run();
        weapon_parameters_audit::run();
        weapon_reload_audit::run();
        weapon_accuracy_audit::run();
        weapon_dispersion_audit::run();
        unit_transition_audit::run();
        composite_research_audit::run();
        weapon_groups_audit::run();
        weapon_aiming_audit::run();
        mine_times_audit::run();
        unit_energy_audit::run();
        mine_growth_audit::run();
        support_ability_audit::run();
        control_point_content_audit::run();
        control_point_layout_audit::run();
        ffa_seats_audit::run();
        effective_box_audit::run();
        modification_effect_audit::run();
        modification_service_audit::run();
        modification_contract_audit::run();
        building_unlock_audit::run();
        boost_conversion_audit::run();
        warpoint_audit::run();
        career_star_audit::run();
        reward_skill_audit::run();
        skill_hero_attack_audit::run();
        formal_credit_audit::run();
        supply_audit::run();
        shop_box_audit::run();
        contract_box_audit::run();
        item_inventory_audit::run();
        cooperative_credit_audit::run();
        competition_track_audit::run();
        if(!access(DATA_ROOT "ai-batch-lab.request",F_OK)){
            unlink(DATA_ROOT "ai-batch-lab.request");
            bool allowed=!local_match::playing&&!local_coop::playing&&offline_world_runtime::arm_batch();
            bool started=allowed&&start_local_campaign(offline_world_runtime::batch_job.scenario);
            if(!started){FILE* f=fopen(DATA_ROOT "ai-batch-start-error.json","w");if(f){fprintf(f,"{\"status\":\"start_error\",\"accepted\":%s}\n",allowed?"true":"false");fclose(f);chmod(DATA_ROOT "ai-batch-start-error.json",0644);}}
            LOG("AI batch private launch allowed=%d started=%d career_writes=0",allowed,started);return;
        }
        if(!access(DATA_ROOT "commander-ai-lab.request",F_OK)){
            unlink(DATA_ROOT "commander-ai-lab.request");
            bool allowed=local_faction::active==2&&!local_match::playing&&!local_coop::playing&&
                !access(DATA_ROOT "commander-ai-lab.enabled",F_OK)&&!access(DATA_ROOT "ai-lab.enabled",F_OK)&&
                !access(DATA_ROOT "native-bot-lab.enabled",F_OK)&&!access(DATA_ROOT "ai-duel-lab.enabled",F_OK)&&
                access(DATA_ROOT "ai-tech-lab.enabled",F_OK)&&access(DATA_ROOT "ai-base-test.enabled",F_OK);
            bool started=allowed&&start_local_campaign(201);
            LOG("Local commander private calibration accepted=%d started=%d career_writes=0",allowed,started);return;
        }
        if(FILE* f=fopen(DATA_ROOT "control-point-live.request","r")){
            int mode=0;char extra=0;bool valid=fscanf(f,"%d %c",&mode,&extra)==1&&(mode==5||mode==6||local_battle_format::free_for_all(mode));fclose(f);unlink(DATA_ROOT "control-point-live.request");
            bool allowed=valid&&local_faction::active==2&&!local_match::playing&&!local_coop::playing&&
                !access(DATA_ROOT "ai-lab.enabled",F_OK)&&!access(DATA_ROOT "native-bot-lab.enabled",F_OK)&&!access(DATA_ROOT "ai-duel-lab.enabled",F_OK);
            local_duel::control_points::private_mode=allowed?mode:0;
            bool started=allowed&&start_local_campaign(201);
            LOG("Control point private native run mode=%d accepted=%d started=%d career_writes=0",mode,allowed,started);return;
        }
        if(!access(DATA_ROOT "ai-tech-lab.request",F_OK)){
            int input=202;if(FILE* request=fopen(DATA_ROOT "ai-tech-lab.request","r")){int parsed=0;char extra=0;if(fscanf(request,"%d %c",&parsed,&extra)==1)input=parsed;fclose(request);}
            unlink(DATA_ROOT "ai-tech-lab.request");
            bool allowed=local_faction::active==2&&!access(DATA_ROOT "ai-tech-lab.enabled",F_OK)&&!access(DATA_ROOT "ai-lab.enabled",F_OK)&&!access(DATA_ROOT "native-bot-lab.enabled",F_OK)&&!access(DATA_ROOT "ai-duel-lab.enabled",F_OK);
            bool naval=!access(DATA_ROOT "ai-naval-lab.enabled",F_OK);const auto* route=local_match_maps::find(input);
            if(naval){allowed=allowed&&route&&route->faction==2;local_duel::naval_fixture_chapter=input;}
            int chapter=naval?input:201;
            LOG("Local technology production fixture allowed=%d started=%d",allowed,allowed&&start_local_campaign(chapter));return;
        }
        if(FILE* f=fopen(DATA_ROOT "campaign-content-lab.request","r")){
            int id=-1;char extra=0;bool valid=fscanf(f,"%d %c",&id,&extra)==1;fclose(f);
            unlink(DATA_ROOT "campaign-content-lab.request");
            const auto* route=valid?local_scenario(id):nullptr;
            bool allowed=route&&route->rebase_growth&&route->faction==local_faction::active;
            LOG("Campaign content sample chapter=%d accepted=%d started=%d save_writes=0",id,allowed,allowed&&start_local_campaign(id,false,true));
            return;
        }
        if(!access(DATA_ROOT "hero-live-lab.request",F_OK)||!access(DATA_ROOT "skill-live-lab.request",F_OK)){
            bool skills=!access(DATA_ROOT "skill-live-lab.request",F_OK);
            unlink(skills?DATA_ROOT "skill-live-lab.request":DATA_ROOT "hero-live-lab.request");hero_live_lab::begin(skills);
            if(!start_local_campaign(202))hero_live_lab::fail("start");return;
        }
        if(local_coop::playing || local_coop::pending_result)local_coop::publish();
        else if(local_match::playing || local_match::pending_result)local_match::publish();
        else if(!local_campaign_results::publish())LOG("Local campaign result presentation deferred or failed");
        offline_entry::on_hall_ready();
    }
}

static void network_state_end_hook(void* self, void*) {
    // The local state deliberately does not start the remote watchdog.
    void* cts = *(void**)((char*)self + 0x20);
    if (!cts) return;
    void* exception = nullptr;
    invoke_named(cts, "Cancel", 0, nullptr, &exception);
}

static void (*pve_start_original)(void*, int, void*);
static void local_pve_start(void* mission, int difficulty, void* method) {
    if (!offline_network) { pve_start_original(mission, difficulty, method); return; }
    void* description = mission ? *(void**)((char*)mission + 0x10) : nullptr;
    int id = description ? *(int*)((char*)description + 0x10) : -1;
    LOG("Original PVE start requested mission=%d difficulty=%d", id, difficulty);
    // Only the archived scenario is implemented. Never substitute the wrong map.
    if (local_scenario(id) && difficulty == 0)
        LOG("Original PVE local start=%d", start_local_campaign(id));
}
static void (*pve_restart_original)(void*);
static void local_pve_restart(void* method) {
    if(!offline_network){pve_restart_original(method);return;}
    if(local_history_view::active){
        const auto record=local_history_view::state;const auto selected=local_history_view::selected;
        bool ok=selected.faction==uint32_t(local_faction::active)&&
            (selected.kind==local_history_store::Cooperative?local_coop::start():
             start_local_match(record.scenario,record.opponent,record.mode,local_match_store::formal(record)));
        LOG("Local history retry new_session=%d old_run=%llu active_run=%llu",ok,(unsigned long long)record.settled,
            (unsigned long long)(selected.kind==local_history_store::Cooperative?local_coop::state.next:local_match::state.next));return;
    }
    if(local_coop::presenting){LOG("Local cooperative retry started=%d",local_coop::start());return;}
    if(local_match::presenting){LOG("Local AI retry started=%d",start_local_match(local_match::state.scenario,local_match::state.opponent,local_match::state.mode,local_match_store::formal(local_match::state)));return;}
    const auto& result=local_campaign_results::latest;
    bool ready=progress_ready&&!local_run_active&&result.run&&result.run==progress.settled_run&&
        result.faction==local_faction::active&&local_scenario(result.chapter);
    bool started=ready&&start_local_campaign(result.chapter);
    LOG("Local campaign retry chapter=%d started=%d",result.chapter,started);
}

static void disabled_commercial_click(void*, void*) {
    LOG("Disabled offline commercial entry tapped");
}

static int supply_box_count(void* self) {
    void* boxes = *(void**)((char*)self + 0x78);
    void* exception = nullptr;
    void* count = invoke_named(boxes, "get_Count", 0, nullptr, &exception);
    return count && !exception ? *(int*)((char*)count + 16) : 0;
}

static float supply_progress_hook(void* self, void* method) {
    return supply_box_count(self) ? supply_progress_original(self, method) : 0.0f;
}

static int supply_state_hook(void* self, void*) {
    if (!supply_box_count(self)) return 0;
    void* exception = nullptr;
    void* actual = invoke_named(self, "get_ActualBox", 0, nullptr, &exception);
    if (!actual || exception) return 0;
    void* arguments[] = {actual};
    void* state = invoke_named(self, "GetBoxState", 1, arguments, &exception);
    return state && !exception ? *(int*)((char*)state + 16) : 0;
}
static void (*plugin_initiate_original)(void*, void*, bool, void*);
static void plugin_initiate_hook(void* self, void* settings, bool billing, void* method) {
#ifdef ART3_MAIN
    // Offline is a property of this APK, including the first launch before
    // packaged data has created its legacy activation marker.
    return;
#else
    if (!access(DATA_ROOT "offline-main.enabled", F_OK)) return;
    plugin_initiate_original(self, settings, billing, method);
#endif
}

static bool offline_supply_all_opened(void* self, void*) {
    void* boxes = *(void**)((char*)self + 0x78);
    void* exception = nullptr;
    void* count_object = invoke_named(boxes, "get_Count", 0, nullptr, &exception);
    int count = count_object && !exception ? *(int*)((char*)count_object + 16) : 0;
    if (count <= 0) return true;
    void* actual = invoke_named(self, "get_ActualBox", 0, nullptr, &exception);
    if (!actual || exception) return true;
    void* state_args[] = {actual};
    void* state = invoke_named(self, "GetBoxState", 1, state_args, &exception);
    if (!state || exception || *(int*)((char*)state + 16) != 1) return false;
    void* type = invoke_named(actual, "get_Type", 0, nullptr, &exception);
    if (type && *(int8_t*)((char*)type + 16) == 1) return true;
    int last_index = count - 1;
    void* last_args[] = {&last_index};
    return invoke_named(boxes, "get_Item", 1, last_args, &exception) == actual && !exception;
}
static bool call_static_report(FILE* report, const char* ns, const char* type,
                               const char* method_name, int count, void** arguments) {
    void* klass = find_class(ns, type);
    const void* method = klass ? class_method(klass, method_name, count) : nullptr;
    void* exception = nullptr;
    void* result = method ? runtime_invoke(method, nullptr, arguments, &exception) : nullptr;
    fprintf(report, "%s %s.%s", method && !exception ? "PASS" : "FAIL", type, method_name);
    if (result) {
        fprintf(report, " result=%s", class_name(object_class(result)));
        if (!strcmp(class_name(object_class(result)), "ConnectorStateErrors"))
            fprintf(report, ":%d", *(int*)((char*)result + 16));
    }
    fputc('\n', report); fflush(report);
    if (exception) {
        visited_count = 0; dump_object(report, exception, 0, 3);
        void* ips_field = class_field_from_name(object_class(exception), "native_trace_ips");
        if (ips_field) {
            void* ips = *(void**)((char*)exception + field_offset(ips_field));
            if (ips) for (size_t i = 0; i < array_length(ips) && i < 32; i++)
                fprintf(report, "trace[%zu]=0x%lx\n", i, (unsigned long)(*(uintptr_t*)((char*)ips + 32 + 8*i) - image_base));
        }
        fflush(report);
    }
    return method && !exception;
}

static bool initialize_offline_main(void* logon_manager) {
    replay_saved_messages();
    restore_saved_dictionaries();
    {void* seed=saved("UserConfiguration"),*p=seed?*(void**)((char*)seed+0x28):nullptr;
     if(p)LOG("Retained startup template credits=%d gold=%d xp=%d stars=%d pvp_after_mission=%d",*(int*)((char*)p+0xd0),*(int*)((char*)seed+0x20),*(int*)((char*)p+0x30),*(int*)((char*)p+0x68),*(int*)((char*)seed+0x1b8));}
    if (!access(DATA_ROOT "repeat-restore-test.enabled", F_OK)) {
        // Developer regression probe: exercise replacing all pointer-sized roots.
        for (int cycle = 0; cycle < 2; cycle++) {
            replay_saved_messages();
            restore_saved_dictionaries();
            LOG("Repeat restore cycle=%d roots=%d", cycle + 1, saved_count);
        }
    }
    // Session messages and dictionaries must share entity instances, not merely
    // equal IDs: original models compare configuration entities by reference.
    if (!access(DATA_ROOT "linked-replay.enabled", F_OK)) replay_saved_messages(true);
    if(!local_faction::prepare()||!local_faction::prepare_user()){LOG("Local faction preparation refused; saves preserved");return false;}
    if(!local_mobility_runtime::annotate()){LOG("Local mobility presentation/identity validation refused");return false;}
    FILE* report = fopen(DATA_ROOT "art3offline-main.txt", "w");
    if (!report) return false;
    chmod(DATA_ROOT "art3offline-main.txt", 0644);
    const char* receiver_ns = "com.geargames.aow.scenes.logon.network";
    void* exception = nullptr;
    void* network = nullptr;
    void* context = find_class("com.geargames.aow.scenes.logon", "ClientLogonContext");
    const void* getter = context ? class_method(context, "get_Network", 0) : nullptr;
    if (getter) network = runtime_invoke(getter, nullptr, nullptr, &exception);
    void* initialize_args[] = {network};
    bool ok = call_static_report(report, "com.geargames.aow.scenes.main", "ClientMainContext", "Initialize", 1, initialize_args);
    void* hosts = nullptr;
    void* general_args[] = {saved("GeneralConfiguration"), &hosts};
    ok = call_static_report(report, receiver_ns, "CSMainGeneralConfigurationReceiver", "ProcessMessage", 2, general_args) && ok;
    struct Step { const char* key; const char* type; const char* method; } steps[] = {
        {"ClientResourceConfiguration", "CSMainClientResourceConfigurationReceiver", "ProcessMessage"},
        {"BATTLE", "CSMainBattleBaseDictionaryReceiver", "ProcessDictionary"},
        {"SHARED", "CSMainSharedBaseDictionaryReceiver", "ProcessDictionary"},
        {local_faction::key(), "CSMainBaseDictionaryReceiver", "ProcessDictionary"},
        {"MissionDictionary", "CSMainMissionDictionaryReceiver", "ProcessDictionary"},
        {"ChangeEntityComplex", "CSMainChangeEntitiesReceiver", "ProcessMessage"}
    };
    for (const Step& step : steps) {
        void* args[] = {!strcmp(step.key,"MissionDictionary")?local_runtime_mission_dictionary():
                       !strcmp(step.key,"SHARED")?local_skill::runtime_shared():saved(step.key)};
        if (!args[0]) { fprintf(report, "MISSING %s\n", step.key); ok = false; continue; }
        ok = call_static_report(report, receiver_ns, step.type, step.method, 1, args) && ok;
    }
    void* plugin = *(void**)((char*)logon_manager + 0xa0);
    fprintf(report, "PluginSettings=%p\n", plugin); fflush(report);
    if (!plugin) {
        void* klass = find_class("com.geargames.aow.services", "PluginSettings");
        plugin = klass ? object_new(klass) : nullptr;
        if (plugin) invoke_named(plugin, ".ctor", 0, nullptr, &exception);
    }
    void* user_args[] = {saved("UserConfiguration"), plugin};
    ok = apply_local_progress() && ok;
    if(ok)generic_unit_research::audit_hero_acquisition();
    // The current dictionaries use career progression. Online login supplies this
    // feature flag before UserConfiguration; the legacy level field is absent.
    void* main_class = find_class("com.geargames.aow.scenes.main", "ClientMainContext");
    void* feature_field = class_field_from_name(main_class, "FeatureToggleService");
    void* feature_service = nullptr;
    if (feature_field) static_get(feature_field, &feature_service);
    if (feature_service) {
        void* permanent = *(void**)((char*)feature_service + 0x10);
        void* temporary = *(void**)((char*)feature_service + 0x18);
        // SetData also signals the original readiness subscriptions. Mutating
        // the HashSet alone leaves BattleBaseDictionaryDataSource (and the
        // workshop repository downstream) waiting forever for feature data.
        // Use fresh sets: SetData clears its owned sets before copying inputs.
        void* local_permanent = permanent ? object_new(object_class(permanent)) : nullptr;
        void* local_temporary = temporary ? object_new(object_class(temporary)) : nullptr;
        invoke_named(local_permanent, ".ctor", 0, nullptr, &exception);
        invoke_named(local_temporary, ".ctor", 0, nullptr, &exception);
        // Read-only capture from the matching release guest session: heroes,
        // career progression, and the current news-window implementation.
        const int recorded_features[] = {3, 7, 8};
        for (int feature : recorded_features) {
            void* flag_args[] = {&feature};
            invoke_named(local_temporary, "Add", 1, flag_args, &exception);
        }
        void* feature_args[] = {local_permanent, local_temporary};
        if (local_permanent && local_temporary && !exception)
            invoke_named(feature_service, "SetData", 2, feature_args, &exception);
        ok = local_permanent && local_temporary && !exception && ok;
        fprintf(report, "Recorded feature flags SetData exception=%p\n", exception);
    }
    void* social_class = find_class("GearGames.GGSignIn.Profile", "GGSocialProfile");
    void* social = social_class ? object_new(social_class) : nullptr;
    if (social) {
        invoke_named(social, ".ctor", 0, nullptr, &exception);
        void* identity = string_new("art3-offline-local");
        void* identity_args[] = {identity};
        invoke_named(social, "set_UserId", 1, identity_args, &exception);
        void* client_class = find_class("com.geargames.aow.config", "ClientContext");
        void* social_field = class_field_from_name(client_class, "m_socialProfile");
        // This runtime's field setter takes reference objects directly; only
        // primitive/value-type assignments take an address to their storage.
        if (social_field) static_set(social_field, social);
    }
    ok = call_static_report(report, receiver_ns, "CSMainUserConfigurationReceiver", "ProcessMessage", 2, user_args) && ok;
    // Preserve constructor dependencies with the game's built-in unavailable
    // billing provider. This provider cannot initiate purchases.
    void* billing_class = find_class("GearGames.Billing.Service", "GGEmptyBillingService");
    void* billing_config_class = find_class("com.geargames.aow.services.billing", "GGBillingServiceConfiguration");
    void* billing_provider = billing_class ? object_new(billing_class) : nullptr;
    void* billing_config = billing_config_class ? object_new(billing_config_class) : nullptr;
    exception = nullptr;
    if (billing_provider && billing_config) {
        invoke_named(billing_config, ".ctor", 0, nullptr, &exception);
        void* billing_args[] = {nullptr, billing_config};
        invoke_named(billing_provider, ".ctor", 2, billing_args, &exception);
        void* client_class = find_class("com.geargames.aow.config", "ClientContext");
        void* billing_field = class_field_from_name(client_class, "m_billing");
        void* billing = nullptr;
        if (billing_field) static_get(billing_field, &billing);
        void* service_field = billing ? class_field_from_name(object_class(billing), "m_service") : nullptr;
        if (service_field && !exception) field_set(billing, service_field, billing_provider);
        fprintf(report, "Empty billing provider installed=%d\n", service_field && !exception);
        void* installed = invoke_named(billing, "get_Service", 0, nullptr, &exception);
        fprintf(report, "Billing provider readback=%s\n", installed ? class_name(object_class(installed)) : "null");
    }
    if (!ok) {
        void* main_class = find_class("com.geargames.aow.scenes.main", "ClientMainContext");
        void* iter = nullptr;
        while (void* field = class_fields(main_class, &iter)) {
            if (!(field_flags(field) & 0x10) || (field_flags(field) & 0x40)) continue;
            uintptr_t value = 0;
            static_get(field, &value);
            fprintf(report, "STATIC %s = %lx\n", field_name(field), (unsigned long)value);
        }
    }
    fprintf(report, "Initialization completed=%d\n", ok); fclose(report);
    if (ok) {
        offline_network = network;
        save_object("@local-network", network);
        // Non-tutorial campaign maps use the original remote-category loader.
        // Initialize packaged catalogues before the hall, independently of
        // visiting cooperation/history screens first. Readiness remains native.
        local_resources::prepare();
        *(uint8_t*)((char*)network + 0x38) = 2; // Local provider is ready.
        void* sender_queue = *(void**)((char*)network + 0x60);
        invoke_named(sender_queue, "SessionRestored", 0, nullptr, &exception);
        void* client_class = find_class("com.geargames.aow.config", "ClientContext");
        void* session_field = class_field_from_name(client_class, "m_sessionId");
        if (session_field) static_set(session_field, string_new("art3-offline-session"));
        research_effects_probe();
        export_rifle_research_catalog();
        export_local_career_catalog();
        verify_local_battle_research();
        verify_local_rifle_batch();
        research_batch_audit::run();
        local_mobility_audit::run();
        growth_closure_audit::run();
        campaign_seeds::audit();
        preplaced_research_audit::run();
        building_research_audit::run();
        building_economy_audit::run();
        hq_capacity_audit::run();
        building_weapon_audit::run();
        building_sight_audit::run();
        hero_retained_classes_audit::run();
        cooperative_input_audit::run();
        campaign_capture_audit::run();
        skill_hero_protection_audit::run_guard();
        campaign_growth_audit::run();
        ai_spawn_audit::run();
        hero_battle_projection_audit::run();
        hero_class_effects_audit::run();
        hero_activation_audit::run();
        battery_native_audit::run();
        obsolete_speedup_audit::run();
        hero_seraphim_audit::run();
        hero_wasp_audit::run();
        hero_beholder_audit::run();
        hero_gatling_audit::run();
        hero_solaris_audit::run();
        hero_salamander_audit::run();
        hero_atlant_audit::run();
        hero_coil_audit::run();
        state_armor_audit::run();
        unit_regen_audit::run();
        skill_runtime_audit::run();
        skill_waiver_audit::run();
    }
    return ok;
}

static void logon_update_hook(void* self, void* method) {
    bool packaged_ready = install_packaged_data();
    static void* observed_manager;
    static int observed_state = -1;
    int state = *(int*)((char*)self + 0x1c);
    if (observed_manager != self || observed_state != state) {
        observed_manager = self; observed_state = state;
        LOG("Offline startup state=%d packaged_ready=%d", state, packaged_ready);
    }
    if (logon_manager != self) { logon_manager = self; main_attempted = false; }
#ifdef ART3_LOCAL_MENU
    if (main_attempted) { process_menu_action(); return; }
#endif
    if (access(DATA_ROOT "offline-main.enabled", F_OK)) { logon_update_original(self, method); return; }
    if (!main_attempted && *(int*)((char*)self + 0x1c) == 6) {
        main_attempted = true;
        bool ok = initialize_offline_main(self);
        LOG("Offline main configuration applied=%d", ok);
#ifdef ART3_LOCAL_MENU
        *(int*)((char*)self + 0x1c) = 33;
        if (ok) LOG("Native local menu shown=%d", menu_call("show"));
        return;
#endif
        // Keep the original scene transition and its event ordering.
        if (ok) *(int*)((char*)self + 0x1c) = 32;
        else *(int*)((char*)self + 0x1c) = 33;
    }
    logon_update_original(self, method);
}

struct TextureUrlLists { void* current; void* fallback; };
static TextureUrlLists (*texture_sort_original)(void*, void*);

static bool local_texture_uri(void* text, char* uri, size_t capacity) {
    if (!text) return false;
    int length = *(int*)((char*)text + 16);
    if (length <= 0 || length >= 2048) return false;
    auto chars = (uint16_t*)((char*)text + 20);
    char url[2048];
    for (int i = 0; i < length; ++i) {
        if (chars[i] > 127) return false;
        url[i] = (char)chars[i];
    }
    url[length] = 0;
    if (strncmp(url, "http://", 7) && strncmp(url, "https://", 8)) return false;
    if (char* query = strpbrk(url, "?#")) *query = 0;
    const char* name = strrchr(url, '/');
    if (!name || !*++name || strlen(name) > 255) return false;
    for (const char* c = name; *c; ++c)
        if (!((*c >= 'a' && *c <= 'z') || (*c >= 'A' && *c <= 'Z') ||
              (*c >= '0' && *c <= '9') || *c == '_' || *c == '-' || *c == '.')) return false;
    char path[768];
    snprintf(path, sizeof(path), DATA_ROOT "ResourcesCache/CachedTexture_%s", name);
    struct stat status;
    if (stat(path, &status) || !S_ISREG(status.st_mode) || status.st_size <= 0) return false;
    return snprintf(uri, capacity, "file://%s", path) < (int)capacity;
}

static void* copy_local_texture_urls(void* list, uintptr_t& handle, unsigned& mapped) {
    if (!list || strcmp(class_name(object_class(list)), "List`1")) return list;
    int count = *(int*)((char*)list + 0x18);
    if (count <= 0 || count > 4096) return list;
    char uri[800]; unsigned available = 0;
    for (int i = 0; i < count; ++i)
        if (local_texture_uri(list_item(list, i), uri, sizeof(uri))) ++available;
    if (!available) return list;
    void* copy = object_new(object_class(list));
    if (!copy) return list;
    handle = gchandle_new(copy, true);
    void* exception = nullptr;
    invoke_named(copy, ".ctor", 0, nullptr, &exception);
    for (int i = 0; i < count && !exception; ++i) {
        void* value = list_item(list, i);
        if (local_texture_uri(value, uri, sizeof(uri))) value = string_new(uri);
        void* args[] = {value};
        invoke_named(copy, "Add", 1, args, &exception);
    }
    if (exception) {
        LOG("Local menu texture URL copy failed; original URLs preserved");
        gchandle_free(handle); handle = 0; return list;
    }
    mapped += available;
    return copy;
}

static TextureUrlLists local_texture_sort(void* urls, void* method) {
    TextureUrlLists lists = texture_sort_original(urls, method);
    if (!offline_network) return lists;
    uintptr_t old_current = lists.current ? gchandle_new(lists.current, true) : 0;
    uintptr_t old_fallback = lists.fallback ? gchandle_new(lists.fallback, true) : 0;
    uintptr_t new_current = 0, new_fallback = 0; unsigned mapped = 0;
    lists.current = copy_local_texture_urls(lists.current, new_current, mapped);
    lists.fallback = copy_local_texture_urls(lists.fallback, new_fallback, mapped);
    if (old_current) gchandle_free(old_current);
    if (old_fallback) gchandle_free(old_fallback);
    if (new_current) gchandle_free(new_current);
    if (new_fallback) gchandle_free(new_fallback);
    if (mapped) LOG("Local menu texture URLs mapped=%u", mapped);
    return lists;
}

static void hook_offline_main() {
    const uint32_t expected[] = {0xd101c3ff, 0xf90013fe, 0xa90367fa, 0xa9045ff8};
    logon_update_original = reinterpret_cast<decltype(logon_update_original)>(
        install_hook(0x7e2f6a0, expected, (void*)logon_update_hook));
    LOG("Offline main experiment installed=%d", logon_update_original != nullptr);
    const uint32_t plugin_signature[] = {0xd10283ff, 0xa9075ffe, 0xa90857f6, 0xa9094ff4};
    plugin_initiate_original = reinterpret_cast<decltype(plugin_initiate_original)>(
        install_hook(0x7e15ef4, plugin_signature, (void*)plugin_initiate_hook));
#ifndef ART3_MAIN
    if (!access(DATA_ROOT "offline-main.enabled", F_OK))
#endif
    {
        // GetUrlsSort returns two managed list references in x0/x1 (verified
        // ARM64 ABI). Copy them so original configuration is never rewritten.
        const uint32_t texture_signature[] = {0xd10203ff, 0xa9036ffe, 0xa90467fa, 0xa9055ff8};
        texture_sort_original = reinterpret_cast<decltype(texture_sort_original)>(
            install_hook(0x7991f10, texture_signature, (void*)local_texture_sort));
        LOG("Local menu texture mapping installed=%d", texture_sort_original != nullptr);
        const uint32_t faction_switch_signature[]={0xa9be57fe,0xa9014ff4,0xb000f994,0xb000c415};
        LOG("Local faction switch installed=%d",install_hook(0x7e81378,faction_switch_signature,(void*)local_faction::switch_request)!=nullptr);
        const uint32_t profile_statistics_signature[]={0xa9bc67fe,0xa9015ff8,0xa90257f6,0xa9034ff4};
        local_faction::profile_show_original=reinterpret_cast<decltype(local_faction::profile_show_original)>(install_hook(0x7f63274,profile_statistics_signature,(void*)local_faction::profile_show));
        offline_world_runtime::install();
        const uint32_t network_signature[] = {0xa9bf4ffe, 0xaa0003f3, 0x97ffff81, 0xf9403a60};
        install_hook(0x79d6970, network_signature, (void*)local_connection_status);
        const uint32_t supply_signature[] = {0xf81e0ffe, 0xa9014ff4, 0x90010d74, 0xaa0003f3};
        // Full local implementation; the copied trampoline is intentionally unused.
        install_hook(0x7c0433c, supply_signature, (void*)offline_supply_all_opened);
        local_warpoints::install();
        local_supply::install();
        const uint32_t progress_signature[] = {0xf81d0ffe, 0xa90157f6, 0xa9024ff4, 0x90010d73};
        supply_progress_original = reinterpret_cast<decltype(supply_progress_original)>(
            install_hook(0x7c0440c, progress_signature, (void*)supply_progress_hook));
        const uint32_t state_signature[] = {0xa9bf4ffe, 0xaa0003f3, 0x97fff935, 0xaa0003e1};
        install_hook(0x7c043f0, state_signature, (void*)supply_state_hook);
        const uint32_t icon_signature[] = {0xd10143ff, 0xfd000be8, 0xf9000ffe, 0xa9025ff8};
        supply_icon_original = reinterpret_cast<decltype(supply_icon_original)>(
            install_hook(0x82a4204, icon_signature, (void*)supply_icon_hook));
        const uint32_t change_signature[] = {0xa9bc67fe, 0xa9015ff8, 0xa90257f6, 0xa9034ff4};
        supply_change_original = reinterpret_cast<decltype(supply_change_original)>(
            install_hook(0x82a4b54, change_signature, (void*)supply_change_hook));
        const uint32_t ready_signature[] = {0xd100c3ff, 0xa90157fe, 0xa9024ff4, 0xb000f9d5};
        install_hook(0x7e79efc, ready_signature, (void*)local_ready_to_act);
        const uint32_t monitor_signature[] = {0xa9bd5ffe, 0xa90157f6, 0xa9024ff4, 0x90011bd5};
        network_state_start_original = reinterpret_cast<decltype(network_state_start_original)>(
            install_hook(0x7a37fc4, monitor_signature, (void*)network_state_start_hook));
        const uint32_t unlock_signature[] = {0xd10103ff, 0xa9015ffe, 0xa90257f6, 0xa9034ff4};
        install_hook(0x7e7e9f4, unlock_signature, (void*)local_unlock_update);
        const uint32_t monitor_end_signature[] = {0xf9401000, 0xb4000060, 0xaa1f03e1, 0x145811d5};
        install_hook(0x7a380e8, monitor_end_signature, (void*)network_state_end_hook);
        const uint32_t pve_signature[] = {0xd10103ff, 0xf9000bfe, 0xa90257f6, 0xa9034ff4};
        pve_start_original = reinterpret_cast<decltype(pve_start_original)>(
            install_hook(0x801fe34, pve_signature, (void*)local_pve_start));
        const uint32_t restart_signature[]={0xa9be57fe,0xa9014ff4,0xb000ec74,0xf000b813};
        pve_restart_original=(decltype(pve_restart_original))install_hook(0x8026714,restart_signature,(void*)local_pve_restart);
        const uint32_t mission_info_signature[] = {0xa9be57fe, 0xa9014ff4, 0xaa0003f3, 0xf8420c08};
        mission_info_original = reinterpret_cast<decltype(mission_info_original)>(
            install_hook(0x80206d0, mission_info_signature, (void*)local_mission_info));
        const uint32_t stop_signature[] = {0xf81c0ffe, 0xa9015ff8, 0xa90257f6, 0xa9034ff4};
        logic_stop_original = reinterpret_cast<decltype(logic_stop_original)>(
            install_hook(0x81974b4, stop_signature, (void*)local_logic_stop));
        interaction_trace::install();
        ai_lab::install();
        local_match::install();
        local_training::install();
        local_hero::project_display=generic_unit_research::display_projection;
        local_hero::install();
        hero_parameters::install();
        research_view::install();
        local_skill::effect_supported=skill_combat::supported;
        local_hero::capacity_projector=skill_hero_capacity::project;
        local_skill::prepare_combat=skill_combat::prepare;
        local_quests::install();
        local_contracts::install();
        battle_contract_runtime::install();
        local_contracts::content_observer=daily_box_content_audit::capture;
        local_boost_conversion::install();
        local_skill::install();
        local_hero_battle::install();
        local_item_guards::install();
        local_modifications::install();
        local_shop_boxes::install();
        local_contract_boxes::install();
        effective_boxes::install();
        local_advanced_trace::install();
        local_career::box_rewards=career_boxes::resolve;
        local_reward_skills::install();
        local_reward_skills::cooperative_credit_installed=true;
        local_resources::install();
        local_coop::install();
        local_competition_tracks::install();
        local_history::install();
        profile_statistics_runtime::install();
        local_inventory::install();
        offline_entry::install();
        const uint32_t replay_input_signature[]={0xf81e0ffe,0xa9014ff4,0xaa0003f3,0xaa1f03e0};
        battle_upgrade_check_original=(decltype(battle_upgrade_check_original))install_hook(0x820ca60,replay_input_signature,(void*)research_battle_upgrade_check);
        if (!access(DATA_ROOT "research-live-audit.enabled", F_OK) || interaction_trace::enabled) {
            const uint32_t max_health_signature[] = {0xa9bf4ffe, 0x52800381, 0x2a1f03e2, 0xaa0003f3};
            building_max_health_original = reinterpret_cast<decltype(building_max_health_original)>(
                install_hook(0x80dc060, max_health_signature, (void*)local_building_max_health));
            LOG("Research optional live audit installed=%d", battle_upgrade_check_original != nullptr);
        }
        const uint32_t army_tab_signature[] = {0xd101c3ff, 0xa9017bfd, 0xa9026ffc, 0xa90367fa};
        army_tab_original = reinterpret_cast<decltype(army_tab_original)>(
            install_hook(0x7c89190, army_tab_signature, (void*)army_tab_hook));
        const uint32_t upgrade_signature[] = {0xd100c3ff, 0xa90157fe, 0xa9024ff4, 0xd00104f3};
        upgrade_action_original = reinterpret_cast<decltype(upgrade_action_original)>(
            install_hook(0x7d131ac, upgrade_signature, (void*)local_upgrade_action));
        // Preserve the original artwork/layout; these entries intentionally do nothing.
        // Original shop navigation is owned by the local warehouse adapter.
        const uint32_t purchase_signature[] = {0xf81d0ffe, 0xa90157f6, 0xa9024ff4, 0x9000d8b4};
        install_hook(0x82a0904, purchase_signature, (void*)disabled_commercial_click);
        const uint32_t tapjoy_signature[] = {0xf81c0ffe, 0xa9015ff8, 0xa90257f6, 0xa9034ff4};
        install_hook(0x82a0bc8, tapjoy_signature, (void*)disabled_commercial_click);
        const uint32_t revu_signature[] = {0xf9402000, 0xb4000080, 0x52800521, 0xaa1f03e2};
        install_hook(0x82a0f78, revu_signature, (void*)disabled_commercial_click);
        const uint32_t ads_signature[] = {0xa9bd5ffe, 0xa90157f6, 0xa9024ff4, 0xf0011173};
        install_hook(0x7b81f00, ads_signature, (void*)disabled_commercial_click);
    }
}
#endif

#ifdef ART3_CAPTURE
#ifndef ART3_CAPTURE_LEGACY
#include "content_capture.h"
#endif
static void (*dispatch_original)(void*, void*, void*);
static unsigned capture_number;

static void capture_feature_flags() {
    void* klass = find_class("com.geargames.aow.scenes.main", "ClientMainContext");
    void* field = klass ? class_field_from_name(klass, "FeatureToggleService") : nullptr;
    void* service = nullptr;
    if (field) static_get(field, &service);
    if (!service) return;
    FILE* file = fopen(DATA_ROOT "captures/meta-feature-flags.txt", "w");
    if (!file) return;
    const char* fields[] = {"m_permanentFeatureToggles", "m_temporaryFeatureToggles"};
    for (const char* name : fields) {
        field = class_field_from_name(object_class(service), name);
        void* set = field ? *(void**)((char*)service + field_offset(field)) : nullptr;
        field = set ? class_field_from_name(object_class(set), "_slots") : nullptr;
        if (!field && set) field = class_field_from_name(object_class(set), "_entries");
        void* entries = field ? *(void**)((char*)set + field_offset(field)) : nullptr;
        if (!entries) continue;
        void* entry_class = class_element(object_class(entries));
        void* value_field = class_field_from_name(entry_class, "value");
        void* hash_field = class_field_from_name(entry_class, "hashCode");
        if (!value_field || !hash_field) continue;
        uint32_t alignment = 0;
        int stride = class_value_size(entry_class, &alignment);
        int vo = field_offset(value_field) - 16, ho = field_offset(hash_field) - 16;
        void* count_field = class_field_from_name(object_class(set), "_lastIndex");
        if (!count_field) count_field = class_field_from_name(object_class(set), "_count");
        size_t count = count_field ? *(int*)((char*)set + field_offset(count_field)) : array_length(entries);
        if (count > array_length(entries)) count = array_length(entries);
        fprintf(file, "%s:", name);
        for (size_t i = 0; i < count; i++) {
            char* entry = (char*)entries + 32 + stride*i;
            if (*(int*)(entry + ho) >= 0) fprintf(file, " %d", *(int*)(entry + vo));
        }
        fputc('\n', file);
    }
    fclose(file); chmod(DATA_ROOT "captures/meta-feature-flags.txt", 0644);
}

static void dispatch_hook(void* self, void* message, void* method) {
    const char* name = message ? class_name(object_class(message)) : "";
#ifndef ART3_CAPTURE_LEGACY
    content_capture::dispatch(message, name);
#else
    // Only game configuration / battle data; omit chat and account authentication messages.
    bool wanted = strstr(name, "Configuration") || strstr(name, "ChangeEntit") ||
                  strstr(name, "OnlineProfile") || strstr(name, "JoinToPvE") ||
                  strstr(name, "GoToBattle") || strstr(name, "LoginToBattleAnswer") ||
                  strstr(name, "InitBattle") || strstr(name, "BattleInitialization") ||
                  !strcmp(name, "StartBattleNotification") || !strcmp(name, "TimeSyncAnswer") ||
                  !strcmp(name, "ResourceAnswer");
    if (wanted) {
        void* helper = find_class("com.geargames.common.serialization", "SerializationHelpers");
        const void* serialize = helper ? class_method(helper, "SerializeMessage", 3) : nullptr;
        if (serialize) {
            int initial_capacity = 4096, maximum_capacity = 64 * 1024 * 1024;
            void* arguments[] = {message, &initial_capacity, &maximum_capacity};
            void* exception = nullptr;
            void* buffer = runtime_invoke(serialize, nullptr, arguments, &exception);
            if (!exception && buffer) {
                void* array = *(void**)((char*)buffer + 0x10);
                int length = *(int*)((char*)buffer + 0x1c);
                if (array && length > 0 && length <= maximum_capacity && (size_t)length <= array_length(array)) {
                    mkdir(DATA_ROOT "captures", 0777);
                    chmod(DATA_ROOT "captures", 0777);
                    char filename[512];
                    unsigned number = capture_number++;
                    snprintf(filename, sizeof(filename), DATA_ROOT "captures/%04u_%s.ggs", number, name);
                    FILE* file = fopen(filename, "wb");
                    if (file) { fwrite((char*)array + 32, 1, length, file); fclose(file); }
                    chmod(filename, 0644);
                    const void* get_type = class_method(object_class(message), "get_Type", 0);
                    void* type_exception = nullptr;
                    void* boxed_type = get_type ? runtime_invoke(get_type, message, nullptr, &type_exception) : nullptr;
                    if (boxed_type && !type_exception) {
                        snprintf(filename, sizeof(filename), DATA_ROOT "captures/%04u_%s.meta.txt", number, name);
                        file = fopen(filename, "w");
                        if (file) {
                            fprintf(file, "{\"message_type\":%d,\"class\":\"%s\",\"size\":%d}\n", *(int*)((char*)boxed_type + 16), name, length);
                            fclose(file);
                            chmod(filename, 0644);
                        }
                    }
                    snprintf(filename, sizeof(filename), DATA_ROOT "captures/%04u_%s.txt", number, name);
                    file = fopen(filename, "w");
                    if (file) { visited_count = 0; dump_object(file, message, 0, 5); fclose(file); }
                    chmod(filename, 0644);
                    LOG("Captured %s (%d bytes)", name, length);
                    if (!strcmp(name, "UserConfiguration")) capture_feature_flags();
                }
            } else LOG("Could not serialize %s", name);
        }
    }
#endif
    dispatch_original(self, message, method);
}

static void hook_dispatch() {
#ifndef ART3_CAPTURE_LEGACY
    content_capture::initialize();
#else
    if (DIR* directory = opendir(DATA_ROOT "captures")) {
        while (dirent* entry = readdir(directory)) {
            if (entry->d_name[0] == '.') continue;
            unsigned recorded_number = 0;
            char separator = 0;
            if (sscanf(entry->d_name, "%u%c", &recorded_number, &separator) == 2 && separator == '_' && recorded_number < 1000000)
                if (capture_number <= recorded_number) capture_number = recorded_number + 1;
            char filename[512];
            snprintf(filename, sizeof(filename), DATA_ROOT "captures/%s", entry->d_name);
            chmod(filename, 0644);
        }
        closedir(directory);
    }
#endif
    const uint32_t expected[] = {0xd10103ff, 0xf9000bfe, 0xa90257f6, 0xa9034ff4};
    dispatch_original = reinterpret_cast<decltype(dispatch_original)>(
        install_hook(0x79cd310, expected, (void*)dispatch_hook));
    LOG("Incoming configuration recorder installed=%d", dispatch_original != nullptr);
}
#endif

static bool hook_prepare() {
    auto target = (uint8_t*)(image_base + 0x7e0e84c);
    // Exactly four stack-only instructions; no PC-relative relocation is needed.
    const uint32_t expected[] = {0xd10143ff, 0xa9025ffe, 0xa90357f6, 0xa9044ff4};
    if (memcmp(target, expected, sizeof(expected))) {
        LOG("Unsupported code signature; refusing hook"); return false;
    }
    void* trampoline = mmap(nullptr, 4096, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (trampoline == MAP_FAILED) return false;
    memcpy(trampoline, target, 16);
    absolute_jump((char*)trampoline + 16, target + 16);
    __builtin___clear_cache((char*)trampoline, (char*)trampoline + 32);
    if (mprotect(trampoline, 4096, PROT_READ | PROT_EXEC)) return false;
    prepare_original = reinterpret_cast<decltype(prepare_original)>(trampoline);
    size_t page_size = (size_t)sysconf(_SC_PAGESIZE);
    auto page = (void*)((uintptr_t)target & ~(page_size - 1));
    if (mprotect(page, page_size, PROT_READ | PROT_WRITE | PROT_EXEC)) return false;
    absolute_jump(target, (void*)prepare_hook);
    __builtin___clear_cache((char*)target, (char*)target + 16);
    mprotect(page, page_size, PROT_READ | PROT_EXEC);
    LOG("Local battle hook installed");
    return true;
}

static void* bootstrap(void*) {
    for (int i = 0; i < 1200; i++) {
        il2cpp = dlopen("libil2cpp.so", RTLD_NOW | RTLD_NOLOAD);
        if (il2cpp) break;
        usleep(50000);
    }
    if (!il2cpp) { LOG("IL2CPP load timed out"); return nullptr; }
    bool ok = true;
#define API(var, name) ok = api(var, "il2cpp_" name) && ok
    API(domain_get, "domain_get"); API(domain_assemblies, "domain_get_assemblies");
    API(assembly_image, "assembly_get_image"); API(class_from_name, "class_from_name");
    API(object_class, "object_get_class"); API(class_name, "class_get_name");
    API(class_namespace, "class_get_namespace"); API(class_parent, "class_get_parent");
    API(class_fields, "class_get_fields"); API(class_field_from_name, "class_get_field_from_name");
    API(field_name, "field_get_name"); API(field_type, "field_get_type");
    API(field_offset, "field_get_offset"); API(field_flags, "field_get_flags");
    API(type_type, "type_get_type"); API(type_name, "type_get_name");
    API(static_get, "field_static_get_value"); API(array_length, "array_length");
    API(static_set, "field_static_set_value");
    API(field_set, "field_set_value");
    API(class_element, "class_get_element_class"); API(class_valuetype, "class_is_valuetype");
    API(class_from_type, "class_from_type");
    API(class_from_system_type, "class_from_system_type");
    API(il_free, "free");
    API(class_value_size, "class_value_size");
    API(class_method, "class_get_method_from_name"); API(runtime_invoke, "runtime_invoke");
    API(object_new, "object_new"); API(array_new, "array_new");
    API(string_new, "string_new");
    API(class_type, "class_get_type"); API(type_object, "type_get_object");
    API(gchandle_new, "gchandle_new");
    API(gchandle_free, "gchandle_free");
#undef API
    if (!ok) return nullptr;
    Dl_info info{};
    if (!dladdr((void*)domain_get, &info)) return nullptr;
    image_base = (uintptr_t)info.dli_fbase;
    LOG("IL2CPP base=%p", info.dli_fbase);
#ifdef ART3_CAPTURE
    hook_dispatch();
#else
    hook_prepare();
    hook_offline_main();
#endif
    return nullptr;
}

extern "C" JNIEXPORT jint JNI_OnLoad(JavaVM* vm, void* reserved) {
    java_vm = vm;
    void* original = dlopen("libart3originalmain.so", RTLD_NOW | RTLD_LOCAL);
    if (!original) { LOG("Original NativeLoader failed: %s", dlerror()); return JNI_ERR; }
    auto onload = reinterpret_cast<jint(*)(JavaVM*, void*)>(dlsym(original, "JNI_OnLoad"));
    if (!onload) return JNI_ERR;
    jint result = onload(vm, reserved);
    if (result < 0) return result;
    pthread_t thread;
    if (!pthread_create(&thread, nullptr, bootstrap, nullptr)) pthread_detach(thread);
    LOG("Development bootstrap loaded");
    return result;
}
