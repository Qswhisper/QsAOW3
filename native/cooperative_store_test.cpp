#include "../tests/test_paths.h"
#include "cooperative_store.h"
#include <assert.h>
#include <stdlib.h>
#include <initializer_list>
using namespace cooperative_store;
using namespace local_match_store;
static void cleanup(const char* root){
    char p[640];for(const char* stem:{"cooperative","local-ai"})for(int i=0;i<2;++i){
        snprintf(p,sizeof(p),"%s/%s.%d",root,stem,i);unlink(p);
        snprintf(p,sizeof(p),"%s/%s.%d.tmp",root,stem,i);unlink(p);
    }rmdir(root);
}
int main(){
    char root[]=QSAOW3_TEST_TMP "art3-coop-test-XXXXXX";assert(mkdtemp(root));
    cooperative_store::State s{},disk{};assert(cooperative_store::load(root,s)==0&&valid(s));
    assert(cooperative_store::begin(s)&&cooperative_store::write(root,s));
    assert(cooperative_store::settle(s,1,Victory,100,1,1,nullptr,nullptr,10));
    assert(s.award==100&&s.points==100&&s.first_win==1&&cooperative_store::write(root,s));
    auto committed=s;assert(!cooperative_store::settle(s,1,Victory,100,1,1,nullptr,nullptr,10));
    assert(!memcmp(&s,&committed,sizeof(s)));
    assert(cooperative_store::load(root,disk)==1&&disk.points==100&&disk.match.wins==1);
    const Outcome cases[]={Victory,Defeat,Defeat,Defeat,Defeat,Surrender,Interrupted,Error,Draw,Defeat};
    const uint32_t goals[]={10,0,1,5,17,10,10,10,10,3};
    const uint32_t awards[]={50,0,5,25,25,0,0,0,0,0};
    uint64_t total=100;
    for(int i=0;i<10;++i){
        assert(cooperative_store::begin(s)&&cooperative_store::write(root,s));
        assert(cooperative_store::settle(s,s.match.next,cases[i],200,i==9?3:1,2,nullptr,nullptr,goals[i]));
        total+=awards[i];assert(s.award==awards[i]&&s.points==total&&cooperative_store::write(root,s));
        assert(cooperative_store::load(root,disk)==1&&disk.points==total&&disk.match.settled==s.match.next);
    }
    // A cold unfinished run is an interruption with no points, exactly once.
    assert(cooperative_store::begin(s)&&cooperative_store::write(root,s));
    assert(cooperative_store::load(root,disk)==1&&disk.match.next>disk.match.settled);
    assert(cooperative_store::settle(disk,disk.match.next,Interrupted,0,0,-1,nullptr,nullptr,0));
    assert(disk.points==total&&disk.award==0&&cooperative_store::write(root,disk));
    // A failed write cannot mutate the caller's generation or balance.
    committed=disk;char missing[640];snprintf(missing,sizeof(missing),"%s/missing",root);
    assert(!cooperative_store::write(missing,disk)&&!memcmp(&disk,&committed,sizeof(disk)));
    auto overflow=disk;assert(cooperative_store::begin(overflow));overflow.points=UINT64_MAX;
    auto before=overflow;assert(!cooperative_store::settle(overflow,overflow.match.next,Victory,100,1,1,nullptr,nullptr,10));
    assert(!memcmp(&overflow,&before,sizeof(before)));
    // Corruption falls back to the other complete atomic slot, never a partial result.
    char p[640];snprintf(p,sizeof(p),"%s/cooperative.%u",root,unsigned(disk.generation&1));
    FILE* f=fopen(p,"wb");assert(f);fputs("broken",f);fclose(f);
    assert(cooperative_store::load(root,s)==1&&s.generation==disk.generation-1&&s.points==total);
    cleanup(root);
    // Legacy matches migrate only in memory, without retroactive reward guesses.
    char legacy[]=QSAOW3_TEST_TMP "art3-coop-legacy-XXXXXX";assert(mkdtemp(legacy));
    local_match_store::State old{};old.scenario=973;old.map=558;old.mode=3;old.opponent=1;
    assert(local_match_store::begin(old)&&local_match_store::write(legacy,old));
    assert(local_match_store::settle(old,1,Victory,500,1,1,nullptr,nullptr)&&local_match_store::write(legacy,old));
    auto original=old;
    assert(cooperative_store::load(legacy,s)==2&&s.points==0&&s.award==0&&s.first_win==0&&s.match.wins==1&&s.pumpkins==UINT32_MAX);
    snprintf(p,sizeof(p),"%s/cooperative.0",legacy);assert(access(p,F_OK)!=0);
    assert(cooperative_store::begin(s)&&cooperative_store::write(legacy,s));
    assert(cooperative_store::settle(s,s.match.next,Victory,200,1,1,nullptr,nullptr,10)&&s.award==100&&cooperative_store::write(legacy,s));
    assert(local_match_store::load(legacy,old)==1&&!memcmp(&old,&original,sizeof(old)));
    for(int i=0;i<2;++i){snprintf(p,sizeof(p),"%s/cooperative.%d",legacy,i);f=fopen(p,"wb");assert(f);fputs("bad",f);fclose(f);}
    assert(cooperative_store::load(legacy,s)==-1); // No old balance resurrection.
    cleanup(legacy);
    puts("cooperative atomic ledger and local reward policy: PASS");
}
