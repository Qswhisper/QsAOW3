#include "battle_contract_policy.h"
#include "boost_use_policy.h"
#include <cassert>
#include <cstring>
#include <cstdio>
#include <climits>
#include <initializer_list>
using local_match_store::State;
static void seed(local_progress::State& s){
    s=local_progress::seed(1690,30,197,9);assert(contract_policy::initialize(s,20010,0));
    for(const auto& rule:contract_content::battle_rules)assert(contract_policy::initialize(s,rule.id,0));
    for(const auto& rule:contract_content::result_rules)assert(contract_policy::initialize(s,rule.id,0));
    assert(contract_policy::initialize(s,21290,0));
    for(const auto& rule:contract_content::boost_rules)assert(contract_policy::initialize(s,rule.id,0));
}
static State result(int mode,bool formal,int outcome,int reason,uint32_t requested){
    State m;m.mode=mode;assert(local_match_store::begin_pvp(m,formal)&&local_match_store::freeze_career_stars(m,formal,formal?4:0,formal?4:0)&&
        local_match_store::freeze_victory_credits(m,0)&&local_match_store::freeze_contracts(m,formal?battle_contract_events::boost_mask:0));
    uint32_t used=0;for(int i=0;i<2;++i)if(requested&(1u<<i))assert(boost_contract_events::add(m.contract_mask,used,contract_content::boost_rules[i].boost));
    assert(local_match_store::settle(m,1,(local_match_store::Outcome)outcome,10000,reason,outcome==1?1:2,nullptr,nullptr)&&local_match_store::stamp_contracts(m,{},{},used));
    m.checksum=local_match_store::hash(m);assert(local_match_store::valid(m));return m;
}
int main(int argc,char** argv){
    assert(argc==2&&sizeof(State)==136&&sizeof(local_progress::State)==60280);
    struct Use {int result;bool committed,refused;int pb,pa,gb,ga;bool expected;};
    const Use uses[]={
        {0,true,false,4,3,0,0,true}, // Exact real4231/run31 observation.
        {0,true,false,1,0,0,0,true},{0,false,false,4,4,2,1,true},{0,false,false,0,0,1,0,true},
        {-1,false,true,4,4,0,0,false},{-1,true,false,4,3,0,0,false},{0,true,true,4,3,0,0,false},
        {0,false,false,4,3,0,0,false},{0,true,false,4,4,0,0,false},{0,false,false,4,4,0,0,false},
        {0,true,false,4,2,0,0,false},{0,true,false,4,5,0,0,false},{0,true,false,4,3,2,1,false},
        {0,false,false,4,3,2,1,false},{0,false,false,0,0,-1,-1,false},{0,true,false,-1,0,0,0,false}};
    for(const auto& u:uses)assert(boost_use_policy::consumed(u.result,u.committed,u.refused,u.pb,u.pa,u.gb,u.ga)==u.expected);
    local_progress::StateBuffer a,b,c;assert(a.data&&b.data&&c.data);int cases=0;
    for(int mode:{1,2,4})for(bool formal:{false,true})for(int outcome=1;outcome<=6;++outcome)for(int reason:{1,6})for(uint32_t requested:{0u,1u,2u,3u}){
        seed(*a.data);auto m=result(mode,formal,outcome,reason,requested);assert(battle_contract_policy::reconcile(*a.data,m));
        for(int i=0;i<2;++i){bool expected=formal&&outcome==1&&reason==1&&(requested&(1u<<i));assert(battle_contract_policy::count(*a.data,contract_content::boost_rules[i].id)==expected);}
        assert(a.data->credits==1690&&a.data->contract_points==0);*b.data=*a.data;assert(battle_contract_policy::reconcile(*a.data,m)&&!memcmp(a.data,b.data,sizeof(*a.data)));++cases;
    }
    assert(contract_content::boost_rules[0].id==20460&&contract_content::boost_rules[0].boost==85&&contract_content::boost_rules[1].id==20490&&contract_content::boost_rules[1].boost==105);
    for(const auto& r:contract_content::boost_rules)assert(r.value==1&&r.flags==1&&!r.faction&&r.rank_min==4&&r.rank_max==6);
    uint32_t unknown=0;assert(boost_contract_events::add(battle_contract_events::boost_mask,unknown,16)&&!unknown&&!boost_contract_events::valid(0,1));
    seed(*a.data);auto m=result(1,true,1,1,3);assert(m.version==12&&m.contract_mask==0xf00&&local_match_store::write(argv[1],m));State disk;
    assert(local_match_store::load(argv[1],disk)==1&&disk.version==12&&disk.size==136&&disk.contract_mask==m.contract_mask&&battle_contract_policy::reconcile(*a.data,disk));
    for(const auto& r:contract_content::boost_rules){
        const contract_content::Definition* d=nullptr;for(const auto& x:contract_content::definitions)if(x.id==r.id)d=&x;
        assert(d&&d->kind==8&&d->param==r.boost&&d->rewards[0].type==0&&!d->rewards[0].id&&d->points==10);
        assert(battle_contract_policy::claim(*a.data,r.id,d->rewards[0].amount,0,0,d->points)&&!battle_contract_policy::claim(*a.data,r.id,d->rewards[0].amount,0,0,d->points));
    }
    assert(a.data->credits==2590&&a.data->contract_points==20&&local_progress::write(argv[1],*a.data)&&local_progress::load(argv[1],*b.data)==1);
    *c.data=*b.data;assert(battle_contract_policy::reconcile(*b.data,disk)&&!memcmp(b.data,c.data,sizeof(*b.data)));
    assert(local_match_store::begin_pvp(disk,true)&&local_match_store::freeze_career_stars(disk,true,4,4)&&local_match_store::freeze_victory_credits(disk,0)&&local_match_store::freeze_contracts(disk,battle_contract_events::boost_mask));
    assert(local_match_store::settle(disk,2,local_match_store::Victory,10000,1,1,nullptr,nullptr)&&local_match_store::stamp_contracts(disk,{}));disk.checksum=local_match_store::hash(disk);
    assert(battle_contract_policy::reconcile(*b.data,disk));for(const auto& r:contract_content::boost_rules)assert(!battle_contract_policy::count(*b.data,r.id));
    // v8 events, v9 fast victory and v10 timed building retain their exact old
    // semantics. They cannot retroactively earn either newly supported task.
    for(int version:{7,8,9,10}){
        seed(*a.data);auto old=m;old.version=version;old.size=version==7?112:136;old.contract_mask=version==8?2:version==9?32:version==10?192:0;old.contract_events={};
        if(version==8)old.contract_events.values[1]=3;old.checksum=local_match_store::hash(old);
        assert(local_match_store::valid(old)&&local_match_store::write(argv[1],old)&&local_match_store::load(argv[1],disk)==1&&battle_contract_policy::reconcile(*a.data,disk));
        assert(battle_contract_policy::count(*a.data,20760)==(version==8?3:0)&&battle_contract_policy::count(*a.data,20080)==(version==9?1:0)&&battle_contract_policy::count(*a.data,21290)==(version==10?1:0));
        for(const auto& r:contract_content::boost_rules)assert(!battle_contract_policy::count(*a.data,r.id));
        old.contract_mask=battle_contract_events::boost_mask;old.checksum=local_match_store::hash(old);assert(!local_match_store::valid(old));
    }
    seed(*a.data);*b.data=*a.data;auto bad=m;bad.contract_mask=boost_contract_events::completed(0);bad.checksum=local_match_store::hash(bad);assert(!local_match_store::valid(bad));
    bad=m;bad.outcome=local_match_store::Defeat;bad.checksum=local_match_store::hash(bad);assert(!local_match_store::valid(bad));
    bad=m;bad.contract_mask|=16384;bad.checksum=local_match_store::hash(bad);assert(!local_match_store::valid(bad));
    --a.data->contract_count;*b.data=*a.data;assert(!battle_contract_policy::reconcile(*a.data,m)&&!memcmp(a.data,b.data,sizeof(*a.data)));
    seed(*a.data);assert(battle_contract_policy::reconcile(*a.data,m));a.data->credits=INT_MAX;*b.data=*a.data;
    assert(!battle_contract_policy::claim(*a.data,20460,600,0,0,10)&&!memcmp(a.data,b.data,sizeof(*a.data)));
    State start;assert(local_match_store::begin_pvp(start,true)&&local_match_store::freeze_career_stars(start,true,4,4)&&local_match_store::freeze_victory_credits(start,0));auto prior=start;
    assert(!local_match_store::freeze_contracts(start,0xf00)&&!memcmp(&prior,&start,sizeof(start)));
    char absent[640];snprintf(absent,sizeof(absent),"%s/missing",argv[1]);assert(!local_match_store::write(absent,m));
    printf("PASS: type9 original85/105,16 consumed-card boundary fixtures including real16 observation, %d mode/source/outcome/reason/use cases; frozen eligibility/normal victory v12/136; legacy v7/v8/v9/v10 no-backpay with old goal preservation; manual900-credit20-point/cold duplicate/reset/new behavior; missing/invalid/overflow/failed-write refusal\n",cases);
}
