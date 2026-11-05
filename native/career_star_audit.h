#pragma once
namespace career_star_audit {
static void run(){
    if(!progress_ready||access(DATA_ROOT "career-star-audit.request",F_OK))return;unlink(DATA_ROOT "career-star-audit.request");
    FILE* f=fopen(DATA_ROOT "career-star-audit.jsonl","w");if(!f)return;chmod(DATA_ROOT "career-star-audit.jsonl",0644);local_progress::StateBuffer backup;if(!backup.data){fclose(f);return;}*backup.data=progress;
    local_faction::Roots roots;auto formatter=reinterpret_cast<void*(*)(int,int,void*,void*)>(image_base+0x7f258c0);void* label=formatter(6,1,nullptr,nullptr);
    bool premium=local_career::obsolete_premium_gifts&&roots.keep(label)&&local_items::text_equal(label,"单机无需会员");
    bool ok=premium;int ranks_checked=0,stones_checked=0;void* ranks=local_career::list("SHARED","profileRanks");
    for(int i=0;ranks&&i<*(int*)((char*)ranks+0x18);++i){void* rank=list_item(ranks,i);int amount=0,floor=0,next=0;bool pass=local_career_stars::from_rank(rank,amount,floor,next);local_match_store::State game;
        pass=pass&&local_match_store::begin_pvp(game,true)&&local_match_store::freeze_career_stars(game,true,*(int*)((char*)rank+0x10),amount)&&local_match_store::settle(game,1,local_match_store::Victory,5000,1,1,nullptr,nullptr);game.checksum=local_match_store::hash(game);
        auto wallet=local_progress::seed(0,0,0,floor);bool changed=false;pass=pass&&warpoint_store::reconcile(wallet,game,changed)&&changed&&wallet.stars==floor+amount;ok=ok&&pass;++ranks_checked;
        int league_id=*(int*)((char*)rank+0x88);bool original_league=local_career::league(league_id)!=nullptr;ok=ok&&original_league;
        fprintf(f,"{\"phase\":\"original_rank\",\"id\":%d,\"floor\":%d,\"next\":%d,\"budget\":%d,\"league\":%d,\"original_league\":%s,\"pass\":%s}\n",*(int*)((char*)rank+0x10),floor,next,amount,league_id,original_league?"true":"false",pass&&original_league?"true":"false");}
    for(int faction:{1,2}){local_progress::StateBuffer storage;auto& state=*storage.data;state=local_progress::seed(0,0,0,360);void* stones=local_career::list(local_career::key(faction),"careerStones");
        for(int i=0;stones&&i<*(int*)((char*)stones+0x18);++i){void* stone=list_item(stones,i);int stars=*(int*)((char*)stone+0x20),id=*(int*)((char*)stone+0x10);if(stars>360)continue;
            local_progress::CareerReward rewards[128];int count=0;bool pass=local_career::resolve_stone(saved("UserConfiguration"),state,faction,stone,rewards,128,count)&&local_progress::claim_career(state,id,stars,rewards,count);ok=ok&&pass;++stones_checked;
            fprintf(f,"{\"phase\":\"private_milestone_to_360\",\"faction\":%d,\"id\":%d,\"stars\":%d,\"pass\":%s}\n",faction,id,stars,pass?"true":"false");}}
    local_match_store::State actual;bool frozen=local_match_store::begin_pvp(actual,true)&&local_career_stars::freeze(actual,true);ok=ok&&frozen;
    bool isolated=!memcmp(backup.data,&progress,sizeof(progress));fprintf(f,"{\"summary\":true,\"pass\":%s,\"original_ranks\":%d,\"milestones_to_360\":%d,\"premium_gift_formatter\":%s,\"actual_start_rank\":%d,\"actual_frozen_budget\":%d,\"player_state_unchanged\":%s,\"player_save_writes\":0,\"player_star_grants\":0,\"battle_runs\":0}\n",ok&&isolated?"true":"false",ranks_checked,stones_checked,premium?"true":"false",local_match_store::start_rank(actual),local_match_store::star_budget(actual),isolated?"true":"false");fclose(f);
}
}
