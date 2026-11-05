#include <initializer_list>
#include "ai_base_policy.h"
#include <assert.h>
#include <stdio.h>
int main() {
    using namespace ai_base_policy;
    Phase stages[]={Missing,Missing,Missing,Missing};uint8_t history=0;
    // Startup retains the verified sequence and cannot duplicate a queued site.
    for(int i=0;i<4;++i) {
        auto p=inspect(true,stages,history);assert(!p.reserve&&p.count==1&&p.candidates[0]==i);
        stages[i]=Constructing;p=inspect(true,stages,history);assert(p.count==0);
        stages[i]=Ready;history=inspect(true,stages,history).completed;
    }
    assert(inspect(true,stages,history).complete&&history==15);
    // Every loss combination can return to the complete base without wiping
    // surviving structures; multiple losses retain alternate prerequisite paths.
    for(unsigned lost=1;lost<16;++lost) {
        for(int i=0;i<4;++i)stages[i]=(lost&(1u<<i))?Missing:Ready;
        auto p=inspect(true,stages,history);assert(p.reserve&&!p.complete);
        unsigned requested=0;
        for(int j=0;j<p.count;++j)requested|=1u<<p.candidates[j];
        assert(requested==lost);
        for(int j=0;j<p.count;++j)stages[p.candidates[j]]=Constructing;
        p=inspect(true,stages,history);assert(!p.reserve&&p.count==0&&!p.complete);
        for(auto& stage:stages)stage=Ready;
        assert(inspect(true,stages,history).complete);
    }
    stages[0]=Missing;
    auto defeated=inspect(false,stages,history);assert(defeated.count==0&&!defeated.reserve);
    // A loss during the opening is also recoverable, before all four existed.
    stages[0]=Missing;stages[1]=Ready;stages[2]=Constructing;stages[3]=Missing;
    auto early=inspect(true,stages,3);assert(early.reserve&&early.count==2);
    puts("PASS AI base policy: native startup sequence, construction deduplication, all 15 loss combinations, prerequisite alternatives, budget reservation, no HQ resurrection, opening loss");
}
