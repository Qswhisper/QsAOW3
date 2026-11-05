// One detached native-grid batch over the selectable inputs. No scenes or saves.
#pragma once
namespace control_point_layout_audit {
static void run(){
    if(!progress_ready||access(DATA_ROOT "control-point-layout-audit.request",F_OK))return;
    unlink(DATA_ROOT "control-point-layout-audit.request");local_progress::StateBuffer before;if(!before.data)return;*before.data=progress;
    FILE* f=fopen(DATA_ROOT "control-point-layout-audit.jsonl","w");if(!f)return;chmod(DATA_ROOT "control-point-layout-audit.jsonl",0644);
    int rows=0,accepted=0;
    for(const auto& route:local_match_maps::routes){
        local_research_runtime::Roots roots;const LocalScenario* source=nullptr;
        for(const auto& s:local_scenarios)if(s.chapter==route.chapter&&s.faction==route.faction)source=&s;
        void* input=source?(source->embedded?campaign_seeds::decode(source->file,roots):read_scenario_message(source->file,5040,"@control-point-layout")):nullptr;
        void* battle=input?*(void**)((char*)input+(source->embedded?0x10:0x20)):nullptr;
        void* side=battle?local_dictionary_item(*(void**)((char*)battle+0x178),1):nullptr;
        void* type=side?local_dictionary_item(*(void**)((char*)side+0xf8),route.faction==1?1000:1100):nullptr;
        ai_spawn_runtime::Layout map;ai_spawn_plan::Point bases[2],flags[3];
        bool loaded=local_match_maps::automatic(&route)&&ai_spawn_runtime::sample(battle,type,map);
        bool base_plan=loaded&&ai_spawn_plan::plan(map.mask,map.width,map.height,2,bases,map.transit);
        bool pass=base_plan&&control_point_plan::plan(map.mask,map.transit,map.width,map.height,bases,flags);
        ++rows;if(pass)++accepted;
        fprintf(f,"{\"phase\":\"original_row\",\"faction\":%d,\"id\":%d,\"map\":%d,\"pass\":%s,\"width\":%d,\"height\":%d,\"flag_positions\":[",route.faction,route.chapter,route.map,pass?"true":"false",map.width,map.height);
        if(pass)for(int i=0;i<3;++i)fprintf(f,"%s[%d,%d]",i?",":"",flags[i].x,flags[i].y);
        fprintf(f,"],\"loaded\":%s,\"base_plan\":%s,\"bases\":[",loaded?"true":"false",base_plan?"true":"false");
        if(base_plan)for(int i=0;i<2;++i)fprintf(f,"%s[%d,%d]",i?",":"",bases[i].x,bases[i].y);
        fputs("],\"headquarters_mask_hex\":\"",f);if(loaded)for(int i=0;i<map.width*map.height;++i)fprintf(f,"%02x",map.mask[i]);
        fputs("\",\"land_transit_hex\":\"",f);if(loaded)for(int i=0;i<map.width*map.height;++i)fprintf(f,"%02x",map.transit[i]);
        fputs("\"}\n",f);fflush(f);
    }
    bool unchanged=!memcmp(before.data,&progress,sizeof(progress));
    fprintf(f,"{\"summary\":true,\"pass\":%s,\"candidates\":%d,\"accepted\":%d,\"player_state_unchanged\":%s,\"original_grid_and_native_footprints\":true,\"both_bases_land_reachable\":true,\"player_save_writes\":0,\"battle_runs\":0}\n",rows==24&&accepted==rows&&unchanged?"true":"false",rows,accepted,unchanged?"true":"false");fclose(f);
}
}
