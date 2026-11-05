// Decisions over observed native structures. No entity, currency or result edits.
#pragma once
#include <stdint.h>
#include <initializer_list>
namespace ai_base_policy {
enum Phase : uint8_t { Missing, Constructing, Ready };
constexpr int types[]={1103,1101,1102,1105};
struct Plan {
    int candidates[4]={}, count=0;
    uint8_t completed=0;
    bool complete=false, reserve=false;
};
inline Plan inspect(bool hq,const Phase* phases,uint8_t completed) {
    Plan p{};p.completed=completed;uint8_t missing=0;bool ready=true;
    for(int i=0;i<4;++i) {
        if(phases[i]==Ready)p.completed|=uint8_t(1u<<i);
        if(phases[i]==Missing)missing|=uint8_t(1u<<i);
        if(phases[i]!=Ready)ready=false;
    }
    p.complete=ready;
    if(!hq)return p; // No free HQ resurrection; existing troops/queues remain native.
    p.reserve=(p.completed&missing)!=0;
    if(p.reserve||p.completed==15) {
        // Try all lost requirements through the native validator. A power or
        // factory prerequisite rejection must not hide another feasible repair.
        for(int i:{1,0,2,3})if(phases[i]==Missing)p.candidates[p.count++]=i;
    } else {
        for(int i=0;i<4;++i) {
            if(phases[i]==Missing){p.candidates[p.count++]=i;break;}
            if(phases[i]==Constructing)break;
        }
    }
    return p;
}
}
