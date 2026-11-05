#include "../tests/test_paths.h"
#include "skill_tree_policy.h"
#include "../tests/fixtures/skill_tree_fixture.h"
#include <cassert>
#include <cstdlib>
#include <cstdio>
#include <cstring>
int main(int argc,char** argv){
    using namespace local_progress;auto graph=original_skill_graph();assert(graph.valid()&&graph.count==374);
    auto s=seed(100000000,0,0,10000);for(int i=1;i<100;++i)if(graph.max_level[i])assert(grant_stock(s,8,i,100000));
    auto before=s;assert(!graph.purchase(s,121)&&!memcmp(&s,&before,sizeof(s)));
    int later=0;for(int i=0;i<graph.count;++i)if(graph.nodes[i].stage==2){later=graph.nodes[i].id;break;}
    assert(later&&!graph.purchase(s,later)&&!memcmp(&s,&before,sizeof(s)));
    auto poor=s;poor.credits=1699;before=poor;assert(!graph.purchase(poor,111)&&!memcmp(&poor,&before,sizeof(poor)));
    auto cards=s;assert(consume_stock(cards,8,1,99981));before=cards;assert(!graph.purchase(cards,111)&&!memcmp(&cards,&before,sizeof(cards)));
    auto waived=s;assert(graph.purchase(waived,114)&&graph.purchase(waived,134));
    assert(consume_stock(waived,8,13,100000));waived.credits=0;before=waived;
    assert(graph.purchase(waived,144)&&waived.credits==0&&stock_amount(waived,8,13)==0&&waived.skill_count==before.skill_count+1);
    before=waived;assert(!graph.purchase(waived,144)&&!memcmp(&waived,&before,sizeof(waived)));
    auto locked=seed(0,0,0,10000);before=locked;
    assert(!graph.purchase(locked,144)&&!memcmp(&locked,&before,sizeof(locked)));
    assert(!buy_skill_node(locked,144,1,13,1)&&!buy_skill_node(locked,113,1,12,0));
    assert(graph.purchase(locked,113)&&locked.credits==0&&stock_amount(locked,8,12)==0);
    before=locked;assert(!graph.purchase(locked,113)&&!memcmp(&locked,&before,sizeof(locked)));
    int raw_price=0,waived_price=0,waived_nodes=0;
    for(int i=0;i<graph.count;++i){const auto& n=graph.nodes[i];raw_price+=n.credits;
        if(waived_skill_card(n.card)){waived_price+=n.credits;++waived_nodes;assert(skill_tree_policy::credits(n)==0&&skill_tree_policy::cards(n)==0);}
        else assert(skill_tree_policy::credits(n)==n.credits&&skill_tree_policy::cards(n)==n.cards);}
    assert(raw_price==78184700&&waived_nodes==44);
    int total=0;bool changed=true;
    while(changed){changed=false;for(int i=0;i<graph.count;++i)if(graph.purchase(s,graph.nodes[i].id)){++total;changed=true;}}
    assert(total==374&&s.skill_count==374&&s.credits==100000000-raw_price+waived_price&&s.upgrade_count==0&&stock_amount(s,8,11)==100000&&stock_amount(s,8,12)==100000&&stock_amount(s,8,13)==100000&&stock_amount(s,8,22)==100000);
    assert(buy_upgrade(s,111,0,0)&&s.upgrade_count==1&&has_skill_node(s,111));
    skill_tree_policy::Summary summary;assert(graph.summarize(s,summary)&&summary.stage==22);
    for(int i=1;i<100;++i)if(graph.max_level[i])assert(summary.level[i]==11);
    before=s;assert(!graph.purchase(s,111)&&!memcmp(&before,&s,sizeof(s)));
    char dir[]=QSAOW3_TEST_TMP "art3-skill-store-XXXXXX";assert(mkdtemp(dir));
    assert(write(dir,s));State readback;assert(load(dir,readback)==1&&!memcmp(&readback,&s,sizeof(s)));
    auto v5=seed(80,0,0,9);assert(grant_stock(v5,8,1,20));v5.version=5;v5.size=kV5Size;v5.checksum=checksum_bytes(v5,kV5Size);
    char path[512];snprintf(path,sizeof(path),"%s/v5",dir);FILE* f=fopen(path,"wb");assert(f&&fwrite(&v5,1,kV5Size,f)==kV5Size&&!fclose(f));
    assert(read(path,readback)&&readback.version==kVersion&&readback.skill_count==0&&stock_amount(readback,8,1)==20);
    for(int version=1;version<=5;++version){auto old=seed(80,0,0,9);
        if(version>=3)assert(grant_stock(old,4,1,2));if(version>=4)assert(grant_stock(old,9,170,3));if(version>=5)assert(grant_stock(old,8,1,20));
        uint32_t bytes=version==1?kLegacySize:version==2?kV2Size:kV5Size;old.version=version;old.size=bytes;old.checksum=checksum_bytes(old,bytes);
        f=fopen(path,"wb");assert(f&&fwrite(&old,1,bytes,f)==bytes&&!fclose(f));
        assert(read(path,readback)&&readback.version==kVersion&&readback.skill_count==0&&readback.credits==80);
    }
    // New skill payload cannot be smuggled under an old header/size.
    f=fopen(path,"wb");assert(f&&fwrite(&v5,1,sizeof(v5),f)==sizeof(v5)&&!fclose(f));assert(!read(path,readback));
    auto wrong=v5;wrong.version=4;wrong.checksum=checksum_bytes(wrong,kV5Size);
    f=fopen(path,"wb");assert(f&&fwrite(&wrong,1,kV5Size,f)==kV5Size&&!fclose(f));assert(!read(path,readback));
    auto invalid=readback;invalid.skill_count=2;invalid.skill_nodes[0]=invalid.skill_nodes[1]=111;invalid.checksum=checksum(invalid);assert(!valid(invalid));
    for(int i=1;i<argc;++i){State migrated;assert(read(argv[i],migrated)&&migrated.version==kVersion&&migrated.skill_count==0);}
    assert(unlink(path)==0);for(int i=0;i<2;++i){snprintf(path,sizeof(path),"%s/local-progress.%d",dir,i);unlink(path);}assert(rmdir(dir)==0);
    printf("PASS: original374-node graph/all34 levels; cards11/12/13/22 44 free manual nodes, waived_credits=%d, untouched_other_prices, no-card/no-credit completion, dependency+stage+duplicate refusal; atomic receipts, v1..v5 migration and current save/reload\n",waived_price);
}
