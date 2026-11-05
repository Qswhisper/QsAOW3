#include "local_progress.h"
#include "competition_track_policy.h"
#include "competition_track_store.h"
#include <assert.h>
#include <string>
int main(int argc,char** argv){
    assert(argc==2);using namespace competition_track_policy;
    Ledger l;assert(competition_track_types::valid(l));bool changed=true;
    assert(settle(l,10140,9,true,20,changed)&&!changed&&l.count==0); // No history replay.
    assert(begin(l,10140,101406,10));auto before=l;
    assert(!begin(l,10140,101406,11)&&!memcmp(&l,&before,sizeof(l)));
    assert(!begin(l,10140,101407,10)&&!memcmp(&l,&before,sizeof(l)));
    assert(!settle(l,10140,11,true,20,changed)&&!memcmp(&l,&before,sizeof(l)));
    assert(settle(l,10140,10,false,20,changed)&&changed&&l.tracks[0].points==0);
    assert(settle(l,10140,10,true,20,changed)&&!changed&&l.tracks[0].points==0);
    for(int i=11;i<=35;++i){assert(begin(l,10140,101406,i));assert(settle(l,10140,i,true,20,changed)&&changed);}
    assert(l.tracks[0].points==20&&l.tracks[0].cursor==35);
    supply_types::Grant g{1,0,10};
    assert(stage(l,10140,101406,1,false,2,&g,1));auto pending=l;
    assert(!stage(l,10140,101406,1,true,2,&g,1)&&!memcmp(&l,&pending,sizeof(l)));
    assert(dismiss(l));assert(!stage(l,10140,101406,1,false,2,&g,1));
    assert(stage(l,10140,101406,1,true,2,&g,1)&&l.claims==2);assert(dismiss(l));
    auto s=local_progress::seed(100,10,0,110);s.competition_tracks=l;
    local_progress::CareerReward gold{1,0,10};assert(local_progress::apply_rewards(s,&gold,1)&&s.gold==20);
    assert(stage(s.competition_tracks,10140,101406,2,false,3,&g,1));
    assert(local_progress::write(argv[1],s));local_progress::State cold;assert(local_progress::load(argv[1],cold)==1);
    assert(cold.gold==20&&cold.competition_tracks.pending&&cold.competition_tracks.grants[0].amount==10);
    assert(!can_claim(cold.competition_tracks,10140,101406,2,false,3));
    auto saved=cold;local_progress::CareerReward over{1,0,INT_MAX};assert(!local_progress::apply_rewards(cold,&over,1)&&!memcmp(&saved,&cold,sizeof(cold)));
    local_progress::CareerReward career{0,0,5};assert(local_progress::claim_career(cold,123,0,&career,1)&&cold.credits==105&&local_progress::has_career(cold,123));
    assert(!local_progress::claim_career(cold,123,0,&career,1));
    // Exercise exact v15 wire prefix, checksum, stocks and read-only migration.
    auto legacy=local_progress::seed(55,6,7,8);assert(local_progress::grant_stock(legacy,17,123,4));
    legacy.version=15;legacy.size=local_progress::kV15Size;legacy.checksum=local_progress::checksum_bytes(legacy,legacy.size);
    std::string path=std::string(argv[1])+"/legacy15";FILE* f=fopen(path.c_str(),"wb");assert(f);assert(fwrite(&legacy,1,legacy.size,f)==legacy.size);assert(!fclose(f));
    local_progress::State migrated;assert(local_progress::read(path.c_str(),migrated)&&migrated.version==16&&migrated.size==60280);
    assert(migrated.credits==55&&migrated.gold==6&&local_progress::stock_amount(migrated,17,123)==4&&migrated.competition_tracks.count==0);
    struct stat info{};assert(!stat(path.c_str(),&info)&&info.st_size==58072);
    // The production bridge consumes the same sealed normal result used by
    // existing cooperative credit and free-box services, once in one wallet.
    for(auto outcome:{local_match_store::Victory,local_match_store::Defeat,local_match_store::Surrender,local_match_store::Interrupted,local_match_store::Error}){
        auto wallet=local_progress::seed(100,0,0,110);cooperative_store::State game;
        game.match.scenario=973;game.match.map=558;game.match.mode=3;game.match.opponent=1;cooperative_store::seal(game);
        assert(cooperative_store::begin(game)&&cooperative_store::freeze_credits(game,0));
        assert(begin(wallet.competition_tracks,10140,101406,game.match.next));
        int reason=outcome==local_match_store::Victory||outcome==local_match_store::Defeat?1:outcome==local_match_store::Surrender?6:7;
        assert(cooperative_store::settle(game,game.match.next,outcome,100,reason,outcome==local_match_store::Victory?1:2,nullptr,nullptr,0));cooperative_store::seal(game);
        assert(competition_track_store::reconcile(wallet,game,changed)&&changed);
        assert(wallet.competition_tracks.tracks[0].points==(outcome==local_match_store::Victory?1:0));
        assert(wallet.credits==(outcome==local_match_store::Victory?2870:100));
        auto once=wallet;assert(competition_track_store::reconcile(wallet,game,changed)&&!changed&&!memcmp(&once,&wallet,sizeof(wallet)));
    }
    puts("PASS: track new-run provenance, normal-result selection, no history, duplicate/definition/refusal isolation, twenty-point cap, independent rows, pending cold read, atomic wallet/receipt, career reuse and read-only v15 migration");
}
