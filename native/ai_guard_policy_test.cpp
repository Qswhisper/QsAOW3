#include "ai_guard_policy.h"
#include <assert.h>
#include <stdio.h>
int main() {
    using namespace ai_guard_policy;
    Threat t{};
    consider(t,1,1,1,true,false,0,0);assert(!t.id); // Fog remains authoritative.
    consider(t,2,1,1,false,true,0,0);assert(!t.id);
    consider(t,3,8,8,true,true,0,0);assert(!t.id); // Circle, not a square.
    consider(t,4,10,0,true,true,0,0);assert(t.id==4);
    consider(t,9,4,0,true,true,0,0);assert(t.id==9);
    consider(t,7,0,4,true,true,0,0);assert(t.id==7); // Stable tie, independent of iteration.
    consider(t,8,0,4,true,true,0,0);assert(t.id==7);
    consider(t,10,400,400,true,true,399,399);assert(t.id==10); // Any surviving base.
    State state{};
    auto d=update(state,false,10);assert(!d.defending&&!d.hold&&!d.changed);
    d=update(state,true,20);assert(d.defending&&!d.hold&&d.changed);
    d=update(state,true,30);assert(d.defending&&!d.hold&&!d.changed);
    d=update(state,false,79);assert(d.defending&&d.hold&&!d.changed);
    d=update(state,false,80);assert(!d.defending&&!d.hold&&d.changed);
    d=update(state,true,81);assert(d.defending&&d.changed);
    puts("PASS AI guard policy: native visibility/death filters, base radius, nearest threat/stable ties, surviving base, immediate response, hold and attack resume");
}
