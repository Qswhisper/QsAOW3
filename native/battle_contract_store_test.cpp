#include "battle_contract_policy.h"
#include "warpoint_store.h"
#include <cassert>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <initializer_list>
#include <pthread.h>
using local_match_store::State;
static void seed(local_progress::State& s){
    s=local_progress::seed(1690,30,197,9);
    assert(contract_policy::initialize(s,20010,0));
    for(const auto& r:contract_content::battle_rules)assert(contract_policy::initialize(s,r.id,0));
}
static State result(int mode,bool formal,int outcome,int reason,const battle_contract_events::Counts& events,bool legacy=false){
    State m;m.mode=mode;
    assert(local_match_store::begin_pvp(m,formal)&&local_match_store::freeze_career_stars(m,formal,formal?4:0,formal?4:0)&&local_match_store::freeze_victory_credits(m,formal?1:0));
    if(!legacy)assert(local_match_store::freeze_contracts(m,formal?31u:0u));
    assert(local_match_store::settle(m,1,(local_match_store::Outcome)outcome,19000,reason,outcome==1?1:2,nullptr,nullptr));
    assert(local_match_store::stamp_contracts(m,events));
    m.reserved=formal&&reason==1&&(outcome==1||outcome==2)?100:0;m.checksum=local_match_store::hash(m);assert(local_match_store::valid(m));return m;
}
static void* small_stack(void* p){
    const char* root=(const char*)p;local_progress::StateBuffer a,b;assert(a.data&&b.data);seed(*a.data);
    battle_contract_events::Counts n;assert(battle_contract_events::add(31,n,27,3));
    auto m=result(4,true,local_match_store::Victory,1,n);assert(local_match_store::write(root,m));
    assert(battle_contract_policy::reconcile(*a.data,m));bool changed=false;
    assert(warpoint_store::reconcile(*a.data,m,changed)&&changed&&local_progress::write(root,*a.data));
    State disk;assert(local_match_store::load(root,disk)==1&&local_progress::load(root,*b.data)==1);
    assert(battle_contract_policy::reconcile(*b.data,disk)&&battle_contract_policy::count(*b.data,20760)==3&&b.data->credits==4598);
    return (void*)1;
}
int main(int argc,char** argv){
    assert(argc==2&&sizeof(State)==136&&sizeof(local_progress::State)==60280);const char* root=argv[1];
    local_progress::StateBuffer a,b,c;assert(a.data&&b.data&&c.data);seed(*a.data);
    battle_contract_events::Counts events;
    for(const auto& r:contract_content::battle_rules){assert(battle_contract_events::add(31,events,r.subtype,r.value));}
    auto before_events=events;assert(battle_contract_events::add(31,events,999,-1)&&!memcmp(&events,&before_events,sizeof(events)));
    assert(!battle_contract_events::add(31,events,27,-1)&&!memcmp(&events,&before_events,sizeof(events)));
    assert(battle_contract_events::add(31,events,27,INT_MAX)&&events.values[battle_contract_policy::slot(20760)]==10);
    auto m=result(1,true,local_match_store::Victory,1,events);assert(m.version==12&&m.size==136);
    assert(local_match_store::write(root,m));State disk;assert(local_match_store::load(root,disk)==1&&disk.contract_mask==31&&disk.contract_events.values[1]==10);
    assert(battle_contract_policy::reconcile(*a.data,disk));bool changed=false;assert(warpoint_store::reconcile(*a.data,disk,changed)&&changed);
    assert(a.data->credits==4598&&a.data->stars==13&&a.data->contract_points==0&&a.data->contract_pvp_wins==1);
    assert(local_progress::write(root,*a.data)&&local_progress::load(root,*b.data)==1);*c.data=*b.data;
    assert(battle_contract_policy::reconcile(*b.data,disk)&&!memcmp(b.data,c.data,sizeof(*b.data)));
    int credit_total=0,point_total=0;
    for(const auto& rule:contract_content::battle_rules){
        assert(battle_contract_policy::count(*b.data,rule.id)==rule.value);const contract_content::Definition* d=nullptr;
        for(const auto& x:contract_content::definitions)if(x.id==rule.id)d=&x;
        assert(d&&d->kind==5&&d->reward_count==1&&d->rewards[0].type==0&&d->rewards[0].id==0);
        assert(battle_contract_policy::claim(*b.data,rule.id,d->rewards[0].amount,0,d->xp,d->points));
        assert(battle_contract_policy::count(*b.data,rule.id)==0&&contract_policy::receipt(*b.data,rule.id)->claims==1);
        int credits=b.data->credits,points=b.data->contract_points;
        assert(!battle_contract_policy::claim(*b.data,rule.id,d->rewards[0].amount,0,d->xp,d->points)&&b.data->credits==credits&&b.data->contract_points==points);
        credit_total+=d->rewards[0].amount;point_total+=d->points;
    }
    assert(credit_total==4200&&point_total==75&&b.data->credits==8798&&b.data->contract_points==75);
    assert(local_progress::write(root,*b.data)&&local_progress::load(root,*a.data)==1);
    assert(battle_contract_policy::reconcile(*a.data,disk));
    for(const auto& r:contract_content::battle_rules)assert(battle_contract_policy::count(*a.data,r.id)==0);
    auto next=disk;assert(local_match_store::begin_pvp(next,true)&&local_match_store::freeze_career_stars(next,true,4,4)&&local_match_store::freeze_victory_credits(next,1)&&local_match_store::freeze_contracts(next,31));
    assert(local_match_store::settle(next,2,local_match_store::Victory,20000,1,1,nullptr,nullptr)&&local_match_store::stamp_contracts(next,{}));
    next.checksum=local_match_store::hash(next);assert(battle_contract_policy::reconcile(*a.data,next));
    for(const auto& r:contract_content::battle_rules)assert(battle_contract_policy::count(*a.data,r.id)==0); // No reuse of old battle events.
    int cases=0;
    for(int mode:{1,2,4})for(bool formal:{false,true})for(int outcome=1;outcome<=6;++outcome)for(int reason:{1,6}){
        seed(*a.data);auto candidate=result(mode,formal,outcome,reason,events);
        assert(battle_contract_policy::reconcile(*a.data,candidate));
        bool normal=formal&&reason==1&&(outcome==1||outcome==2||outcome==4);
        for(const auto& r:contract_content::battle_rules){int expected=normal&&(!(r.flags&1)||outcome==1)?r.value:0;assert(battle_contract_policy::count(*a.data,r.id)==expected);}
        assert(a.data->contract_pvp_wins==(uint64_t)(formal&&mode==1&&reason==1&&outcome==1));++cases;
    }
    // Old v7 keeps exactly112 wire bytes and contributes no newly supported events.
    seed(*a.data);auto old=result(1,true,1,1,{},true);assert(old.version==7&&old.size==112&&local_match_store::write(root,old));
    assert(local_match_store::load(root,disk)==1&&disk.version==7&&disk.size==112&&battle_contract_policy::reconcile(*a.data,disk));
    for(const auto& r:contract_content::battle_rules)assert(battle_contract_policy::count(*a.data,r.id)==0);
    seed(*a.data);*b.data=*a.data;auto gap=result(1,true,1,1,events);gap.next=gap.settled=2;gap.checksum=local_match_store::hash(gap);
    assert(!battle_contract_policy::reconcile(*a.data,gap)&&!memcmp(a.data,b.data,sizeof(*a.data)));
    auto corrupt=result(1,true,1,1,events);corrupt.contract_events.values[1]=11;corrupt.checksum=local_match_store::hash(corrupt);
    assert(!local_match_store::valid(corrupt)&&!battle_contract_policy::reconcile(*a.data,corrupt)&&!memcmp(a.data,b.data,sizeof(*a.data)));
    auto missing=result(1,true,1,1,events);a.data->contract_count=1;*b.data=*a.data;
    assert(!battle_contract_policy::reconcile(*a.data,missing)&&!memcmp(a.data,b.data,sizeof(*a.data)));
    seed(*a.data);*b.data=*a.data;assert(battle_contract_policy::reconcile(*b.data,missing));
    char absent[640];snprintf(absent,sizeof(absent),"%s/missing",root);assert(!local_progress::write(absent,*b.data)&&a.data->contract_pvp_cursor==0);
    char thread_root[640];snprintf(thread_root,sizeof(thread_root),"%s/stack",root);assert(mkdir(thread_root,0700)==0);
    pthread_attr_t attr;assert(!pthread_attr_init(&attr)&&!pthread_attr_setstacksize(&attr,65536));pthread_t thread;
    assert(!pthread_create(&thread,&attr,small_stack,thread_root));void* completed=nullptr;assert(!pthread_join(thread,&completed)&&completed==(void*)1);pthread_attr_destroy(&attr);
    printf("PASS: original5 battle contracts, %d mode/source/outcome/reason cases, win-only filtering, frozen events/cold136-byte journal, legacy112-byte no-backpay, manual4200 credits/75 points/reset, new behavior, cursor/overflow/failed-write guards, shared wallet and64KiB cold reconciliation\n",cases);
}
