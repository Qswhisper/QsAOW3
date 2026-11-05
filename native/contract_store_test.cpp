#include "../tests/test_paths.h"
#include "contract_policy.h"
#include <cassert>
#include <cstdlib>
#include <initializer_list>
int main(int argc,char** argv){
    using namespace local_progress;
    auto s=seed(1500,10,177,30);s.upgrade_count=8;
    for(int i=0;i<8;++i)s.upgrade_ids[i]=10000+i;
    assert(claim_quest(s,1000,0,0,0,10));
    assert(contract_policy::initialize(s,20060,s.upgrade_count));
    assert(contract_policy::initialize(s,20040,contract_policy::quest_claims(s)));
    assert(contract_policy::initialize(s,20010,s.contract_pvp_wins));s.contract_pvp_cursor=23;
    assert(contract_policy::count(s,20060,8,1)==0);
    auto before=s;assert(!contract_policy::claim(s,20060,8,1,1200,0,0,15)&&!memcmp(&before,&s,sizeof(s)));
    assert(buy_upgrade(s,10008,100,1));
    assert(contract_policy::claim(s,20060,s.upgrade_count,1,1200,0,0,15));
    assert(s.credits==2600&&s.experience==178&&s.contract_points==15&&s.stars==30);
    before=s;assert(!contract_policy::claim(s,20060,9,1,1200,0,0,15)&&!memcmp(&before,&s,sizeof(s)));
    assert(buy_upgrade(s,10009,100,1)&&buy_upgrade(s,10010,100,1));
    assert(contract_policy::count(s,20060,11,1)==1&&contract_policy::claim(s,20060,11,1,1200,0,0,15));
    assert(contract_policy::count(s,20060,11,1)==0); // Old overflow never pays the next contract.
    assert(read_quest(s,1002));assert(contract_policy::count(s,20040,contract_policy::quest_claims(s),1)==0);
    assert(claim_quest(s,1002,50,0,10,3));
    assert(contract_policy::claim(s,20040,contract_policy::quest_claims(s),1,1000,0,0,10));
    assert(contract_policy::record_pvp(s,24,2,true)&&s.contract_pvp_wins==0);
    assert(contract_policy::record_pvp(s,25,1,false)&&s.contract_pvp_wins==0);
    assert(contract_policy::record_pvp(s,26,1,true)&&s.contract_pvp_wins==1);
    assert(contract_policy::record_pvp(s,26,1,true)&&s.contract_pvp_wins==1); // Replay is idempotent.
    before=s;assert(!contract_policy::record_pvp(s,28,1,true)&&!memcmp(&before,&s,sizeof(s)));
    assert(contract_policy::record_pvp(s,27,1,true)&&contract_policy::claim(s,20010,s.contract_pvp_wins,2,1000,0,0,15));
    assert(contract_policy::count(s,20010,s.contract_pvp_wins,2)==0);
    auto overflow=s;overflow.credits=INT_MAX;before=overflow;
    assert(!contract_policy::claim(overflow,20010,4,2,1,0,0,0)&&!memcmp(&overflow,&before,sizeof(before)));
    before=s;assert(!contract_policy::claim(s,20010,4,2,-1,0,0,0)&&!memcmp(&before,&s,sizeof(s)));
    for(int level=0;level<=11;++level){int amount=-1;assert(contract_policy::credits_bonus(1000,100,level,amount)&&amount==1000+100*level);}
    int amount=0;assert(!contract_policy::credits_bonus(INT_MAX,100,1,amount));
    char root[]=QSAOW3_TEST_TMP "art3-contract-store-XXXXXX";assert(mkdtemp(root));
    assert(write(root,s));State restored;assert(load(root,restored)==1&&!memcmp(&s,&restored,sizeof(s)));
    char path[640];snprintf(path,sizeof(path),"%s/old",root);
    for(uint32_t v=1;v<=7;++v){auto old=seed(80,0,65,5);
        uint32_t size=v==1?kLegacySize:v==2?kV2Size:v<6?kV5Size:v==6?kV6Size:kV7Size;
        if(v>=6){old.skill_count=1;old.skill_nodes[0]=111;}
        if(v==7){old.quest_count=1;old.quests[0]={1000,32};old.quest_warpoints=10;}
        old.version=v;old.size=size;old.checksum=checksum_bytes(old,size);
        FILE* f=fopen(path,"wb");assert(f&&fwrite(&old,1,size,f)==size&&!fclose(f));
        assert(read(path,restored)&&restored.version==kVersion&&restored.contract_count==0&&restored.contract_points==0);
        assert(restored.skill_count==(v>=6?1u:0u)&&restored.quest_count==(v==7?1u:0u));
        if(v==7)assert(restored.quests[0].state==32&&restored.quest_warpoints==10);
    }
    for(int i=1;i<argc;++i){State old;assert(read(argv[i],old)&&old.version==kVersion&&old.contract_count==0&&old.quest_count==2);}
    assert(unlink(path)==0);for(int i:{0,1}){snprintf(path,sizeof(path),"%s/local-progress.%d",root,i);unlink(path);}assert(rmdir(root)==0);
    puts("PASS: v8 contract wallet+receipt atomic; new-event baseline/reset and no overflow reuse; read-only quest is not completion; 1v1 result cursor/dedup/gap refusal; v1..v7 read-only migration preserving skills+quests; private reload; negative/overflow refusal");
}
