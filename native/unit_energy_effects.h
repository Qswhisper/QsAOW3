#pragma once
#include "rifle_research_effects.h"
namespace research_effects {
struct UnitEnergy {int capacity=0,recovery=0,jump=0;};
inline bool unit_energy_parse(const char* p,UnitEnergy& out){
    if(!p||!*p)return false;UnitEnergy next;unsigned seen=0;
    while(*p){int token=0,value=0;if(*p++!='{'||*p++!='#'||!number(p,token)||*p++!='<'||!number(p,value)||
        value<=0||*p++!='>'||*p++!='}')return false;
        int bit=token==20020?0:token==20021?1:token==20018?2:-1;if(bit<0||(seen&(1u<<bit)))return false;seen|=1u<<bit;
        if(bit==0)next.capacity=value;else if(bit==1)next.recovery=value;else next.jump=value;
        if(*p){if(*p!=' '&&*p!='\n'&&*p!='\r'&&*p!='?')return false;
            while(*p==' '||*p=='\n'||*p=='\r'||*p=='?')++p;if(!*p)return false;}}
    if(!seen)return false;out=next;return true;
}
inline bool unit_energy_add(const UnitEnergy& a,const UnitEnergy& b,UnitEnergy& out){
    UnitEnergy next;auto add=[](int a,int b,int limit,int& out){int64_t value=int64_t(a)+b;
        if(a<0||b<0||value>limit)return false;out=int(value);return true;};
    if(!add(a.capacity,b.capacity,INT16_MAX,next.capacity)||!add(a.recovery,b.recovery,INT16_MAX,next.recovery)||
        !add(a.jump,b.jump,INT8_MAX,next.jump))return false;out=next;return true;
}
inline bool unit_energy_any(const UnitEnergy& e){return e.capacity||e.recovery||e.jump;}
inline bool unit_energy_values(int capacity,int recovery,int jump,const UnitEnergy& e,UnitEnergy& out){
    if((e.capacity||e.recovery)&&(capacity<=0||recovery<0))return false;
    if(e.jump&&jump<=0)return false;
    // Zero recovery is an explicitly disabled state (active shield/fog), not
    // a small baseline to enable by research. Original mod171 likewise names
    // only the recovering shield state20 for en_regen, but both for en_max.
    UnitEnergy base{capacity,recovery,jump},effective=e;if(!recovery)effective.recovery=0;
    return unit_energy_add(base,effective,out);
}
}
