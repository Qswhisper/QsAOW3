// Bounded, opt-in tracing; preserves every original callback and return value.
#pragma once
#include <stdarg.h>
namespace interaction_trace {
static bool enabled;
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static unsigned lines;
static void record(const char* format, ...) {
    if (!enabled || !local_run_active) return;
    pthread_mutex_lock(&lock);
    if (lines++ < 1000) {
        FILE* file=fopen(DATA_ROOT "art3offline-interaction-trace.txt","a");
        if(file) { fprintf(file,"run=%llu ",(unsigned long long)local_run);
            va_list args; va_start(args,format); vfprintf(file,format,args); va_end(args);
            fputc('\n',file); fclose(file); chmod(DATA_ROOT "art3offline-interaction-trace.txt",0644); }
    }
    pthread_mutex_unlock(&lock);
}
static void* context(const char* field) { return static_object("com.geargames.aow.scenes.battle","ClientBattleContext",field); }
static int tick() { return reinterpret_cast<int(*)(void*)>(image_base+0x8078058)(nullptr); }
static void update_hold() {
    static void* owned_pause;
    void* executor=context("m_logicExecutor");
    if(!executor) return;
    if(owned_pause && owned_pause!=executor) owned_pause=nullptr;
    bool requested=!access(DATA_ROOT "interaction-hold.enabled",F_OK);
    if(requested && !owned_pause && !*((uint8_t*)executor+0x80)) {
        reinterpret_cast<void(*)(void*,void*)>(image_base+0x81a2648)(executor,nullptr);
        owned_pause=executor;record("DEBUG pause requested tick=%d",tick());
    } else if(!requested && owned_pause) {
        void* battle=*(void**)((char*)executor+0x18);
        if(battle && *((uint8_t*)executor+0x80)) {
            // Same original Battle tick getter used by ResumeLogic's consistency check.
            void* klass=object_class(battle);
            auto getter=*(int(**)(void*,void*))((char*)klass+0x1018);
            void* method=*(void**)((char*)klass+0x1020);
            int current=getter(battle,method);
            reinterpret_cast<void(*)(void*,int,void*)>(image_base+0x819ba0c)(executor,current,nullptr);
        }
        owned_pause=nullptr;record("DEBUG resumed tick=%d",tick());
    }
    static int previous=-1;
    int paused=owned_pause && *((uint8_t*)executor+0x80);
    if(previous!=paused) {
        FILE* f=fopen(DATA_ROOT "art3offline-interaction-pause.txt","w");
        if(f) {fprintf(f,"paused=%d tick=%d\n",paused,tick());fclose(f);chmod(DATA_ROOT "art3offline-interaction-pause.txt",0644);}
        previous=paused;
    }
}
static void frame(void* gui) {
    if(!enabled || !local_run_active) return;
    update_hold();
    static int last=-100; static uint64_t previous;
    int now=tick(); bool request=!access(DATA_ROOT "interaction-trace.request",F_OK);
    if(!request && now>=last && now-last<10) return;
    last=now;
    void* own=context("m_ownSide"); void* side=own?*(void**)((char*)own+0x28):nullptr;
    if(!side) return;
    void* ui=reinterpret_cast<void*(*)(void*)>(image_base+0x81e05b4)(nullptr);
    void* tutorial=*(void**)((char*)side+0xc0);
    void* gesture=context("<ActiveGestureManager>k__BackingField");
    void* handler=gesture?*(void**)((char*)gesture+0x90):nullptr;
    void* screen=context("m_screen");
    void* dispatcher=screen?*(void**)((char*)screen+0x1d0):nullptr;
    void* layout=dispatcher?*(void**)((char*)dispatcher+0x28):nullptr;
    uint64_t hash=(uintptr_t)handler+(uintptr_t)layout;
    for(int i=0x10;ui&&i<0x32;++i) hash=hash*131+*((uint8_t*)ui+i);
    if(tutorial) hash=hash*131+*(int*)((char*)tutorial+0x18);
    if(!request && hash==previous) return;
    previous=hash;
    void* buildings=*(void**)((char*)own+0x50); void* units=*(void**)((char*)own+0x48);
    void* controller=context("m_tutorialController");
    record("STATE tick=%d own=%d gold=%d buildings=%d units=%d handler=%s controller=%s",now,
        *(int*)((char*)own+0x10),*(int*)((char*)own+0x64),buildings?*(int*)((char*)buildings+0x18):-1,
        units?*(int*)((char*)units+0x18):-1,handler?class_name(object_class(handler)):"null",controller?class_name(object_class(controller)):"null");
    record("LAYOUT active=%s busy=%d",layout?class_name(object_class(layout)):"null",dispatcher?*((uint8_t*)dispatcher+0x20):-1);
    for(int offset:{0x20,0x1b0,0x1d8}) {
        void* node=*(void**)((char*)gui+offset);void* exception=nullptr;
        void* active=invoke_named(node,"get_activeSelf",0,nullptr,&exception);
        void* hierarchy=invoke_named(node,"get_activeInHierarchy",0,nullptr,&exception);
        record("NODE +%x self=%d hierarchy=%d",offset,active?*((uint8_t*)active+16):-1,hierarchy?*((uint8_t*)hierarchy+16):-1);
    }
    if(ui) record("UI build=%d spawn=%d order=%d/%d/%d settings=%d events=%d filter=%d",*((uint8_t*)ui+0x10),
        *((uint8_t*)ui+0x11),*((uint8_t*)ui+0x1f),*((uint8_t*)ui+0x20),*((uint8_t*)ui+0x21),*((uint8_t*)ui+0x27),*(int16_t*)((char*)ui+0x2c),*((uint8_t*)ui+0x2a));
    if(tutorial) record("CONSTRUCTION a=%d b=%d flag=%d c=%d",*(int*)((char*)tutorial+0x18),*(int*)((char*)tutorial+0x1c),*((uint8_t*)tutorial+0x20),*(int*)((char*)tutorial+0x24));
    if(request) {
        FILE* file=fopen(DATA_ROOT "art3offline-interaction-state.txt","w");
        if(file) { for(void* value:{controller,gesture,handler,ui,tutorial}) { visited_count=0;dump_object(file,value,0,2); }
            fprintf(file,"GUI mode\n");visited_count=0;dump_object(file,*(void**)((char*)gui+0x1f0),0,2);
            fprintf(file,"Dispatcher\n");visited_count=0;dump_object(file,dispatcher,0,1);
            for(int offset:{0x20,0x1b0,0x1c8,0x1d8}) {
                void* node=*(void**)((char*)gui+offset);void* exception=nullptr;
                void* active=invoke_named(node,"get_activeSelf",0,nullptr,&exception);
                void* hierarchy=invoke_named(node,"get_activeInHierarchy",0,nullptr,&exception);
                fprintf(file,"GUI node +%x activeSelf=%d activeInHierarchy=%d exception=%d\n",offset,
                    active?*((uint8_t*)active+16):-1,hierarchy?*((uint8_t*)hierarchy+16):-1,exception!=nullptr);
            }
            fclose(file);chmod(DATA_ROOT "art3offline-interaction-state.txt",0644); }
        unlink(DATA_ROOT "interaction-trace.request");
    }
}
static void (*build_original)(void*,void*);
static void build(void* self,void* method) { record("CLICK build tick=%d",tick());build_original(self,method); }
static void (*list_original)(void*,void*);
static void list(void* self,void* method) {
    void* type=*(void**)((char*)self+0x108);
    record("CLICK item=%d state=%d tick=%d",type?*(int*)((char*)type+0x10):-1,*(int*)((char*)self+0x30),tick());list_original(self,method);
}
static void (*ok_original)(void*,void*);
static void ok(void* self,void* method) {
    void* type=*(void**)((char*)self+0x88);
    record("CLICK placement type=%d status=%d cell=%d/%d tick=%d",type?*(int*)((char*)type+0x10):-1,
        *((uint8_t*)self+0x1a4),*(int*)((char*)self+0xe4),*(int*)((char*)self+0xe8),tick());ok_original(self,method);
}
static void (*train_original)(void*,void*,void*,void*);
static void train(void* self,void* building,void* type,void* method) {
    record("CLICK train unit=%d tick=%d",type?*(int*)((char*)type+0x10):-1,tick());train_original(self,building,type,method);
}
static void (*switch_original)(void*,void*,void*);
static void ui_switch(void* self,void* battle,void* method) {
    record("SCRIPT UISwitcher side=%d",*(int*)((char*)self+0x18));switch_original(self,battle,method);
}
static void (*auto_original)(void*,void*,void*);
static void automatic(void* self,void* battle,void* method) {
    record("SCRIPT construction side=%d type=%d",*(int*)((char*)self+0x18),*(int*)((char*)self+0x30));auto_original(self,battle,method);
}
static void (*perform_original)(void*,void*);
static void perform(void* command,void* method) {
    record("INPUT command=%s tick=%d",command?class_name(object_class(command)):"null",tick());
    perform_original(command,method);
}
static void (*error_original)(int16_t,void*,int,void*);
static void command_error(int16_t code,void* command,int current,void* method) {
    record("RESULT command=%s code=%d tick=%d",command?class_name(object_class(command)):"null",code,current);
    error_original(code,command,current,method);
}
static void install() {
    enabled=!access(DATA_ROOT "interaction-trace.enabled",F_OK); if(!enabled) return;
    const uint32_t a[]={0xf81e0ffe,0xa9014ff4,0xd000d7f4,0xb000a253};
    const uint32_t b[]={0xf81c0ffe,0xa9015ff8,0xa90257f6,0xa9034ff4};
    const uint32_t c[]={0xfc1b0fea,0x6d00a3e9,0xf9000ffe,0xa9025ff8};
    const uint32_t d[]={0xf81d0ffe,0xa90157f6,0xa9024ff4,0xf000d7b5};
    const uint32_t e[]={0xa9be57fe,0xa9014ff4,0xb002bb55,0xaa0103f4};
    const uint32_t f[]={0xd101c3ff,0xa90367fe,0xa9045ff8,0xa90557f6};
    build_original=(decltype(build_original))install_hook(0x82b6e9c,a,(void*)build);
    list_original=(decltype(list_original))install_hook(0x8208a78,b,(void*)list);
    ok_original=(decltype(ok_original))install_hook(0x8304530,c,(void*)ok);
    train_original=(decltype(train_original))install_hook(0x82bd058,d,(void*)train);
    switch_original=(decltype(switch_original))install_hook(0x4639740,e,(void*)ui_switch);
    auto_original=(decltype(auto_original))install_hook(0x460a52c,f,(void*)automatic);
    const uint32_t g[]={0xa9be57fe,0xa9014ff4,0xf000d354,0xaa0003f3};
    const uint32_t h[]={0xd10143ff,0xa90167fe,0xa9025ff8,0xa90357f6};
    perform_original=(decltype(perform_original))install_hook(0x834a690,g,(void*)perform);
    error_original=(decltype(error_original))install_hook(0x81a1814,h,(void*)command_error);
    LOG("Interaction trace hooks installed=%d",build_original&&list_original&&ok_original&&train_original&&switch_original&&auto_original);
}
}
