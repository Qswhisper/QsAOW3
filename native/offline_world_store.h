#pragma once
#include "offline_world_policy.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
namespace offline_world_store {
template<class T> inline bool read(const char* path,T& out){FILE* f=fopen(path,"rb");if(!f)return false;T s;
 bool ok=fread(&s,1,sizeof(s),f)==sizeof(s)&&fgetc(f)==EOF;fclose(f);if(!ok||!offline_world::valid(s))return false;out=s;return true;}
template<class T> inline bool atomic(const char* path,const T& s){char tmp[800];if(!offline_world::valid(s)||snprintf(tmp,sizeof(tmp),"%s.tmp",path)>=int(sizeof(tmp)))return false;
 FILE* f=fopen(tmp,"wb");if(!f)return false;bool ok=fwrite(&s,1,sizeof(s),f)==sizeof(s);if(fflush(f)||fsync(fileno(f)))ok=false;if(fclose(f))ok=false;
 if(!ok||rename(tmp,path))return false;return true;}
inline int load(const char* root,int faction,offline_world::State& out){offline_world::State best;bool found=false,exists=false;
 for(int slot=0;slot<2;++slot){char path[760];if(snprintf(path,sizeof(path),"%s/offline-world.%d",root,slot)>=int(sizeof(path)))return -1;
  if(access(path,F_OK)==0)exists=true;else if(errno!=ENOENT)return -1;offline_world::State s;
  if(read(path,s)&&s.faction==uint32_t(faction)&&(!found||s.generation>best.generation)){best=s;found=true;}}
 if(found){out=best;return 1;}return exists?-1:0;}
inline bool write(const char* root,offline_world::State& s){if(!offline_world::valid(s)||s.generation==UINT64_MAX)return false;
 auto next=s;++next.generation;offline_world::seal(next);char path[760];if(snprintf(path,sizeof(path),"%s/offline-world.%d",root,int(next.generation&1))>=int(sizeof(path))||!atomic(path,next))return false;
 int d=open(root,O_RDONLY|O_DIRECTORY);if(d<0)return false;bool ok=fsync(d)==0;close(d);if(ok)s=next;return ok;}
inline bool path(const char* root,uint64_t run,char* out,size_t size,bool cooperative=false){return snprintf(out,size,"%s/offline-commanders/%s-%020llu.bin",root,cooperative?"coop":"pvp",(unsigned long long)run)<int(size);}
inline bool snapshot(const char* root,uint64_t run,offline_world::Snapshot& out,bool cooperative=false){char p[760];return path(root,run,p,sizeof(p),cooperative)&&read(p,out)&&out.run==run&&(out.mode==3)==cooperative;}
inline bool retain(const char* root,const offline_world::Snapshot& s){if(!offline_world::valid(s))return false;char directory[720],p[760];
 if(snprintf(directory,sizeof(directory),"%s/offline-commanders",root)>=int(sizeof(directory))||!path(root,s.run,p,sizeof(p),s.mode==3))return false;
 bool created=mkdir(directory,0777)==0;if(!created&&errno!=EEXIST)return false;
 if(created){int d=open(root,O_RDONLY|O_DIRECTORY);if(d<0)return false;bool ok=fsync(d)==0;close(d);if(!ok)return false;}
 if(access(p,F_OK)==0){offline_world::Snapshot old;return read(p,old)&&!memcmp(&old,&s,sizeof(s));}
 if(errno!=ENOENT||!atomic(p,s))return false;int d=open(directory,O_RDONLY|O_DIRECTORY);if(d<0)return false;bool ok=fsync(d)==0;close(d);return ok;}
}
