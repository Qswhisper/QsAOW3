// Relevant pre-initialization battle contract for newly acquired heroes.
// Uses independently decoded original inputs; never starts a live battle.
#pragma once
namespace hero_battle_projection_audit {
using local_research_runtime::Roots;
static uint64_t type_hash(void* dictionary){
    uintptr_t handle=0;void* values=local_dictionary_values(dictionary,find_class("com.geargames.aow.entities","UnitType"),handle);
    if(!values||!handle)return 0;uint64_t hash=1469598103934665603ULL;
    for(size_t i=0;i<array_length(values);++i){void* row=((void**)((char*)values+32))[i];if(!row){gchandle_free(handle);return 0;}
        for(int j=0x10;j<0xd8;++j)hash=(hash^((uint8_t*)row)[j])*1099511628211ULL;}
    gchandle_free(handle);return hash;
}
static int opened(void* side,int faction){
    uintptr_t handle=0;void* values=local_dictionary_values(local_hero::get(side,0xf0),find_class("com.geargames.aow.entities","UnitType"),handle);
    if(!values||!handle)return -1;int count=0;
    for(size_t i=0;i<array_length(values);++i){void* row=((void**)((char*)values+32))[i];void* fraction=local_hero::get(row,0x20);
        if(!fraction||*(int*)((char*)fraction+0x10)!=faction)continue;
        if(*(int8_t*)((char*)row+0x31)>=60&&*(int8_t*)((char*)row+0x31)<=76&&*(int*)((char*)row+0xa0)==1)++count;}
    gchandle_free(handle);return count;
}
static void run(){
    if(!progress_ready||access(DATA_ROOT "hero-battle-projection.request",F_OK))return;
    unlink(DATA_ROOT "hero-battle-projection.request");auto before=progress;bool ok=true;int factions=0;
    FILE* file=fopen(DATA_ROOT "hero-battle-projection.jsonl","w");if(!file)return;chmod(DATA_ROOT "hero-battle-projection.jsonl",0644);
    for(int faction:{1,2}){
        Roots roots;char path[512];snprintf(path,sizeof(path),DATA_ROOT "offline-seeds/fmc%dm.ggs",faction);
        void* config=campaign_seeds::decode(path,roots);void* battle=local_hero::get(config,0x10);
        void* own=battle?local_dictionary_item(local_hero::get(battle,0x178),1):nullptr;
        void* enemy=battle?local_dictionary_item(local_hero::get(battle,0x178),2):nullptr;
        void* baseline=local_hero::get(own,0xf0),*enemy_types=local_hero::get(enemy,0xf0);
        uint64_t own_hash=type_hash(baseline),enemy_hash=type_hash(enemy_types);generic_unit_research::Plan plan;
        void* cards=local_career::list("SHARED","heroCards");int count=cards?*(int*)((char*)cards+0x18):0;
        bool pass=own&&enemy&&own_hash&&enemy_hash;
        for(int i=0;i<count&&pass;++i){void* card=list_item(cards,i);int id=*(int*)((char*)card+0x10);if(!local_hero::card(id,faction))continue;
            generic_unit_research::Effect effect;pass=generic_unit_research::resolve(local_hero::first_level(id,faction),effect)&&plan.add(effect);}
        int matched=0;pass=pass&&plan.count==6&&generic_unit_research::apply(own,baseline,plan,matched)&&matched==6;
        int owned=pass?opened(own,faction):-1;uint64_t first=type_hash(local_hero::get(own,0xf0));int repeated_matched=0;
        bool repeat=pass&&generic_unit_research::apply(own,baseline,plan,repeated_matched)&&repeated_matched==6&&opened(own,faction)==owned;
        // Copies have different references; compare class/shape and source hashes,
        // not the pointer bytes of two independently allocated dictionaries.
        bool isolated=own_hash==type_hash(baseline)&&enemy_hash==type_hash(enemy_types)&&enemy_types==local_hero::get(enemy,0xf0);
        pass=pass&&owned==6&&first&&repeat&&isolated;ok=ok&&pass;++factions;
        fprintf(file,"{\"faction\":%d,\"pass\":%s,\"matched_hero_families\":%d,\"opened\":%d,\"repeat_from_baseline\":%s,\"original_and_enemy_unchanged\":%s}\n",faction,pass?"true":"false",matched,owned,repeat?"true":"false",isolated?"true":"false");
        LOG("Local hero battle projection faction=%d pass=%d matched=%d opened=%d isolated=%d",faction,pass,matched,owned,isolated);
    }
    bool isolated=!memcmp(&before,&progress,sizeof(progress));fprintf(file,"{\"summary\":true,\"pass\":%s,\"factions\":%d,\"player_state_unchanged\":%s,\"save_written\":false,\"battle_started\":false}\n",ok&&isolated?"true":"false",factions,isolated?"true":"false");fclose(file);
}
}
