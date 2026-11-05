// Reuse the reviewed weapon token parser and accuracy formula for building weapons.
#pragma once
#include "weapon_research_effects.h"
namespace research_effects {
inline bool building_weapon_add(const WeaponParameters& a,const WeaponParameters& b,WeaponParameters& out){
    WeaponParameters next;auto add=[](int x,int y,int limit,int& z){
        int64_t n=(int64_t)x+y;if(x<0||y<0||n>limit)return false;z=(int)n;return true;};
    for(int i=0;i<3;++i)if(!add(a.damage[i],b.damage[i],INT_MAX,next.damage[i])||
        !add(a.accuracy[i],b.accuracy[i],100,next.accuracy[i]))return false;
    if(!add(a.range,b.range,INT_MAX,next.range)||!add(a.explosion,b.explosion,INT_MAX,next.explosion)||
        !add(a.reload,b.reload,INT16_MAX,next.reload)||!add(a.shot_start,b.shot_start,INT16_MAX,next.shot_start))return false;
    out=next;return true;
}
inline bool building_weapon_values(const WeaponParameters& base,const WeaponParameters& delta,int type,int guided,int walking,WeaponParameters& out){
    WeaponParameters next=base;auto add=[](int x,int y,int& z){
        if(x<0||y<0||(int64_t)x+y>INT_MAX)return false;z=x+y;return true;};
    for(int i=0;i<3;++i)if(!add(base.damage[i],delta.damage[i],next.damage[i]))return false;
    if(!add(base.range,delta.range,next.range)||!add(base.explosion,delta.explosion,next.explosion))return false;
    if(delta.reload<0||base.reload<=delta.reload||base.reload>INT16_MAX)return false;
    next.reload=base.reload-delta.reload;
    if(!weapon_shot_start(base.shot_start,delta.shot_start,next.shot_start))return false;
    int64_t padded=next.range>1000?int64_t(next.range)+50:int64_t(next.range)*105/100;
    if(next.range<=0||padded>46340||next.explosion>46340)return false;
    if(delta.accuracy[0]||delta.accuracy[1]||delta.accuracy[2]){
        const char* why=nullptr;int accuracy[3];
        if(!weapon_accuracy_apply(type,guided,walking,base.accuracy,delta.accuracy,accuracy,why))return false;
        for(int i=0;i<3;++i)next.accuracy[i]=accuracy[i];
    }
    out=next;return true;
}
}
