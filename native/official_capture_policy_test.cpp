#include "official_capture_policy.h"
#include <assert.h>
#include <stdio.h>
int main(){
    official_capture_policy::Scope scope;
    assert(scope.allows(973,5,20,1,0));
    assert(!scope.allows(2,1,10,1,0));
    assert(!scope.select(-1)&&scope.campaign==-1);
    assert(scope.select(2));
    assert(scope.allows(2,1,10,1,0)&&scope.allows(2,2,10,1,0));
    assert(!scope.allows(973,5,20,1,0));
    assert(!scope.allows(3,1,10,1,0));
    assert(!scope.allows(2,1,10,1,1));
    assert(!scope.allows(2,1,10,2,0));
    assert(!scope.allows(2,3,10,1,0));
    assert(!scope.allows(2,1,20,1,0));
    using namespace capture_start_policy;
    assert(fresh(2,2,-1,1,10,1,1)==Campaign);
    assert(fresh(2,2,1,1,10,1,1)==None);
    assert(fresh(2,3,-1,1,10,1,1)==None);
    scope.clear();assert(scope.campaign==-1&&scope.allows(973,5,20,1,0));
    puts("PASS: default cooperative scope; exact campaign/ordinary difficulty/single player only; reject other mission/PvP/rejoin/mismatched context; explicit clear restores default");
}
