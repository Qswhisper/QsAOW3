// Wallet-owned progress and receipts for original, non-repeating reward tracks.
#pragma once
#include "supply_types.h"
namespace competition_track_types {
constexpr uint32_t capacity=16,rows=20;
struct Track {
    int32_t competition=0,definition=0,points=0,reserved=0;
    uint64_t cursor=0,started=0;
    uint32_t taken[2]={};
};
struct Ledger {
    uint32_t count=0,pending=0,pending_index=0,pending_premium=0,grant_count=0,reserved=0;
    uint64_t claims=0;
    Track tracks[capacity];
    supply_types::Grant grants[128];
};
static_assert(sizeof(Track)==40&&sizeof(Ledger)==2208,"Track storage ABI changed");
inline uint32_t bits(uint32_t n){uint32_t out=0;while(n){out+=n&1u;n>>=1;}return out;}
inline bool valid(const Ledger& s){
    if(s.count>capacity||s.reserved||s.pending>s.count||s.pending_index>=rows||s.pending_premium>1||s.grant_count>128)return false;
    uint64_t claims=0;
    for(uint32_t i=0;i<s.count;++i){const auto& t=s.tracks[i];
        if(t.competition<=0||t.definition<=0||t.points<0||t.reserved||(t.started&&t.started<=t.cursor)||
           (t.taken[0]>>rows)||(t.taken[1]>>rows))return false;
        for(uint32_t j=0;j<i;++j)if(t.competition==s.tracks[j].competition)return false;
        claims+=bits(t.taken[0])+bits(t.taken[1]);
    }
    if(claims!=s.claims)return false;
    if(!s.pending&&(s.pending_index||s.pending_premium||s.grant_count))return false;
    if(s.pending&&!(s.tracks[s.pending-1].taken[s.pending_premium]&(1u<<s.pending_index)))return false;
    for(uint32_t i=0;i<s.grant_count;++i){const auto& g=s.grants[i];
        if(g.id<0||g.amount<=0||!((g.type>=0&&g.type<=2&&g.id==0)||(inventory_types::stock(g.type)&&g.type!=11)))return false;}
    return true;
}
}
