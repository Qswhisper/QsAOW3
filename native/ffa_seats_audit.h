// Detached original allocations: all newly added seats/alliances are independent.
#pragma once
namespace ffa_seats_audit {
using Roots=local_research_runtime::Roots;
static void* input(int faction,int chapter,Roots& roots){
    for(const auto& source:local_scenarios)if(source.faction==faction&&source.chapter==chapter){
        void* value=source.embedded?campaign_seeds::decode(source.file,roots):read_scenario_message(source.file,5040,"@ffa-seats");
        return value?*(void**)((char*)value+(source.embedded?0x10:0x20)):nullptr;}
    return nullptr;
}
static void run(){
    if(!progress_ready||access(DATA_ROOT "ffa-seats-audit.request",F_OK))return;unlink(DATA_ROOT "ffa-seats-audit.request");
    local_progress::StateBuffer before;if(!before.data)return;*before.data=progress;
    FILE* f=fopen(DATA_ROOT "ffa-seats-audit.jsonl","w");if(!f)return;chmod(DATA_ROOT "ffa-seats-audit.jsonl",0644);
    int accepted=0;
    for(int faction:{1,2}){Roots roots;int donor_faction=3-faction;int donor_chapter=donor_faction==1?0:200;
        void* battle=input(faction,faction==1?0:201,roots),*donor=input(donor_faction,donor_chapter,roots);
        local_duel::control_points::private_mode=10;
        bool pass=battle&&donor&&local_duel::prepare(battle,donor);
        for(int id=3;pass&&id<=6;++id){void* independent=input(donor_faction,donor_chapter,roots);pass=independent&&local_duel::extend_free_for_all(battle,independent);}
        void* alliances=battle?*(void**)((char*)battle+0x170):nullptr;
        pass=pass&&alliances&&array_length(alliances)==6&&local_duel::seat_count()==6;
        for(int id=1;pass&&id<=6;++id){void* s=local_duel::side(battle,id),*c=local_duel::side_config(battle,id);
            void* alliance=((void**)((char*)alliances+32))[id-1],*members=*(void**)((char*)alliance+0x18);
            pass=s&&c&&*(int*)((char*)s+0x10)==id&&*(int*)((char*)c+0x14)==id&&
                *(int*)((char*)alliance+0x10)==id&&members&&*(int*)((char*)members+0x18)==1&&list_item(members,0)==s;
        }
        if(pass)++accepted;fprintf(f,"{\"phase\":\"original_row\",\"faction\":%d,\"id\":%d,\"pass\":%s,\"independent_seats\":6,\"independent_alliances\":6,\"mixed_factions\":true}\n",faction,faction,pass?"true":"false");
    }
    local_duel::control_points::private_mode=0;local_duel::control_points::reset(1);local_duel::configured_seats=2;
    bool unchanged=!memcmp(before.data,&progress,sizeof(progress));
    fprintf(f,"{\"summary\":true,\"pass\":%s,\"candidates\":2,\"accepted\":%d,\"player_state_unchanged\":%s,\"player_save_writes\":0,\"battle_runs\":0}\n",accepted==2&&unchanged?"true":"false",accepted,unchanged?"true":"false");fclose(f);
}
}
