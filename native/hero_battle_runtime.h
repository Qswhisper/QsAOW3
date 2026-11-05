// local policy: refill to the original cap before each battle;
// retain native activation costs, slots, building/CP/gold requirements.
#pragma once
namespace local_hero_battle {
using local_research_runtime::Roots;
static void* prepared_battle;
static int prepared_side,prepared_cap;
static bool observed;
static bool installed,private_capture,auditing;
static uintptr_t captured[64];static void* captured_objects[64];static void* captured_methods[64];
static bool capture_failed;static int captured_count,captured_energy_count,captured_energy=-1;
static void (*add_original)(void*,void*);
static void add(void* command,void* method){
    if(!private_capture){add_original(command,method);return;}
    if(captured_count<64){auto handle=gchandle_new(command,true);if(handle){captured_methods[captured_count]=method;captured_objects[captured_count]=command;captured[captured_count++]=handle;return;}}
    capture_failed=true;
}
static void capture_begin(){private_capture=true;capture_failed=false;captured_count=captured_energy_count=0;captured_energy=-1;}
static void capture_end(){private_capture=false;while(captured_count)gchandle_free(captured[--captured_count]);}
static void capture_commit(){private_capture=false;for(int i=0;i<captured_count;++i)add_original(captured_objects[i],captured_methods[i]);capture_end();}
static void (*energy_original)(void*,void*,int,void*);
static void energy(void* self,void* side,int value,void* method){
    energy_original(self,side,value,method);
    if(private_capture){++captured_energy_count;captured_energy=value;}
    if(offline_network&&prepared_battle&&side&&side==local_dictionary_item(*(void**)((char*)prepared_battle+0x178),prepared_side)){
        int cap=*(int*)((char*)side+0x48);
        int current=*(int*)((char*)side+0x40);
        if(value<0||value>(current>cap?current:cap)){LOG("Local hero energy refused invalid original balance=%d cap=%d",value,cap);return;}
        // Original native code publishes the absolute balance as a client logic
        // command; there is no server to supply the next authoritative value.
        // Commit that exact output for subsequent native checks/calculations.
        *(int*)((char*)side+0x40)=value;
        LOG("Local hero original energy balance committed side=%d value=%d cap=%d",prepared_side,value,prepared_cap);
    }
}
static void* get(void* o,int offset){return o?*(void**)((char*)o+offset):nullptr;}
static bool owned(void* battle,int side_id,int type_id){
    // Training removes its GAI script and uses the career as authority. Keep
    // campaign/cooperative script-granted temporary heroes on the original
    // engine path; their original player UI still checks research state.
    if(!offline_network||(!local_match::playing&&!auditing)||private_capture||battle!=prepared_battle||side_id!=prepared_side)return true;
    void* side=local_dictionary_item(get(battle,0x178),side_id);void* ex=nullptr;
    auto slot=(void**)(image_base+160992016);reinterpret_cast<void(*)(void*)>(image_base+0x3cfb0d4)(slot);
    void* args[]={battle,get(side,0xf0),&type_id,side};void* type=runtime_invoke(*slot,nullptr,args,&ex);
    if(ex||!type||*(int*)((char*)type+0xa0)<1)return false;
    void* definitions=local_career::list("SHARED","heroCards");int n=definitions?*(int*)((char*)definitions+0x18):0;
    for(int i=0;i<n;++i){void* definition=list_item(definitions,i);int hero=*(int*)((char*)definition+0x10);
        if(!local_hero::card(hero,local_faction::active))continue;
        if(generic_unit_research::identity(type,local_career::lookup("BATTLE","unitTypes",hero)))return local_hero::acquired(progress,hero,local_faction::active);}
    return false;
}
static int16_t (*activate_original)(void*,int,int,int,void*);
static int16_t activate(void* battle,int side,int type,int slot,void* method){
    if(!owned(battle,side,type)){LOG("Local hero activation refused unowned side=%d type=%d",side,type);return -1;}
    return activate_original(battle,side,type,slot,method);
}
static int16_t (*buy_original)(void*,int,int,int,int,void*);
static int16_t buy(void* battle,int side,int type,int slot,int building,void* method){
    if(!owned(battle,side,type)){LOG("Local hero production refused unowned side=%d type=%d",side,type);return -1;}
    return buy_original(battle,side,type,slot,building,method);
}
static void* batteries(void* source,Roots& roots){
    if(!source)return nullptr;void* ex=nullptr;
    void* keys=invoke_named(source,"get_Keys",0,nullptr,&ex);
    void* boxed=keys&&!ex?invoke_named(keys,"get_Count",0,nullptr,&ex):nullptr;
    int count=boxed&&!ex?*(int*)((char*)boxed+16):-1;
    if(count<0||count>32||!roots.keep(keys))return nullptr;
    void* array=array_new(find_class("com.geargames.aow.entities","Battery"),count);
    void* copy=object_new(object_class(source));if(!roots.keep(array)||!roots.keep(copy))return nullptr;
    int zero=0;void* args[]={array,&zero};invoke_named(keys,"CopyTo",2,args,&ex);
    if(!ex)invoke_named(copy,".ctor",0,nullptr,&ex);if(ex)return nullptr;
    for(int i=0;i<count;++i){void* key=((void**)((char*)array+32))[i];int value=-1;
        void* query[]={key,&value};void* found=invoke_named(source,"TryGetValue",2,query,&ex);
        if(ex||!key||!found||!*(bool*)((char*)found+16)||value<0)return nullptr;
        void* add[]={key,&value};invoke_named(copy,"Add",2,add,&ex);if(ex)return nullptr;
    }
    return copy;
}
static bool prepare(void* info,void* battle,int own_id,void* configuration=nullptr){
    if(!installed||!offline_network||!progress_ready||!info||!battle)return false;
    if(!configuration)configuration=get(saved("UserConfiguration"),0x110);
    void* own=local_dictionary_item(get(battle,0x178),own_id);
    int cap=configuration?*(int*)((char*)configuration+0x28):0;
    if(!own||cap<1||cap>INT16_MAX)return false;
    Roots roots;void* stock=batteries(get(configuration,0x18),roots);if(!stock)return false;
    // Prepare every allocation before publishing either original object.
    *(int*)((char*)info+0x34)=cap;*(int*)((char*)info+0x38)=cap;
    field_set(info,class_field_from_name(object_class(info),"Batteries"),stock);
    *(int*)((char*)own+0x40)=cap;*(int*)((char*)own+0x48)=cap;
    field_set(own,class_field_from_name(object_class(own),"$qF"),stock);
    prepared_battle=battle;prepared_side=own_id;prepared_cap=cap;observed=false;
    LOG("Local hero battle input side=%d uranium=%d cap=%d batteries_granted=0 native_costs=1",own_id,cap,cap);
    return true;
}
static void observe(void* battle){
    if(observed||battle!=prepared_battle)return;
    void* own=static_object("com.geargames.aow.scenes.battle","ClientBattleContext","m_ownSide");
    void* side=local_dictionary_item(get(battle,0x178),prepared_side);
    if(!own||get(own,0x28)!=side||!get(side,0x78))return;
    void* model=get(own,0x1c8);if(!model)return;
    observed=true;int energy=*(int*)((char*)side+0x40),cap=*(int*)((char*)side+0x48);
    int client=*(int*)((char*)own+0x1b8),client_cap=*(int*)((char*)own+0x1bc);
    int model_energy=*(int*)((char*)model+0x38),model_cap=*(int*)((char*)model+0x28);
    bool pass=energy==prepared_cap&&cap==prepared_cap&&client==energy&&client_cap==cap&&model_energy==energy&&model_cap==cap;
    FILE* f=fopen(DATA_ROOT "hero-battle-input-live.jsonl","a");if(f){
        fprintf(f,"{\"pass\":%s,\"side\":%d,\"native_energy\":%d,\"native_cap\":%d,\"client_energy\":%d,\"client_cap\":%d,\"model_energy\":%d,\"model_cap\":%d,\"original_slots\":%zu}\n",pass?"true":"false",prepared_side,energy,cap,client,client_cap,model_energy,model_cap,array_length(get(side,0x78)));
        fclose(f);chmod(DATA_ROOT "hero-battle-input-live.jsonl",0644);}
    LOG("Local hero original battle models initialized pass=%d native=%d/%d client=%d/%d model=%d/%d",pass,energy,cap,client,client_cap,model_energy,model_cap);
}
static void install(){
    const uint32_t energy_sig[]={0xa9bd5ffe,0xa90157f6,0xa9024ff4,0xf000e136};
    energy_original=(decltype(energy_original))install_hook(0x818c810,energy_sig,(void*)energy);
    const uint32_t hero_sig[]={0xd101c3ff,0xa9017bfd,0xa9026ffc,0xa90367fa};
    activate_original=(decltype(activate_original))install_hook(0x4770190,hero_sig,(void*)activate);
    buy_original=(decltype(buy_original))install_hook(0x477241c,hero_sig,(void*)buy);
    installed=energy_original&&activate_original&&buy_original;
    {
        const uint32_t add_sig[]={0xa9be57fe,0xa9014ff4,0xf000e195,0xf000abf4};
        add_original=(decltype(add_original))install_hook(0x818087c,add_sig,(void*)add);
    }
    LOG("Local hero battle energy hook=%d private_event_capture=%d",installed,add_original!=nullptr);
}
}
