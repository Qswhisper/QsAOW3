// Explicit two-team layout; player seat 1, allies odd, enemies even.
#pragma once
#include "local_battle_format.h"
namespace ai_team_policy {
inline int seats(int mode){return local_battle_format::seats(mode);}
inline int team(int side){return side>0&&side<=6?(side-1)%2:-1;}
inline bool allied(int a,int b){return team(a)>=0&&team(a)==team(b);}
inline bool opponent(int a,int b){return team(a)>=0&&team(b)>=0&&team(a)!=team(b);}
// A dead player seat is not a dead team while its AI ally survives.
inline int winner(const int* buildings,const int* units,int count) {
    if(count!=2&&count!=4&&count!=6)return -1;
    int alive[2]={};for(int i=0;i<count;++i)alive[team(i+1)]+=buildings[i]+units[i];
    return (alive[0]>0)!=(alive[1]>0)?(alive[0]>0?1:2):-1;
}
}
