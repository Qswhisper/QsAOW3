// Detached original input audit: no scene, player run, save, or grants.
#pragma once
#include "ai_spawn_runtime.h"
namespace ai_spawn_audit {
static void run(){
    if(!progress_ready||access(DATA_ROOT "ai-spawn-audit.request",F_OK))return;
    unlink(DATA_ROOT "ai-spawn-audit.request");auto before=progress;
    FILE* f=fopen(DATA_ROOT "ai-spawn-audit.jsonl","w");if(!f)return;chmod(DATA_ROOT "ai-spawn-audit.jsonl",0644);
    int rows=0,sampled=0,layouts=0;
    for(const auto& route:local_scenarios){
        local_research_runtime::Roots roots;
        void* input=route.embedded?campaign_seeds::decode(route.file,roots):read_scenario_message(route.file,5040,"@ai-spawn-audit");
        void* battle=input?*(void**)((char*)input+(route.embedded?0x10:0x20)):nullptr;
        void* side=battle?local_dictionary_item(*(void**)((char*)battle+0x178),1):nullptr;
        int hq=route.faction==1?1000:1100;
        void* type=side?local_dictionary_item(*(void**)((char*)side+0xf8),hq):nullptr;
        ai_spawn_runtime::Layout map;bool loaded=ai_spawn_runtime::sample(battle,type,map);++rows;if(loaded)++sampled;
        fprintf(f,"{\"record\":\"input\",\"grid_order\":\"rows-y-x\",\"chapter\":%d,\"map\":%d,\"faction\":%d,\"width\":%d,\"height\":%d,\"native_valid\":%d,\"valid\":%d,\"terrain\":[%d,%d,%d,%d,%d],\"build_ground\":%d,\"sampled\":%s,\"layouts\":[",route.chapter,route.map,route.faction,map.width,map.height,map.native_valid,map.valid,map.terrain[0],map.terrain[1],map.terrain[2],map.terrain[3],map.terrain[4],map.build_ground,loaded?"true":"false");
        for(int count:{2,4,6}){ai_spawn_plan::Point points[6];bool pass=loaded&&ai_spawn_plan::plan(map.mask,map.width,map.height,count,points,map.transit);if(pass)++layouts;
            fprintf(f,"%s{\"seats\":%d,\"planned\":%s,\"points\":[",count==2?"":",",count,pass?"true":"false");
            if(pass)for(int i=0;i<count;++i)fprintf(f,"%s[%d,%d]",i?",":"",points[i].x,points[i].y);fputs("]}",f);}
        fputs("]}\n",f);fflush(f);
    }
    bool unchanged=!memcmp(&before,&progress,sizeof(before));
    fprintf(f,"{\"summary\":true,\"complete\":true,\"inputs\":%d,\"sampled\":%d,\"planned_layouts\":%d,\"player_unchanged\":%s,\"scene_runs\":0,\"save_writes\":0}\n",rows,sampled,layouts,unchanged?"true":"false");fclose(f);
}
}
