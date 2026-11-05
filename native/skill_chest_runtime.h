// Original per-side chest contracts: passive dictionary key 4 is integer %;
// BonusChestAct.$Rb uses side.container_capture_time unless the script overrides it.
#pragma once
namespace skill_chest {
using Roots=local_research_runtime::Roots;
static void* get(void* p,int o){return p?*(void**)((char*)p+o):nullptr;}
// Native percentage keys use Effect/10; key20 adds native CP units.
// Key29 is consumed by UnitAct.$NH at real hero death; key35 by SideAct.$zf.
// Key34 is integer income percentage: original GetBoostedValueInt(100,skill,
// false)-100 truncates the accumulated Effect15, not each learned level.
static bool native_card(int card){return card==6||card==7||card==9||card==10||card==20||card==21||card==29||card==34||card==35;}
static bool initial_resources(void* side){
    // BattleAct.$I applies key21 to the initial budget only on native PvP.
    // Local training retains a PVE container for original geometry/scripts;
    // its one-time seat seeding must perform that same integer formula.
    void* map=get(side,0x3e8);if(!side||!map)return false;
    int key=21,percent=0;void* ex=nullptr;void* args[]={&key,&percent};
    void* found=invoke_named(map,"TryGetValue",2,args,&ex);
    if(ex||!found||!*(bool*)((char*)found+16)||percent<0)return false;
    int base=*(int*)((char*)side+0x16c);int64_t amount=int64_t(base)*(100+int64_t(percent))/100;
    if(base<0||amount>INT_MAX)return false;*(int*)((char*)side+0x16c)=(int)amount;return true;
}
static bool prepare(void* side,int resources,int capture,const int* bonuses=nullptr){
    if(!side||resources<0||resources%10||capture<0||capture%10||capture>=1000)return false;
    void* old=get(side,0x3e8),*fraction=get(side,0xd8);if(!old||!fraction)return false;
    void* base=local_faction::fraction(*(int*)((char*)fraction+0x10));
    int time=base?*(int16_t*)((char*)base+0xec):0;
    if(time<=0||time>INT16_MAX)return false;
    Roots roots;void* next=object_new(object_class(old));if(!roots.keep(next))return false;
    void* ex=nullptr;invoke_named(next,".ctor",0,nullptr,&ex);if(ex)return false;
    void* keys=invoke_named(old,"get_Keys",0,nullptr,&ex);if(ex||!roots.keep(keys))return false;
    void* boxed=invoke_named(keys,"get_Count",0,nullptr,&ex);
    int n=boxed&&!ex?*(int*)((char*)boxed+16):-1;if(n<0||n>128)return false;
    if(n){void* array=array_new(find_class("System","Int32"),n);if(!roots.keep(array))return false;
        int zero=0;void* args[]={array,&zero};invoke_named(keys,"CopyTo",2,args,&ex);if(ex)return false;
        for(int i=0;i<n;++i){int key=((int*)((char*)array+32))[i];if(!local_career::skill_card(key))return false;
            int zero=0;void* b[]={&key,&zero};invoke_named(next,"Add",2,b,&ex);if(ex)return false;}}
    for(int key:{4,5}){int percent=(key==4?resources:capture)/10;void* args[]={&key,&percent};invoke_named(next,"set_Item",2,args,&ex);if(ex)return false;}
    for(int key:{6,7,9,10,20,21,29,34,35}){int raw=bonuses?bonuses[key]:0;
        if(raw<0||(key!=34&&raw%10)||((key==6||key==9||key==29)&&raw>=1000))return false;
        int percent=raw/10;void* args[]={&key,&percent};invoke_named(next,"set_Item",2,args,&ex);if(ex)return false;}
    // The native side initializer consumes key 5 once. Keep the pre-init time
    // canonical, and publish only local passive coefficients. The eight older
    // mechanisms are already projected on prototypes, so their map values stay
    // neutral; captured account skill coefficients must never run a second time.
    local_skill::ref(side,"$IH",next);*(int16_t*)((char*)side+0x218)=time;
    return true;
}
}
