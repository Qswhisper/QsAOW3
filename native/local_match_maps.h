// Explicit supported native inputs, not synthetic copies of a renamed map.
#pragma once
namespace local_match_maps {
struct Route { int chapter,map,faction; const char* label; int x1,y1,x2,y2,max_seats; };
constexpr Route routes[]={
#include "ai_map_routes.inc"
};
inline bool automatic(const Route* r){return r&&!r->x1&&!r->y1&&!r->x2&&!r->y2;}
inline int count(int faction){int n=0;for(const auto& r:routes)if(r.faction==faction)++n;return n;}
inline const Route* find(int chapter) {
    for(const auto& r:routes)if(r.chapter==chapter)return &r;
    return nullptr;
}
inline const Route* first(int faction) {
    for(const auto& r:routes)if(r.faction==faction)return &r;
    return nullptr;
}
}
