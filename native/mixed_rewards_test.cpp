#include "../tests/test_paths.h"
#include "local_progress.h"
#include "hero_card_policy.h"
#include "../tests/fixtures/skill_card_fixture.h"
#include <assert.h>
#include <initializer_list>

int main(){
    using namespace local_progress;
    char root[256],path[300];snprintf(root,sizeof(root),QSAOW3_TEST_TMP "art3-mixed-test-%d",getpid());assert(!mkdir(root,0700));
    using hero_card_policy::Candidate;using hero_card_policy::Grant;
    Candidate cards[128];int cn=0;
    for(const auto& row:skill_card_fixture)cards[cn++]={row.id,2,row.rarity,1,0};
    for(int rarity:{1,2,3})for(int quantity:{1,25,5000}){
        Grant out[128]{},retry[128]{};int n=0,m=0;
        assert(hero_card_policy::distribute(cards,cn,2,1,rarity,quantity,123456789,out,128,n));
        int sum=0;for(int i=0;i<n;++i){sum+=out[i].amount;bool found=false;for(const auto& c:skill_card_fixture)if(c.id==out[i].id&&c.rarity==rarity)found=true;assert(found);}
        assert(sum==quantity&&n>0);
        Candidate reversed[128];for(int i=0;i<cn;++i)reversed[i]=cards[cn-1-i];
        assert(hero_card_policy::distribute(reversed,cn,2,1,rarity,quantity,123456789,retry,128,m)&&m==n&&!memcmp(out,retry,n*sizeof(Grant)));
    }
    State s=seed(100,0,0,75),before=s;
    CareerReward mixed[]={{19,107,1},{0,0,5000},{8,1,10},{9,170,25},{4,2,3}};
    assert(!claim_career(s,10027,120,mixed,5)&&!memcmp(&s,&before,sizeof(s)));
    s.stars=120;assert(claim_career(s,10027,120,mixed,5));
    assert(s.credits==5100&&stock_amount(s,8,1)==10&&stock_amount(s,9,170)==25&&stock_amount(s,4,2)==3);
    before=s;assert(!claim_career(s,10027,120,mixed,5)&&!memcmp(&s,&before,sizeof(s)));
    CareerReward acquisition[]={{22,6,1},{18,20,1},{21,170,1},{8,20,3},{8,23,5}};
    assert(claim_career(s,10021,75,acquisition,5)&&stock_amount(s,8,20)==3&&stock_amount(s,8,23)==5);
    assert(grant_stock(s,4,1,2)&&grant_stock(s,9,1,3));assert(stock_amount(s,8,1)==10&&stock_amount(s,4,1)==2&&stock_amount(s,9,1)==3);
    assert(consume_stock(s,8,1,4)&&stock_amount(s,8,1)==6&&stock_amount(s,4,1)==2&&stock_amount(s,9,1)==3);
    before=s;CareerReward unknown[]={{0,0,1000},{99,999,1}};
    assert(!claim_career(s,90001,120,unknown,2)&&!memcmp(&s,&before,sizeof(s)));
    assert(write(root,s));State loaded{};assert(load(root,loaded)==1&&!memcmp(&s,&loaded,sizeof(s)));
    // v4 is accepted without rewriting, but cannot contain a v5-only skill row.
    State legacy=seed(1540,30,177,9);assert(grant_stock(legacy,9,170,10));legacy.version=4;legacy.size=45128;legacy.checksum=checksum_bytes(legacy,legacy.size);
    snprintf(path,sizeof(path),"%s/v4",root);FILE* f=fopen(path,"wb");assert(f&&fwrite(&legacy,1,legacy.size,f)==legacy.size&&!fclose(f));
    assert(read(path,loaded)&&loaded.version==kVersion&&stock_amount(loaded,9,170)==10);
    State disk{};f=fopen(path,"rb");assert(f&&fread(&disk,1,legacy.size,f)==legacy.size&&fgetc(f)==EOF&&!fclose(f));assert(!memcmp(&disk,&legacy,legacy.size));
    assert(grant_stock(legacy,8,1,1));legacy.checksum=checksum_bytes(legacy,legacy.size);f=fopen(path,"wb");assert(f&&fwrite(&legacy,1,legacy.size,f)==legacy.size&&!fclose(f));assert(!read(path,loaded));
    puts("PASS 34 canonical skill cards / three rarities / conserved stable draws; whole mixed reward gates, typed inventories, duplicate/unknown rollback; v5 round-trip and strict read-only v4 migration; no player save used");
}
