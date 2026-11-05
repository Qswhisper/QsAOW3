#include "cooperative_goal_policy.h"
#include <cassert>
#include <cstdio>
using namespace cooperative_goal_policy;
int main(){
    Goal goals[]={{11,20,20,2,4},{12,21,20,2,6},{13,22,20,2,6},{14,10,10,1,0}};
    int claims[3]={};
    for(int i=0;i<3;++i){Goal selected{};for(const auto& g:goals)consider(selected,g,10,10,0,claims,i);assert(selected.id==11+i);claims[i]=selected.id;}
    Goal only_resource{};for(const auto& g:goals)consider(only_resource,g,10,10,0,claims,3);assert(only_resource.id==14);
    Goal sticky{};consider(sticky,goals[0],10,10,12,nullptr,0);consider(sticky,goals[1],10,10,12,nullptr,0);assert(sticky.id==12);
    Goal promotion=goals[3];consider(promotion,goals[0],10,10,14,nullptr,0);assert(promotion.id==11);
    Goal unknown{};assert(!consider(unknown,{0,20,20,2,4},10,10,0,nullptr,0));assert(!consider(unknown,{20,100,20,2,4},10,10,0,nullptr,0));assert(!consider(unknown,{20,20,20,3,4},10,10,0,nullptr,0));
    Goal vanished{};consider(vanished,goals[0],10,10,99,nullptr,0);assert(vanished.id==11);
    assert(reserved(12,claims,3)&&!reserved(0,claims,3)&&!reserved(12,nullptr,3));
    std::puts("PASS cooperative objective reservation, priority, continuity and stale assignment");
}
