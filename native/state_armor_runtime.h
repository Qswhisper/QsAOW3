// Original UnitArmorStat first/second order and GetComment locale contract.
// Bind the complete original family/state identity, never arbitrary list order.
#pragma once
struct StateArmorRoute {int faction,owner,kind,group,states[2];const char* model;};
static const StateArmorRoute state_armor_routes[]={
    {1,16,25,1,{18,19},"f1/f1_veh_torrent"},
    {1,17,22,2,{20,21},"f1/f1_veh_shield"},
    {1,71,61,3,{202,203},"f1/f1_hero_seraphim"},
    {2,115,20,1,{114,115},"f2/f2_veh_jaguar"},
    {2,116,25,1,{118,119},"f2/f2_veh_mammoth"},
    {2,117,23,2,{120,121},"f2/f2_veh_chameleon"},
    {2,171,71,4,{301,302},"f2/f2_hero_codomash"},
};
static bool same_state_armor_identity(void* a,void* b){
    if(!a||!b||strcmp(class_name(object_class(a)),"UnitStateType")||strcmp(class_name(object_class(b)),"UnitStateType"))return false;
    if(*(int*)((char*)a+0x80)!=*(int*)((char*)b+0x80)||*(int8_t*)((char*)a+0x20)!=*(int8_t*)((char*)b+0x20)||
       *(int8_t*)((char*)a+0x26)!=*(int8_t*)((char*)b+0x26)||*(int8_t*)((char*)a+0xa0)!=*(int8_t*)((char*)b+0xa0))return false;
    void* x=*(void**)((char*)a+0x18),*y=*(void**)((char*)b+0x18);
    int nx=x?*(int*)((char*)x+0x18):0,ny=y?*(int*)((char*)y+0x18):0;if(nx<0||nx>32||nx!=ny)return false;
    for(int i=0;i<nx;++i){void* u=list_item(x,i),*v=list_item(y,i);
        if(!u||!v||memcmp((char*)u+0x20,(char*)v+0x20,6))return false;}
    return true;
}
static bool state_armor_deltas(void* type,const Effect& e,int (&deltas)[2]){
    deltas[0]=deltas[1]=0;if(!e.state_armor.group)return !e.state_armor.value[0]&&!e.state_armor.value[1];
    if(!type||!e.owner||!identity(type,e.owner))return false;
    void* faction=*(void**)((char*)e.owner+0x20);char model[256];
    if(!faction||!local_research_runtime::ascii(*(void**)((char*)e.owner+0x60),model,sizeof(model)))return false;
    const StateArmorRoute* route=nullptr;
    for(const auto& r:state_armor_routes)if(r.faction==*(int*)((char*)faction+0x10)&&r.owner==*(int*)((char*)e.owner+0x10)&&
        r.kind==*(int8_t*)((char*)type+0x31)&&r.group==e.state_armor.group&&!strcmp(r.model,model))route=&r;
    void* states=*(void**)((char*)type+0x18),*original=*(void**)((char*)e.owner+0x18);
    if(!route||!states||!original||*(int*)((char*)states+0x18)!=2||*(int*)((char*)original+0x18)!=2)return false;
    void* peers[2]={};
    for(int i=0;i<2;++i){void* s=list_item(original,i);if(!s)return false;int id=*(int*)((char*)s+0x10);
        int index=id==route->states[0]?0:id==route->states[1]?1:-1;if(index<0||peers[index])return false;peers[index]=s;}
    if(same_state_armor_identity(peers[0],peers[1]))return false;
    unsigned seen=0;
    for(int i=0;i<2;++i){void* state=list_item(states,i);if(!state||strcmp(class_name(object_class(state)),"UnitStateType"))return false;
        // Battle prototype IDs are seat-scoped. Match the full immutable mode
        // fingerprint and named weapons to the canonical ID-bound peers.
        int index=-1;for(int j=0;j<2;++j)if(same_state_armor_identity(state,peers[j])){if(index>=0)return false;index=j;}
        if(index<0||(seen&(1u<<index))||e.state_armor.value[index]<=0)return false;
        seen|=1u<<index;deltas[i]=e.state_armor.value[index];}
    return seen==3;
}
