#include "../tests/test_paths.h"
#include "local_progress.h"
#include "hero_card_policy.h"
#include "../tests/fixtures/hero_cost_fixture.h"
#include <assert.h>

// Isolated native transaction tests. Never loads or changes a player save.
int main(){
    using namespace local_progress;
    // Stable receipt draws conserve the original quantity and cannot reroll
    // when catalog ordering changes. Ineligible heroes never enter the pool.
    using hero_card_policy::Candidate;using hero_card_policy::Grant;
    Candidate pool[]={{173,2,1,9,0},{70,1,1,6,0},{171,2,2,6,0},{170,2,1,6,0},{176,2,1,12,0}};
    Grant grants[8]{},repeat[8]{};int n=0,m=0;
    assert(!hero_card_policy::distribute(pool,5,2,2,1,50,123,grants,8,n)&&n==0);
    assert(hero_card_policy::distribute(pool,5,2,6,1,50,123,grants,8,n)&&n==1&&grants[0].id==170&&grants[0].amount==50);
    assert(hero_card_policy::distribute(pool,5,2,9,1,50000,123,grants,8,n)&&n==2);
    assert(grants[0].id==170&&grants[1].id==173&&grants[0].amount+grants[1].amount==50000);
    for(int i=0;i<2;++i)assert(grants[i].amount>24000&&grants[i].amount<26000);
    Candidate reverse[5];for(int i=0;i<5;++i)reverse[i]=pool[4-i];
    assert(hero_card_policy::distribute(reverse,5,2,9,1,50000,123,repeat,8,m)&&m==n&&!memcmp(grants,repeat,n*sizeof(Grant)));
    assert(hero_card_policy::distribute(pool,5,1,9,1,50,123,grants,8,n)&&n==1&&grants[0].id==70&&grants[0].amount==50);
    assert(hero_card_policy::distribute(pool,5,2,9,2,25,123,grants,8,n)&&n==1&&grants[0].id==171&&grants[0].amount==25);
    assert(!hero_card_policy::distribute(pool,5,2,9,1,50,123,grants,1,n));
    assert(!hero_card_policy::distribute(pool,5,2,9,1,50,0,grants,8,n));
    Candidate duplicate[]={{170,2,1,6,0},{170,2,1,6,0}};
    assert(!hero_card_policy::distribute(duplicate,2,2,9,1,50,123,grants,8,n));
    Candidate expired[]={{170,2,1,6,8}};
    assert(!hero_card_policy::distribute(expired,1,2,9,1,50,123,grants,8,n));
    char root[256],path[300];snprintf(root,sizeof(root),QSAOW3_TEST_TMP "art3-hero-test-%d",getpid());assert(!mkdir(root,0700));
    State state=seed(100,0,0,9),before{};assert(grant_stock(state,4,170,2)&&grant_stock(state,9,170,250));
    assert(stock_amount(state,4,170)==2&&stock_amount(state,9,170)==250);
    assert(stock_types(state,4)==1&&stock_types(state,9)==1);
    {State only_cards=seed(0,0,0,9);assert(grant_stock(only_cards,9,170,250));assert(stock_types(only_cards,4)==0&&stock_types(only_cards,9)==1);}
    before=state;assert(!buy_card_upgrade(state,32300,101,0,170,250)&&!memcmp(&state,&before,sizeof(state)));
    assert(!buy_card_upgrade(state,32300,0,-1,170,250)&&!memcmp(&state,&before,sizeof(state)));
    assert(!buy_card_upgrade(state,32300,0,0,170,251)&&!memcmp(&state,&before,sizeof(state)));
    assert(!buy_card_upgrade(state,32300,0,0,170,0)&&!memcmp(&state,&before,sizeof(state)));
    assert(buy_card_upgrade(state,32300,0,0,170,250)&&stock_amount(state,9,170)==0&&stock_amount(state,4,170)==2);
    assert(state.credits==100&&has_upgrade(state,32300));before=state;
    assert(!buy_card_upgrade(state,32300,0,0,170,250)&&!memcmp(&state,&before,sizeof(state)));
    assert(write(root,state));State loaded{};assert(load(root,loaded)==1&&!memcmp(&state,&loaded,sizeof(state)));
    before=state;assert(!write("/does-not-exist/art3-hero",state)&&!memcmp(&state,&before,sizeof(state)));
    // Resolved canonical reward + wallet are covered by one idempotent receipt.
    CareerReward reward[]={{9,170,600},{0,0,46000}};
    assert(claim_career(state,90000,9,reward,2));before=state;
    assert(!claim_career(state,90000,9,reward,2)&&!memcmp(&state,&before,sizeof(state)));
    assert(buy_card_upgrade(state,32301,46000,0,170,550));assert(state.credits==100&&stock_amount(state,9,170)==50);
    assert(write(root,state));assert(load(root,loaded)==1&&has_upgrade(loaded,32300)&&has_upgrade(loaded,32301)&&stock_amount(loaded,9,170)==50);
    // v3 migration preserves every field and never rewrites the source file.
    State legacy=seed(1540,30,177,9);legacy.version=3;legacy.size=45128;legacy.generation=51;legacy.checksum=checksum_bytes(legacy,legacy.size);
    snprintf(path,sizeof(path),"%s/v3",root);FILE* f=fopen(path,"wb");assert(f&&fwrite(&legacy,1,legacy.size,f)==legacy.size&&!fclose(f));
    assert(read(path,loaded)&&loaded.version==kVersion&&loaded.generation==51&&loaded.credits==1540&&loaded.stars==9);
    State disk{};f=fopen(path,"rb");assert(f&&fread(&disk,1,legacy.size,f)==legacy.size&&fgetc(f)==EOF&&!fclose(f));assert(!memcmp(&disk,&legacy,legacy.size));
    // Old version cannot smuggle a new card row through a valid checksum.
    assert(grant_stock(legacy,9,170,1));legacy.checksum=checksum_bytes(legacy,legacy.size);f=fopen(path,"wb");assert(f&&fwrite(&legacy,1,legacy.size,f)==legacy.size&&!fclose(f));assert(!read(path,loaded));
    // Every retained price/quantity is exercised as transaction data. This does
    // not claim that every hero combat/ability effect has been implemented.
    for(const auto& c:hero_cost_fixture){State s=seed(c.cost,0,0,10000);assert(grant_stock(s,9,c.hero,c.cards));
        assert(buy_card_upgrade(s,c.level,c.cost,0,c.hero,c.cards));assert(s.credits==0&&stock_amount(s,9,c.hero)==0&&has_upgrade(s,c.level));
        before=s;assert(!buy_card_upgrade(s,c.level,c.cost,0,c.hero,c.cards)&&!memcmp(&s,&before,sizeof(s)));}
    printf("PASS uniform rank/faction/rarity policy, conserved quantity, stable receipt/order; atomic wallet/card/receipt; duplicate/insufficient/invalid rollback; isolated v3 migration; %zu retained price rows; no player save used\n",sizeof(hero_cost_fixture)/sizeof(*hero_cost_fixture));
}
