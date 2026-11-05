#pragma once
#include "research_effects.h"
namespace research_effects {
struct RifleDelta {
    int hp = 0, armor = 0, speed = 0, sight = 0, regen = 0;
    int damage[2][3] = {};
};
inline bool rifle_add(const RifleDelta& a, const RifleDelta& b, RifleDelta& result) {
    RifleDelta next;
    // Explicit members avoid pointer arithmetic across distinct struct members.
    auto add = [](int x, int y, int& z) {
        if (x < 0 || y < 0 || (int64_t)x + y > INT_MAX) return false;
        z = x + y; return true;
    };
    if (!add(a.hp,b.hp,next.hp) || !add(a.armor,b.armor,next.armor) ||
        !add(a.speed,b.speed,next.speed) || !add(a.sight,b.sight,next.sight) ||
        !add(a.regen,b.regen,next.regen)) return false;
    for (int w = 0; w < 2; ++w) for (int t = 0; t < 3; ++t)
        if (!add(a.damage[w][t],b.damage[w][t],next.damage[w][t])) return false;
    result = next; return true;
}
inline bool rifle_any(const RifleDelta& d) {
    if (d.hp || d.armor || d.speed || d.sight || d.regen) return true;
    for (auto& w : d.damage) for (int v : w) if (v) return true;
    return false;
}
inline bool rifle_equal(const RifleDelta& a, const RifleDelta& b) {
    if (a.hp!=b.hp || a.armor!=b.armor || a.speed!=b.speed || a.sight!=b.sight || a.regen!=b.regen) return false;
    for (int w=0;w<2;++w) for(int t=0;t<3;++t) if(a.damage[w][t]!=b.damage[w][t]) return false;
    return true;
}
inline bool rifle_parse(const char* p, int slot, RifleDelta& result) {
    if (!p || slot < 0 || slot > 5 || slot == 2) return false;
    RifleDelta delta; unsigned mask=0;
    while (*p) {
        if (*p++!='{' || *p++!='#') return false;
        int token, value;
        if (!number(p,token) || *p++!='<' || !number(p,value) || *p++!='>' || *p++!='}' || value<=0) return false;
        int index = token==20000?0:token==20001?1:token==20014?2:token==20034?3:
                    token>=20004 && token<=20006?token-20004+4:-1;
        if(index<0 || (mask & (1u<<index))) return false;
        bool allowed = slot==0?(index==0 || index==2):slot==1?index==1:
                       slot==3 || slot==4?index>=4:slot==5?(index==0 || index==1 || index==3):false;
        if(!allowed) return false;
        mask |= 1u<<index;
        if(index==0) delta.hp=value; else if(index==1) delta.armor=value;
        else if(index==2) delta.speed=value; else if(index==3) delta.sight=value;
        else delta.damage[slot==4?1:0][index-4]=value;
        if (*p) {
            if (*p!=' ' && *p!='\n' && *p!='\r' && *p!='?') return false;
            while (*p==' ' || *p=='\n' || *p=='\r' || *p=='?') ++p;
            if (!*p) return false;
        }
    }
    // Whole original descriptions only. A partial damage triple is not a
    // supported upgrade, even if the individual token is recognizable.
    if(!mask || ((slot==3 || slot==4) && mask!=112)) return false;
    if(slot==0 && !(mask&1)) return false;
    if(slot==5 && !(mask&1)) return false;
    result=delta; return true;
}
} // namespace research_effects
