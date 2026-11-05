// Future random boxes contain effective rewards only.
// Original definitions and historical draw/grant journals remain intact.
#pragma once
namespace effective_box_policy {
inline bool removed(int type,int id){
    return (id==0&&(type==8||type==9||type==10||type==20||type==21))||(type==25&&id==1);
}
inline bool future_grant(int type){return type!=7&&type!=11&&type!=15;}
}
