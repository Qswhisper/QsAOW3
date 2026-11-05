// Public original WarPointsInfo rules; supply-cycle/draw policies stay separate.
#pragma once
#include <climits>
namespace warpoint_rewards {
struct Rule {int victory=0,loss10=0,loss20=0,loss30=0;};
enum Origin {Training,FormalPvp,PrivateDiagnostic};
enum Outcome {Victory,Defeat,Surrender,Draw,Interrupted,Error};
inline bool from_hint(const int* p,int count,int minimum,int maximum,Rule& out){
    if(!p||count!=4||minimum<=0||maximum<=0||minimum!=p[3]||maximum!=p[0]||
       p[3]<=0||p[3]>p[2]||p[2]>p[1]||p[1]>p[0])return false;
    Rule next{p[0],p[3],p[2],p[1]};out=next;return true;
}
inline bool valid(const Rule& r){
    return r.loss10>0&&r.loss10<=r.loss20&&r.loss20<=r.loss30&&r.loss30<=r.victory;
}
inline bool award(const Rule& r,Origin origin,Outcome outcome,int native_reason,int ticks,int& out){
    if(!valid(r)||ticks<0||origin<Training||origin>PrivateDiagnostic||outcome<Victory||outcome>Error)return false;
    int next=0;
    if(origin==FormalPvp&&native_reason==1){
        if(outcome==Victory)next=r.victory;
        else if(outcome==Defeat)next=ticks>18000?r.loss30:ticks>12000?r.loss20:ticks>6000?r.loss10:0;
    }
    out=next;return true;
}
}
