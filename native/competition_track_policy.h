// Only an explicitly registered new run can advance an original reward track.
#pragma once
#include <string.h>
#include "competition_track_types.h"
namespace competition_track_policy {
using competition_track_types::Ledger;
using competition_track_types::Track;
inline int index(const Ledger& s,int competition){for(uint32_t i=0;i<s.count;++i)if(s.tracks[i].competition==competition)return (int)i;return -1;}
inline bool begin(Ledger& s,int competition,int definition,uint64_t run){
    if(!competition_track_types::valid(s)||competition<=0||definition<=0||!run)return false;
    Ledger next=s;int i=index(next,competition);
    if(i<0){if(next.count==competition_track_types::capacity)return false;i=(int)next.count++;next.tracks[i]={};next.tracks[i].competition=competition;next.tracks[i].definition=definition;}
    auto& t=next.tracks[i];if(t.definition!=definition||run<=t.cursor||(t.started&&t.started!=run))return false;
    t.started=run;if(!competition_track_types::valid(next))return false;s=next;return true;
}
inline bool settle(Ledger& s,int competition,uint64_t run,bool normal_victory,int maximum,bool& changed){
    changed=false;if(!competition_track_types::valid(s)||competition<=0||!run||maximum<=0)return false;
    int i=index(s,competition);if(i<0)return true;const auto& old=s.tracks[i];
    if(run<=old.cursor||!old.started)return true;
    if(run!=old.started||old.points>maximum)return false;
    Ledger next=s;auto& t=next.tracks[i];t.cursor=run;t.started=0;
    if(normal_victory&&t.points<maximum)++t.points;
    if(!competition_track_types::valid(next))return false;s=next;changed=true;return true;
}
inline bool can_claim(const Ledger& s,int competition,int definition,int row,bool premium,int threshold){
    if(!competition_track_types::valid(s)||s.pending||row<0||row>=int(competition_track_types::rows)||threshold<=0)return false;
    int i=index(s,competition);return i>=0&&s.tracks[i].definition==definition&&s.tracks[i].points>=threshold&&!(s.tracks[i].taken[premium?1:0]&(1u<<row));
}
inline bool stage(Ledger& s,int competition,int definition,int row,bool premium,int threshold,const supply_types::Grant* grants,int count){
    if(count<0||count>128||(!grants&&count)||!can_claim(s,competition,definition,row,premium,threshold))return false;
    Ledger next=s;int i=index(next,competition);next.tracks[i].taken[premium?1:0]|=1u<<row;
    ++next.claims;next.pending=uint32_t(i+1);next.pending_index=uint32_t(row);next.pending_premium=premium?1:0;next.grant_count=uint32_t(count);
    memset(next.grants,0,sizeof(next.grants));for(int j=0;j<count;++j)next.grants[j]=grants[j];
    if(!competition_track_types::valid(next))return false;s=next;return true;
}
inline bool dismiss(Ledger& s){
    if(!competition_track_types::valid(s)||!s.pending)return false;
    s.pending=s.pending_index=s.pending_premium=s.grant_count=0;memset(s.grants,0,sizeof(s.grants));return true;
}
}
