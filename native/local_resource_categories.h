// Reconnect the original resource configurator to retained client configuration.
// Original resource dispatch still owns category readiness and catalogue loading.
#pragma once
namespace local_resources {
static void* configurator;
static bool submitted;
static bool retained_maps;
static bool (*condition_original)(void*,void*);
static bool condition(void* self,void* method){
    // A download progression condition is unnecessary for verified, packaged
    // local content. This does not change profile rank, chapter completion or
    // category readiness; the original dispatcher must still load the assets.
    if(offline_network&&retained_maps)return true;
    return condition_original(self,method);
}
static void install(){
    const uint32_t signature[]={0xf81e0ffe,0xa9014ff4,0xf9400008,0xaa0003f3};
    condition_original=(decltype(condition_original))install_hook(0x7e9f144,signature,(void*)condition);
    LOG("Local retained-map download condition hook=%d",condition_original!=nullptr);
}
static bool ready(){
    if(!submitted||!configurator)return false;
    void* tracker=*(void**)((char*)configurator+0x20);void* name=string_new("Maps");
    void* args[]={&name};void* ex=nullptr;
    void* value=tracker?invoke_named(tracker,"IsCategoryInitialized",1,args,&ex):nullptr;
    return value&&!ex&&*((bool*)value+16);
}
static bool prepare(){
    if(submitted)return true;
    if(!condition_original||access(DATA_ROOT "AddressablesCatalogs/Maps/catalog.json",F_OK))return false;
    retained_maps=true;
    void* project_class=find_class("Zenject","ProjectContext");
    const void* instance=project_class?class_method(project_class,"get_Instance",0):nullptr;
    void* ex=nullptr;void* project=instance?runtime_invoke(instance,nullptr,nullptr,&ex):nullptr;
    void* container=project&&!ex?invoke_named(project,"get_Container",0,nullptr,&ex):nullptr;
    if(!container||ex){LOG("Local map categories: project container unavailable");return false;}
    const char* interfaces[]={"IResourceCategoriesConfigurator","ResourceCategoriesBootstrapper"};
    for(const char* name:interfaces){
        void* klass=find_class("Domains.ResourceManagement.Impl",name);
        if(!klass)klass=find_class("Domains.ResourceManagement.Interfaces",name);
        if(!klass)klass=find_class("Domains.ResourceManagement.Api",name);
        if(!klass)continue;
        void* contract=type_object(class_type(klass));void* args[]={contract};
        configurator=invoke_named(container,"TryResolve",1,args,&ex);
        if(configurator&&!ex)break;ex=nullptr;
    }
    if(!configurator||ex){LOG("Local map categories: configurator binding unavailable");return false;}
    FILE* f=fopen(DATA_ROOT "art3offline-resource-categories.txt","w");
    if(f){visited_count=0;dump_object(f,configurator,0,2);fclose(f);chmod(DATA_ROOT "art3offline-resource-categories.txt",0644);}
    void* error=nullptr;void* args[]={saved("ClientResourceConfiguration"),&error};
    void* result=invoke_named(configurator,"TryApplyClientResourceConfiguration",2,args,&ex);
    bool ok=result&&!ex&&*((bool*)result+16);
    if(ok)invoke_named(configurator,"ApplyCrpSettings",0,nullptr,&ex);
    char reason[256]={};if(error)local_research_runtime::ascii(error,reason,sizeof(reason));
    LOG("Local map categories original configuration applied=%d exception=%s error=%s",ok&&!ex,ex?class_name(object_class(ex)):"none",reason);
    submitted=ok&&!ex;return submitted;
}
}
