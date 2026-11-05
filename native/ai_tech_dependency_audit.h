// Bounded original technology contract. Independently decode both factions;
// do not enter a battle, bind a player model, grant requirements or save.
#pragma once
namespace ai_tech_dependency_audit {
static void run(){
    if(access(DATA_ROOT "ai-tech-dependency-audit.request",F_OK))return;unlink(DATA_ROOT "ai-tech-dependency-audit.request");
    auto before=progress;bool ok=true;int tiers=0,requirements=0;
    FILE* f=fopen(DATA_ROOT "ai-tech-dependency-audit.jsonl","w");if(!f)return;chmod(DATA_ROOT "ai-tech-dependency-audit.jsonl",0644);
    void* c=find_class("com.geargames.aow.entities.battle","BattleSide");void* counter=c?class_field_from_name(c,"$Sh"):nullptr;
    ok=counter&&field_offset(counter)==0x350;
    for(int faction:{1,2}){
        local_research_runtime::Roots roots;char path[512];snprintf(path,sizeof(path),DATA_ROOT "offline-seeds/fmc%dm.ggs",faction);
        void* cfg=campaign_seeds::decode(path,roots),*battle=cfg?*(void**)((char*)cfg+0x10):nullptr;
        void* side=battle?local_dictionary_item(*(void**)((char*)battle+0x178),1):nullptr;
        uintptr_t handle=0;void* values=side?local_dictionary_values(*(void**)((char*)side+0xf8),find_class("com.geargames.aow.entities","BuildingType"),handle):nullptr;
        bool valid=values&&handle&&array_length(values)<=64;if(!valid){ok=false;continue;}
        for(size_t j=0;j<array_length(values);++j){
            void* type=((void**)((char*)values+32))[j],*levels=type?*(void**)((char*)type+0x18):nullptr;
            int n=levels?*(int*)((char*)levels+0x18):0;if(n<1||n>16){ok=false;continue;}
            int type_id=*(int*)((char*)type+0x10);
            for(int tier=0;tier<n;++tier){
                void* level=list_item(levels,tier),*ex=nullptr;if(!level){ok=false;continue;}
                invoke_named(level,"initContribs",0,nullptr,&ex);void* list=*(void**)((char*)level+0x98);
                int count=list?*(int*)((char*)list+0x18):0;
                bool pass=!ex&&list&&count>=0&&count<=16;ok=ok&&pass;++tiers;
                fprintf(f,"{\"faction\":%d,\"type\":%d,\"tier\":%d,\"level_id\":%d,\"price\":%d,\"pass\":%s,\"requirements\":[",faction,type_id,tier,*(int*)((char*)level+0x10),*(int*)((char*)level+0x6c),pass?"true":"false");
                for(int k=0;pass&&k<count;++k){void* item=list_item(list,k);if(!item){ok=false;break;}int id=*(int*)((char*)item+0x10),quantity=*(int8_t*)((char*)item+0x14);
                    if(id<0||quantity<1)ok=false;fprintf(f,"%s{\"id\":%d,\"index\":%d,\"count\":%d}",k?",":"",id,id%1000%100,quantity);++requirements;}
                fputs("]}\n",f);
            }
        }
        gchandle_free(handle);
    }
    bool unchanged=!memcmp(&before,&progress,sizeof(progress));
    fprintf(f,"{\"summary\":true,\"pass\":%s,\"tiers\":%d,\"requirements\":%d,\"native_counter_offset\":848,\"player_state_unchanged\":%s,\"player_save_writes\":0,\"battle_started\":false}\n",ok&&unchanged?"true":"false",tiers,requirements,unchanged?"true":"false");fclose(f);
}
}
