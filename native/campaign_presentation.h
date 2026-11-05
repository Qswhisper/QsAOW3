// Original variant metadata in the original campaign UI, with stable local
// chapter receipts. Canonical retained descriptions are never modified.
#pragma once
static void* local_runtime_mission_dictionary(){
    if(void* existing=saved("@local-runtime-missions"))return existing;
    if(!local_scenarios.load(DATA_ROOT))return nullptr;
    local_faction::Roots roots;void* source=saved("MissionDictionary");
    void* dictionary=local_faction::clone(source,roots);
    void* field=source?class_field_from_name(object_class(source),"missionDescriptions"):nullptr;
    void* list=field?*(void**)((char*)source+field_offset(field)):nullptr;
    void* next=list?object_new(object_class(list)):nullptr;if(!dictionary||!roots.keep(next))return nullptr;
    void* ex=nullptr;invoke_named(next,".ctor",0,nullptr,&ex);if(ex)return nullptr;
    for(int i=0;i<*(int*)((char*)list+0x18);++i){void* item=list_item(list,i);if(!item)return nullptr;
        int id=*(int*)((char*)item+0x10);void* published=item;
        for(const auto& route:local_scenarios)if(route.chapter==id&&route.recorded_presentation){
            published=local_faction::clone(local_mission_description(route.recorded),roots);if(!published)return nullptr;
            if(*(int*)((char*)published+0x14)!=route.map)return nullptr;*(int*)((char*)published+0x10)=id;}
        void* args[]={published};invoke_named(next,"Add",1,args,&ex);if(ex)return nullptr;
    }
    field_set(dictionary,field,next);save_object("@local-runtime-missions",dictionary);return dictionary;
}
static void* local_presented_mission(int id){
    void* dictionary=local_runtime_mission_dictionary();void* field=dictionary?class_field_from_name(object_class(dictionary),"missionDescriptions"):nullptr;
    void* list=field?*(void**)((char*)dictionary+field_offset(field)):nullptr;
    for(int i=0;list&&i<*(int*)((char*)list+0x18);++i){void* item=list_item(list,i);if(item&&*(int*)((char*)item+0x10)==id)return item;}
    return nullptr;
}
