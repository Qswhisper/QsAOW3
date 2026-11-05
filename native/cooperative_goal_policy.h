#pragma once
#include <cstddef>
namespace cooperative_goal_policy {
constexpr int workers=3;
struct Scout {int unit=0,order_tick=-1000,goal=-1,explore=0,x=0,y=0;};
struct Goal {int id=0,x=0,y=0,kind=0,type=0;};
inline bool reserved(int id,const int* ids,int count){
    if(!id||!ids)return false;
    for(int i=0;i<count;++i)if(ids[i]==id)return true;
    return false;
}
inline bool consider(Goal& best,const Goal& candidate,int x,int y,int previous,const int* claims,int count){
    if(candidate.id<=0||(candidate.kind!=1&&candidate.kind!=2)||candidate.x<0||candidate.x>=100||candidate.y<0||candidate.y>=100||reserved(candidate.id,claims,count))return false;
    const auto distance=[&](const Goal& g){return (g.x-x)*(g.x-x)+(g.y-y)*(g.y-y);};
    bool take=!best.id||candidate.kind>best.kind;
    if(best.id&&candidate.kind==best.kind){
        if(candidate.id==previous)take=true;
        else if(best.id!=previous)take=distance(candidate)<distance(best)||(distance(candidate)==distance(best)&&candidate.id<best.id);
    }
    if(take)best=candidate;
    return take;
}
}
