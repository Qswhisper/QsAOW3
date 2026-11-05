// Base threat selection over native observations, with no fog or combat edits.
#pragma once
#include <limits.h>
namespace ai_guard_policy {
constexpr int radius=10, hold_ticks=50, move_ticks=30;
struct Threat { int id=0,x=0,y=0,distance=INT_MAX; };
inline void consider(Threat& best,int id,int x,int y,bool alive,bool visible,int bx,int by) {
    if(id<=0||!alive||!visible)return;
    int dx=x-bx,dy=y-by;
    if(dx < -radius||dx > radius||dy < -radius||dy > radius)return;
    int d=dx*dx+dy*dy;
    if(d<=radius*radius&&(d<best.distance||(d==best.distance&&id<best.id)))best={id,x,y,d};
}
struct State { bool defending=false; int last_seen=-10000; };
struct Decision { bool defending,hold,changed; };
inline Decision update(State& s,bool threat,int now) {
    bool previous=s.defending;
    if(threat){s.defending=true;s.last_seen=now;}
    else if(s.defending&&now-s.last_seen>=hold_ticks)s.defending=false;
    return {s.defending,s.defending&&!threat,s.defending!=previous};
}
}
