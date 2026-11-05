#include "battle_contract_policy.h"
#include "warpoint_store.h"
#include "cooperative_store.h"
#include <cassert>
#include <climits>
#include <cstdio>
#include <cstring>
#include <initializer_list>
using local_match_store::State;
static constexpr int id=20080;
static constexpr uint32_t bit=1u<<battle_contract_events::count;
static void seed(local_progress::State& s){
    s=local_progress::seed(1690,30,197,9);
    assert(contract_policy::initialize(s,20010,0));
    for(const auto& r:contract_content::battle_rules)assert(contract_policy::initialize(s,r.id,0));
    assert(contract_policy::initialize(s,id,0));
}
static State result(int mode,bool formal,int outcome,int reason,int tick,uint32_t mask=bit){
    State m;m.mode=mode;
    assert(local_match_store::begin_pvp(m,formal)&&local_match_store::freeze_career_stars(m,formal,formal?4:0,formal?4:0)&&
        local_match_store::freeze_victory_credits(m,0)&&local_match_store::freeze_contracts(m,formal?mask:0));
    assert(local_match_store::settle(m,1,(local_match_store::Outcome)outcome,tick,reason,outcome==1?1:2,nullptr,nullptr)&&
        local_match_store::stamp_contracts(m,{}));
    m.checksum=local_match_store::hash(m);assert(local_match_store::valid(m));return m;
}
int main(int argc,char** argv){
    assert(argc==2&&sizeof(State)==136&&sizeof(local_progress::State)==60280&&sizeof(cooperative_store::WireState)==176);
    const auto& r=contract_content::result_rules[0];
    assert(r.id==id&&r.subtype==25&&r.value==1&&r.flags==2&&r.rank_min==4&&r.rank_max==6&&!r.faction);
    const contract_content::Definition* d=nullptr;for(const auto& x:contract_content::definitions)if(x.id==id)d=&x;
    assert(d&&d->kind==6&&d->type==16&&d->param==0&&d->reward_count==1&&d->rewards[0].amount==1200&&d->points==20&&!d->xp);
    local_progress::StateBuffer a,b,c;assert(a.data&&b.data&&c.data);int cases=0;
    for(int mode:{1,2,4})for(bool formal:{false,true})for(int outcome=1;outcome<=6;++outcome)for(int reason:{1,6})
    for(int tick:{0,1,14999,15000,15001,INT_MAX}){
        seed(*a.data);auto m=result(mode,formal,outcome,reason,tick);assert(battle_contract_policy::reconcile(*a.data,m));
        int expected=formal&&outcome==1&&reason==1&&tick<=15000;
        assert(battle_contract_policy::count(*a.data,id)==expected&&a.data->credits==1690&&a.data->contract_points==0);
        *b.data=*a.data;assert(battle_contract_policy::reconcile(*a.data,m)&&!memcmp(a.data,b.data,sizeof(*a.data)));++cases;
    }
    seed(*a.data);auto m=result(1,true,1,1,15000);assert(m.version==12&&m.size==136);
    assert(local_match_store::write(argv[1],m));State disk;assert(local_match_store::load(argv[1],disk)==1&&disk.version==12&&disk.contract_mask==bit);
    assert(battle_contract_policy::reconcile(*a.data,disk));bool changed=false;
    assert(warpoint_store::reconcile(*a.data,disk,changed)&&changed&&a.data->credits==4460&&a.data->stars==13);
    assert(battle_contract_policy::claim(*a.data,id,1200,0,0,20)&&a.data->credits==5660&&a.data->contract_points==20&&battle_contract_policy::count(*a.data,id)==0);
    assert(local_progress::write(argv[1],*a.data)&&local_progress::load(argv[1],*b.data)==1);
    *c.data=*b.data;assert(battle_contract_policy::reconcile(*b.data,disk)&&!memcmp(b.data,c.data,sizeof(*b.data)));
    assert(!battle_contract_policy::claim(*b.data,id,1200,0,0,20)&&!memcmp(b.data,c.data,sizeof(*b.data)));
    assert(local_match_store::begin_pvp(disk,true)&&local_match_store::freeze_career_stars(disk,true,4,4)&&local_match_store::freeze_victory_credits(disk,0)&&local_match_store::freeze_contracts(disk,bit));
    assert(local_match_store::settle(disk,2,local_match_store::Victory,14000,1,1,nullptr,nullptr)&&local_match_store::stamp_contracts(disk,{}));
    disk.checksum=local_match_store::hash(disk);assert(battle_contract_policy::reconcile(*b.data,disk)&&battle_contract_policy::count(*b.data,id)==1);
    assert(battle_contract_policy::claim(*b.data,id,1200,0,0,20)&&contract_policy::receipt(*b.data,id)->claims==2&&b.data->contract_points==40);
    // Every historical v7/v8 and a newly ineligible result has no result bit.
    for(int version:{7,8,9}){
        seed(*a.data);auto old=result(1,true,1,1,1,0);old.version=version;old.size=version==7?112:136;old.checksum=local_match_store::hash(old);
        assert(local_match_store::valid(old)&&local_match_store::write(argv[1],old)&&local_match_store::load(argv[1],disk)==1);
        assert(disk.version==(uint32_t)version&&battle_contract_policy::reconcile(*a.data,disk)&&!battle_contract_policy::count(*a.data,id));
    }
    seed(*a.data);*b.data=*a.data;
    auto bad=m;bad.version=8;bad.checksum=local_match_store::hash(bad);assert(!local_match_store::valid(bad)&&!battle_contract_policy::reconcile(*a.data,bad));
    bad=m;bad.contract_mask=16384;bad.checksum=local_match_store::hash(bad);assert(!local_match_store::valid(bad));
    bad=m;bad.tick=-1;bad.checksum=local_match_store::hash(bad);assert(!local_match_store::valid(bad));
    bad=m;bad.next=bad.settled=2;bad.checksum=local_match_store::hash(bad);assert(!battle_contract_policy::reconcile(*a.data,bad));
    assert(!memcmp(a.data,b.data,sizeof(*a.data)));
    // Missing/corrupt new receipt cannot partially apply the five old events.
    bad=result(1,true,1,1,1000,63);battle_contract_events::Counts events;assert(battle_contract_events::add(63,events,27,3));
    assert(local_match_store::stamp_contracts(bad,events));bad.checksum=local_match_store::hash(bad);
    a.data->contracts[a.data->contract_count-1].baseline=2;*b.data=*a.data;
    assert(!battle_contract_policy::reconcile(*a.data,bad)&&!memcmp(a.data,b.data,sizeof(*a.data)));
    --a.data->contract_count;*b.data=*a.data;
    assert(!battle_contract_policy::reconcile(*a.data,bad)&&!memcmp(a.data,b.data,sizeof(*a.data)));
    seed(*a.data);auto* receipt=const_cast<local_progress::ContractReceipt*>(contract_policy::receipt(*a.data,id));receipt->baseline=1;
    a.data->credits=INT_MAX;*b.data=*a.data;assert(!battle_contract_policy::claim(*a.data,id,1200,0,0,20)&&!memcmp(a.data,b.data,sizeof(*a.data)));
    char absent[640];snprintf(absent,sizeof(absent),"%s/missing",argv[1]);assert(!local_match_store::write(absent,m));
    State coop;coop.mode=3;assert(!local_match_store::begin_pvp(coop,true));
    printf("PASS: original timed-win20080, %d mode/source/outcome/reason/time cases; inclusive25-minute native-tick boundary; frozen v10/136 result bit, legacy v7/112 and v8/136 no-backpay; manual1200-credit20-point reset/cold duplicate/new behavior; shared wallet stars/credits; corruption/atomic refusal/overflow/failed-write guards; cooperative176 unchanged\n",cases);
}
