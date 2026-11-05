// Original embedded first-battle objects. No server or captured user session.
#pragma once
namespace campaign_seeds {
using local_research_runtime::Roots;
static void* decode(const char* path,Roots& roots) {
    FILE* f=fopen(path,"rb");if(!f)return nullptr;
    fseek(f,0,SEEK_END);long length=ftell(f);rewind(f);
    if(length<32||length>16*1024*1024){fclose(f);return nullptr;}
    void* bytes=array_new(find_class("System","Byte"),length);
    bool loaded=roots.keep(bytes)&&fread((char*)bytes+32,1,length,f)==(size_t)length;fclose(f);if(!loaded)return nullptr;
    void* buffer=object_new(find_class("com.geargames.common.utils","ClientByteBuffer"));if(!roots.keep(buffer))return nullptr;
    reinterpret_cast<void(*)(void*,void*,void*)>(image_base+0x7a02ee8)(buffer,bytes,nullptr);
    // Exact release metadata slot, from script.json and the original
    // FirstMissionUtils call site. Use runtime_invoke to retain exceptions.
    auto slot=(void**)(image_base+0x99a85e8);
    reinterpret_cast<void(*)(void*)>(image_base+0x3cfb0d4)(slot);
    const void* method=*slot;
    if(!method||*(uintptr_t*)method!=image_base+0x50a68d8){LOG("Embedded seed rejected: generic decoder metadata mismatch");return nullptr;}
    void* exception=nullptr;void* args[]={buffer};void* value=runtime_invoke(method,nullptr,args,&exception);
    if(exception||!value||strcmp(class_name(object_class(value)),"FirstMissionConfiguration")||!roots.keep(value)){
        LOG("Embedded seed object decode rejected exception=%s",exception?class_name(object_class(exception)):"none");return nullptr;}
    return value;
}
static bool inspect(void* config,int expected_fraction,FILE* out,const char* name) {
    void* battle=config?*(void**)((char*)config+0x10):nullptr;int own=config?*(int*)((char*)config+0x18):0;
    void* description=config?*(void**)((char*)config+0x20):nullptr;
    void* mission=description?*(void**)((char*)description+0x18):nullptr;
    void* map_description=config?*(void**)((char*)config+0x40):nullptr;
    void* sides=battle?*(void**)((char*)battle+0x178):nullptr;void* side=local_dictionary_item(sides,own);
    void* fraction=side?*(void**)((char*)side+0xd8):nullptr;
    void* map=battle?*(void**)((char*)battle+0x160):nullptr;
    void* gai=battle?*(void**)((char*)battle+0x140):nullptr;void* commands=gai?*(void**)((char*)gai+0x30):nullptr;
    int count=commands?*(int*)((char*)commands+0x18):0;
    void* canonical=mission?local_mission_description(*(int*)((char*)mission+0x10)):nullptr;
    bool valid=canonical&&battle&&mission&&side&&fraction&&map&&map_description&&*(int*)((char*)canonical+0x14)==*(int*)((char*)map_description+0x10)&&count>0&&count<100000&&
        *(int*)((char*)fraction+0x10)==expected_fraction&&*(int*)((char*)battle+0x28)==*(int*)((char*)mission+0x10)&&
        *(int*)((char*)map_description+0x10)==*(int*)((char*)mission+0x14)&&*(void**)((char*)map+0x18);
    if(out)fprintf(out,"{\"record\":\"embedded_seed\",\"name\":\"%s\",\"passed\":%s,\"fraction\":%d,\"mission\":%d,\"map\":%d,\"own_side\":%d,\"commands\":%d,\"width\":%d,\"height\":%d}\n",name,valid?"true":"false",fraction?*(int*)((char*)fraction+0x10):-1,mission?*(int*)((char*)mission+0x10):-1,map_description?*(int*)((char*)map_description+0x10):-1,own,count,map?*(int16_t*)((char*)map+0x14):0,map?*(int16_t*)((char*)map+0x16):0);
    return valid;
}
static void audit() {
    if(access(DATA_ROOT "campaign-seeds-audit.enabled",F_OK))return;
    FILE* out=fopen(DATA_ROOT "art3offline-campaign-seeds.jsonl","w");if(!out)return;chmod(DATA_ROOT "art3offline-campaign-seeds.jsonl",0644);
    bool ok=true;
    for(const char* name:{"fmc1m","fmc1r","fmc2m","fmc2r"}) {
        char path[512];snprintf(path,sizeof(path),DATA_ROOT "offline-seeds/%s.ggs",name);
        Roots roots;void* config=decode(path,roots);ok=inspect(config,name[3]-'0',out,name)&&ok;
        void* another=decode(path,roots);bool fresh=another&&config&&another!=config&&
            *(void**)((char*)another+0x10)!=*(void**)((char*)config+0x10);
        if(fresh){void* a=*(void**)((char*)config+0x10);void* b=*(void**)((char*)another+0x10);
            for(int offset:{0x140,0x160,0x178})fresh=fresh&&*(void**)((char*)a+offset)!=*(void**)((char*)b+offset);}

        fprintf(out,"{\"record\":\"fresh_decode\",\"name\":\"%s\",\"passed\":%s}\n",name,fresh?"true":"false");ok=ok&&fresh;
        if(config&&another&&name[3]=='1') {
            // Targeted contract for a second player faction, with no battle
            // initialization, profile writes, unlocks or speculative data.
            void* battle=*(void**)((char*)config+0x10);
            void* donor=*(void**)((char*)another+0x10);
            void* own=local_dictionary_item(*(void**)((char*)battle+0x178),1);
            void* other=local_dictionary_item(*(void**)((char*)donor+0x178),1);
            void* enemy=local_dictionary_item(*(void**)((char*)battle+0x178),2);
            void* alliances=*(void**)((char*)battle+0x170);
            bool independent=own&&other&&own!=other;
            if(independent)for(int offset:{0xf0,0xf8})independent=independent&&
                *(void**)((char*)own+offset)&&*(void**)((char*)own+offset)!=*(void**)((char*)other+offset);
            void* types=own?*(void**)((char*)own+0xf8):nullptr;
            int available=0;
            for(int id:{1000,1001,1002,1003,1005})if(local_dictionary_item(types,id))++available;
            uintptr_t handle=0;
            void* units=own?local_dictionary_values(*(void**)((char*)own+0xf0),find_class("com.geargames.aow.entities","UnitType"),handle):nullptr;
            int weighted=0,basic=0;
            if(units&&roots.take(handle))for(size_t i=0;i<array_length(units);++i) {
                void* type=((void**)((char*)units+32))[i];if(!type)continue;
                void* weights=*(void**)((char*)type+0x88);
                if(weights&&array_length(weights)&&*(int*)((char*)type+0x90)>0)++weighted;
                if(*(int*)((char*)type+0x10)==1000)basic=1;
            }
            fprintf(out,"{\"record\":\"ai_faction_contract\",\"name\":\"%s\",\"independent_dictionaries\":%s,\"original_enemy_seat\":%s,\"alliances\":%zu,\"required_building_types\":%d,\"basic_unit_1000\":%d,\"weighted_types\":%d,\"battle_runs\":0,\"save_writes\":0}\n",
                name,independent?"true":"false",enemy?"true":"false",alliances?array_length(alliances):0,available,basic,weighted);
        }
        if(config&&name[3]=='2') {
            void* battle=*(void**)((char*)config+0x10);
            bool applied=apply_local_battle_research(battle,*(int*)((char*)config+0x18));ok=ok&&applied;
            fprintf(out,"{\"record\":\"saved_research\",\"name\":\"%s\",\"passed\":%s}\n",name,applied?"true":"false");
        }
    }
    fprintf(out,"{\"record\":\"summary\",\"passed\":%s,\"seeds\":4,\"battle_runs\":0,\"save_writes\":0}\n",ok?"true":"false");fclose(out);
    LOG("Embedded campaign seed audit passed=%d",ok);
}
}
