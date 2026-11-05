#include "battle_contract_policy.h"
#include <cassert>
#include <climits>
#include <cstring>
#include <cstdio>
#include <initializer_list>
using local_match_store::State;
static constexpr int id=21290;
static constexpr uint32_t eligibility=battle_contract_events::timed_mask,completed=battle_contract_events::timed_completed;
static void seed(local_progress::State& s){
    s=local_progress::seed(1690,30,197,9);assert(contract_policy::initialize(s,20010,0));
    for(const auto& rule:contract_content::battle_rules)assert(contract_policy::initialize(s,rule.id,0));
    for(const auto& rule:contract_content::result_rules)assert(contract_policy::initialize(s,rule.id,0));
    assert(contract_policy::initialize(s,id,0));
}
static State result(int mode,bool formal,int outcome,int reason,int tick,int amount){
    State m;m.mode=mode;assert(local_match_store::begin_pvp(m,formal)&&local_match_store::freeze_career_stars(m,formal,formal?4:0,formal?4:0)&&
        local_match_store::freeze_victory_credits(m,0)&&local_match_store::freeze_contracts(m,formal?eligibility:0));
    timed_contract_events::Counts events;assert(timed_contract_events::add(m.contract_mask,events,30,amount,tick));
    assert(local_match_store::settle(m,1,(local_match_store::Outcome)outcome,18000,reason,outcome==1?1:2,nullptr,nullptr)&&
        local_match_store::stamp_contracts(m,{},events));
    m.checksum=local_match_store::hash(m);assert(local_match_store::valid(m));return m;
}
int main(int argc,char** argv){
    assert(argc==2&&sizeof(State)==136&&sizeof(local_progress::State)==60280);
    const auto& rule=contract_content::timed_rules[0];assert(rule.id==id&&rule.minutes==6&&rule.buildings==4&&rule.value==1&&rule.flags==1);
    const contract_content::Definition* d=nullptr;for(const auto& x:contract_content::definitions)if(x.id==id)d=&x;
    assert(d&&d->kind==7&&d->type==15&&d->subtype==21&&d->param_count==2&&d->params[0]==6&&d->params[1]==4&&d->rewards[0].amount==1200&&d->points==15);
    local_progress::StateBuffer a,b,c;assert(a.data&&b.data&&c.data);int cases=0;
    for(int mode:{1,2,4})for(bool formal:{false,true})for(int outcome=1;outcome<=6;++outcome)for(int reason:{1,6})
    for(int tick:{0,3599,3600,3601})for(int amount:{0,3,4}){
        seed(*a.data);auto m=result(mode,formal,outcome,reason,tick,amount);
        bool qualified=formal&&outcome==1&&reason==1&&tick<=3600&&amount>=4;
        assert(bool(m.contract_mask&completed)==qualified&&battle_contract_policy::reconcile(*a.data,m)&&battle_contract_policy::count(*a.data,id)==qualified);
        assert(a.data->credits==1690&&a.data->contract_points==0);*b.data=*a.data;
        assert(battle_contract_policy::reconcile(*a.data,m)&&!memcmp(a.data,b.data,sizeof(*a.data)));++cases;
    }
    timed_contract_events::Counts n;assert(timed_contract_events::add(eligibility,n,32,4,1)&&!n.supplies);
    assert(timed_contract_events::add(eligibility,n,30,3,3599)&&n.supplies==3&&timed_contract_events::add(eligibility,n,30,1,3601)&&n.supplies==3);
    auto prior=n;assert(!timed_contract_events::add(eligibility,n,30,-1,1)&&!memcmp(&n,&prior,sizeof(n)));
    assert(!timed_contract_events::add(eligibility,n,30,1,-1)&&!memcmp(&n,&prior,sizeof(n)));
    assert(timed_contract_events::add(eligibility,n,30,INT_MAX,3600)&&n.supplies==4&&!timed_contract_events::valid(0,n));
    seed(*a.data);auto m=result(1,true,1,1,3600,4);assert(m.version==12&&m.contract_mask==(eligibility|completed));
    assert(local_match_store::write(argv[1],m));State disk;assert(local_match_store::load(argv[1],disk)==1&&disk.contract_mask==m.contract_mask);
    assert(battle_contract_policy::reconcile(*a.data,disk)&&battle_contract_policy::count(*a.data,id)==1);
    assert(battle_contract_policy::claim(*a.data,id,1200,0,0,15)&&a.data->credits==2890&&a.data->contract_points==15&&battle_contract_policy::count(*a.data,id)==0);
    assert(local_progress::write(argv[1],*a.data)&&local_progress::load(argv[1],*b.data)==1);*c.data=*b.data;
    assert(battle_contract_policy::reconcile(*b.data,disk)&&!battle_contract_policy::claim(*b.data,id,1200,0,0,15)&&!memcmp(b.data,c.data,sizeof(*b.data)));
    // A new battle must satisfy all four again. Three from each of two wins
    // never accumulate into the single-battle condition.
    for(uint64_t run:{2ull,3ull}){
        assert(local_match_store::begin_pvp(disk,true)&&local_match_store::freeze_career_stars(disk,true,4,4)&&local_match_store::freeze_victory_credits(disk,0)&&local_match_store::freeze_contracts(disk,eligibility));
        timed_contract_events::Counts partial;assert(timed_contract_events::add(eligibility,partial,30,3,1000)&&
            local_match_store::settle(disk,run,local_match_store::Victory,18000,1,1,nullptr,nullptr)&&local_match_store::stamp_contracts(disk,{},partial));
        disk.checksum=local_match_store::hash(disk);assert(battle_contract_policy::reconcile(*b.data,disk)&&!battle_contract_policy::count(*b.data,id));
    }
    for(int version:{7,8,9}){
        seed(*a.data);auto old=m;old.version=version;old.size=version==7?112:136;old.contract_mask=0;old.checksum=local_match_store::hash(old);
        assert(local_match_store::valid(old)&&local_match_store::write(argv[1],old)&&local_match_store::load(argv[1],disk)==1&&battle_contract_policy::reconcile(*a.data,disk)&&!battle_contract_policy::count(*a.data,id));
        if(version>=8){old.contract_mask=eligibility;old.checksum=local_match_store::hash(old);assert(!local_match_store::valid(old));}
    }
    seed(*a.data);*b.data=*a.data;auto bad=m;bad.contract_mask=completed;bad.checksum=local_match_store::hash(bad);assert(!local_match_store::valid(bad));
    bad=m;bad.outcome=local_match_store::Defeat;bad.checksum=local_match_store::hash(bad);assert(!local_match_store::valid(bad));
    bad=m;bad.settled=0;bad.checksum=local_match_store::hash(bad);assert(!local_match_store::valid(bad));
    bad=m;bad.contract_mask|=16384;bad.checksum=local_match_store::hash(bad);assert(!local_match_store::valid(bad));
    // Missing new receipt cannot partially advance old frozen event receipts.
    bad=m;bad.contract_mask|=31;assert(battle_contract_events::add(bad.contract_mask,bad.contract_events,27,3));bad.checksum=local_match_store::hash(bad);
    --a.data->contract_count;*b.data=*a.data;assert(!battle_contract_policy::reconcile(*a.data,bad)&&!memcmp(a.data,b.data,sizeof(*a.data)));
    seed(*a.data);auto* receipt=const_cast<local_progress::ContractReceipt*>(contract_policy::receipt(*a.data,id));receipt->baseline=1;a.data->credits=INT_MAX;*b.data=*a.data;
    assert(!battle_contract_policy::claim(*a.data,id,1200,0,0,15)&&!memcmp(a.data,b.data,sizeof(*a.data)));
    State start;assert(local_match_store::begin_pvp(start,true)&&local_match_store::freeze_career_stars(start,true,4,4)&&local_match_store::freeze_victory_credits(start,0));
    auto start_before=start;assert(!local_match_store::freeze_contracts(start,eligibility|completed)&&!memcmp(&start,&start_before,sizeof(start)));
    char absent[640];snprintf(absent,sizeof(absent),"%s/missing",argv[1]);assert(!local_match_store::write(absent,m));
    printf("PASS: timed-building21290, %d mode/source/outcome/reason/time/count cases; original30 only,4 in first6minutes/3600 ticks inclusive, same-battle victory, no partial carry, player eligibility frozen; v10/136 terminal qualification and v7/v8/v9 no-backpay; manual1200-credit15-point reset/cold duplicate; invalid/overflow/missing-receipt/failed-write atomic refusal\n",cases);
}
