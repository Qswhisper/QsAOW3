// Strict private job format; no player save or progression fields.
#pragma once
#include <stdio.h>
#include <string.h>
#include <stdint.h>
namespace ai_replay {
struct Seat {int controller=1,tier=1,style=1;};
struct Job {
    char id[49]={};int scenario=201,mode=1,opponent=2,max_tick=18000,growth=0;
    uint32_t policy_seed=1;int count=2;Seat seats[6];
};
inline bool read(FILE* f,Job& out){
    Job j;char magic[32]={},extra=0;unsigned version=0,seed=0;
    if(!f||fscanf(f,"%31s %u %48s %d %d %d %d %d %u %d",magic,&version,j.id,&j.scenario,&j.mode,&j.opponent,&j.max_tick,&j.growth,&seed,&j.count)!=10||
       strcmp(magic,"ART3_AI_BATCH")||version!=1||!j.id[0]||j.opponent<1||j.opponent>2||j.max_tick<1000||j.max_tick>60000||j.growth<0||j.growth>1||
       (j.mode!=1&&j.mode!=2&&j.mode!=4)||j.count!=(j.mode==1?2:j.mode==2?4:6))return false;
    for(const char* p=j.id;*p;++p)if(!((*p>='a'&&*p<='z')||(*p>='0'&&*p<='9')||*p=='-'||*p=='_'))return false;
    for(int i=0;i<j.count;++i){int id=0;auto& s=j.seats[i];
        if(fscanf(f," %d %d %d %d",&id,&s.controller,&s.tier,&s.style)!=4||id!=i+1||s.controller<0||s.controller>1||s.tier<0||s.tier>2||s.style<0||s.style>3)return false;}
    if(fscanf(f," %c",&extra)==1)return false;j.policy_seed=seed;out=j;return true;
}
}
