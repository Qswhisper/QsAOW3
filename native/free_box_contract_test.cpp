#include "contract_policy.h"
#include "shop_box_store.h"
#include <cassert>

static supply_policy::Draw draw(){supply_policy::Draw d;d.count=2;d.rows[0]={12000,300};d.rows[1]={12001,3};return d;}
static void open(local_progress::State& s){
    const auto d=draw();supply_types::Grant g[]={{0,0,300},{1,0,3}};
    if(s.shop.claims){s.shop.pending=0;for(int i=0;i<3;++i){uint64_t run=s.shop.allowed[0]+1;bool changed=false;
        assert(shop_box_policy::begin(s.shop,shop_box_policy::Campaign,run));
        assert(shop_box_policy::finish(s.shop,shop_box_policy::Campaign,run,true,changed)&&changed);}}
    assert(shop_box_store::claim(s,d,g,2));
}
int main(int argc,char** argv){
    assert(argc==2);local_progress::StateBuffer a,b,before;assert(a.data&&b.data&&before.data);auto& s=*a.data;
    s=local_progress::seed(0,0,0,30);open(s);open(s);assert(contract_policy::event_counter(s,4)==2);
    assert(contract_policy::initialize(s,20020,contract_policy::event_counter(s,4)));
    assert(contract_policy::count(s,20020,contract_policy::event_counter(s,4),3)==0); // Existing boxes never backpay.
    *b.data=s;b.data->upgrade_count=7;b.data->contract_pvp_wins=11;b.data->quest_count=2;b.data->quests[0]={1,32};b.data->quests[1]={2,8};
    assert(contract_policy::event_counter(*b.data,1)==7&&contract_policy::event_counter(*b.data,2)==1&&contract_policy::event_counter(*b.data,3)==11);
    assert(contract_policy::count(*b.data,20020,contract_policy::event_counter(*b.data,4),3)==0); // Counter-only synthetic fields never enter the durable fixture.
    for(int i=1;i<=3;++i){open(s);assert(contract_policy::count(s,20020,contract_policy::event_counter(s,4),3)==i);
        *before.data=s;const auto d=draw();supply_types::Grant g[]={{0,0,300},{1,0,3}};
        assert(!shop_box_store::claim(s,d,g,2)&&!memcmp(&s,before.data,sizeof(s))); // Pending restore is not another open.
        assert(local_progress::write(argv[1],s)&&local_progress::load(argv[1],*b.data)==1);
        assert(contract_policy::count(*b.data,20020,contract_policy::event_counter(*b.data,4),3)==i);}
    int credits=s.credits,points=s.contract_points;
    assert(contract_policy::claim(s,20020,contract_policy::event_counter(s,4),3,600,0,0,10));
    assert(s.credits==credits+600&&s.contract_points==points+10&&contract_policy::count(s,20020,contract_policy::event_counter(s,4),3)==0);
    *before.data=s;assert(!contract_policy::claim(s,20020,contract_policy::event_counter(s,4),3,600,0,0,10)&&!memcmp(&s,before.data,sizeof(s)));
    assert(local_progress::write(argv[1],s)&&local_progress::load(argv[1],*b.data)==1);
    assert(contract_policy::receipt(*b.data,20020)->claims==1&&contract_policy::count(*b.data,20020,contract_policy::event_counter(*b.data,4),3)==0);
    open(s);assert(contract_policy::count(s,20020,contract_policy::event_counter(s,4),3)==1);
    s.credits=INT_MAX;s.shop.pending=0;s.shop.games=3;*before.data=s;const auto d=draw();supply_types::Grant g[]={{0,0,300},{1,0,3}};
    assert(!shop_box_store::claim(s,d,g,2)&&!memcmp(&s,before.data,sizeof(s)));
    auto& transaction=*b.data;transaction=s;transaction.credits=0;assert(shop_box_store::claim(transaction,d,g,2));
    assert(!local_progress::write("/proc/art3-free-box-contract-denied",transaction));
    assert(!memcmp(&s,before.data,sizeof(s))); // Failed disk publication cannot advance the live event counter.
    puts("PASS: category4 contract uses committed shop receipts only; old history baseline, three new box grants, pending duplicate refusal, cold read, original 600/10 manual reward, zero reset, new behavior cycle, other event isolation and atomic overflow/save failure");
}
