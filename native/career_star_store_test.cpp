#include "../tests/test_paths.h"
#include "career_star_rewards.h"
#include "warpoint_store.h"
#include "cooperative_store.h"
#include <cassert>
#include <cstdlib>
#include <initializer_list>
#include <pthread.h>
static void* small_stack_receipt(void* result){
    local_progress::StateBuffer storage,loaded;assert(storage.data&&loaded.data);auto& s=*storage.data;
    s.magic=0x33545241;s.version=local_progress::kVersion;s.size=sizeof(s);s.stars=9;s.checksum=local_progress::checksum(s);
    char root[]=QSAOW3_TEST_TMP "art3-star-stack-XXXXXX";assert(mkdtemp(root)&&local_progress::write(root,s));
    local_match_store::State game,cold;assert(local_match_store::begin_pvp(game,true)&&local_match_store::freeze_career_stars(game,true,2,4)&&local_match_store::write(root,game));
    assert(local_match_store::settle(game,1,local_match_store::Victory,999,1,1,nullptr,nullptr));game.reserved=100;assert(local_match_store::write(root,game)&&local_match_store::load(root,cold)==1);
    bool changed=false;assert(warpoint_store::reconcile(s,cold,changed)&&changed&&s.stars==13&&s.quest_warpoints==100&&local_progress::write(root,s));
    assert(local_progress::load(root,*loaded.data)==1&&warpoint_store::reconcile(*loaded.data,cold,changed)&&!changed&&loaded.data->stars==13);
    for(int slot=0;slot<2;++slot){char path[160];snprintf(path,sizeof(path),"%s/local-ai.%d",root,slot);unlink(path);snprintf(path,sizeof(path),"%s/local-progress.%d",root,slot);unlink(path);}assert(!rmdir(root));*(bool*)result=true;return nullptr;
}
int main(int argc,char** argv){
    assert(argc==2);static_assert(sizeof(local_match_store::State)==136&&sizeof(cooperative_store::WireState)==176&&sizeof(cooperative_store::State)==200,"Ledger layout changed");
    int budget=0;assert(career_star_rewards::budget(6,18,false,budget)&&budget==4);assert(career_star_rewards::budget(50,75,false,budget)&&budget==9);
    assert(career_star_rewards::budget(0,1,false,budget)&&budget==1);assert(career_star_rewards::budget(INT_MAX,0,true,budget)&&!budget);assert(!career_star_rewards::budget(9,9,false,budget));
    char root[]=QSAOW3_TEST_TMP "art3-stars-XXXXXX";assert(mkdtemp(root));auto progress=local_progress::seed(1690,30,197,9);assert(local_progress::write(root,progress));
    local_match_store::State match{};assert(local_match_store::begin_pvp(match,true)&&local_match_store::freeze_career_stars(match,true,2,4)&&local_match_store::write(root,match));
    local_match_store::State loaded;assert(local_match_store::load(root,loaded)==1&&loaded.version==6&&local_match_store::start_rank(loaded)==2&&local_match_store::star_budget(loaded)==4&&!local_match_store::career_stars(loaded));
    assert(local_match_store::settle(loaded,1,local_match_store::Victory,9000,1,1,nullptr,nullptr));loaded.reserved=100;assert(local_match_store::write(root,loaded));
    auto prior=progress;bool changed=false;assert(warpoint_store::reconcile(progress,loaded,changed)&&changed&&progress.stars==13&&progress.quest_warpoints==100&&progress.credits==1690&&progress.gold==30&&progress.experience==197);
    char missing[640];snprintf(missing,sizeof(missing),"%s/missing",root);auto uncommitted=progress;assert(!local_progress::write(missing,progress)&&!memcmp(&progress,&uncommitted,sizeof(progress)));
    assert(local_progress::load(root,progress)==1&&!memcmp(&progress,&prior,sizeof(progress)));assert(warpoint_store::reconcile(progress,loaded,changed)&&changed&&local_progress::write(root,progress));
    assert(local_progress::load(root,progress)==1&&warpoint_store::reconcile(progress,loaded,changed)&&!changed&&progress.stars==13&&progress.quest_warpoints==100);
    auto bad_wallet=prior;bad_wallet.stars=INT_MAX-2;auto before=bad_wallet;assert(!warpoint_store::reconcile(bad_wallet,loaded,changed)&&!memcmp(&bad_wallet,&before,sizeof(before)));
    bad_wallet=prior;bad_wallet.quest_warpoints=INT_MAX-10;before=bad_wallet;assert(!warpoint_store::reconcile(bad_wallet,loaded,changed)&&!memcmp(&bad_wallet,&before,sizeof(before)));
    int cases=0;
    for(int mode:{1,2,4})for(int outcome=1;outcome<=6;++outcome)for(int reason:{0,1,6,7}){
        local_match_store::State game{};game.mode=mode;assert(local_match_store::begin_pvp(game,true)&&local_match_store::freeze_career_stars(game,true,5,9));
        assert(local_match_store::settle(game,1,(local_match_store::Outcome)outcome,1000,reason,1,nullptr,nullptr));game.checksum=local_match_store::hash(game);
        auto wallet=local_progress::seed(1,2,3,50);assert(warpoint_store::reconcile(wallet,game,changed)&&changed&&wallet.stars==50+((outcome==1&&reason==1)?9:0)&&wallet.credits==1&&wallet.gold==2&&wallet.experience==3);++cases;
    }
    local_match_store::State legacy{};assert(local_match_store::begin_pvp(legacy,true)&&local_match_store::settle(legacy,1,local_match_store::Victory,999,1,1,nullptr,nullptr));legacy.reserved=100;legacy.checksum=local_match_store::hash(legacy);
    auto wallet=local_progress::seed(0,0,0,9);assert(warpoint_store::reconcile(wallet,legacy,changed)&&changed&&wallet.stars==9); // v5 never receives retrospective stars.
    auto backdated=loaded;backdated.version=5;backdated.checksum=local_match_store::hash(backdated);assert(!local_match_store::valid(backdated));
    for(int mask:{1,2,-1}){auto corrupt=loaded;corrupt.reserved2=mask;corrupt.checksum=local_match_store::hash(corrupt);assert(!local_match_store::valid(corrupt));}
    local_match_store::State training{};assert(local_match_store::begin_pvp(training,false));auto frozen=training;assert(!local_match_store::freeze_career_stars(training,false,2,4)&&!memcmp(&training,&frozen,sizeof(training)));
    assert(local_match_store::freeze_career_stars(training,false,0,0)&&local_match_store::settle(training,1,local_match_store::Victory,1000,1,1,nullptr,nullptr));training.checksum=local_match_store::hash(training);wallet=local_progress::seed(0,0,0,9);assert(warpoint_store::reconcile(wallet,training,changed)&&!changed&&wallet.stars==9);
    local_match_store::State highest{};assert(local_match_store::begin_pvp(highest,true)&&local_match_store::freeze_career_stars(highest,true,29,0)&&local_match_store::settle(highest,1,local_match_store::Victory,1000,1,1,nullptr,nullptr));highest.checksum=local_match_store::hash(highest);assert(!local_match_store::career_stars(highest));
    auto milestone=local_progress::seed(0,0,0,160);local_progress::CareerReward gifts[]={{22,8,1},{6,1,1},{8,5,5}};
    assert(local_progress::claim_career(milestone,31,160,gifts,3)&&local_progress::has_career(milestone,31)&&local_progress::stock_amount(milestone,6,1)==0&&local_progress::stock_amount(milestone,8,5)==5&&!milestone.credits&&!milestone.gold&&!milestone.experience);
    auto acknowledged=milestone;assert(!local_progress::claim_career(milestone,31,160,gifts,3)&&!memcmp(&milestone,&acknowledged,sizeof(milestone)));
    auto longer=local_progress::seed(0,0,0,4600);local_progress::CareerReward longer_gifts[]={{22,19,1},{6,3,3},{0,0,123}};
    assert(local_progress::claim_career(longer,86,4600,longer_gifts,3)&&local_progress::stock_amount(longer,6,3)==0&&longer.credits==123&&!longer.gold&&!longer.experience);
    auto longer_once=longer;assert(!local_progress::claim_career(longer,86,4600,longer_gifts,3)&&!memcmp(&longer,&longer_once,sizeof(longer)));
    FILE* f=fopen(argv[1],"wb");assert(f&&fwrite(&loaded,1,loaded.size,f)==loaded.size);fclose(f);
    for(int slot=0;slot<2;++slot){char path[640];snprintf(path,sizeof(path),"%s/local-ai.%d",root,slot);unlink(path);snprintf(path,sizeof(path),"%s/local-progress.%d",root,slot);unlink(path);}assert(!rmdir(root));
    pthread_attr_t attributes;assert(!pthread_attr_init(&attributes)&&!pthread_attr_setstacksize(&attributes,65536));pthread_t thread;bool small_stack=false;
    assert(!pthread_create(&thread,&attributes,small_stack_receipt,&small_stack)&&!pthread_join(thread,nullptr)&&small_stack);pthread_attr_destroy(&attributes);
    printf("PASS: %d mode/outcome/reason cases, frozen rank/budget cold replay, one combined stars/warpoints receipt, failed-write retry, atomic overflow, no v5 backpay/training/abnormal reward, highest-rank zero, backdated/corrupt refusal, PvP112/cooperative176-byte ledgers, complete cold receipt on 64KiB thread\n",cases);
}
