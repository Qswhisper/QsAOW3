// Independent local 1v1 ledger. Never contains campaign currency or progress.
#pragma once
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stddef.h>
#include "formal_credit_rewards.h"
#include "battle_contract_events.h"
#include "timed_contract_events.h"
#include "boost_contract_events.h"
#include "activity_contract_events.h"
#include "control_point_policy.h"
namespace local_match_store {
enum Outcome : int32_t { None=0, Victory=1, Defeat=2, Surrender=3, Draw=4, Interrupted=5, Error=6 };
struct State {
    uint32_t magic=0x3149414c, version=4, size=112, checksum=0; // Preserve v4..7 wire bytes.
    uint64_t generation=0, next=0, settled=0;
    int32_t outcome=None, tick=0, reason=0, winner=-1;
    uint32_t wins=0, losses=0, draws=0, exits=0;
    int32_t buildings[2]={}, units[2]={};
    int32_t scenario=201, map=4;
    int32_t opponent=2, reserved=0; // v5: immutable settled war-point award.
    int32_t mode=1, reserved2=0; // v6 stars; v7 marker30, level37[22..25], stars[8..21], rank[1..7], formal0.
    uint32_t contract_mask=0; // v8:event0..4; v9:timed-win5; v10:timed-building6/7; v11:boost8..11; v12:activity12/13.
    battle_contract_events::Counts contract_events; // v8: frozen concrete normal-result events.
};
static_assert(sizeof(State)==136,"Local AI event ledger ABI changed");
inline uint32_t hash_bytes(const State& s,size_t size) {
    uint32_t h=2166136261u;
    for(size_t i=0;i<size;++i)h=(h^(i>=12&&i<16?0:((const uint8_t*)&s)[i]))*16777619u;
    return h;
}
inline uint32_t hash(const State& s){return hash_bytes(s,s.size<=sizeof(State)?s.size:sizeof(State));}
inline bool formal(const State& s){return s.version==5?s.reserved2==1:(s.version>=6&&s.version<=12)&&(s.reserved2&1);}
inline int start_rank(const State& s){return s.version>=6&&formal(s)?(s.reserved2>>1)&127:0;}
inline int star_budget(const State& s){return s.version>=6&&formal(s)?(s.version>=7?(s.reserved2>>8)&0x3fff:s.reserved2>>8):0;}
inline int credit_level(const State& s){return s.version>=7&&formal(s)?(s.reserved2>>22)&15:0;}
inline int career_stars(const State& s){return s.version>=6&&formal(s)&&s.next==s.settled&&s.outcome==Victory&&s.reason==1?star_budget(s):0;}
inline int victory_credits(const State& s){int amount=0;return s.version>=7&&formal(s)&&s.next==s.settled&&s.outcome==Victory&&s.reason==1&&formal_credit_rewards::budget(credit_level(s),amount)?amount:0;}
inline bool normal(const State& s){return s.reason==1&&(s.outcome==Victory||s.outcome==Defeat||s.outcome==Draw);}
inline bool valid(const State& s) {
    if(s.magic!=0x3149414c||(s.version<4||s.version>14)||s.size!=(s.version>=8?sizeof(s):112)||s.checksum!=hash(s)||
       s.settled>s.next||s.outcome<None||s.outcome>Error||s.tick<0||s.winner<-1||s.winner>(s.version==14?local_battle_format::seats(s.mode):2)||s.scenario<0||s.map<=0||s.opponent<1||s.opponent>2||
       (!control_point_policy::valid_mode(s.mode)&&s.mode!=3)||
       (local_battle_format::control_points(s.mode)&&s.version!=13)||(s.version==13&&!local_battle_format::control_points(s.mode))||
       (local_battle_format::free_for_all(s.mode)&&s.version!=14)||(s.version==14&&!local_battle_format::free_for_all(s.mode)))return false;
    if(s.version==4&&(s.reserved||s.reserved2))return false;
    if(s.version>=5&&(s.reserved<0||s.reserved2<0||(s.version==5&&s.reserved2>1)||
       (s.version==6&&s.reserved2&&(!formal(s)||!start_rank(s)||star_budget(s)>0x3fff))||
       (s.version>=7&&s.reserved2&&(!formal(s)||!start_rank(s)||!(s.reserved2&(1<<30))||(s.reserved2&~0x43ffffff)||credit_level(s)>11))||
       ((!s.reserved2||s.mode==3)&&s.reserved)||
       (s.mode==3&&s.reserved2)||
       ((s.outcome!=Victory&&s.outcome!=Defeat||s.reason!=1)&&s.reserved)))return false;
    for(int i=0;i<2;++i)if(s.buildings[i]<0||s.units[i]<0)return false;
    if(s.version==14&&s.next==s.settled&&s.reason==1&&
       ((s.outcome==Victory&&s.winner!=1)||(s.outcome==Defeat&&s.winner==1)||(s.outcome==Draw&&s.winner!=-1)))return false;
    if(!battle_contract_events::valid(s.contract_mask,s.contract_events)||
       (s.version<8&&s.contract_mask)||(s.version<9&&(s.contract_mask&~battle_contract_events::event_mask))||
       (s.version<10&&(s.contract_mask&~battle_contract_events::result_mask))||
       (s.version<11&&(s.contract_mask&~battle_contract_events::v10_mask))||
       (s.version<12&&(s.contract_mask&~battle_contract_events::v11_mask))||(!formal(s)&&s.contract_mask))return false;
    if(s.contract_mask&battle_contract_events::timed_completed){
        if(!(s.contract_mask&battle_contract_events::timed_mask)||s.next!=s.settled||!normal(s)||s.outcome!=Victory)return false;}
    for(int i=0;i<boost_contract_events::count;++i)if(s.contract_mask&boost_contract_events::completed(i)){
        if(!(s.contract_mask&boost_contract_events::eligible(i))||s.next!=s.settled||!normal(s)||s.outcome!=Victory)return false;}
    if(s.contract_mask&activity_contract_events::completed){
        if(!(s.contract_mask&activity_contract_events::eligible)||s.next!=s.settled||!normal(s)||s.outcome!=Victory)return false;}
    for(int i=0;i<battle_contract_events::count;++i)if(s.contract_events.values[i]&&
        (s.version<8||s.next!=s.settled||!normal(s)))return false;
    return true;
}
inline bool read(const char* path,State& s) {
    FILE* f=fopen(path,"rb");if(!f)return false;State c{};
    bool ok=fread(&c,1,16,f)==16;
    size_t bytes=c.size;
    bool v1=c.version==1&&bytes==88,v2=c.version==2&&bytes==96,v3=c.version==3&&bytes==104,legacy=v1||v2||v3;
    ok=ok&&(legacy||(c.version>=4&&c.version<=7&&bytes==112)||((c.version>=8&&c.version<=14)&&bytes==sizeof(c)));
    if(ok)ok=fread((char*)&c+16,1,bytes-16,f)==bytes-16&&fgetc(f)==EOF;
    fclose(f);
    if(ok&&legacy) {
        ok=c.magic==0x3149414c&&c.checksum==hash_bytes(c,bytes);
        if(ok&&v3)ok=c.opponent>=1&&c.opponent<=2&&!c.reserved;
        if(ok){if(v1){c.scenario=201;c.map=4;}if(!v3)c.opponent=c.scenario==0?1:2;c.reserved=0;c.mode=1;c.reserved2=0;c.version=4;c.size=112;c.checksum=hash(c);}
    }
    if(ok&&valid(c)){s=c;return true;}return false;
}
inline int load(const char* root,State& s) {
    State a{},b{};char p[640];
    snprintf(p,sizeof(p),"%s/local-ai.0",root);bool va=read(p,a),exists=access(p,F_OK)==0;
    snprintf(p,sizeof(p),"%s/local-ai.1",root);bool vb=read(p,b);exists|=access(p,F_OK)==0;
    if(!va&&!vb)return exists?-1:0;
    s=va&&(!vb||a.generation>=b.generation)?a:b;return 1;
}
inline bool write(const char* root,State& s) {
    State c=s;if(c.generation==UINT64_MAX)return false;++c.generation;c.checksum=hash(c);
    if(!valid(c))return false;
    char p[640],t[660];snprintf(p,sizeof(p),"%s/local-ai.%u",root,unsigned(c.generation&1));
    snprintf(t,sizeof(t),"%s.tmp",p);FILE* f=fopen(t,"wb");if(!f)return false;
    bool ok=fwrite(&c,1,c.size,f)==c.size;
    if(fflush(f)||fsync(fileno(f)))ok=false;if(fclose(f))ok=false;
    if(!ok||rename(t,p))return false;
    int d=open(root,O_RDONLY|O_DIRECTORY);if(d<0)return false;ok=fsync(d)==0;close(d);
    if(ok)s=c;return ok;
}
inline bool begin(State& s) {
    if(s.next!=s.settled||s.next==UINT64_MAX)return false;
    ++s.next;s.contract_mask=0;s.contract_events={};return true;
}
inline bool begin_pvp(State& s,bool formal){
    if(s.mode==3)return false;State next=s;if(!begin(next))return false;
    next.version=5;next.size=112;next.reserved=0;next.reserved2=formal?1:0;
    // The previous receipt is archived before reservation. A six-player winner
    // must not leak into a new duel's pending outcome or invalidate its ABI.
    next.outcome=None;next.tick=next.reason=0;next.winner=-1;
    for(int i=0;i<2;++i)next.buildings[i]=next.units[i]=0;
    s=next;return true;
}
inline bool freeze_career_stars(State& s,bool is_formal,int rank,int budget){
    if(s.mode==3||s.next<=s.settled||s.version!=5||s.reserved||s.reserved2!=(is_formal?1:0)||rank<0||rank>127||budget<0||budget>0x3fff||(!is_formal&&(rank||budget))||(is_formal&&!rank))return false;
    s.version=6;s.reserved2=is_formal?1|(rank<<1)|(budget<<8):0;return true;
}
inline bool freeze_victory_credits(State& s,int level){
    if(s.version!=6||s.next<=s.settled||s.mode==3||s.reserved||level<0||level>11||(!formal(s)&&level)||star_budget(s)>0x3fff)return false;
    int rank=start_rank(s),stars=star_budget(s);if(formal(s)&&!rank)return false;
    s.version=7;s.reserved2=formal(s)?(1<<30)|(level<<22)|(stars<<8)|(rank<<1)|1:0;return true;
}
inline bool settle(State& s,uint64_t run,Outcome result,int tick,int reason,int winner,const int* buildings,const int* units) {
    if(!run||run!=s.next||run<=s.settled||result<=None||result>Error||tick<0)return false;
    s.settled=run;s.outcome=result;s.tick=tick;s.reason=reason;s.winner=winner;
    for(int i=0;i<2;++i){s.buildings[i]=buildings?buildings[i]:0;s.units[i]=units?units[i]:0;}
    if(result==Victory)++s.wins;else if(result==Defeat||result==Surrender)++s.losses;
    else if(result==Draw)++s.draws;else ++s.exits;
    return true;
}
inline bool freeze_contracts(State& s,uint32_t mask){
    if(s.version!=7||s.size!=112||s.next<=s.settled||mask&~battle_contract_events::start_mask||(!formal(s)&&mask))return false;
    if(!formal(s))return true;
    s.version=12;s.size=sizeof(State);s.contract_mask=mask;s.contract_events={};return true;
}
inline bool freeze_rules(State& s){
    if(!control_point_policy::valid_mode(s.mode)||s.next<=s.settled)return false;
    if(s.mode<5)return true;
    // Rule identity reuses mode without reinterpreting any old wire record.
    // Training control-point formats retain the no-reward contract.
    if(formal(s)||s.contract_mask||s.reserved||s.reserved2||s.version!=7||s.size!=112)return false;
    s.version=local_battle_format::free_for_all(s.mode)?14:13;s.size=sizeof(State);return true;
}
inline bool stamp_contracts(State& s,const battle_contract_events::Counts& counts,const timed_contract_events::Counts& timed={},uint32_t boost_used=0,int activity_raw=-1){
    if(activity_raw<-1)return false;
    if(s.version<8)return !s.contract_mask&&!timed.supplies&&!boost_used&&activity_raw<0;
    if(s.next!=s.settled||!battle_contract_events::valid(s.contract_mask,counts)||!timed_contract_events::valid(s.contract_mask,timed)||
       !boost_contract_events::valid(s.contract_mask,boost_used)||(s.version<10&&timed.supplies)||(s.version<11&&boost_used)||
       (activity_raw>=0&&(s.version<12||!(s.contract_mask&activity_contract_events::eligible)||!activity_contract_events::valid(activity_raw))))return false;
    s.contract_events=normal(s)?counts:battle_contract_events::Counts{};
    s.contract_mask&=~(battle_contract_events::timed_completed|battle_contract_events::boost_completed|activity_contract_events::completed);
    if(s.version>=10&&(s.contract_mask&battle_contract_events::timed_mask)&&normal(s)&&s.outcome==Victory&&timed_contract_events::complete(timed))
        s.contract_mask|=battle_contract_events::timed_completed;
    if(s.version>=11&&normal(s)&&s.outcome==Victory)s.contract_mask|=boost_contract_events::qualified(boost_used);
    if(s.version>=12&&normal(s)&&s.outcome==Victory&&activity_contract_events::qualifies(activity_raw))s.contract_mask|=activity_contract_events::completed;
    return true;
}
}
