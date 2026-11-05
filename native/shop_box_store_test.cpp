#include "shop_box_store.h"
#include "warpoint_store.h"
#include "cooperative_credit_store.h"
#include <cassert>
static void completed(local_progress::State& s,shop_box_policy::Mode mode,uint64_t run,bool normal){
    assert(shop_box_policy::begin(s.shop,mode,run));bool changed=false;
    assert(shop_box_policy::finish(s.shop,mode,run,normal,changed)&&changed);
    assert(shop_box_policy::finish(s.shop,mode,run,normal,changed)&&!changed);
}
int main(int argc,char** argv){
    assert(argc==2);auto s=local_progress::seed(10,20,30,9);assert(shop_box_policy::ready(s.shop));
    supply_policy::Draw d;d.count=2;d.rows[0]={12000,300};d.rows[1]={12001,3};
    supply_types::Grant grants[]={{0,0,300},{1,0,3}};
    completed(s,shop_box_policy::Campaign,1,true);assert(!s.shop.games);
    assert(shop_box_store::claim(s,d,grants,2)&&s.credits==310&&s.gold==23&&s.shop.pending&&s.shop.claims==1);
    assert(!local_progress::stock_amount(s,15,1));auto before=s;
    assert(!shop_box_store::claim(s,d,grants,2)&&!memcmp(&s,&before,sizeof(s)));
    completed(s,shop_box_policy::Formal,1,true);assert(!s.shop.games);s.shop.pending=0;
    completed(s,shop_box_policy::Campaign,2,false);assert(!s.shop.games);
    completed(s,shop_box_policy::Campaign,3,true);assert(s.shop.games==1);
    completed(s,shop_box_policy::Formal,2,true);assert(s.shop.games==2);
    completed(s,shop_box_policy::Cooperative,1,true);assert(s.shop.games==3);
    completed(s,shop_box_policy::Campaign,4,true);assert(s.shop.games==3);
    assert(shop_box_store::claim(s,d,grants,2)&&s.shop.games==0&&s.shop.claims==2);
    s.shop.pending=0;bool changed=false;
    assert(shop_box_policy::finish(s.shop,shop_box_policy::Campaign,4,true,changed)&&!changed&&!s.shop.games);
    assert(shop_box_policy::begin(s.shop,shop_box_policy::Formal,3));assert(shop_box_policy::begin(s.shop,shop_box_policy::Formal,3));
    assert(shop_box_policy::finish(s.shop,shop_box_policy::Formal,3,true,changed)&&changed&&s.shop.games==1);
    assert(!shop_box_policy::begin(s.shop,shop_box_policy::Formal,3));
    s.shop.games=3;s.credits=INT_MAX;before=s;assert(!shop_box_store::claim(s,d,grants,2)&&!memcmp(&s,&before,sizeof(s)));
    s.credits=0;assert(local_progress::write(argv[1],s));local_progress::State read;assert(local_progress::load(argv[1],read)==1&&read.shop.claims==2&&read.shop.games==3);
    before=read;assert(!local_progress::write("/proc/art3-shop-save-denied",read)&&!memcmp(&read,&before,sizeof(read)));
    auto transaction=read;assert(shop_box_store::claim(transaction,d,grants,2));
    assert(!local_progress::write("/proc/art3-shop-save-denied",transaction));
    assert(local_progress::load(argv[1],read)==1&&read.shop.claims==2&&read.shop.games==3&&read.credits==0);
    assert(shop_box_store::claim(read,d,grants,2)&&local_progress::write(argv[1],read));
    assert(local_progress::load(argv[1],read)==1&&read.shop.claims==3&&read.shop.pending&&read.credits==300);
    // Durable old v13 file: no result is registered just by loading history.
    auto old=local_progress::seed(91,2,3,4);old.version=13;old.size=local_progress::kV13Size;old.checksum=local_progress::checksum_bytes(old,old.size);
    char path[640];snprintf(path,sizeof(path),"%s/legacy13",argv[1]);FILE* f=fopen(path,"wb");assert(f);assert(fwrite(&old,1,old.size,f)==old.size);fclose(f);
    assert(local_progress::read(path,read)&&read.version==local_progress::kVersion&&read.credits==91&&!read.shop.claims);
    assert(shop_box_policy::finish(read.shop,shop_box_policy::Formal,100,true,changed)&&!changed);
    supply_policy::Eligibility e[33];for(auto& x:e)x=supply_policy::Eligible;
    int tickets=0;
    for(int i=1;i<=1000;++i){supply_policy::Draw a,b;
        assert(supply_policy::draw_pool(shop_box_content::box,shop_box_content::items,e,33,i,a));
        assert(supply_policy::draw_pool(shop_box_content::box,shop_box_content::items,e,33,i,b));
        assert(a.count==b.count&&!memcmp(a.rows,b.rows,a.count*sizeof(a.rows[0]))&&supply_policy::valid_draw_pool(shop_box_content::box,shop_box_content::items,a));
        for(int j=0;j<a.count;++j){assert(a.rows[j].item!=12032);tickets+=a.rows[j].item==12019;}
    }assert(!tickets);e[19]=supply_policy::Unsupported;supply_policy::Draw a;
    assert(supply_policy::draw_pool(shop_box_content::box,shop_box_content::items,e,33,1,a));
    e[2]=supply_policy::Unsupported;assert(!supply_policy::draw_pool(shop_box_content::box,shop_box_content::items,e,33,1,a));
    // Actual durable result bridges: old/unregistered, training, surrender,
    // normal defeat, repeated cold reconciliation, and mixed-mode progress.
    auto p=local_progress::seed(0,0,0,9);assert(shop_box_store::claim(p,d,grants,2));p.shop.pending=0;
    local_match_store::State m;m.version=5;m.next=m.settled=1;m.outcome=local_match_store::Defeat;m.reason=1;m.reserved2=1;m.checksum=local_match_store::hash(m);
    assert(warpoint_store::reconcile(p,m,changed)&&!p.shop.games);
    assert(shop_box_policy::begin(p.shop,shop_box_policy::Formal,2));m.next=m.settled=2;m.checksum=local_match_store::hash(m);
    assert(warpoint_store::reconcile(p,m,changed)&&changed&&p.shop.games==1);
    assert(warpoint_store::reconcile(p,m,changed)&&!changed&&p.shop.games==1);
    assert(shop_box_policy::begin(p.shop,shop_box_policy::Formal,3));m.next=m.settled=3;m.outcome=local_match_store::Surrender;m.reason=6;m.checksum=local_match_store::hash(m);
    assert(warpoint_store::reconcile(p,m,changed)&&p.shop.games==1);
    assert(shop_box_policy::begin(p.shop,shop_box_policy::Formal,4));m.next=m.settled=4;m.outcome=local_match_store::Victory;m.reason=1;m.reserved2=0;m.checksum=local_match_store::hash(m);
    assert(warpoint_store::reconcile(p,m,changed)&&p.shop.games==1); // Training, even if a start was accidentally registered.
    cooperative_store::State c;c.match.mode=3;c.match.scenario=973;c.match.map=558;c.match.opponent=1;c.match.next=c.match.settled=1;c.match.outcome=local_match_store::Defeat;c.match.reason=1;c.pumpkins=0;cooperative_store::seal(c);
    assert(shop_box_policy::begin(p.shop,shop_box_policy::Cooperative,1));assert(cooperative_credit_store::reconcile(p,c,changed)&&changed&&p.shop.games==2);
    assert(cooperative_credit_store::reconcile(p,c,changed)&&!changed&&p.shop.games==2);
    puts("PASS: free shop entitlement, three-mode exactly-once results, no retroactive history, pending cap, atomic overflow/save failure, 1000 future effective-pool draws with removed tickets excluded, unknown effective candidate refusal, v13 read-only migration");
}
