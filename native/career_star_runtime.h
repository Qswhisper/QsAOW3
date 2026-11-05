#pragma once
#include "career_star_rewards.h"
namespace local_career_stars {
static bool from_rank(void* rank,int& amount,int& floor,int& next){
    if(!rank)return false;int id=*(int*)((char*)rank+0x10);floor=*(int*)((char*)rank+0x14);next=INT_MAX;
    void* ranks=local_career::list("SHARED","profileRanks");int count=ranks?*(int*)((char*)ranks+0x18):0;if(id<1||id>127||count<1||count>128||local_career::lookup("SHARED","profileRanks",id)!=rank)return false;
    bool highest=true;for(int i=0;i<count;++i){void* r=list_item(ranks,i);if(!r)return false;int value=*(int*)((char*)r+0x14);if(value>floor&&value<next){next=value;highest=false;}}
    return career_star_rewards::budget(floor,next,highest,amount);
}
static bool freeze(local_match_store::State& state,bool formal){
    if(!formal)return local_match_store::freeze_career_stars(state,false,0,0)&&local_match_store::freeze_victory_credits(state,0);
    void* user=saved("UserConfiguration"),*profile=user?*(void**)((char*)user+0x28):nullptr,*field=profile?class_field_from_name(object_class(profile),"rank"):nullptr;
    void* rank=field?*(void**)((char*)profile+field_offset(field)):nullptr;int amount=0,floor=0,next=0;if(!from_rank(rank,amount,floor,next)||progress.stars<floor)return false;
    int level=0;bool ok=local_reward_skills::pvp_level(progress,level)&&local_match_store::freeze_career_stars(state,true,*(int*)((char*)rank+0x10),amount)&&local_match_store::freeze_victory_credits(state,level);
    LOG("Local approved career stars frozen=%d run=%llu rank=%d floor=%d next=%d victory_budget=%d",ok,(unsigned long long)state.next,rank?*(int*)((char*)rank+0x10):0,floor,next,amount);return ok;
}
}
