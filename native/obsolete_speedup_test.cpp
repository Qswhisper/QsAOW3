#include "../tests/test_paths.h"
#include "supply_store.h"
#include <cassert>
#include <cstdlib>
#include <initializer_list>
int main(){
    char root[]=QSAOW3_TEST_TMP "art3-speedup-XXXXXX";assert(mkdtemp(root));
    auto s=local_progress::seed(1000,30,70,9);
    for(int id:{1,2,3,4})assert(local_progress::grant_stock(s,7,id,id));
    assert(local_progress::grant_stock(s,10,1,2));
    auto before=s;
    local_progress::CareerReward rewards[]={{7,1,5},{7,2,6},{7,3,7},{7,4,8},{0,0,100},{10,1,1}};
    assert(local_progress::claim_career(s,999,9,rewards,6));
    for(int id:{1,2,3,4})assert(local_progress::stock_amount(s,7,id)==id);
    assert(s.credits==1100&&s.gold==before.gold&&s.experience==before.experience&&s.stars==before.stars&&local_progress::stock_amount(s,10,1)==3);
    before=s;assert(!local_progress::claim_career(s,999,9,rewards,6)&&!memcmp(&before,&s,sizeof(s)));
    for(int id:{0,5}){local_progress::CareerReward bad[]={{0,0,100},{7,id,1}};assert(!local_progress::claim_career(s,1000,0,bad,2)&&!memcmp(&before,&s,sizeof(s)));}
    auto box=supply_policy::box(401);supply_policy::Eligibility e[128];for(int i=0;i<box->count;++i)e[i]=supply_policy::Eligible;
    supply_policy::Draw draw;assert(supply_policy::draw(*box,e,box->count,4,draw));
    s.quest_warpoints=10000;supply_types::Grant grants[]={{7,1,2},{7,2,4},{7,3,6},{7,4,8},{0,0,55},{10,1,1}};
    before=s;assert(!supply_store::claim(s,2,401,draw,grants,6)&&!memcmp(&s,&before,sizeof(s)));
    // Construct an already committed historical receipt. New random draws
    // reject obsolete types; existing journals/inventory remain readable.
    assert(supply_policy::advance(s.supply,2,401,200,draw,grants,6));
    s.quest_warpoints-=200;s.credits+=55;assert(local_progress::grant_stock(s,10,1,1));
    assert(s.quest_warpoints==9800&&s.credits==1155&&s.supply.grant_count==6&&s.supply.pending);
    for(int id:{1,2,3,4})assert(local_progress::stock_amount(s,7,id)==id);
    assert(!memcmp(s.supply.grants,grants,sizeof(grants))&&local_progress::stock_amount(s,10,1)==4);
    before=s;assert(!supply_store::claim(s,2,401,draw,grants,6)&&!memcmp(&s,&before,sizeof(s)));
    assert(local_progress::write(root,s));local_progress::StateBuffer cold;assert(cold.data&&local_progress::load(root,*cold.data)==1&&!memcmp(cold.data,&s,sizeof(s)));
    // Old pending journals and old inventory survive loading unchanged.
    assert(local_progress::stock_amount(*cold.data,7,4)==4&&cold.data->supply.grants[3].amount==8);
    for(int i=0;i<2;++i){char p[256];snprintf(p,sizeof(p),"%s/local-progress.%d",root,i);unlink(p);}assert(!rmdir(root));
    puts("PASS original speedup receipts/counts retained; future stock suppressed; old inventory unchanged; mixed rewards atomic; duplicate/unknown refused; pending cold load unchanged");
}
