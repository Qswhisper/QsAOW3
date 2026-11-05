#include "../tests/test_paths.h"
#include "local_match_store.h"
#include <assert.h>
#include <stdlib.h>
int main() {
    using namespace local_match_store;
    char root[]=QSAOW3_TEST_TMP "art3-ai-test-XXXXXX";assert(mkdtemp(root));
    State s{};assert(load(root,s)==0);assert(begin(s));assert(write(root,s));
    uint64_t run=s.next;int b[]={5,0},u[]={3,0};
    assert(settle(s,run,Victory,100,1,1,b,u));assert(write(root,s));
    auto committed=s;assert(!settle(s,run,Victory,100,1,1,b,u));assert(s.wins==1);
    State disk{};assert(load(root,disk)==1&&disk.wins==1&&disk.settled==run);
    assert(begin(s)&&write(root,s));assert(!begin(s));
    assert(load(root,disk)==1&&disk.next==run+1&&disk.settled==run);
    assert(settle(s,s.next,Interrupted,0,0,-1,nullptr,nullptr)&&write(root,s));
    assert(s.wins==1&&s.losses==0&&s.exits==1);
    assert(begin(s)&&write(root,s));assert(settle(s,s.next,Surrender,250,2,2,b,u)&&write(root,s));
    assert(s.wins==1&&s.losses==1&&s.exits==1);
    char p[640];snprintf(p,sizeof(p),"%s/local-ai.%u",root,unsigned(s.generation&1));
    FILE* f=fopen(p,"wb");assert(f);fputs("truncated",f);fclose(f);
    assert(load(root,disk)==1&&disk.generation==s.generation-1);
    assert(disk.next>disk.settled); // Recovery retains an unfinished run, not a false victory.
    char nonexistent[640];snprintf(nonexistent,sizeof(nonexistent),"%s/missing",root);
    assert(!write(nonexistent,disk));assert(load(root,s)==1&&s.generation==disk.generation);
    s=committed;s.wins++;assert(!valid(s));
    for(int i=0;i<2;++i){snprintf(p,sizeof(p),"%s/local-ai.%d",root,i);unlink(p);}
    char legacy_root[640];snprintf(legacy_root,sizeof(legacy_root),"%s/legacy",root);assert(mkdir(legacy_root,0700)==0);
    auto legacy=committed;legacy.version=1;legacy.size=88;legacy.checksum=hash_bytes(legacy,88);
    snprintf(p,sizeof(p),"%s/local-ai.0",legacy_root);f=fopen(p,"wb");assert(f);
    assert(fwrite(&legacy,1,88,f)==88);fclose(f);
    State migrated{};assert(load(legacy_root,migrated)==1);
    assert(migrated.version==4&&migrated.size==112&&migrated.mode==1&&migrated.generation==legacy.generation&&migrated.wins==1&&migrated.scenario==201&&migrated.map==4&&migrated.opponent==2);
    assert(begin(migrated));migrated.scenario=202;migrated.map=2;assert(write(legacy_root,migrated));
    assert(load(legacy_root,disk)==1&&disk.scenario==202&&disk.map==2&&disk.wins==1&&disk.next==2&&disk.settled==1);
    for(int i=0;i<2;++i){snprintf(p,sizeof(p),"%s/local-ai.%d",legacy_root,i);unlink(p);}
    auto v2=committed;v2.version=2;v2.size=96;v2.scenario=0;v2.map=510;v2.checksum=hash_bytes(v2,96);
    snprintf(p,sizeof(p),"%s/local-ai.0",legacy_root);f=fopen(p,"wb");assert(f);
    assert(fwrite(&v2,1,96,f)==96);fclose(f);
    assert(load(legacy_root,migrated)==1&&migrated.version==4&&migrated.mode==1&&migrated.scenario==0&&migrated.map==510&&migrated.opponent==1&&migrated.generation==v2.generation);
    assert(begin(migrated));migrated.opponent=2;assert(write(legacy_root,migrated));
    assert(load(legacy_root,disk)==1&&disk.opponent==2&&disk.wins==1&&disk.next==2&&disk.settled==1);
    for(int i=0;i<2;++i){snprintf(p,sizeof(p),"%s/local-ai.%d",legacy_root,i);unlink(p);}
    auto v3=committed;v3.version=3;v3.size=104;v3.opponent=1;v3.checksum=hash_bytes(v3,104);
    snprintf(p,sizeof(p),"%s/local-ai.0",legacy_root);f=fopen(p,"wb");assert(f);
    assert(fwrite(&v3,1,104,f)==104);fclose(f);
    assert(load(legacy_root,migrated)==1&&migrated.mode==1&&migrated.opponent==1&&migrated.generation==v3.generation);
    assert(begin(migrated));migrated.mode=2;assert(write(legacy_root,migrated));
    assert(load(legacy_root,disk)==1&&disk.mode==2&&disk.opponent==1&&disk.wins==1&&disk.next==2&&disk.settled==1);
    assert(settle(disk,disk.next,Defeat,4000,1,2,b,u)&&write(legacy_root,disk));
    assert(load(legacy_root,migrated)==1&&migrated.mode==2&&migrated.losses==1);
    auto cooperative=migrated;cooperative.mode=3;cooperative.scenario=973;cooperative.map=558;cooperative.checksum=hash(cooperative);assert(valid(cooperative));
    assert(begin(cooperative)&&write(legacy_root,cooperative));assert(load(legacy_root,disk)==1&&disk.mode==3&&disk.scenario==973&&disk.map==558);
    auto three=migrated;three.mode=4;assert(begin(three)&&write(legacy_root,three));
    assert(load(legacy_root,disk)==1&&disk.mode==4&&disk.next==three.next);
    assert(settle(disk,disk.next,Surrender,4500,6,2,b,u)&&write(legacy_root,disk));
    assert(load(legacy_root,three)==1&&three.mode==4&&three.outcome==Surrender);
    auto invalid=migrated;invalid.mode=5;invalid.checksum=hash(invalid);assert(!valid(invalid));
    for(int i=0;i<2;++i){snprintf(p,sizeof(p),"%s/local-ai.%d",legacy_root,i);unlink(p);}
    assert(rmdir(legacy_root)==0);
    assert(rmdir(root)==0);
    puts("PASS local-ai ledger: commit, idempotence, pending/interruption, surrender, corrupt-slot fallback, failed-write retention, v1/v2/v3 migration, map/opponent/1v1/2v2 identity");
}
