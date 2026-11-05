#include "battle_contract_policy.h"
#include "local_history_store.h"
#include <cassert>
#include <cstring>
#include <cstdio>
#include <string>
using local_match_store::State;
static void seed(local_progress::State& s){
    s=local_progress::seed(1690,30,197,9);assert(contract_policy::initialize(s,20010,0));
    for(const auto& d:contract_content::definitions)if(d.kind>=5)assert(contract_policy::initialize(s,d.id,0));
}
static State result(int mode,bool formal,int outcome,int reason,int raw){
    State m;m.mode=mode;assert(local_match_store::begin_pvp(m,formal)&&local_match_store::freeze_career_stars(m,formal,formal?4:0,formal?4:0)&&
        local_match_store::freeze_victory_credits(m,0)&&local_match_store::freeze_contracts(m,formal?activity_contract_events::eligible:0));
    assert(local_match_store::settle(m,1,(local_match_store::Outcome)outcome,9000,reason,outcome==1?1:2,nullptr,nullptr)&&
        local_match_store::stamp_contracts(m,{},{},0,formal?raw:-1));
    m.checksum=local_match_store::hash(m);assert(local_match_store::valid(m));return m;
}
int main(int argc,char** argv){
    assert(argc==2&&sizeof(State)==136&&sizeof(local_progress::State)==60280&&sizeof(cooperative_store::WireState)==176&&sizeof(local_history_store::Record)==216);
    const auto& rule=contract_content::activity_rules[0];assert(rule.id==20090&&rule.subtype==70&&rule.value==1&&rule.flags==3&&!rule.faction&&rule.rank_min==4&&rule.rank_max==6);
    int cases=0;local_progress::StateBuffer a,b,c;assert(a.data&&b.data&&c.data);
    for(int mode:{1,2,4})for(bool formal:{false,true})for(int outcome=1;outcome<=6;++outcome)for(int reason:{1,6})for(int raw:{0,699,700,701,710,1000}){
        seed(*a.data);auto m=result(mode,formal,outcome,reason,raw);assert(battle_contract_policy::reconcile(*a.data,m));
        bool expected=formal&&outcome==1&&reason==1&&raw>700;
        assert(battle_contract_policy::count(*a.data,20090)==expected&&a.data->credits==1690&&!a.data->contract_points);
        *b.data=*a.data;assert(battle_contract_policy::reconcile(*a.data,m)&&!memcmp(a.data,b.data,sizeof(*a.data)));++cases;
    }
    assert(!activity_contract_events::qualifies(-1)&&!activity_contract_events::qualifies(700)&&activity_contract_events::qualifies(701)&&!activity_contract_events::qualifies(1001));
    seed(*a.data);auto m=result(1,true,1,1,701);assert(m.version==12&&m.contract_mask==0x3000&&local_match_store::write(argv[1],m));State disk;
    assert(local_match_store::load(argv[1],disk)==1&&disk.version==12&&disk.size==136&&battle_contract_policy::reconcile(*a.data,disk));
    assert(battle_contract_policy::claim(*a.data,20090,1000,0,0,15)&&a.data->credits==2690&&a.data->contract_points==15&&!battle_contract_policy::claim(*a.data,20090,1000,0,0,15));
    assert(local_progress::write(argv[1],*a.data)&&local_progress::load(argv[1],*b.data)==1);*c.data=*b.data;
    assert(battle_contract_policy::reconcile(*b.data,disk)&&!memcmp(b.data,c.data,sizeof(*b.data))&&!battle_contract_policy::count(*b.data,20090));
    auto next=disk;assert(local_match_store::begin_pvp(next,true)&&local_match_store::freeze_career_stars(next,true,4,4)&&local_match_store::freeze_victory_credits(next,0)&&local_match_store::freeze_contracts(next,activity_contract_events::eligible));
    assert(local_match_store::settle(next,2,local_match_store::Victory,9000,1,1,nullptr,nullptr)&&local_match_store::stamp_contracts(next,{},{},0,700));next.checksum=local_match_store::hash(next);
    assert(battle_contract_policy::reconcile(*b.data,next)&&!battle_contract_policy::count(*b.data,20090));
    for(int version:{7,8,9,10,11}){
        seed(*a.data);auto old=m;old.version=version;old.size=version==7?112:136;old.contract_mask=version==8?2:version==9?32:version==10?192:version==11?0xf00:0;old.contract_events={};
        if(version==8)old.contract_events.values[1]=3;old.checksum=local_match_store::hash(old);assert(local_match_store::valid(old)&&battle_contract_policy::reconcile(*a.data,old)&&!battle_contract_policy::count(*a.data,20090));
        if(version==8)assert(battle_contract_policy::count(*a.data,20760)==3);
        if(version==9)assert(battle_contract_policy::count(*a.data,20080)==1);
        if(version==10)assert(battle_contract_policy::count(*a.data,21290)==1);
        if(version==11)assert(battle_contract_policy::count(*a.data,20460)==1&&battle_contract_policy::count(*a.data,20490)==1);
    }
    for(int bad_raw:{-2,1001}){auto invalid=m;assert(!local_match_store::stamp_contracts(invalid,{},{},0,bad_raw));}
    for(uint32_t mask:{0x2000u,0x3000u}){auto invalid=m;invalid.version=11;invalid.contract_mask=mask;invalid.checksum=local_match_store::hash(invalid);assert(!local_match_store::valid(invalid));}
    auto missing=m;missing.contract_mask=activity_contract_events::completed;missing.checksum=local_match_store::hash(missing);assert(!local_match_store::valid(missing));
    seed(*a.data);auto* receipt=const_cast<local_progress::ContractReceipt*>(contract_policy::receipt(*a.data,20090));receipt->baseline=2;*b.data=*a.data;
    assert(!battle_contract_policy::reconcile(*a.data,m)&&!memcmp(a.data,b.data,sizeof(*a.data)));
    receipt->baseline=1;a.data->credits=INT_MAX;*b.data=*a.data;assert(!battle_contract_policy::claim(*a.data,20090,1000,0,0,15)&&!memcmp(a.data,b.data,sizeof(*a.data)));
    std::string path=std::string(argv[1])+"/missing";assert(!local_progress::write(path.c_str(),*a.data)&&!memcmp(a.data,b.data,sizeof(*a.data)));
    local_history_store::Record r;assert(local_history_store::make(2,m,1800000000,r)&&local_history_store::retain(argv[1],r));std::vector<local_history_store::Record> rows;unsigned damaged;
    assert(local_history_store::load(argv[1],2,rows,damaged)&&rows.size()==1&&!damaged&&local_history_store::match(rows[0]).version==12);
    printf("PASS: %d activity mode/formal/outcome/reason/raw cases; strict700/701 boundary, v12/136, immutable history216, legacyv7..11 old-goal preservation/no-backpay, manual1000-credit15-point/cold/reset/new-behavior, invalid/overflow/failed-write isolation\n",cases);
}
