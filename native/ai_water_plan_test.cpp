#include "ai_water_plan.h"
#include <cassert>
#include <cstdio>
int main(){
    uint8_t cells[30*20]={};for(int y=2;y<=16;++y)for(int x=2;x<=13;++x)cells[y*30+x]=2;
    for(int y=3;y<=10;++y)for(int x=20;x<=27;++x)cells[y*30+x]=2;
    ai_water_plan::Plan p;assert(p.init(cells,30,20)&&p.count==2&&p.sizes[p.largest]==180&&p.site(3,3));
    int x=-9,y=-9;assert(p.target(3,3,29,19,x,y)&&p.label(x,y)==p.label(3,3)&&x<=12&&y<=15);
    assert(p.nearest(0,0,x,y)&&x==3&&y==3);assert(p.nearest(29,19,x,y)&&x==26&&y==9);
    assert(!p.site(1,1)&&p.coastal_site(1,1)&&!p.coastal_site(18,19));
    assert(p.footprint(20,3,8,8)&&p.footprint(2,2,12,15));
    assert(!p.footprint(19,3,8,8)&&!p.footprint(2,2,13,15)&&!p.footprint(-1,2,3,3)&&!p.footprint(2,2,0,3));
    assert(!p.footprint(INT_MAX,3,3,3)&&!p.footprint(3,INT_MAX,3,3));
    x=-9;y=-9;assert(!p.target(16,12,10,10,x,y)&&x==-9&&y==-9);
    assert(p.target(22,6,0,0,x,y)&&x>=21&&y>=4&&p.label(x,y)!=p.label(3,3));
    cells[15*30+25]=2;assert(p.init(cells,30,20)&&p.count==3&&!p.site(25,15));
    x=-9;y=-9;assert(!p.target(25,15,0,0,x,y)&&x==-9&&y==-9);
    uint8_t thin[100]={};for(int i=0;i<10;++i)thin[5*10+i]=2;assert(p.init(thin,10,10)&&!p.target(1,5,8,5,x,y));
    assert(!p.init(nullptr,10,10)&&!p.labels);uint8_t dry[100]={};assert(p.init(dry,10,10)&&!p.count&&!p.site(3,3));
    puts("PASS water components; water-only footprint bounds; no crossing islands; interior destinations; no target mutation on refusal; pocket/narrow/dry-map refusal");
}
