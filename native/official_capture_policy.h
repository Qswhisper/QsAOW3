// Scope for the existing in-place recorder. Campaign export is opt-in and
// pinned to one ID; the shared fresh-start validator still checks live input.
#pragma once
#include "capture_start_policy.h"
namespace official_capture_policy {
struct Scope {
    int campaign=-1;
    bool select(int id){if(id<0)return false;campaign=id;return true;}
    void clear(){campaign=-1;}
    bool allows(int mission,int type,int format,int players,int difficulty)const{
        auto kind=capture_start_policy::classify(type,format,players);
        if(campaign<0)return kind==capture_start_policy::Cooperative;
        return mission==campaign&&difficulty==0&&kind==capture_start_policy::Campaign;
    }
};
}
