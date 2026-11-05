// Local commanders and match strength. No wallet/rank/engine mutations.
#pragma once
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <cmath>
#include <algorithm>
namespace offline_world {
constexpr int npc_count=24, max_actors=6;
enum Style { Pressure=0, Balanced=1, Technology=2, Support=3 };
struct Definition {const char* name;int faction,tier,style;};
static constexpr Definition definitions[npc_count]={
 {"灰隼",1,0,Pressure},{"坚盾",1,0,Balanced},{"晨星",1,0,Technology},{"长弓",1,0,Support},
 {"铁砧",1,1,Pressure},{"白锋",1,1,Balanced},{"黑曜",1,1,Technology},{"霜狼",1,1,Support},
 {"雷隼",1,2,Pressure},{"岩隼",1,2,Balanced},{"银刃",1,2,Technology},{"赤枫",1,2,Support},
 {"沙狐",2,0,Pressure},{"夜莺",2,0,Balanced},{"流火",2,0,Technology},{"逐风",2,0,Support},
 {"渡鸦",2,1,Pressure},{"岩鹫",2,1,Balanced},{"银狐",2,1,Technology},{"荒狼",2,1,Support},
 {"赤隼",2,2,Pressure},{"断刃",2,2,Balanced},{"苍鹰",2,2,Technology},{"远山",2,2,Support},
};
inline int actor_id(int index){return 0x6f100001+index;}
inline int actor_index(int id){return id>=0x6f100001&&id<0x6f100001+npc_count?id-0x6f100001:-1;}
inline const char* tier_name(int tier){return tier==0?"新锐":tier==2?"精锐":"老练";}
inline const char* style_name(int style){return style==Pressure?"突击":style==Technology?"科技":style==Support?"协同":"均衡";}
inline const char* player_name(int faction){return faction==1?"北辰":"星火";}
inline int lane(int mode){return mode==2?1:mode==4?2:0;}
inline int rating_tier(int rating){return rating<850?0:rating<1150?1:2;}
inline int npc_floor(int tier){return tier==0?400:tier==1?850:1150;}
inline int npc_ceiling(int tier){return tier==0?849:tier==1?1149:2000;}
inline int seats(int mode){return mode==2||mode==8?4:mode==4||mode==10?6:mode==3||mode==7?3:mode==9?5:2;}
inline bool formal_mode(int mode){return mode==1||mode==2||mode==4;}
inline int team(int mode,int seat){return mode>=7?seat-1:(seat-1)%2;}
inline uint32_t mix(uint64_t x){x^=x>>33;x*=0xff51afd7ed558ccdULL;x^=x>>33;return uint32_t(x^(x>>32));}
struct Lane {int32_t rating=1000;uint32_t games=0,wins=0,losses=0,draws=0;int32_t last_opponent=0;};
struct Npc {int32_t rating=1000;uint32_t games=0,wins=0,losses=0,draws=0;};
struct State {
 uint32_t magic=0x31575241,version=1,size=0,checksum=0;
 uint64_t generation=0,first_run=0,last_run=0;
 uint32_t faction=0,last_result=0;
 Lane lanes[3];Npc npcs[npc_count];
};
struct Actor {int32_t id=0,faction=0,tier=1,style=Balanced,rating=1000;char name[32]={};};
struct Snapshot {
 uint32_t magic=0x314d4341,version=1,size=0,checksum=0;
 uint64_t run=0;
 uint32_t faction=0,mode=0,map=0,opponent=0,formal=0,actor_count=0;
 int32_t player_rating=1000;uint32_t player_games=0;
 Actor actors[max_actors];
};
static_assert(sizeof(State)==600&&sizeof(Snapshot)==368,"Local world wire layout changed");
template<class T> inline uint32_t hash(const T& v){uint32_t h=2166136261u;
 for(size_t i=0;i<sizeof(v);++i)h=(h^(i>=12&&i<16?0:((const uint8_t*)&v)[i]))*16777619u;return h;}
inline void seal(State& s){s.size=sizeof(s);s.checksum=hash(s);}
inline void seal(Snapshot& s){s.size=sizeof(s);s.checksum=hash(s);}
inline bool valid(const State& s){
 if(s.magic!=0x31575241||s.version!=1||s.size!=sizeof(s)||s.checksum!=hash(s)||s.faction<1||s.faction>2||!s.first_run||s.last_run&&s.last_run<s.first_run)return false;
 for(const auto& l:s.lanes)if(l.rating<400||l.rating>2400||l.games!=uint64_t(l.wins)+l.losses+l.draws)return false;
 for(const auto& n:s.npcs)if(n.rating<400||n.rating>2400||n.games!=uint64_t(n.wins)+n.losses+n.draws)return false;
 return true;
}
inline bool valid(const Snapshot& s){
 if(s.magic!=0x314d4341||s.version!=1||s.size!=sizeof(s)||s.checksum!=hash(s)||!s.run||s.faction<1||s.faction>2||s.mode<1||s.mode>10||s.mode==3&&s.faction!=2||!s.map||s.opponent<1||s.opponent>2||s.formal>1||s.formal&&!formal_mode(s.mode)||s.actor_count!=uint32_t(seats(s.mode))||s.player_rating<400||s.player_rating>2400)return false;
 for(unsigned i=0;i<s.actor_count;++i){const auto& a=s.actors[i];
  if(a.faction<1||a.faction>2||a.tier<0||a.tier>2||a.style<0||a.style>3||a.rating<400||a.rating>2400||!a.name[0]||!memchr(a.name,0,sizeof(a.name)))return false;
  if(i==0){if(a.id!=0||a.faction!=int(s.faction))return false;continue;}
  int index=actor_index(a.id);if(index<0||definitions[index].faction!=a.faction||definitions[index].tier!=a.tier||definitions[index].style!=a.style)return false;
  for(unsigned j=1;j<i;++j)if(a.id==s.actors[j].id)return false;
 }return true;
}
inline State initial(int faction,uint64_t first){State s;s.faction=faction;s.first_run=first;
 for(int i=0;i<npc_count;++i)s.npcs[i].rating=700+definitions[i].tier*300+(definitions[i].style-1)*20;
 seal(s);return s;}
inline bool assign(const State& state,uint64_t run,int mode,int map,int opponent,bool formal,Snapshot& out){
 if(!valid(state)||run<state.first_run||mode<1||mode>10||mode==3&&state.faction!=2||map<=0||opponent<1||opponent>2||formal&&!formal_mode(mode))return false;
 Snapshot s;s.run=run;s.faction=state.faction;s.mode=mode;s.map=map;s.opponent=opponent;s.formal=formal;s.actor_count=seats(mode);
 const auto& l=state.lanes[lane(mode)];s.player_rating=l.rating;s.player_games=l.games;
 s.actors[0].faction=state.faction;s.actors[0].rating=l.rating;snprintf(s.actors[0].name,32,"%s",player_name(state.faction));
 for(unsigned seat=1;seat<s.actor_count;++seat){int faction=team(mode,seat+1)==0?state.faction:opponent,best=-1;
  int candidates[npc_count]={},weights[npc_count]={},n=0,min_band=3,min_distance=10000;
  for(int i=0;i<npc_count;++i){if(definitions[i].faction!=faction)continue;bool used=false;
   for(unsigned j=1;j<seat;++j)used|=s.actors[j].id==actor_id(i);if(used)continue;
   int band=std::abs(definitions[i].tier-rating_tier(l.rating));
   if(band<min_band){n=0;min_band=band;min_distance=10000;}
   if(band!=min_band)continue;candidates[n++]=i;min_distance=std::min(min_distance,std::abs(l.rating-state.npcs[i].rating));}
  int total=0;
  for(int k=0;k<n;++k){int i=candidates[k],distance=std::abs(l.rating-state.npcs[i].rating);
   // Keep uncertainty inside the nearest available tier and a modest rating
   // neighborhood. Random selection must not silently fetch a distant tier.
   if(distance>min_distance+180)continue;
   int weight=20000/(100+distance);
   if(actor_id(i)==l.last_opponent)weight=std::max(1,weight/4);
   for(unsigned j=1;j<seat;++j)if(team(mode,j+1)==team(mode,seat+1)&&s.actors[j].style==definitions[i].style)weight=std::max(1,weight/3);
   weights[k]=std::max(1,weight);total+=weights[k];}
  if(total){unsigned pick=mix(run*977+uint64_t(map)*31+seat*17)%unsigned(total);
   for(int k=0;k<n;++k)if(weights[k]){if(pick<unsigned(weights[k])){best=candidates[k];break;}pick-=weights[k];}}
  if(best<0)return false;const auto& d=definitions[best];auto& a=s.actors[seat];
  a.id=actor_id(best);a.faction=faction;a.tier=d.tier;a.style=d.style;a.rating=state.npcs[best].rating;snprintf(a.name,32,"%s",d.name);
 }seal(s);if(!valid(s))return false;out=s;return true;
}
inline int enemy_rating(const Snapshot& s){int sum=0,n=0;for(unsigned i=1;i<s.actor_count;++i)if(team(s.mode,i+1)!=0){sum+=s.actors[i].rating;++n;}return n?sum/n:s.player_rating;}
inline int rating_delta(int rating,int other,double score,int games){
 double expected=1.0/(1.0+std::pow(10.0,double(other-rating)/400.0));
 return int(std::lround((games<6?80.0:32.0)*(score-expected)));
}
// Outcome values match the existing ledger. Surrender is a real concession;
// only its new strength rating changes, never the old reward/star convention.
inline bool settle(State& state,const Snapshot& s,int outcome,int reason,uint32_t digest){
 if(!valid(state)||!valid(s)||state.faction!=s.faction||s.run<state.first_run)return false;
 if(s.run<=state.last_run)return s.run==state.last_run&&digest==state.last_result;
 if(!s.formal||!((reason==1&&(outcome==1||outcome==2||outcome==4))||(reason==6&&outcome==3)))return true;
 auto next=state;auto& l=next.lanes[lane(s.mode)];if(l.rating!=s.player_rating||l.games!=s.player_games)return false;
 if(l.games==UINT32_MAX)return false;double score=outcome==1?1.0:outcome==4?.5:0.0;
 int delta=rating_delta(l.rating,enemy_rating(s),score,l.games);l.rating=std::clamp(l.rating+delta,400,2400);++l.games;
 if(score==1)++l.wins;else if(score==0)++l.losses;else ++l.draws;
 l.last_opponent=s.actors[1].id;
 for(unsigned i=1;i<s.actor_count;++i){int index=actor_index(s.actors[i].id);auto& n=next.npcs[index];if(n.games==UINT32_MAX)return false;
  bool allied=team(s.mode,i+1)==0;double own_score=allied?score:1.0-score;
  int tier=definitions[index].tier;
  n.rating=std::clamp(n.rating+rating_delta(n.rating,s.player_rating,own_score,n.games)/2,npc_floor(tier),npc_ceiling(tier));++n.games;
  if(own_score==1)++n.wins;else if(own_score==0)++n.losses;else ++n.draws;
 }next.last_run=s.run;next.last_result=digest;seal(next);if(!valid(next))return false;state=next;return true;
}
struct Tuning {int think,move,defend,assemble,wait;bool tactical;};
inline Tuning tuning(int tier,int style){
 Tuning t=tier==0?Tuning{30,220,60,2,300,false}:tier==2?Tuning{10,70,20,6,500,true}:Tuning{10,150,30,3,300,false};
 if(style==Pressure)t.assemble=std::max(2,t.assemble-2);
 if(style==Technology)t.assemble+=2;
 if(style==Support)t.defend=std::max(10,t.defend-10);return t;
}
}
