// Original BattleMap stores row[y][x]. Square inputs must not hide a transpose.
#pragma once
#include <stddef.h>
#include <stdint.h>
namespace native_battle_grid {
inline size_t length(void* array){return array?*(uintptr_t*)((char*)array+24):0;}
struct View {
    void* rows=nullptr;int width=0,height=0;
    bool init_rows(void* value,int w,int h){
        rows=nullptr;width=height=0;
        if(!value||w<1||h<1||w>128||h>128||length(value)!=(size_t)h)return false;
        for(int y=0;y<h;++y){void* row=((void**)((char*)value+32))[y];if(!row||length(row)!=(size_t)w)return false;}
        rows=value;width=w;height=h;return true;
    }
    bool init(void* battle){
        void* map=battle?*(void**)((char*)battle+0x160):nullptr;
        return map?init_rows(*(void**)((char*)map+0x18),*(int16_t*)((char*)map+0x14),*(int16_t*)((char*)map+0x16)):init_rows(nullptr,0,0);
    }
    void* cell(int x,int y)const{
        if(!rows||x<0||y<0||x>=width||y>=height)return nullptr;
        void* row=((void**)((char*)rows+32))[y];return ((void**)((char*)row+32))[x];
    }
};
}
