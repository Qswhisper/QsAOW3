#include "../tests/test_paths.h"
#include "local_progress.h"
#include <cassert>
#include <cstdlib>
#include <initializer_list>
int main(int argc,char** argv){
    using namespace local_progress;
    auto s=seed(1540,0,177,9);auto before=s;
    assert(read_quest(s,1000)&&s.credits==1540&&quest_state(s,1000)==8&&!has_quest(s,1000));
    assert(claim_quest(s,1000,100,0,10,10));
    assert(s.credits==1640&&s.experience==187&&s.stars==9&&s.quest_count==1&&s.quest_warpoints==10);
    assert(s.next_run==before.next_run&&s.settled_run==before.settled_run&&s.career_count==0&&s.skill_count==0);
    before=s;assert(!claim_quest(s,1000,100,0,10,10)&&!memcmp(&s,&before,sizeof(s)));
    assert(!claim_quest(s,1001,-1,0,0,0)&&!memcmp(&s,&before,sizeof(s)));
    auto overflow=s;overflow.credits=INT_MAX;before=overflow;
    assert(!claim_quest(overflow,1001,1,0,0,0)&&!memcmp(&overflow,&before,sizeof(before)));
    char root[]=QSAOW3_TEST_TMP "art3-quest-store-XXXXXX";assert(mkdtemp(root));
    assert(write(root,s));State restored;assert(load(root,restored)==1&&!memcmp(&s,&restored,sizeof(s)));
    char path[640];snprintf(path,sizeof(path),"%s/old",root);
    for(uint32_t v=1;v<=7;++v){
        auto old=seed(80,0,65,5);uint32_t size=v==1?kLegacySize:v==2?kV2Size:v<6?kV5Size:v==6?kV6Size:kV7Size;
        if(v>=6){old.skill_count=1;old.skill_nodes[0]=111;}
        old.version=v;old.size=size;old.checksum=checksum_bytes(old,size);
        FILE* f=fopen(path,"wb");assert(f&&fwrite(&old,1,size,f)==size&&!fclose(f));
        assert(read(path,restored)&&restored.version==kVersion&&restored.quest_count==0&&restored.quest_warpoints==0&&restored.credits==80);
        assert(restored.skill_count==(v>=6?1u:0u));
        if(v>=6){assert(restored.skill_nodes[0]==111);assert(write(root,restored));State again;assert(load(root,again)==1&&again.skill_nodes[0]==111);}
    }
    // Append-only payload must not be accepted under an old header/size.
    auto wrong=seed(1,0,0,0);wrong.version=6;wrong.size=kV6Size;wrong.checksum=checksum_bytes(wrong,kV6Size);
    FILE* f=fopen(path,"wb");assert(f&&fwrite(&wrong,1,sizeof(wrong),f)==sizeof(wrong)&&!fclose(f));assert(!read(path,restored));
    for(int i=1;i<argc;++i){State old;assert(read(argv[i],old)&&old.version==kVersion&&old.quest_count==0);}
    assert(unlink(path)==0);for(int i:{0,1}){snprintf(path,sizeof(path),"%s/local-progress.%d",root,i);unlink(path);}assert(rmdir(root)==0);
    puts("PASS: v7 quest receipt+credits/gold/XP/legacy points atomic; duplicate/negative/overflow refusal; v1..v6 read-only migration with v6 skills retained; two-slot persistence; strict old-size rejection");
}
