#include "../tests/test_paths.h"
#include "capture_start_policy.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <initializer_list>
int main(){
    using namespace capture_start_policy;
    assert(fresh(388,388,-1,2,10,1,1)==Campaign);
    assert(fresh(9003,9003,0,5,20,1,2)==Cooperative);
    assert(fresh(9003,9003,-1,5,20,1,8)==Cooperative);
    assert(fresh(9003,9003,1,5,20,1,2)==None);
    assert(fresh(9003,9004,0,5,20,1,2)==None);
    assert(fresh(-1,9003,0,5,20,1,2)==None);
    for(int side:{-1,0,9})assert(fresh(9003,9003,0,5,20,1,side)==None);
    for(int format:{11,22,33,111,1111,10})assert(classify(5,format,1)==None);
    assert(classify(3,22,1)==None&&classify(4,20,1)==None&&classify(5,20,8)==None);
    char dir[]=QSAOW3_TEST_TMP "art3-capture-test-XXXXXX";assert(mkdtemp(dir));
    char root[640],path[640];snprintf(root,sizeof(root),"%s/",dir);
    snprintf(path,sizeof(path),"%s000000_StartBattleNotification.ggs",root);
    FILE* f=fopen(path,"wb");assert(f);assert(fputs("retained",f)>=0);assert(!fclose(f));
    snprintf(path,sizeof(path),"%s000001_StartBattleNotification.json",root);
    f=fopen(path,"wb");assert(f);assert(fputs("retained-meta",f)>=0);assert(!fclose(f));
    unsigned sequence=0,id=99;int fd=reserve(root,sequence,id,path,sizeof(path));assert(fd>=0&&id==2);close(fd);
    sequence=0;fd=reserve(root,sequence,id,path,sizeof(path));assert(fd>=0&&id==3);close(fd);
    snprintf(path,sizeof(path),"%s000000_StartBattleNotification.ggs",root);
    char text[32]={};f=fopen(path,"rb");assert(f);assert(fread(text,1,8,f)==8&&!strcmp(text,"retained"));fclose(f);
    assert(!unlink(path));
    snprintf(path,sizeof(path),"%s000001_StartBattleNotification.json",root);assert(!unlink(path));
    for(int i:{2,3}){snprintf(path,sizeof(path),"%s%06d_StartBattleNotification.ggs",root,i);assert(!unlink(path));}
    assert(!rmdir(dir));
    puts("PASS content capture policy: fresh campaign/cooperative only, own-side range, reject PvP/rejoin/mismatched context; retain existing and incomplete capture pairs across process restarts");
}
