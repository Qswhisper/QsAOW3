#include "ai_spawn_plan.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <initializer_list>
using namespace ai_spawn_plan;
static uint8_t mask[128*128];
static uint8_t transit[128*128];
static void check(int w,int h,int count){Point p[6],again[6];assert(plan(mask,w,h,count,p));assert(plan(mask,w,h,count,again));
    assert(!memcmp(p,again,count*sizeof(Point)));
    for(int i=0;i<count;++i){assert(mask[p[i].y*w+p[i].x]);for(int j=0;j<i;++j)assert(distance(p[i],p[j])>=(i%2==j%2?100:256));}}
int main(){
    for(int w:{40,60,70,100,128})for(int h:{40,70,100}){memset(mask,1,w*h);for(int n:{2,4,6})check(w,h,n);}
    // Wide water barrier: no selecting opposite banks merely because both fit.
    memset(mask,1,70*70);for(int y=0;y<70;++y)for(int x=30;x<40;++x)mask[y*70+x]=0;
    Point p[6];assert(plan(mask,70,70,6,p));for(int i=0;i<6;++i)assert(p[i].x<30);
    // A genuinely connected, bent land corridor remains one component.
    memset(mask,1,70*70);for(int y=0;y<60;++y)for(int x=30;x<40;++x)mask[y*70+x]=0;check(70,70,6);
    // A unit-width land passage connects two base areas even though no HQ can
    // fit in the passage. Different height codes must not connect by accident.
    memset(mask,1,70*70);memset(transit,1,70*70);
    for(int y=0;y<70;++y)for(int x=30;x<40;++x){mask[y*70+x]=0;transit[y*70+x]=y==35?1:0;}
    assert(plan(mask,70,70,6,p,transit));assert(p[0].x<30&&p[1].x>=40);
    for(int y=0;y<70;++y)for(int x=35;x<70;++x)if(transit[y*70+x])transit[y*70+x]=2;
    assert(plan(mask,70,70,6,p,transit));for(int i=0;i<6;++i)assert(p[i].x<30);
    // No partial result on refusal, including tiny/disconnected islands.
    memset(mask,0,sizeof(mask));Point sentinel[6];for(auto& q:sentinel)q={-77,-88};assert(!plan(mask,70,70,6,sentinel));
    for(auto q:sentinel)assert(q.x==-77&&q.y==-88);
    assert(!plan(mask,129,70,6,sentinel));assert(!plan(mask,70,70,3,sentinel));
    puts("PASS open rectangles / native-mask islands / connected corridor / separation / determinism / refusal atomicity");
}
