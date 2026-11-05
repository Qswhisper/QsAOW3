#include "ai_coastal_recon_plan.h"
#include <cassert>
#include <cstdio>
using ai_coastal_recon_plan::Point;
int main(){
    uint8_t a[20*12]={};for(int y=1;y<11;++y)for(int x=1;x<10;++x)a[y*20+x]=1;
    Point out{-7,-8};assert(ai_coastal_recon_plan::select(a,20,12,{2,5},10,4,3,3,out)&&out.x==9&&out.y==5);
    // A disconnected closer bank cannot win over the starting component.
    for(int y=1;y<11;++y)for(int x=12;x<19;++x)a[y*20+x]=1;
    assert(ai_coastal_recon_plan::select(a,20,12,{2,5},10,4,3,3,out)&&out.x==9);
    for(int y=1;y<11;++y)a[y*20+8]=2;
    out={-7,-8};assert(!ai_coastal_recon_plan::select(a,20,12,{2,5},10,4,3,3,out)&&out.x==-7&&out.y==-8);
    a[5*20+8]=1;assert(ai_coastal_recon_plan::select(a,20,12,{2,5},10,4,3,3,out)&&out.x==9);
    out={-7,-8};assert(!ai_coastal_recon_plan::select(a,20,12,{-1,0},10,4,3,3,out));
    assert(!ai_coastal_recon_plan::select(a,20,12,{2,5},INT_MAX,4,3,3,out));
    assert(!ai_coastal_recon_plan::select(a,20,12,{2,5},10,4,INT_MAX,3,out));
    assert(!ai_coastal_recon_plan::select(nullptr,20,12,{2,5},10,4,3,3,out)&&out.x==-7&&out.y==-8);
    puts("PASS coastal vantage uses reachable original land/height; ignores disconnected closer bank; narrow path; remote bank and invalid bounds refuse without output mutation");
}
