// Explicit no-save mechanism samples. Coefficient fixtures are not earned
// player skills: unsupported ancestors and original rank gates remain intact.
#pragma once
namespace skill_economy_audit {
using Roots=local_research_runtime::Roots;
static bool pending;static int initial_gold;static local_progress::State before;
static void* get(void* p,int o){return p?*(void**)((char*)p+o):nullptr;}
static const char* path=DATA_ROOT "skill-economy-audit.jsonl";
static void log(const char* format,...){FILE* f=fopen(path,"a");if(!f)return;va_list a;va_start(a,format);vfprintf(f,format,a);va_end(a);fputc('\n',f);fclose(f);chmod(path,0644);}
static bool prepare(void* side){
    if(access(DATA_ROOT "skill-economy-audit.request",F_OK))return true;
    FILE* f=fopen(path,"w");if(f)fclose(f);before=progress;initial_gold=*(int*)((char*)side+0x16c);
    int bonus[100]={};for(int card:{9,20,21}){void* c=local_career::skill_card(card);if(!c)return false;bonus[card]=*(int*)((char*)c+0x38);}
    pending=skill_chest::prepare(side,0,0,bonus);
    log("{\"phase\":\"private_coefficient_input\",\"pass\":%s,\"initial_gold\":%d,\"raw_effects\":[%d,%d,%d],\"earned_player_skills\":false,\"player_save_writes\":0}",pending?"true":"false",initial_gold,bonus[9],bonus[20],bonus[21]);return pending;
}
static void run(void* battle){
    if(!pending)return;pending=false;unlink(DATA_ROOT "skill-economy-audit.request");
    Roots roots;void* side=local_dictionary_item(get(battle,0x178),1);
    int actual=side?*(int*)((char*)side+0x16c):-1;
    bool init=side&&initial_gold>0&&actual==initial_gold&&*((bool*)battle+0xdc);
    log("{\"phase\":\"native_pve_start_unchanged\",\"pass\":%s,\"before\":%d,\"after\":%d,\"pve_container\":true,\"training_adapter_required\":true}",init?"true":"false",initial_gold,actual);
    bool cp=true,deploy=true,resources=true,source=true;int cp_cases=0,deploy_cases=0,base_cp=-1;
    void* hq=nullptr,*a=get(side,0x148);
    for(size_t i=0;a&&i<array_length(a);++i){void* b=((void**)((char*)a+32))[i];if(b&&get(b,0x38)){hq=b;break;}}
    void* original_map=get(side,0x3e8);int original_gold=actual,original_cp=*(int*)((char*)side+0x34c);
    if(!roots.keep(original_map)){log("{\"failure\":\"retain_original_map\"}");return;}
    auto recalc=[&](){void* ex=nullptr;void* args[]={battle,side};runtime_invoke(class_method(find_class("com.geargames.aow.entities.battle","SideAct"),"$Hf",2),nullptr,args,&ex);return !ex;};
    for(int level=0;side&&level<=11;++level){Roots one;void* copy=local_research_runtime::clone(side,one),*building=local_research_runtime::clone(hq,one);
        int bonus[100]={};bonus[9]=40*level;bonus[20]=20*level;bonus[21]=20*level;
        bool prepared=copy&&building&&skill_chest::prepare(copy,0,0,bonus);
        if(!prepared){cp=deploy=false;break;}
        // Hf recomputes actual capacity from the fixture's real HQ, then adds
        // key20 in native CP units. gf is the distinct income/card34 consumer.
        local_skill::ref(side,"$IH",get(copy,0x3e8));bool recalculated=recalc();
        int result=*(int*)((char*)side+0x34c);if(!level)base_cp=result;
        int expected=base_cp+2*level;bool pass=recalculated&&result==expected;cp=cp&&pass;++cp_cases;
        local_skill::ref(building,"side",copy);int base=10003;*(int*)((char*)copy+0x1f4)=base;
        *(bool*)((char*)building+0x138)=false;
        int ordinary=reinterpret_cast<int(*)(void*,void*,int,void*)>(image_base+0x46e4374)(battle,building,base,nullptr);
        bool ordinary_pass=ordinary==base;
        *(bool*)((char*)building+0x138)=true;
        int time=reinterpret_cast<int(*)(void*,void*,int,void*)>(image_base+0x46e4374)(battle,building,base,nullptr);
        int expected_time=base*(100-4*level)/100;bool time_pass=time==expected_time&&ordinary_pass;deploy=deploy&&time_pass;++deploy_cases;
        *(int*)((char*)copy+0x16c)=1201;int expected_gold=1201*(100+2*level)/100;
        bool resource_pass=skill_chest::initial_resources(copy)&&*(int*)((char*)copy+0x16c)==expected_gold;resources=resources&&resource_pass;
        log("{\"phase\":\"native_getters\",\"level\":%d,\"cp\":%d,\"expected_cp\":%d,\"deploy\":%d,\"expected_deploy\":%d,\"cp_pass\":%s,\"deploy_pass\":%s,\"non_deploy_preserved\":%s,\"training_resource_adapter_pass\":%s,\"initial_resources\":%d}",level,result,expected,time,expected_time,pass?"true":"false",time_pass?"true":"false",ordinary_pass?"true":"false",resource_pass?"true":"false",*(int*)((char*)copy+0x16c));
        if(!skill_chest::prepare(copy,0,0,bonus))source=false;
    }
    local_skill::ref(side,"$IH",original_map);source=source&&recalc()&&get(side,0x3e8)==original_map&&*(int*)((char*)side+0x16c)==original_gold&&*(int*)((char*)side+0x34c)==original_cp;
    auto fixture=local_progress::seed(1000000,0,0,10000);local_progress::State next;
    bool gates=local_progress::grant_stock(fixture,8,21,1000)&&!local_skill::plan(fixture,143,next)&&
        local_progress::grant_stock(fixture,8,9,1000)&&!local_skill::plan(fixture,214,next);
    bool unchanged=!memcmp(&before,&progress,sizeof(before));
    log("{\"summary\":true,\"pass\":%s,\"cp_levels\":%d,\"deploy_levels\":%d,\"resource_adapter_levels\":%d,\"original_ancestor_and_stage_gates\":%s,\"source_side_unchanged\":%s,\"player_state_unchanged\":%s,\"player_save_writes\":0}",init&&cp&&deploy&&resources&&source&&gates&&unchanged?"true":"false",cp_cases,deploy_cases,resources?cp_cases:0,gates?"true":"false",source?"true":"false",unchanged?"true":"false");
}
}
