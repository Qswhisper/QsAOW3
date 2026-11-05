#pragma once
#include "contract_box_content.generated.h"
#include "contract_box_types.h"
namespace contract_box_policy {
inline bool initialize(contract_box_types::Ledger& l,int points){
    if(!contract_box_types::valid(l,points))return false;
    if(!l.initialized){l.initialized=1;l.baseline=points;}return true;
}
inline int group(const contract_box_types::Ledger& l,int track,int rank){
    if(track<0||track>1)return 0;return l.tracks[track].group?l.tracks[track].group:(rank>=4?4:0);
}
inline int balance(const contract_box_types::Ledger& l,int track,int points){return l.initialized&&track>=0&&track<2?points-l.baseline-l.tracks[track].spent:0;}
inline const supply_content::Box* next(const contract_box_types::Ledger& l,int track,int rank){
    if(rank<4||!l.initialized||track<0||track>1||group(l,track,rank)!=4)return nullptr;
    return &contract_box_content::boxes[(track?5:0)+l.tracks[track].step];
}
inline bool can_claim(const contract_box_types::Ledger& l,int points,int track,int rank,int id){
    const auto* b=next(l,track,rank);return contract_box_types::valid(l,points)&&!l.pending&&l.claims!=UINT64_MAX&&b&&b->id==id&&balance(l,track,points)>=b->points;
}
inline bool advance(contract_box_types::Ledger& l,int points,int track,int rank,int id){
    if(!can_claim(l,points,track,rank,id))return false;
    auto& t=l.tracks[track];const auto* b=next(l,track,rank);++l.claims;++t.step;t.group=b->rank;
    if(t.step==uint32_t(track?2:5)){if(t.cycles==UINT64_MAX)return false;t.spent+=b->points;++t.cycles;t.step=0;t.group=0;}
    l.last_box=id;l.pending=1;return true;
}
}
