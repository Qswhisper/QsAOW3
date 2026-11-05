#include "warpoint_rewards.h"
#include "../tests/fixtures/warpoint_fixture.h"
#include <cassert>
#include <cstdio>
#include <initializer_list>
int main(){
    using namespace warpoint_rewards;int cases=0;
    const int ticks[]={0,1,5999,6000,6001,11999,12000,12001,17999,18000,18001,INT_MAX};
    const int solo[]={0,0,0,0,10,10,10,17,17,17,25,25};
    const int team[]={0,0,0,0,20,20,20,35,35,35,50,50};
    for(const auto& original:war_rules){Rule r;assert(from_hint(original.params,4,original.minimum,original.maximum,r));++cases;
        for(int i=0;i<12;++i){int result=-1;
            assert(award(r,FormalPvp,Defeat,1,ticks[i],result)&&result==(original.source==1?solo[i]:team[i]));++cases;
            assert(award(r,FormalPvp,Victory,1,ticks[i],result)&&result==(original.source==1?100:200));++cases;
            for(Origin origin:{Training,PrivateDiagnostic})for(Outcome outcome:{Victory,Defeat,Surrender,Draw,Interrupted,Error}){
                assert(award(r,origin,outcome,1,ticks[i],result)&&result==0);++cases;}
            for(Outcome outcome:{Surrender,Draw,Interrupted,Error}){
                assert(award(r,FormalPvp,outcome,1,ticks[i],result)&&result==0);++cases;}
            assert(award(r,FormalPvp,Victory,6,ticks[i],result)&&result==0);++cases;
        }
        int result=77;assert(!award(r,FormalPvp,Victory,1,-1,result)&&result==77);++cases;
        Rule before=r;int bad[]={100,25,17,-1};assert(!from_hint(bad,4,10,100,r)&&r.victory==before.victory&&r.loss10==before.loss10);++cases;
    }
    printf("PASS: %d public war-point rule/strict-minute/source/abnormal-result boundaries; no player credit or supply policy enabled\n",cases);
}
