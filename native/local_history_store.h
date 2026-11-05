// Immutable per-result records. These files never serve as a growth ledger.
#pragma once
#include "cooperative_store.h"
#include <dirent.h>
#include <errno.h>
#include <vector>
#include <algorithm>
namespace local_history_store {
enum Kind : uint32_t { Pvp=0, Cooperative=1 };
struct Record {
    uint32_t magic=0x3153484c,version=1,size=216,checksum=0;
    uint32_t faction=0,kind=Pvp;
    uint64_t ended_utc=0; // Zero means the old ledger did not record a date.
    uint32_t payload_size=0,reserved=0;
    uint8_t payload[176]={};
};
static_assert(sizeof(Record)==216,"History record wire changed");
inline uint32_t hash(const Record& r){
    uint32_t h=2166136261u;for(size_t i=0;i<sizeof(r);++i)
        h=(h^(i>=12&&i<16?0:((const uint8_t*)&r)[i]))*16777619u;return h;
}
inline local_match_store::State match(const Record& r){
    local_match_store::State s{};
    if(r.kind==Cooperative){cooperative_store::WireState w{};memcpy(&w,r.payload,sizeof(w));s=cooperative_store::from_wire(w).match;}
    else if(r.payload_size<=sizeof(s))memcpy(&s,r.payload,r.payload_size);
    return s;
}
inline cooperative_store::State cooperative(const Record& r){
    cooperative_store::WireState w{};memcpy(&w,r.payload,sizeof(w));return cooperative_store::from_wire(w);
}
inline bool scored(const local_match_store::State& s){
    return s.settled&&s.settled==s.next&&s.outcome>=local_match_store::Victory&&s.outcome<=local_match_store::Draw;
}
inline bool valid(const Record& r){
    if(r.magic!=0x3153484c||(r.version<1||r.version>3)||r.size!=sizeof(r)||r.checksum!=hash(r)||r.reserved||
       r.faction<1||r.faction>2||r.kind>Cooperative||r.ended_utc>253402300799ULL)return false;
    if(r.kind==Cooperative){if(r.version!=1||r.faction!=2||r.payload_size!=176||!cooperative_store::valid(cooperative(r)))return false;}
    else if((r.payload_size!=112&&r.payload_size!=136)||r.payload_size!=match(r).size||!local_match_store::valid(match(r))||match(r).mode==3)return false;
    if(r.kind==Pvp&&((r.version==1&&match(r).version>=13)||(r.version==2&&match(r).version!=13)||(r.version==3&&match(r).version!=14)))return false;
    for(size_t i=r.payload_size;i<sizeof(r.payload);++i)if(r.payload[i])return false;
    return scored(match(r));
}
inline bool make(uint32_t faction,const local_match_store::State& s,uint64_t utc,Record& out){
    if(!local_match_store::valid(s)||s.size>sizeof(s))return false;
    Record r;r.version=s.version==14?3:s.version==13?2:1;r.faction=faction;r.ended_utc=utc;r.payload_size=s.size;memcpy(r.payload,&s,s.size);r.checksum=hash(r);
    if(!valid(r))return false;out=r;return true;
}
inline bool make(const cooperative_store::State& s,uint64_t utc,Record& out){
    if(!cooperative_store::valid(s))return false;
    Record r;r.faction=2;r.kind=Cooperative;r.ended_utc=utc;r.payload_size=176;
    auto wire=cooperative_store::to_wire(s);memcpy(r.payload,&wire,sizeof(wire));r.checksum=hash(r);
    if(!valid(r))return false;out=r;return true;
}
inline bool same_result(const Record& a,const Record& b){
    return a.faction==b.faction&&a.kind==b.kind&&a.payload_size==b.payload_size&&!memcmp(a.payload,b.payload,sizeof(a.payload));
}
inline bool read(const char* path,Record& out){
    FILE* f=fopen(path,"rb");if(!f)return false;Record r;
    bool ok=fread(&r,1,sizeof(r),f)==sizeof(r)&&fgetc(f)==EOF;fclose(f);
    if(!ok||!valid(r))return false;out=r;return true;
}
inline bool filename(char* out,size_t n,const Record& r){
    return snprintf(out,n,"%s-%020llu.bin",r.kind==Cooperative?"coop":"pvp",(unsigned long long)match(r).settled)<int(n);
}
// Runtime callers serialize writes. Existing identities are checked and never
// replaced, including damaged or conflicting records. Only .tmp is replaceable.
inline bool retain(const char* root,const Record& r){
    if(!valid(r))return false;char directory[640],name[64],path[720],tmp[728];
    if(snprintf(directory,sizeof(directory),"%s/battle-history",root)>=int(sizeof(directory))||!filename(name,sizeof(name),r))return false;
    bool created=mkdir(directory,0777)==0;if(!created&&errno!=EEXIST)return false;
    if(created){int parent=open(root,O_RDONLY|O_DIRECTORY);if(parent<0)return false;bool synced=fsync(parent)==0;close(parent);if(!synced)return false;}
    snprintf(path,sizeof(path),"%s/%s",directory,name);Record previous;
    if(access(path,F_OK)==0)return read(path,previous)&&same_result(previous,r);
    if(errno!=ENOENT)return false;
    snprintf(tmp,sizeof(tmp),"%s.tmp",path);FILE* f=fopen(tmp,"wb");if(!f)return false;
    bool ok=fwrite(&r,1,sizeof(r),f)==sizeof(r);
    if(fflush(f)||fsync(fileno(f)))ok=false;if(fclose(f))ok=false;
    if(!ok||rename(tmp,path))return false;
    int d=open(directory,O_RDONLY|O_DIRECTORY);if(d<0)return false;ok=fsync(d)==0;close(d);
    return ok;
}
inline bool newer(const Record& a,const Record& b){
    if(a.ended_utc!=b.ended_utc)return a.ended_utc>b.ended_utc;
    if(a.kind!=b.kind)return a.kind<b.kind;
    return match(a).settled>match(b).settled;
}
// Read all retained records. A damaged entry is reported without hiding the
// other valid results; temporary files and unrelated files are not records.
inline bool load(const char* root,uint32_t faction,std::vector<Record>& out,unsigned& damaged){
    out.clear();damaged=0;char directory[640];
    if(snprintf(directory,sizeof(directory),"%s/battle-history",root)>=int(sizeof(directory)))return false;
    DIR* d=opendir(directory);if(!d)return errno==ENOENT;
    bool ok=true;
    for(;;){
        errno=0;auto* item=readdir(d);if(!item){ok=errno==0;break;}
        const char* name=item->d_name;size_t len=strlen(name);
        bool candidate=(len==28&&!strncmp(name,"pvp-",4))||(len==29&&!strncmp(name,"coop-",5));
        if(!candidate||strcmp(name+len-4,".bin"))continue;
        char path[720],expected[64];snprintf(path,sizeof(path),"%s/%s",directory,name);Record r;
        if(!read(path,r)||r.faction!=faction||!filename(expected,sizeof(expected),r)||strcmp(expected,name)){++damaged;continue;}
        out.push_back(r);
    }
    closedir(d);std::sort(out.begin(),out.end(),newer);return ok;
}
inline bool merge(std::vector<Record>& records,const Record& current){
    if(!valid(current))return false;
    for(const auto& r:records)if(r.kind==current.kind&&match(r).settled==match(current).settled)return same_result(r,current);
    records.push_back(current);std::sort(records.begin(),records.end(),newer);return true;
}
}
