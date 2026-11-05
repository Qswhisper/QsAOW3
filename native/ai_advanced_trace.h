// Opt-in observation of the exact original command path; never alters its result.
#pragma once
namespace local_advanced_trace {
static bool active;static int calls;
static void emit(const char* format,...){FILE* f=fopen(DATA_ROOT "ai-advanced-command-trace.jsonl","a");if(!f)return;va_list args;va_start(args,format);vfprintf(f,format,args);va_end(args);fputc('\n',f);fclose(f);chmod(DATA_ROOT "ai-advanced-command-trace.jsonl",0644);}
static int16_t (*build_original)(void*,void*,void*,void*,void*);
static int16_t build(void* self,void* battle,void* side,void* type,void* method){
    void* counters=side?*(void**)((char*)side+0x350):nullptr;
    bool fourth_yard=type&&*(int8_t*)((char*)type+0x30)==4&&counters&&array_length(counters)>18&&((int16_t*)((char*)counters+32))[18]>=3;
    bool watch=type&&calls<4&&((local_duel::advanced_test&&*(int*)((char*)type+0x10)==1008)||(local_duel::naval_test&&(*(int8_t*)((char*)type+0x30)==11||fourth_yard)));bool previous=active;active=watch;
    if(watch)emit("{\"phase\":\"begin\",\"call\":%d,\"type\":%d,\"same_side_definition\":%s,\"x\":%d,\"y\":%d,\"angle\":%d}",calls+1,*(int*)((char*)type+0x10),type==local_dictionary_item(*(void**)((char*)side+0xf8),*(int*)((char*)type+0x10))?"true":"false",*(int16_t*)((char*)self+0x40),*(int16_t*)((char*)self+0x42),*(int16_t*)((char*)self+0x44));
    int16_t result=build_original(self,battle,side,type,method);active=previous;
    if(watch){++calls;emit("{\"phase\":\"command_return\",\"call\":%d,\"code\":%d}",calls,result);
        if(!result&&local_duel::naval_test&&*(int8_t*)((char*)type+0x30)==11){emit("{\"summary\":true,\"trace_complete\":true,\"calls\":%d,\"last_code\":0,\"production_passed\":false,\"continue_native_production\":true}",calls);calls=4;return result;}
        if(calls==4){emit("{\"summary\":true,\"trace_complete\":true,\"calls\":4,\"last_code\":%d,\"production_passed\":false}",result);local_duel::stopped=true;local_duel::log("END diagnostic-trace original_returns_preserved=1 career_writes=0");}}
    return result;
}
static int16_t (*footprint_original)(void*,void*,void*,void*,int,int,int16_t,void*,void*,int,void*);
static int16_t footprint(void* self,void* battle,void* side,void* type,int x,int y,int16_t angle,void* own,void* other,int extra,void* method){
    int16_t result=footprint_original(self,battle,side,type,x,y,angle,own,other,extra,method);
    if(active)emit("{\"phase\":\"full_footprint\",\"code\":%d,\"x\":%d,\"y\":%d,\"angle\":%d,\"extra\":%d,\"own_units\":%d,\"other_units\":%d}",result,x,y,angle,extra,own?*(int*)((char*)own+0x18):-1,other?*(int*)((char*)other+0x18):-1);return result;
}
static bool (*cell_original)(void*,void*,void*,void*,bool,void*);
static bool cell(void* battle,void* tile,void* own,void* other,bool clear,void* method){
    bool result=cell_original(battle,tile,own,other,clear,method);
    if(active)emit("{\"phase\":\"original_cell_check\",\"allowed\":%s,\"terrain\":%d,\"build_allowed\":%d,\"height\":%d,\"clear\":%s}",result?"true":"false",tile?*(uint8_t*)((char*)tile+0x20):-1,tile?*(uint8_t*)((char*)tile+0x40):-1,tile?*(uint8_t*)((char*)tile+0x22):-1,clear?"true":"false");return result;
}
static bool (*area_original)(void*,void*,void*,int,int,void*);
static bool area(void* battle,void* side,void* type,int x,int y,void* method){bool result=area_original(battle,side,type,x,y,method);if(active)emit("{\"phase\":\"area\",\"allowed\":%s,\"type\":%d,\"side\":%d,\"x\":%d,\"y\":%d}",result?"true":"false",type?*(int*)((char*)type+0x10):-1,side?*(int*)((char*)side+0x10):-1,x,y);return result;}
static bool (*requirements_original)(void*,void*,void*,void*,void*);
static bool requirements(void* battle,void* side,void* level,void* type,void* method){bool result=requirements_original(battle,side,level,type,method);if(active)emit("{\"phase\":\"requirements\",\"level\":%d,\"allowed\":%s}",level?*(int*)((char*)level+0x10):-1,result?"true":"false");return result;}
static void install(){
    if(access(DATA_ROOT "ai-advanced-trace.enabled",F_OK))return;unlink(DATA_ROOT "ai-advanced-command-trace.jsonl");
    const uint32_t a[]={0xd10443ff,0xa90b7bfd,0xa90c6ffc,0xa90d67fa},b[]={0xd102c3ff,0xa9057bfd,0xa9066ffc,0xa90767fa},c[]={0xf81b0ffe,0xa90167fa,0xa9025ff8,0xa90357f6},d[]={0xd10283ff,0xa9047bfd,0xa9056ffc,0xa90667fa};
    build_original=(decltype(build_original))install_hook(0x491fbf4,a,(void*)build);footprint_original=(decltype(footprint_original))install_hook(0x4921858,b,(void*)footprint);area_original=(decltype(area_original))install_hook(0x47e7f08,c,(void*)area);requirements_original=(decltype(requirements_original))install_hook(0x47ed228,d,(void*)requirements);
    cell_original=(decltype(cell_original))install_hook(0x4700720,b,(void*)cell);
    LOG("Private advanced original command trace installed=%d",build_original&&footprint_original&&area_original&&requirements_original&&cell_original);
}
}
