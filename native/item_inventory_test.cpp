#include "../tests/test_paths.h"
#include "supply_store.h"
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <initializer_list>
static void save_exact(const char* path,local_progress::State s,int version){
    s.version=version;s.size=local_progress::kV10Size;s.checksum=local_progress::checksum_bytes(s,s.size);FILE* f=fopen(path,"wb");assert(f&&fwrite(&s,1,s.size,f)==s.size);fclose(f);
}
int main(int argc,char** argv){
    assert(argc==3);local_progress::State previous;assert(local_progress::read(argv[1],previous));
    assert(previous.version==local_progress::kVersion&&previous.supply.claims==1&&previous.supply.pending&&previous.quest_warpoints==20);
    auto s=local_progress::seed(3,1,0,9);
    for(int kind:{7,10,11}){int id=kind==11?0:1;assert(local_progress::grant_stock(s,kind,id,3));auto before=s;
        assert(!local_progress::consume_stock(s,kind,id,4)&&!memcmp(&s,&before,sizeof(s)));
        assert(!local_progress::grant_stock(s,kind,id,INT_MAX)&&!memcmp(&s,&before,sizeof(s)));
        assert(!local_progress::grant_stock(s,kind,id,0)&&!memcmp(&s,&before,sizeof(s)));
        assert(local_progress::consume_stock(s,kind,id,1)&&local_progress::stock_amount(s,kind,id)==2);
    }
    assert(local_progress::stock_amount(s,7,1)==2&&local_progress::stock_amount(s,10,1)==2&&local_progress::stock_amount(s,11,0)==2);
    auto before=s;assert(!local_progress::grant_stock(s,12,0,1)&&!memcmp(&s,&before,sizeof(s)));
    char root[]=QSAOW3_TEST_TMP "art3-items-test-XXXXXX";assert(mkdtemp(root));assert(local_progress::write(root,s));
    local_progress::State read;assert(local_progress::load(root,read)==1&&!memcmp(&read,&s,sizeof(s)));
    for(int kind:{7,10,11}){auto old=local_progress::seed(3,1,0,9);assert(local_progress::grant_stock(old,kind,kind==11?0:1,1));
        char path[640];snprintf(path,sizeof(path),"%s/backdated",root);save_exact(path,old,10);assert(!local_progress::read(path,read));unlink(path);
        old=previous;old.stock_count=0;old.supply.grants[0]={kind,kind==11?0:1,1};save_exact(path,old,10);assert(!local_progress::read(path,read));unlink(path);
    }
    const auto* box=supply_policy::box(401);supply_policy::Eligibility e[128];for(int i=0;i<box->count;++i)e[i]=supply_policy::Eligible;
    supply_policy::Draw drawn;assert(supply_policy::draw(*box,e,box->count,4,drawn));s.quest_warpoints=220;
    before=s;supply_types::Grant obsolete[]={{7,4,2},{10,3,1},{11,0,2}};
    assert(!supply_store::claim(s,2,401,drawn,obsolete,3)&&!memcmp(&s,&before,sizeof(s)));
    supply_types::Grant grants[]={{10,3,1},{0,0,2}};assert(supply_store::claim(s,2,401,drawn,grants,2));assert(local_progress::write(root,s));
    assert(local_progress::load(root,read)==1&&!memcmp(&read,&s,sizeof(s))&&s.quest_warpoints==20&&s.supply.pending);
    assert(local_progress::stock_amount(s,7,1)==2&&local_progress::stock_amount(s,11,0)==2&&local_progress::stock_amount(s,10,3)==1);
    before=s;assert(!supply_store::claim(s,2,401,drawn,grants,2)&&!memcmp(&s,&before,sizeof(s)));
    FILE* f=fopen(argv[2],"wb");assert(f&&fwrite(&s,1,sizeof(s),f)==sizeof(s));fclose(f);
    for(int slot=0;slot<2;++slot){char path[640];snprintf(path,sizeof(path),"%s/local-progress.%d",root,slot);unlink(path);}assert(!rmdir(root));
    puts("PASS: new typed inventory isolation, insufficient/zero/overflow refusal, atomic supply debit/grants/journal, cold-save replay, v10 migration and backdated type rejection");
}
