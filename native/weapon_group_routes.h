// Explicit local I/II identity bindings reviewed against retained named weapons.
// Matching uses Name+Type across every mode; no runtime array-position fallback.
#pragma once
// Included inside generic_unit_research, after WeaponKey/weapon_matches.
struct GroupRoute {int faction,owner,name[2],type[2];const char* model;};
static const GroupRoute group_routes[]={
    {1,0,{2720,2721},{10,20},"f1/f1_inf_light"},
    {1,10,{2724,2725},{10,40},"f1/f1_veh_fortress"},
    {1,15,{2727,2728},{21,30},"f1/f1_veh_zeus"},
    {1,26,{2735,2736},{21,34},"f1/f1_nav_light"},
    {1,27,{2737,2738},{23,30},"f1/f1_nav_heavy"},
    {2,100,{2750,2751},{10,20},"f2/f2_inf_light"},
    {2,112,{2757,2758},{30,10},"f2/f2_veh_porcupine"},
    {2,120,{2760,2761},{30,10},"f2/f2_avia_helicopter"},
    {2,121,{2762,2763},{30,10},"f2/f2_avia_fighter"},
    {2,126,{2766,2767},{21,30},"f2/f2_nav_alligator"},
};
static bool grouped_weapon_keys(void* owner,WeaponKey (&keys)[2]){
    void* faction=owner?*(void**)((char*)owner+0x20):nullptr;char model[256];
    if(!faction||!local_research_runtime::ascii(*(void**)((char*)owner+0x60),model,sizeof(model)))return false;
    void* abilities=*(void**)((char*)owner+0x98);if(abilities&&*(int*)((char*)abilities+0x18))return false;
    const GroupRoute* route=nullptr;
    for(const auto& r:group_routes)if(r.faction==*(int*)((char*)faction+0x10)&&r.owner==*(int*)((char*)owner+0x10)&&!strcmp(r.model,model))route=&r;
    if(!route)return false;
    WeaponKey next[2]={{route->name[0],route->type[0],0},{route->name[1],route->type[1],0}};
    void* states=*(void**)((char*)owner+0x18);int n=states?*(int*)((char*)states+0x18):0;if(n<1||n>32)return false;
    for(int i=0;i<n;++i){void* state=list_item(states,i),*weapons=state?*(void**)((char*)state+0x18):nullptr;
        int count=weapons?*(int*)((char*)weapons+0x18):0;if(!state||count<0||count>32)return false;
        for(int j=0;j<count;++j){void* w=list_item(weapons,j);if(!w)return false;
            int group=weapon_matches(w,next[0])?0:weapon_matches(w,next[1])?1:-1;
            if(group<0)return false;++next[group].refs;}}
    if(!next[0].refs||!next[1].refs)return false;keys[0]=next[0];keys[1]=next[1];return true;
}
