#include "ai_tech_policy.h"
#include <assert.h>
#include <stdio.h>
int main(){
    using namespace ai_tech_policy;
    // A destroyed/unfinished factory, an active base defense, or an army below
    // the existing three-unit reserve must keep recruitment available.
    int cases=0;
    assert(!extended(false,false));++cases;
    assert(extended(true,false)&&extended(false,true));++cases;
    for(int hq=0;hq<6;++hq){
        assert(next(hq,false,0,8,false)==None);++cases;
        assert(next(hq,true,0,2,false)==None);++cases;
        assert(next(hq,true,0,8,true)==None);++cases;
    }
    assert(next(1,true,0,3,false)==Factory);++cases;
    assert(next(2,true,0,3,false)==Factory);++cases;
    assert(next(2,true,1,3,false)==None);++cases;
    assert(next(2,true,1,3,false,true)==Factory);++cases;
    assert(next(2,true,2,3,false,true)==None);++cases;
    assert(next(1,true,1,3,false)==None);++cases;
    assert(next(1,true,1,3,false,true)==Headquarters);++cases;
    assert(next(0,true,0,3,false)==None);++cases;
    assert(!recruit(false,false,3,100,99));++cases;
    assert(recruit(false,false,3,100,100));++cases;
    assert(recruit(false,true,3,100,0));++cases;
    assert(recruit(false,false,2,100,0));++cases;
    assert(recruit(true,false,3,100,0));++cases;
    for(int hq=0;hq<4;++hq)for(int factory=0;factory<4;++factory){
        assert(profile(false,hq,factory)==1);++cases;
        assert(profile(true,hq,factory)==(hq>=2&&factory>=2?3:1));++cases;
    }
    for(int hq=0;hq<4;++hq)for(int factory=0;factory<4;++factory)for(int tier=0;tier<4;++tier){
        bool ready=hq>=2&&factory>=2&&tier>=2;
        assert(air(true,hq,true,factory,tier,3,false)==ready);++cases;
        assert(!air(false,hq,true,factory,tier,3,false));++cases;
        assert(!air(true,hq,false,factory,tier,3,false));++cases;
        assert(!air(true,hq,true,factory,tier,2,false));++cases;
        assert(!air(true,hq,true,factory,tier,3,true));++cases;
    }
    printf("PASS: %d ground/air-tech decision boundaries; native prices, prerequisites and durations remain external\n",cases);
}
