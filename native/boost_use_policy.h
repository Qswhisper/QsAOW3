// A successful original command must actually consume exactly one card.
#pragma once
namespace boost_use_policy {
inline bool consumed(int result,bool committed,bool refused,int paid_before,int paid_after,int grant_before,int grant_after){
    if(result||refused||paid_before<0||paid_after<0||grant_before<0||grant_after<0)return false;
    return committed?(paid_before>0&&paid_after==paid_before-1&&grant_after==grant_before):
        (grant_before>0&&grant_after==grant_before-1&&paid_after==paid_before);
}
}
