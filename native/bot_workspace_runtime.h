// Opt-in local input adapter for the ORIGINAL native recruitment module.
// Construction/attack strategy is deliberately not activated by this adapter.
#pragma once
#include <stdarg.h>
namespace local_bot_runtime {
using local_research_runtime::Roots;
static bool enabled;
struct SeatState {
    void* side = nullptr;
    void* workspace = nullptr;
    void* geometry = nullptr;
    uintptr_t workspace_root = 0, geometry_root = 0;
    int profiles = 5;
};
static SeatState seats[8];
static SeatState* state(void* side) {
    int id=side?*(int*)((char*)side+0x10):0;
    return id>0&&id<=8?&seats[id-1]:nullptr;
}
static bool ready(void* side) { auto s=state(side); return s&&s->side==side&&s->workspace&&s->geometry; }
static void log(const char* format, ...) {
    FILE* f = fopen(DATA_ROOT "art3offline-native-bot.txt", "a");
    if (!f) return;
    va_list args; va_start(args, format); vfprintf(f, format, args); va_end(args);
    fputc('\n',f); fclose(f); chmod(DATA_ROOT "art3offline-native-bot.txt",0644);
}
static void reset(bool lab_active,bool player_match=false) {
    for(auto& s:seats) {
        if(s.workspace_root) gchandle_free(s.workspace_root);
        if(s.geometry_root) gchandle_free(s.geometry_root);
        s=SeatState{};
    }
    enabled = lab_active && (player_match || !access(DATA_ROOT "native-bot-lab.enabled",F_OK));
    if (enabled) {
        FILE* f=fopen(DATA_ROOT "art3offline-native-bot.txt","w");
        if(f) {fputs("schema=native-bot-hire-v1 original_weights=1 local_category_policy=32 profile_index=0 full_bot=0 progression_writes=0\n",f);fclose(f);chmod(DATA_ROOT "art3offline-native-bot.txt",0644);}
    }
}
static void* ref_field(void* object, int offset) {
    for(void* c=object?object_class(object):nullptr;c;c=class_parent(c)) {
        void* iter=nullptr;
        while(void* f=class_fields(c,&iter)) if(!(field_flags(f)&0x10)&&field_offset(f)==offset) return f;
    }
    return nullptr;
}
static bool put(void* object, int offset, void* value) {
    void* f=ref_field(object,offset); if(!f) return false;
    field_set(object,f,value);
    return *(void**)((char*)object+offset)==value;
}
static bool call(void* self, const char* name, int count, void** args, void*& result) {
    const void* method=self?class_method(object_class(self),name,count):nullptr;
    void* exception=nullptr;
    result=method?runtime_invoke(method,self,args,&exception):nullptr;
    if(!method||exception) {
        log("FAIL method=%s exception=%s",name,exception?class_name(object_class(exception)):"method-unavailable");return false;
    }
    return true;
}
static void* construct(void* klass, Roots& roots) {
    void* value=klass?object_new(klass):nullptr; void* ignored=nullptr;
    return roots.keep(value)&&call(value,".ctor",0,nullptr,ignored)?value:nullptr;
}
static void* integers(int value, Roots& roots) {
    void* array=array_new(find_class("System","Int32"),5);
    if(!roots.keep(array)) return nullptr;
    for(int i=0;i<5;++i) ((int*)((char*)array+32))[i]=value;
    return array;
}
// Original metadata-usage slot, independently present in script.json and the
// workspace deserializer. Resolve the slot using IL2CPP's own metadata helper.
static void* category_dictionary_class() {
    auto resolve=reinterpret_cast<void(*)(void*)>(image_base+0x3cfb0d4);
    void* slot=(void*)(image_base+0x9932b28); resolve(slot);
    void* klass=*(void**)slot;
    return klass&&!strcmp(class_name(klass),"Dictionary`2")?klass:nullptr;
}
struct Binding {
    void* battle; void* side; void* old_workspace; void* old_geometry; Roots roots; bool valid;
    Binding(void* b,void* s):battle(b),side(s),valid(false) {
        old_workspace=*(void**)((char*)b+0x138); old_geometry=*(void**)((char*)s+0xe8);
        if((old_workspace&&!roots.keep(old_workspace))||(old_geometry&&!roots.keep(old_geometry))) return;
        auto seat=state(s);
        valid=seat&&seat->side==s&&put(b,0x138,seat->workspace)&&put(s,0xe8,seat->geometry);
    }
    ~Binding() { put(battle,0x138,old_workspace);put(side,0xe8,old_geometry); }
};
static bool native_call(const char* type, const char* name, void* battle, void* side) {
    void* klass=find_class("com.geargames.aow.entities.battle.bot",type);
    const void* method=klass?class_method(klass,name,2):nullptr;
    void* exception=nullptr; void* args[]={battle,side};
    if(method) runtime_invoke(method,nullptr,args,&exception);
    if(!method||exception) {
        log("FAIL native=%s.%s exception=%s",type,name,exception?class_name(object_class(exception)):"method-unavailable");return false;
    }
    return true;
}
static bool prepare(void* battle, void* side, int now) {
    auto seat=state(side);
    if(!seat||seat->workspace||seat->geometry) return false;
    Roots roots;
    void* w=construct(find_class("com.geargames.aow.entities.battle.bot","BuildByGAIActWorkspace"),roots);
    void* g=construct(find_class("com.geargames.aow.entities.battle.bot","BuildGeometry"),roots);
    void* table=construct(category_dictionary_class(),roots);
    void* settings=construct(find_class("com.geargames.aow.entities.battle.bot","GAIBotSettings"),roots);
    if(!w||!g||!table||!settings) return false;
    uintptr_t units_root=0;
    void* values=local_dictionary_values(*(void**)((char*)side+0xf0),find_class("com.geargames.aow.entities","UnitType"),units_root);
    if(!values||!roots.take(units_root)) return false;
    int categories=0, weighted_types=0;
    for(size_t i=0;i<array_length(values);++i) {
        void* type=((void**)((char*)values+32))[i]; if(!type) continue;
        void* probabilities=*(void**)((char*)type+0x88);
        int category=*(int*)((char*)type+0x90);
        size_t count=probabilities?array_length(probabilities):0;
        if(count>32) {log("FAIL probability-length type=%d length=%zu",*(int*)((char*)type+0x10),count);return false;}
        if(!count||category<=0) continue;
        if((int)count<seat->profiles)seat->profiles=(int)count;
        ++weighted_types;
        char weights[512]={};size_t used=0;
        for(size_t j=0;j<count;++j) {
            int written=snprintf(weights+used,sizeof(weights)-used,"%s%d",j?",":"",((int*)((char*)probabilities+32))[j]);
            if(written<0||(size_t)written>=sizeof(weights)-used) return false;
            used+=(size_t)written;
        }
        log("WEIGHT type=%d category=%d length=%zu values=%s",*(int*)((char*)type+0x10),category,count,weights);
        if(local_dictionary_item(table,category)) continue;
        Roots item_roots;
        void* item=construct(find_class("com.geargames.aow.entities.battle.bot","GAIHireCategoryType"),item_roots);
        void* limits=integers(32,item_roots);
        if(!item||!limits||!put(item,0x18,limits)) return false;
        *(int*)((char*)item+0x10)=category;
        void* args[]={&category,item}; void* ignored=nullptr;
        if(!call(table,"set_Item",2,args,ignored)||local_dictionary_item(table,category)!=item) return false;
        ++categories;
    }
    if(!weighted_types||!categories) {log("FAIL no-original-hire-data");return false;}
    // Only this verified timing pair is supplied; no invented values are
    // supplied for unknown full-strategy settings or construction orders.
    void* low=integers(50,roots); void* high=integers(50,roots);
    if(!low||!high||!put(settings,0x78,low)||!put(settings,0x80,high)||
       !put(w,0x18,table)||!put(w,0x20,settings)) return false;
    *(int*)((char*)g+0x20)=1; // Native Hire.$Nk subtracts 1 to index the five weights/quotas.
    seat->side=side;seat->workspace=w;seat->geometry=g;
    seat->workspace_root=gchandle_new(w,true); seat->geometry_root=gchandle_new(g,true);
    if(!seat->workspace_root||!seat->geometry_root) return false;
    Binding binding(battle,side);
    if(!binding.valid||!native_call("BuildOrderByGAIAct","$Oj",battle,side)) return false;
    int next=*(int*)((char*)g+0x78);
    if(next!=now+50) {log("FAIL native-clock actual=%d expected=%d",next,now+50);return false;}
    *(int*)((char*)g+0x78)=now+100;
    if(!native_call("BuildOrderByGAIAct","$Oj",battle,side)||*(int*)((char*)g+0x78)!=now+100) {
        log("FAIL native-clock-regressed");return false;
    }
    *(int*)((char*)g+0x78)=0;
    log("READY tick=%d side=%d weighted_types=%d categories=%d native_clock=%d max_preserved=1 construction_enabled=0 attack_enabled=0",
        now,*(int*)((char*)side+0x10),weighted_types,categories,next);
    return true;
}
static bool recruit(void* battle, void* side, int now, int profile=1) {
    if(!ready(side)&&!prepare(battle,side,now)) {log("FAIL prepare");return false;}
    auto seat=state(side);auto geometry=seat->geometry;
    if(profile<1||profile>5)return false;
    if(profile>seat->profiles)profile=1;
    if(*(int*)((char*)geometry+0x20)!=profile){
        *(int*)((char*)geometry+0x20)=profile;
        log("PROFILE tick=%d side=%d column=%d original_weights=1",now,*(int*)((char*)side+0x10),profile);
    }
    int before=*(int*)((char*)side+0x16c);
    Binding binding(battle,side);
    if(!binding.valid||!native_call("HireByGAIAct","$Nk",battle,side)) return false;
    log("RECRUIT tick=%d side=%d gold=%d->%d selected=%d target=%d phase=%d",now,*(int*)((char*)side+0x10),
        before,*(int*)((char*)side+0x16c),
        *(void**)((char*)geometry+0x48)?*(int*)((char*)*(void**)((char*)geometry+0x48)+0x10):0,
        *(int*)((char*)geometry+0x50),*(int*)((char*)geometry+0x54));
    return true;
}
}
