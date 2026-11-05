// Rule identity and decisions over observed native control points. No results,
// counters, entities, currencies or original profiles are changed here.
#pragma once
#include <stdint.h>
#include <limits.h>
#include "local_battle_format.h"
namespace control_point_policy {
enum class Rule : int { Elimination=1,Domination=2,Maneuvers=3 };
inline bool valid_mode(int mode){return local_battle_format::valid(mode);}
inline int team_mode(int mode){return mode==5||mode==6?1:mode;}
inline Rule rule(int mode){return mode==5?Rule::Domination:mode==6?Rule::Maneuvers:Rule::Elimination;}
inline int lobby_mode(int format,int teams){
    return local_battle_format::lobby_mode(format,teams);
}
enum class Result : int { Pending=0,First=1,Second=2,Draw=3,Invalid=-1 };
inline Result terminal(Rule rules,int first_alive,int second_alive,int first_points,int second_points,int target){
    if(first_alive<0||second_alive<0||first_points<0||second_points<0||
        (rules!=Rule::Elimination&&rules!=Rule::Domination&&rules!=Rule::Maneuvers)||
        (rules==Rule::Domination&&target<=0))return Result::Invalid;
    if(!first_alive&&!second_alive)return Result::Draw;
    if(!first_alive)return Result::Second;if(!second_alive)return Result::First;
    if(rules==Rule::Domination){bool a=first_points>=target,b=second_points>=target;
        if(a&&b)return Result::Draw;if(a)return Result::First;if(b)return Result::Second;}
    return Result::Pending;
}
struct Point {int id=0,x=0,y=0,team=-1;bool active=false;};
struct Target {int id=0,index=-1;bool hold=false;};
inline bool choose(const Point* points,int count,int team,int x,int y,int radius,Target& out){
    if(!points||count<0||count>64||team<0||team>1||x<0||y<0||x>128||y>128||radius<0||radius>12)return false;
    Target next;int64_t best=INT64_MAX;
    for(int i=0;i<count;++i){const auto& p=points[i];if(!p.active||p.team==team)continue;
        if(p.id<=0||p.team< -1||p.team>1||p.x<0||p.y<0||p.x>128||p.y>128)return false;
        int64_t dx=p.x-x,dy=p.y-y,d=dx*dx+dy*dy;
        if(d<best||(d==best&&(next.index<0||p.id<next.id))){best=d;next={p.id,i,d<=int64_t(radius)*radius};}
    }
    out=next;return true;
}
}
