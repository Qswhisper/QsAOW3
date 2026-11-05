// Whitelist fresh original campaign/cooperative inputs; never PvP or rejoin.
#pragma once
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
namespace capture_start_policy {
enum Kind { None=0, Campaign=1, Cooperative=2 };
inline Kind classify(int type,int format,int players) {
    if(players!=1)return None;
    if((type==1||type==2)&&format==10)return Campaign;
    if(type==5&&format==20)return Cooperative;
    return None;
}
inline Kind fresh(int expected,int actual,int tick,int type,int format,int players,int side) {
    if(expected<0||expected!=actual||tick>0)return None;
    Kind kind=classify(type,format,players);
    if(kind==Cooperative&&(side<1||side>8))return None;
    return kind;
}
// The recorder restarts sequence at process launch. Reserve both-name space
// without overwriting old captures or incomplete pairs from earlier launches.
inline int reserve(const char* root,unsigned& sequence,unsigned& id,char* path,size_t capacity) {
    while(sequence<=9999999u){
        id=sequence++;char metadata[640];
        int n=snprintf(metadata,sizeof(metadata),"%s%06u_StartBattleNotification.json",root,id);
        if(n<0||(size_t)n>=sizeof(metadata)){errno=ENAMETOOLONG;return -1;}
        if(access(metadata,F_OK)==0)continue;
        n=snprintf(path,capacity,"%s%06u_StartBattleNotification.ggs",root,id);
        if(n<0||(size_t)n>=capacity){errno=ENAMETOOLONG;return -1;}
        int fd=open(path,O_WRONLY|O_CREAT|O_EXCL,0644);
        if(fd>=0)return fd;
        if(errno!=EEXIST)return -1;
    }
    errno=EOVERFLOW;return -1;
}
}
