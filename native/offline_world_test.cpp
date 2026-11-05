#include "offline_world_store.h"
#include <cassert>
#include <string>
#include <sys/stat.h>
using namespace offline_world;
int main(int argc,char** argv){
 assert(argc==2);const char* root=argv[1];assert(mkdir(root,0700)==0||errno==EEXIST);
 int combinations=0;
 for(int faction:{1,2})for(int opponent:{1,2})for(int mode:{1,2,4,5,6,7,8,9,10}){
  auto state=initial(faction,37);Snapshot a,b;assert(assign(state,37,mode,41,opponent,formal_mode(mode),a));
  assert(assign(state,37,mode,41,opponent,formal_mode(mode),b)&&!memcmp(&a,&b,sizeof(a)));
  for(unsigned i=1;i<a.actor_count;++i)assert(a.actors[i].faction==(team(mode,i+1)==0?faction:opponent));
  ++combinations;
 }
 assert(rating_delta(1000,1300,1,10)>rating_delta(1000,700,1,10));
 for(int rating:{400,849,850,1149,1150,2400}){auto calibrated=initial(2,1);calibrated.lanes[0].rating=rating;
  for(int i=0;i<npc_count;++i)calibrated.npcs[i].rating=npc_floor(definitions[i].tier);seal(calibrated);Snapshot opponent;
  assert(assign(calibrated,1,1,41,1,true,opponent)&&opponent.actors[1].tier==rating_tier(rating));}
 auto state=initial(2,37);Snapshot s;assert(assign(state,37,1,41,1,true,s));auto before=state;
 assert(offline_world_store::write(root,state));assert(offline_world_store::retain(root,s));assert(offline_world_store::retain(root,s));
 auto conflict=s;strcpy(conflict.actors[1].name,"different");seal(conflict);assert(!offline_world_store::retain(root,conflict));
 assert(settle(state,s,1,1,123));assert(state.lanes[0].games==1&&state.lanes[0].rating>1000);
 auto once=state;assert(settle(state,s,1,1,123)&&!memcmp(&once,&state,sizeof(state)));assert(!settle(state,s,2,1,456));
 assert(offline_world_store::write(root,state));State loaded;assert(offline_world_store::load(root,2,loaded)==1&&loaded.lanes[0].games==1);
 Snapshot frozen;assert(offline_world_store::snapshot(root,37,frozen)&&!memcmp(&frozen,&s,sizeof(s)));
 auto coop_state=initial(2,1);Snapshot coop;assert(assign(coop_state,37,3,558,1,false,coop));
 assert(offline_world_store::retain(root,coop)&&offline_world_store::snapshot(root,37,coop,true)&&coop.actor_count==3);
 assert(offline_world_store::snapshot(root,37,frozen)&&frozen.mode==1); // Same run, separate namespaces.
 assert(assign(state,38,1,41,1,true,s));assert(settle(state,s,3,6,789));assert(state.lanes[0].losses==1);
 assert(assign(state,39,2,41,1,true,s));assert(settle(state,s,4,1,901));assert(state.lanes[1].draws==1&&state.lanes[0].games==2);
 assert(assign(state,40,1,41,1,true,s));before=state;assert(settle(state,s,5,7,902)&&!memcmp(&before,&state,sizeof(state)));
 assert(assign(state,40,1,41,1,false,s));assert(settle(state,s,1,1,903)&&!memcmp(&before,&state,sizeof(state)));
 char file[800];snprintf(file,sizeof(file),"%s/offline-world.0",root);FILE* f=fopen(file,"r+b");assert(f);fputc(0,f);fclose(f);
 assert(offline_world_store::load(root,2,loaded)==1&&loaded.lanes[0].games==0);
 // A retained result can recover the lost latest slot without importing old wins.
 assert(offline_world_store::snapshot(root,37,s)&&settle(loaded,s,1,1,123)&&loaded.lanes[0].games==1);
 snprintf(file,sizeof(file),"%s/offline-world.1",root);f=fopen(file,"r+b");assert(f);fputc(0,f);fclose(f);
 assert(offline_world_store::load(root,2,loaded)==-1);
 assert(tuning(0,Balanced).think>tuning(1,Balanced).think&&tuning(2,Balanced).move<tuning(1,Balanced).move);
 // All comparable styles remain reachable, assignments freeze identically,
 // and far ratings cannot win a random tie-break against a nearby peer.
 bool styles[4]={};auto pool=initial(2,1);
 for(uint64_t run=1;run<=128;++run){Snapshot a,b;assert(assign(pool,run,1,4,1,true,a)&&assign(pool,run,1,4,1,true,b)&&!memcmp(&a,&b,sizeof(a)));
  assert(a.actors[1].tier==1);styles[a.actors[1].style]=true;}
 for(bool reached:styles)assert(reached);
 for(int i=0;i<npc_count;++i)if(definitions[i].faction==1&&definitions[i].tier==1)pool.npcs[i].rating=1300;
 pool.npcs[4].rating=1000;seal(pool);
 for(uint64_t run=1;run<=32;++run){Snapshot a;assert(assign(pool,run,1,4,1,true,a)&&a.actors[1].id==actor_id(4));}
 printf("PASS: %d faction/format assignments; persistent identities, independent ratings, real-result/concession policy, excluded training/interruption, duplicate/conflict refusal, immutable snapshots, slot corruption fallback/replay/refusal\n",combinations);
}
