#include "../tests/test_paths.h"
#include "cooperative_credit_store.h"
#include "warpoint_store.h"
#include <cassert>
#include <cstdlib>
#include <initializer_list>
using namespace local_match_store;
static void clean(const char* root){for(const char* stem:{"cooperative","local-progress"})for(int i=0;i<2;++i){char p[256];snprintf(p,sizeof(p),"%s/%s.%d",root,stem,i);unlink(p);}assert(!rmdir(root));}
int main(){
    char root[]=QSAOW3_TEST_TMP "art3-coop-credit-XXXXXX";assert(mkdtemp(root));
    cooperative_store::State game;assert(cooperative_store::load(root,game)==0);
    for(int i=0;i<13;++i){assert(cooperative_store::begin(game)&&cooperative_store::settle(game,game.match.next,Victory,100,1,1,nullptr,nullptr,10));}
    cooperative_store::seal(game);assert(cooperative_store::valid(game));
    // An actual v1 on-disk shape has no credit policy/source, even for wins.
    auto legacy=game;legacy.version=1;legacy.size=160;legacy.checksum=cooperative_store::hash_bytes(legacy,160);
    auto legacy_wire=cooperative_store::to_wire(legacy);
    char path[256];snprintf(path,sizeof(path),"%s/cooperative.0",root);FILE* f=fopen(path,"wb");assert(f&&fwrite(&legacy_wire,1,160,f)==160);fclose(f);
    assert(cooperative_store::load(root,game)==1&&game.version==2&&!game.credit_run&&!game.credits&&game.match.settled==13);
    auto wallet=local_progress::seed(1000,30,70,9);wallet.warpoint_pvp_cursor=26;bool changed=true;
    assert(cooperative_credit_store::reconcile(wallet,game,changed)&&!changed&&wallet.credits==1000&&!wallet.cooperative_credit_cursor);
    assert(local_progress::write(root,wallet));
    int cases=0;for(int level=0;level<=11;++level)for(auto outcome:{Victory,Defeat,Surrender,Interrupted,Error,Draw}){
        auto run=game;auto earned=wallet;int budget=0;assert(cooperative_credit_policy::budget(level,budget)&&budget==2770+277*level);
        assert(cooperative_store::begin(run));auto pending=run;
        assert(!cooperative_store::freeze_credits(run,-1)&&!memcmp(&run,&pending,sizeof(run)));
        assert(!cooperative_store::freeze_credits(run,12)&&!memcmp(&run,&pending,sizeof(run)));
        assert(cooperative_store::freeze_credits(run,level));pending=run;assert(!cooperative_store::freeze_credits(run,level)&&!memcmp(&run,&pending,sizeof(run)));
        int reason=outcome==Surrender?6:outcome==Interrupted?7:1;
        assert(cooperative_store::settle(run,run.match.next,outcome,100,reason,outcome==Victory?1:2,nullptr,nullptr,5));
        auto settled=run;assert(!cooperative_store::settle(run,run.match.next,outcome,100,reason,1,nullptr,nullptr,5)&&!memcmp(&run,&settled,sizeof(run)));
        cooperative_store::seal(run);assert(cooperative_store::valid(run)&&run.credits==(outcome==Victory?budget:0));
        assert(cooperative_credit_store::reconcile(earned,run,changed)&&changed&&earned.credits==1000+(outcome==Victory?budget:0)&&earned.cooperative_credit_cursor==14&&earned.warpoint_pvp_cursor==26&&earned.experience==70&&earned.stars==9&&earned.quest_warpoints==0);
        auto receipt=earned;assert(cooperative_credit_store::reconcile(earned,run,changed)&&!changed&&!memcmp(&earned,&receipt,sizeof(earned)));++cases;
    }
    assert(cooperative_store::begin(game)&&cooperative_store::freeze_credits(game,11)&&cooperative_store::write(root,game));
    assert(cooperative_store::settle(game,14,Victory,900,1,1,nullptr,nullptr,10)&&cooperative_store::write(root,game)&&game.credits==5817);
    auto unchanged=wallet;auto earned=wallet;assert(cooperative_credit_store::reconcile(earned,game,changed)&&changed);
    char missing[300];snprintf(missing,sizeof(missing),"%s/missing",root);assert(!local_progress::write(missing,earned)&&!memcmp(&wallet,&unchanged,sizeof(wallet)));
    local_progress::StateBuffer cold;assert(local_progress::load(root,*cold.data)==1&&cold.data->credits==1000&&!cold.data->cooperative_credit_cursor);
    assert(cooperative_credit_store::reconcile(*cold.data,game,changed)&&changed&&local_progress::write(root,*cold.data));
    auto paid=*cold.data;assert(local_progress::load(root,*cold.data)==1&&cooperative_credit_store::reconcile(*cold.data,game,changed)&&!changed&&!memcmp(&paid,cold.data,sizeof(paid)));
    earned=wallet;earned.credits=INT_MAX;unchanged=earned;assert(!cooperative_credit_store::reconcile(earned,game,changed)&&!memcmp(&earned,&unchanged,sizeof(earned)));
    // v12 is read-only migrated with all inventory and the other cursor kept.
    auto old=wallet;assert(local_progress::grant_stock(old,7,4,7)&&local_progress::grant_stock(old,17,1101,7));
    old.version=12;old.size=54584;old.checksum=local_progress::checksum_bytes(old,54584);
    snprintf(path,sizeof(path),"%s/v12",root);f=fopen(path,"wb");assert(f&&fwrite(&old,1,54584,f)==54584);fclose(f);
    assert(local_progress::read(path,*cold.data)&&cold.data->version==local_progress::kVersion&&!cold.data->cooperative_credit_cursor&&cold.data->warpoint_pvp_cursor==26&&local_progress::stock_amount(*cold.data,7,4)==7&&local_progress::stock_amount(*cold.data,17,1101)==7);unlink(path);
    f=fopen(QSAOW3_TEST_TMP "art3-coop-credit-growth-v15","wb");assert(f&&fwrite(&paid,1,sizeof(paid),f)==sizeof(paid));fclose(f);
    auto game_wire=cooperative_store::to_wire(game);
    f=fopen(QSAOW3_TEST_TMP "art3-coop-credit-ledger-v2","wb");assert(f&&fwrite(&game_wire,1,sizeof(game_wire),f)==sizeof(game_wire));fclose(f);
    clean(root);printf("PASS %d frozen cooperative-credit outcome cases; v1/v12 read-only migration; no backpay/XP/stars; independent cursor; duplicate/save retry/overflow refusal; cold exactly once\n",cases);
}
