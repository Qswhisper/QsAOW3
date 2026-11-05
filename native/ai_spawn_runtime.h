// Original footprint checks, shared by detached input audit and local seeding.
#pragma once
#include "ai_spawn_plan.h"
#include "native_battle_grid.h"
namespace ai_spawn_runtime {
struct Layout {
    int width=0,height=0,valid=0,native_valid=0,terrain[5]={},build_ground=0;uint8_t* mask=nullptr;uint8_t* transit=nullptr;
    ~Layout(){free(mask);free(transit);}
    Layout()=default;Layout(const Layout&)=delete;Layout& operator=(const Layout&)=delete;
};
static bool sample(void* battle,void* headquarters,Layout& result){
    native_battle_grid::View grid;if(!headquarters||!grid.init(battle)||grid.width<20||grid.height<20||result.mask)return false;
    int w=grid.width,h=grid.height;
    auto cell=[&](int x,int y)->void*{return grid.cell(x,y);};
    for(int x=0;x<w;++x)for(int y=0;y<h;++y){void* c=cell(x,y);if(!c)return false;int land=*(uint8_t*)((char*)c+0x20);if(land<0||land>4)return false;
        ++result.terrain[land];if(land==0&&*(bool*)((char*)c+0x40))++result.build_ground;}
    const void* method=class_method(find_class("com.geargames.aow.entities.battle.aicomm","AICommBuSet"),"$Zk",5);
    if(!method||*(uintptr_t*)method!=image_base+0x4922a7c)return false;
    result.mask=(uint8_t*)calloc(w*h,1);result.transit=(uint8_t*)calloc(w*h,1);if(!result.mask||!result.transit)return false;result.width=w;result.height=h;
    for(int y=0;y<h;++y)for(int x=0;x<w;++x){void* c=cell(x,y);int level=*(int8_t*)((char*)c+0x22);if(level<0||level>127)return false;
        if(*(uint8_t*)((char*)c+0x20)==0)result.transit[y*w+x]=(uint8_t)(level+1);}
    // ClientBattleCell's original constructor maps field20 0..4 to
    // Land/Shore/Water/Barrier/Forest, and !field40 to BuildingBarrierMask.
    // Native $Zk validates the configured exit point, not the complete building
    // footprint. Keep the explicit ground/build-flag/height checks below.
    // Cover the whole HQ conservatively, with one common level (field22).
    int bw=*(int8_t*)((char*)headquarters+0x29),bh=*(int8_t*)((char*)headquarters+0x2a);
    if(bw<1||bh<1||bw>12||bh>12)return false;int radius=((bw>bh?bw:bh)+1)/2;
    int16_t angle=0;
    for(int y=2;y<h-2;++y)for(int x=2;x<w-2;++x){
        void* ex=nullptr;void* args[]={battle,headquarters,&x,&y,&angle};void* value=runtime_invoke(method,nullptr,args,&ex);
        if(ex||!value)return false;
        if(*(bool*)((char*)value+16)){
            ++result.native_valid;bool ground=x>=radius&&y>=radius&&x+radius<w&&y+radius<h;
            int level=*(int8_t*)((char*)cell(x,y)+0x22);
            for(int dy=-radius;dy<=radius&&ground;++dy)for(int dx=-radius;dx<=radius;++dx){void* c=cell(x+dx,y+dy);
                if(*(uint8_t*)((char*)c+0x20)!=0||!*(bool*)((char*)c+0x40)||*(int8_t*)((char*)c+0x22)!=level){ground=false;break;}}
            if(ground){result.mask[y*w+x]=1;++result.valid;}
        }
    }
    return true;
}
}
