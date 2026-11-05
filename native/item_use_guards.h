// Stage original effects until the local battery debit is saved.
#pragma once
namespace local_item_guards {
static int amount(void* stock,void* key){int n=-1;void* ex=nullptr;void* args[]={key,&n};
    void* found=stock&&key?invoke_named(stock,"TryGetValue",2,args,&ex):nullptr;
    return found&&!ex&&*(bool*)((char*)found+16)?n:-1;}
static bool set_amount(void* stock,void* key,int n){void* ex=nullptr;void* args[]={key,&n};invoke_named(stock,"set_Item",2,args,&ex);return !ex&&amount(stock,key)==n;}
static int16_t (*command_original)(void*,void*,void*);
static int16_t execute(void* self,void* battle,void* method,local_progress::State& state,const char* root){
    if(!self||!battle||!command_original||!local_hero_battle::add_original||local_hero_battle::private_capture)return -1;
    int side_id=*(int*)((char*)self+0x28),id=*(int*)((char*)self+0x30);
    void* side=local_dictionary_item(*(void**)((char*)battle+0x178),side_id);
    void* key=id>=1&&id<=3?local_items::battery(id):nullptr;
    auto field=side?class_field_from_name(object_class(side),"$qF"):nullptr;
    void* stock=field?*(void**)((char*)side+field_offset(field)):nullptr;
    int count=local_progress::stock_amount(state,10,id),energy=side?*(int*)((char*)side+0x40):-1;
    int capacity=key?*(int*)((char*)key+0x14):0;
    if(!key||!side||count<=0||amount(stock,key)!=count||capacity<=0||energy<0||int64_t(energy)+capacity>INT_MAX)return -1;
    auto next=state;if(!local_progress::consume_stock(next,10,id,1))return -1;
    int cap=*(int*)((char*)side+0x48);
    local_hero_battle::capture_begin();int16_t code=command_original(self,battle,method);
    bool accepted=code==0&&!local_hero_battle::capture_failed&&*(int*)((char*)side+0x40)==energy+capacity&&
        *(int*)((char*)side+0x48)==cap&&amount(stock,key)==count&&local_hero_battle::captured_energy_count==1&&local_hero_battle::captured_energy==energy+capacity;
    bool committed=accepted&&set_amount(stock,key,count-1)&&local_progress::write(root,next);
    if(!committed){*(int*)((char*)side+0x40)=energy;*(int*)((char*)side+0x48)=cap;
        bool restored=set_amount(stock,key,count);local_hero_battle::capture_end();
        LOG("Local battery refused id=%d native=%d accepted=%d restored=%d",id,code,accepted,restored);return -1;}
    state=next;local_hero_battle::capture_commit();
    LOG("Local battery consumed id=%d remaining=%d uranium=%d->%d generation=%llu",id,count-1,energy,energy+capacity,(unsigned long long)state.generation);return code;
}
static bool owned(void* battery){int id=battery?*(int*)((char*)battery+0x10):0;return progress_ready&&id>=1&&id<=3&&
    local_items::battery(id)==battery&&local_progress::stock_amount(progress,10,id)>0;}
static void (*button_original)(void*,void*,bool,void*);
static void button(void* self,void* battery,bool enabled,void* method){button_original(self,battery,enabled&&(!offline_network||owned(battery)),method);}
static void (*consume_original)(void*,void*,void*);
static void consume(void* self,void* battery,void* method){
    if(offline_network){void* own=static_object("com.geargames.aow.scenes.battle","ClientBattleContext","m_ownSide");
        if(!local_items::use_guards_installed||!own||*(void**)((char*)own+0x1c8)!=self||!owned(battery))return;}
    consume_original(self,battery,method);
}
static int16_t command(void* self,void* battle,void* method){
    if(offline_network&&battle==local_hero_battle::prepared_battle&&self&&*(int*)((char*)self+0x28)==local_hero_battle::prepared_side){
        if(!progress_ready||!local_items::use_guards_installed)return -1;
        return execute(self,battle,method,progress,local_faction::root());}
    return command_original(self,battle,method);
}
static void install(){
    const uint32_t b[]={0xf81d0ffe,0xa90157f6,0xa9024ff4,0xb000eab6};
    const uint32_t c[]={0xa9be57fe,0xa9014ff4,0x9000eab5,0xaa0103f3};
    const uint32_t n[]={0xd101c3ff,0xa9026ffe,0xa90367fa,0xa9045ff8};
    button_original=(decltype(button_original))install_hook(0x805e100,b,(void*)button);
    consume_original=(decltype(consume_original))install_hook(0x805f584,c,(void*)consume);
    command_original=(decltype(command_original))install_hook(0x493652c,n,(void*)command);
    local_items::use_guards_installed=button_original&&consume_original&&command_original&&local_hero_battle::add_original;
    local_career::extra_inventory_supported=local_items::supported;
    LOG("Local battery transaction installed=%d",local_items::use_guards_installed);
}
}
