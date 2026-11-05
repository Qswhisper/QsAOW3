// Independent cooperative points and match result, committed in one slot.
#pragma once
#include "local_match_store.h"
#include "cooperative_credit_policy.h"
#include <stddef.h>
namespace cooperative_store {
using local_match_store::Outcome;
// Cooperative wire v2 remains176 bytes. The formal-only event suffix does
// not belong in cooperative files, even though its in-memory match grew.
struct WireState {
    uint32_t magic,version,size,checksum;
    uint64_t generation;
    uint8_t match[112];
    uint64_t points;
    uint32_t award,pumpkins,first_win,policy;
    uint64_t credit_run;
    uint32_t credit_level,credits;
};
static_assert(offsetof(WireState,credit_run)==160&&sizeof(WireState)==176,"Cooperative wire ABI changed");
struct State {
    uint32_t magic=0x31504f43,version=2,size=sizeof(WireState),checksum=0;
    uint64_t generation=0;
    local_match_store::State match;
    uint64_t points=0;
    uint32_t award=0,pumpkins=UINT32_MAX,first_win=0,policy=1;
    uint64_t credit_run=0;
    uint32_t credit_level=0,credits=0;
};
static_assert(offsetof(State,match)==24&&offsetof(State,points)==160&&offsetof(State,credit_run)==184&&sizeof(State)==200,"Cooperative memory layout changed");
inline WireState to_wire(const State& s){
    WireState wire{};memcpy(&wire,&s,24);memcpy(wire.match,&s.match,112);
    memcpy((char*)&wire+136,(const char*)&s+offsetof(State,points),40);return wire;
}
inline State from_wire(const WireState& wire){
    State s{};memcpy(&s,&wire,24);memcpy(&s.match,wire.match,112);
    memcpy((char*)&s+offsetof(State,points),(const char*)&wire+136,40);return s;
}
inline uint32_t hash_bytes(const State& s,size_t size){
    if(size!=160&&size!=sizeof(WireState))return 0;auto wire=to_wire(s);
    uint32_t h=2166136261u;
    for(size_t i=0;i<size;++i)h=(h^(i>=12&&i<16?0:((const uint8_t*)&wire)[i]))*16777619u;
    return h;
}
inline uint32_t hash(const State& s){return hash_bytes(s,sizeof(WireState));}
inline bool valid(const State& s){
    int budget=0;if(!cooperative_credit_policy::budget(s.credit_level,budget))return false;
    bool credit_ok=!s.credit_run?(!s.credit_level&&!s.credits):(s.credit_run==s.match.next&&s.credits==
        (s.match.next==s.match.settled&&s.match.outcome==local_match_store::Victory&&s.match.reason==1?uint32_t(budget):0));
    return credit_ok&&s.magic==0x31504f43&&s.version==2&&s.size==sizeof(WireState)&&s.checksum==hash(s)&&
        s.match.version<=7&&s.match.size==112&&
        s.generation==s.match.generation&&local_match_store::valid(s.match)&&
        s.match.mode==3&&s.match.scenario==973&&s.match.map==558&&s.match.opponent==1&&
        s.policy==1&&s.first_win<=1&&s.award<=100&&s.points>=s.award&&
        (s.pumpkins<=100000||(s.pumpkins==UINT32_MAX&&!s.award&&!s.points&&!s.first_win));
}
inline void seal(State& s){s.match.checksum=local_match_store::hash(s.match);s.checksum=hash(s);}
inline bool read(const char* path,State& s){
    FILE* f=fopen(path,"rb");if(!f)return false;WireState wire{};
    bool ok=fread(&wire,1,16,f)==16;bool legacy=wire.version==1&&wire.size==160;
    size_t size=legacy?160:sizeof(wire);
    ok=ok&&(legacy||(wire.version==2&&wire.size==sizeof(wire)))&&fread((char*)&wire+16,1,size-16,f)==size-16&&fgetc(f)==EOF;fclose(f);
    State c=from_wire(wire);
    if(ok&&legacy){if(c.checksum!=hash_bytes(c,160))return false;c.version=2;c.size=sizeof(wire);c.checksum=hash(c);}
    if(ok&&valid(c)){s=c;return true;}return false;
}
// 1=new ledger, 2=read-only legacy migration, 0=new profile, -1=corrupt.
inline int load(const char* root,State& s){
    State a{},b{};char p[640];
    snprintf(p,sizeof(p),"%s/cooperative.0",root);bool va=read(p,a),exists=access(p,F_OK)==0;
    snprintf(p,sizeof(p),"%s/cooperative.1",root);bool vb=read(p,b);exists|=access(p,F_OK)==0;
    if(va||vb){s=va&&(!vb||a.generation>=b.generation)?a:b;return 1;}
    if(exists)return -1; // Never fall back to an older legacy balance.
    State c{};int legacy=local_match_store::load(root,c.match);if(legacy<0)return -1;
    if(!legacy){c.match.scenario=973;c.match.map=558;c.match.opponent=1;c.match.mode=3;}
    c.generation=c.match.generation;seal(c);if(!valid(c))return -1;
    s=c;return legacy?2:0; // Historical matches have no retroactive points.
}
inline bool write(const char* root,State& s){
    State c=s;if(c.generation==UINT64_MAX)return false;
    ++c.generation;c.match.generation=c.generation;seal(c);if(!valid(c))return false;
    char p[640],t[660];snprintf(p,sizeof(p),"%s/cooperative.%u",root,unsigned(c.generation&1));
    snprintf(t,sizeof(t),"%s.tmp",p);FILE* f=fopen(t,"wb");if(!f)return false;
    auto wire=to_wire(c);bool ok=fwrite(&wire,1,sizeof(wire),f)==sizeof(wire);
    if(fflush(f)||fsync(fileno(f)))ok=false;if(fclose(f))ok=false;
    if(!ok||rename(t,p))return false;
    int d=open(root,O_RDONLY|O_DIRECTORY);if(d<0)return false;ok=fsync(d)==0;close(d);
    if(ok)s=c;return ok;
}
inline bool begin(State& s){State c=s;if(!local_match_store::begin(c.match))return false;c.credit_run=0;c.credit_level=c.credits=0;s=c;return true;}
inline bool freeze_credits(State& s,int level){int budget=0;
    if(s.match.next<=s.match.settled||s.credit_run||!cooperative_credit_policy::budget(level,budget))return false;
    s.credit_run=s.match.next;s.credit_level=level;s.credits=0;return true;
}
inline bool settle(State& s,uint64_t run,Outcome outcome,int tick,int reason,int winner,
                   const int* buildings,const int* units,uint32_t pumpkins){
    if(pumpkins>100000)return false;
    State c=s;
    uint32_t award=outcome==local_match_store::Victory?(c.first_win?50:100):
        (outcome==local_match_store::Defeat&&reason==1?(pumpkins<5?pumpkins:5)*5:0);
    if(c.points>UINT64_MAX-award||!local_match_store::settle(c.match,run,outcome,tick,reason,winner,buildings,units))return false;
    c.points+=award;c.award=award;c.pumpkins=pumpkins;
    int credits=0;if(c.credit_run==run&&outcome==local_match_store::Victory&&reason==1&&!cooperative_credit_policy::budget(c.credit_level,credits))return false;
    c.credits=credits;
    if(outcome==local_match_store::Victory)c.first_win=1;
    s=c;return true;
}
}
