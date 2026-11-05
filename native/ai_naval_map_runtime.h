// Included inside local_duel before the shared original recipe executor.
#pragma once
#include "ai_coastal_recon_plan.h"
static ai_water_plan::Plan naval_water;
static void* naval_map_battle;
static bool naval_need_area[8];
static bool naval_focus_valid[8];static ai_spawn_plan::Point naval_focus[8];
static bool naval_layout_checked[8],naval_layout_valid[8];static ai_spawn_plan::Point naval_layout_focus[8];
// Original training/formal local matches share the verified native path.
// The player seat stays outside the NPC controller.
static bool naval_enabled(){return offline_world_runtime::gameplay_scope()||naval_test;}
static bool naval_map(void* battle){
    if(naval_map_battle==battle)return naval_water.labels!=nullptr;
    naval_map_battle=battle;naval_water.reset();native_battle_grid::View grid;if(!grid.init(battle))return false;
    int w=grid.width,h=grid.height;
    uint8_t* terrain=(uint8_t*)malloc(w*h);if(!terrain)return false;bool ok=true;
    for(int y=0;y<h&&ok;++y)for(int x=0;x<w;++x){void* cell=grid.cell(x,y);if(!cell){ok=false;break;}terrain[y*w+x]=*(uint8_t*)((char*)cell+0x20);}
    ok=ok&&naval_water.init(terrain,w,h);free(terrain);
    log("NAVAL_WATER width=%d height=%d components=%d largest=%d native_terrain=1",w,h,naval_water.count,naval_water.sizes?naval_water.sizes[naval_water.largest]:0);return ok;
}
static bool naval_base_focus(void* battle,void* s,ai_spawn_plan::Point& result){
    if(!naval_enabled()||!naval_map(battle))return false;int slot=*(int*)((char*)s+0x10)-1;
    if(slot<0||slot>=8)return false;
    if(!naval_layout_checked[slot]){
        void* hq=ai_lab::building(s,base_id(s));if(!hq)return false;
        naval_layout_checked[slot]=true;
        void* type=local_dictionary_item(*(void**)((char*)s+0xf8),base_id(s)+11);
        int w=type?*(int8_t*)((char*)type+0x29):0,h=type?*(int8_t*)((char*)type+0x2a):0;
        if(w<1||h<1||w>16||h>16)return false;
        native_battle_grid::View grid;if(!grid.init(battle))return false;
        auto geometry=reinterpret_cast<bool(*)(void*,void*,int,int,int16_t,void*)>(image_base+0x4922a7c);
        int hx=*(int16_t*)((char*)hq+0x40),hy=*(int16_t*)((char*)hq+0x42),best=INT_MAX;
        for(int y=1;y+h<naval_water.height;++y)for(int x=1;x+w<naval_water.width;++x){
            if(!naval_water.footprint(x,y,w,h))continue;bool possible=false;
            bool buildable=true;
            for(int yy=y;yy<y+h&&buildable;++yy)for(int xx=x;xx<x+w;++xx)
                if(!*(bool*)((char*)grid.cell(xx,yy)+0x40)){buildable=false;break;}
            if(!buildable)continue;
            for(int16_t angle:{0,90,180,270})if(geometry(battle,type,x,y,angle,nullptr)){possible=true;break;}
            int dx=x-hx,dy=y-hy,distance=dx*dx+dy*dy;
            if(possible&&distance<best){best=distance;naval_layout_focus[slot]={x,y};naval_layout_valid[slot]=true;}
        }
        if(naval_layout_valid[slot])log("NAVAL_LAYOUT_CANDIDATE side=%d anchor=%d,%d w=%d h=%d native_geometry=1 water_footprint=1 full_site_not_yet_approved=1",slot+1,naval_layout_focus[slot].x,naval_layout_focus[slot].y,w,h);
    }
    if(!naval_layout_valid[slot])return false;result=naval_layout_focus[slot];return true;
}
