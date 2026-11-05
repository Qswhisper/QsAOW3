#include "profile_statistics_policy.h"
#include <cassert>
#include <string>
#include <cstdio>
using namespace profile_statistics_policy;
int main(int argc,char** argv){
    assert(argc==2);local_progress::StateBuffer source,copy;assert(source.data&&copy.data);
    for(int faction:{1,2}){
        std::string root=std::string(argv[1])+"/f"+std::to_string(faction);
        // Self-contained synthetic careers; no retained player files are inputs.
        assert(!mkdir(root.c_str(),0700));
        *source.data=local_progress::seed(0,0,faction==1?65:169,faction==1?5:9);
        source.data->count=faction==1?2:4;const int xp1[]={28,37},xp2[]={28,39,50,52};
        for(unsigned i=0;i<source.data->count;++i)
            source.data->missions[i]={int(i)+(faction==1?0:200),1,0,i?2:3,faction==1?xp1[i]:xp2[i],0,0,0};
        assert(local_progress::write(root.c_str(),*source.data));
        local_match_store::State generated;
        for(int i=0;i<(faction==1?5:27);++i){
            assert(local_match_store::begin(generated));bool won=faction==2&&i==0;
            assert(local_match_store::settle(generated,generated.next,won?local_match_store::Victory:local_match_store::Defeat,100,1,won?1:2,nullptr,nullptr));
        }
        assert(local_match_store::write(root.c_str(),generated));
        assert(local_progress::load(root.c_str(),*source.data)==1);local_match_store::State game;assert(local_match_store::load(root.c_str(),game)==1);
        *copy.data=*source.data;Snapshot s;assert(project(*source.data,game,{},faction,s)&&!memcmp(source.data,copy.data,sizeof(*source.data)));
        assert(s.completed==(faction==1?2:4)&&s.three_star==1&&!s.three_star_hard&&s.campaign_stars==(faction==1?5:9)&&!s.pvp_stars);
        assert(s.campaign_xp==(faction==1?65:169)&&s.pvp_battles==(faction==1?5:27)&&s.pvp_wins==(faction==1?0:1));
        assert(s.total_ratio==(faction==1?0:3)&&s.current_known&&s.current_ratio==s.total_ratio);
        printf("PASS: synthetic faction%d growth/AI bytes -> completed%d/three-star%d/hard%d, campaignXP%d/stars%d, PvP%d/%d, rates%d/%d; read-only projection\n",faction,s.completed,s.three_star,s.three_star_hard,s.campaign_xp,s.campaign_stars,s.pvp_wins,s.pvp_battles,s.total_ratio,s.current_ratio);
    }
    *source.data=local_progress::seed(0,0,0,0);local_match_store::State current;Snapshot result;
    assert(project(*source.data,current,{},2,result)&&!result.total_ratio&&result.current_known&&result.current_ratio==50);
    std::vector<local_history_store::Record> records;std::vector<int> outcomes;
    for(int run=1;run<=80;++run){
        assert(local_match_store::begin(current));
        auto outcome=run%10==0?local_match_store::Interrupted:run%7==0?local_match_store::Draw:
            run%3==0?local_match_store::Victory:run%5==0?local_match_store::Surrender:local_match_store::Defeat;
        assert(local_match_store::settle(current,current.next,outcome,100,outcome==local_match_store::Surrender?6:1,outcome==local_match_store::Victory?1:2,nullptr,nullptr));
        current.checksum=local_match_store::hash(current);
        if(local_history_store::scored(current)){
            local_history_store::Record r;assert(local_history_store::make(2,current,1800000000-run*10,r));records.push_back(r);outcomes.push_back(outcome);
        }
    }
    assert(outcomes.size()==72);int wins=0;for(size_t i=outcomes.size()-50;i<outcomes.size();++i)wins+=outcomes[i]==local_match_store::Victory;
    std::reverse(records.begin(),records.end()); // Reversed calendar dates are irrelevant.
    assert(project(*source.data,current,records,2,result)&&result.current_known&&result.current_ratio==wins*2&&result.total_ratio==int(current.wins*100/72));
    auto complete=result;auto partial=records;partial.erase(partial.begin()+20);
    assert(project(*source.data,current,partial,2,result)&&!result.current_known&&result.current_ratio==-1&&result.total_ratio==complete.total_ratio);
    partial=records;partial.resize(49);assert(project(*source.data,current,partial,2,result)&&!result.current_known);
    auto foreign=records.front();foreign.faction=1;foreign.checksum=local_history_store::hash(foreign);records.push_back(foreign);
    assert(project(*source.data,current,records,2,result)&&result.current_known&&result.current_ratio==wins*2);
    auto pending=current;assert(local_match_store::begin(pending));pending.checksum=local_match_store::hash(pending);
    assert(project(*source.data,pending,records,2,result)&&result.current_known&&result.current_ratio==wins*2&&pending.next>pending.settled);
    partial=records;auto corrupted=local_history_store::match(partial[10]);++corrupted.wins;corrupted.checksum=local_match_store::hash(corrupted);
    assert(local_history_store::make(2,corrupted,1800000000,partial[10]));
    assert(project(*source.data,current,partial,2,result)&&!result.current_known); // Valid bytes, inconsistent cumulative chain.
    *source.data=local_progress::seed(0,0,197,13);source.data->count=2;
    source.data->missions[0]={200,5,7,3,28,0,0,0};source.data->missions[1]={201,1,0,2,39,0,0,0};
    assert(project(*source.data,current,records,2,result)&&result.completed==2&&result.three_star==1&&result.campaign_stars==5&&result.pvp_stars==8&&result.campaign_xp==67);
    auto before=result;source.data->missions[1].id=200;assert(!project(*source.data,current,records,2,result)&&!memcmp(&before,&result,sizeof(result)));
    source.data->missions[1].id=201;source.data->stars=4;assert(!project(*source.data,current,records,2,result));
    source.data->stars=13;auto coop=current;coop.mode=3;assert(!project(*source.data,coop,records,2,result));
    *source.data=local_progress::seed(0,0,0,0);local_match_store::State large;large.wins=large.losses=large.draws=UINT32_MAX;
    assert(project(*source.data,large,{},2,result)&&result.total_ratio==33&&result.pvp_battles==INT_MAX&&result.pvp_wins==INT_MAX&&!result.current_known);
    puts("PASS: original recent50 semantics on actual scored-counter chain; interrupted gaps, pending run, foreign faction, reversed dates, missing/contradictory records, unique missions, star-source split, initial50, overflow/refusal without writes");
}
