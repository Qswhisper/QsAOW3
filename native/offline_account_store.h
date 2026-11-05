// Independent local careers. Slot 0 is the untouched pre-account legacy root.
#pragma once
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdlib.h>

namespace offline_accounts {
struct Choice {
    uint32_t magic=0x3141434c,version=1,size=64,checksum=0;
    uint64_t generation=0,active=0,previous=0,next=1;
    uint32_t has_previous=0,reserved[3]={};
};
static_assert(sizeof(Choice)==64,"Account selector wire layout");
static Choice choice;
static char base[512],career[640],alliance[672],cooperative[672];
static bool ready=false,private_session=false;
inline uint32_t hash(const Choice& s){uint32_t h=2166136261u;const auto* p=(const unsigned char*)&s;
    for(size_t i=0;i<sizeof(s);++i)h=(h^(i>=12&&i<16?0:p[i]))*16777619u;return h;}
inline bool valid(const Choice& s){return s.magic==0x3141434c&&s.version==1&&s.size==sizeof(s)&&s.checksum==hash(s)&&s.generation&&
    s.next&&s.active<s.next&&s.has_previous<=1&&(!s.has_previous||(s.previous<s.next&&s.active!=s.previous))&&
    !s.reserved[0]&&!s.reserved[1]&&!s.reserved[2];}
inline bool directory(const char* p){struct stat st{};return stat(p,&st)==0&&S_ISDIR(st.st_mode);}
inline bool make_directory(const char* p){return !mkdir(p,0700)||(errno==EEXIST&&directory(p));}
inline bool root_for(uint64_t id,char* out,size_t n){
    return snprintf(out,n,id?"%saccounts/slot-%020llu/":"%s",base,(unsigned long long)id)<int(n);
}
inline bool publish_paths(){
    if(!root_for(choice.active,career,sizeof(career))||!directory(career))return false;
    return snprintf(alliance,sizeof(alliance),"%scareer-f1",career)<int(sizeof(alliance))&&
           snprintf(cooperative,sizeof(cooperative),"%scooperative-f2",career)<int(sizeof(cooperative));
}
inline int load(Choice& out){bool present=false,found=false;
    for(int i=0;i<2;++i){char p[600];snprintf(p,sizeof(p),"%slocal-accounts.%d",base,i);FILE* f=fopen(p,"rb");
        if(!f){if(errno!=ENOENT)return -1;continue;}present=true;Choice candidate;
        bool ok=fread(&candidate,1,sizeof(candidate),f)==sizeof(candidate)&&fgetc(f)==EOF;fclose(f);
        if(ok&&valid(candidate)&&(!found||candidate.generation>out.generation)){out=candidate;found=true;}}
    return found?1:present?-1:0;
}
inline bool write(Choice& next){
    if(next.generation==UINT64_MAX)return false;Choice sealed=next;++sealed.generation;sealed.checksum=hash(sealed);
    if(!valid(sealed))return false;char temp[600],p[600];snprintf(temp,sizeof(temp),"%slocal-accounts.tmp",base);
    snprintf(p,sizeof(p),"%slocal-accounts.%d",base,int(sealed.generation&1));FILE* f=fopen(temp,"wb");if(!f)return false;
    bool ok=fchmod(fileno(f),0644)==0&&fwrite(&sealed,1,sizeof(sealed),f)==sizeof(sealed);
    if(fflush(f)||fsync(fileno(f)))ok=false;if(fclose(f))ok=false;if(!ok||rename(temp,p))return false;
    int fd=open(base,O_RDONLY|O_DIRECTORY);if(fd<0)return false;ok=!fsync(fd);close(fd);if(!ok)return false;next=sealed;return true;
}
inline bool legacy_exists(){char p[600];for(int i=0;i<2;++i){snprintf(p,sizeof(p),"%slocal-progress.%d",base,i);if(!access(p,F_OK))return true;}return false;}
inline bool create(){
    if(!ready)return false;char parent[600],p[640];snprintf(parent,sizeof(parent),"%saccounts",base);if(!make_directory(parent))return false;
    Choice next=choice;uint64_t id=next.next;
    for(int tries=0;tries<1024;++tries,++id){if(id==UINT64_MAX||!root_for(id,p,sizeof(p)))return false;
        if(!mkdir(p,0700)){next.previous=next.active;next.has_previous=1;next.active=id;next.next=id+1;
            if(!write(next))return false;choice=next;return publish_paths();}
        if(errno!=EEXIST)return false;}
    return false;
}
inline bool previous(){
    if(!ready||!choice.has_previous)return false;char p[640];if(!root_for(choice.previous,p,sizeof(p))||!directory(p))return false;
    auto next=choice;next.active=choice.previous;next.previous=choice.active;if(!write(next))return false;choice=next;return publish_paths();
}
inline uint64_t cycle_target(){
    uint64_t above=UINT64_MAX,first=legacy_exists()?0:UINT64_MAX;char p[600];snprintf(p,sizeof(p),"%saccounts",base);
    DIR* d=opendir(p);if(d){while(auto* entry=readdir(d)){
        if(strlen(entry->d_name)!=25||strncmp(entry->d_name,"slot-",5))continue;
        bool digits=true;for(int i=5;i<25;++i)digits&=entry->d_name[i]>='0'&&entry->d_name[i]<='9';if(!digits)continue;
        char* end=nullptr;errno=0;uint64_t id=strtoull(entry->d_name+5,&end,10);if(errno||!id||id>=choice.next)continue;
        char dir[640];if(!root_for(id,dir,sizeof(dir))||!directory(dir))continue;
        if(id<first)first=id;if(id>choice.active&&id<above)above=id;
    }closedir(d);}
    return above!=UINT64_MAX?above:first;
}
inline bool cycle(){
    uint64_t target=cycle_target();if(!ready||target==UINT64_MAX||target==choice.active)return false;
    Choice next=choice;next.previous=next.active;next.has_previous=1;next.active=target;
    if(!write(next))return false;choice=next;return publish_paths();
}
inline bool initialize(const char* root,bool isolated=false){
    ready=false;private_session=isolated;if(!root||!*root||strlen(root)>sizeof(base)-2)return false;
    snprintf(base,sizeof(base),"%s%s",root,root[strlen(root)-1]=='/'?"":"/");if(!directory(base))return false;
    Choice state;int status=load(state);if(status<0)return false;
    choice=status?state:Choice{};ready=true;
    if(!status){bool legacy=legacy_exists();if(!write(choice)){ready=false;return false;}
        if(!legacy){if(!create()){ready=false;return false;}choice.has_previous=0;if(!write(choice)){ready=false;return false;}}}
    return publish_paths();
}
inline const char* root(){return ready?career:nullptr;}
inline const char* faction_root(int faction){return ready?(faction==1?alliance:career):nullptr;}
inline const char* cooperative_root(){return ready?cooperative:nullptr;}
inline bool fresh_account(){return ready&&choice.active!=0;}
}
