#pragma once
#include "offline_world_store.h"
#include "ai_replay_config.h"
namespace offline_world_runtime {
static offline_world::Snapshot session;
static bool active;
static bool strategy_fixture;
static bool batch_fixture;
static ai_replay::Job batch_job;
static offline_world::Actor fixture_actors[8];
static int fixture_tiers[8]={1,1,1,1,1,1,1,1};
static bool gameplay_scope(){return local_match::playing||strategy_fixture;}
static bool planner(int side){
 if(!gameplay_scope())return false;
 if(batch_fixture)return side>=1&&side<=batch_job.count&&batch_job.seats[side-1].controller==1;
 // The pressure candidate did not improve the retained short-screen pool.
 // Preserve its original paid production/strategy; identities and saves stay.
 const auto* a=active&&side>=1&&side<=int(session.actor_count)?&session.actors[side-1]:nullptr;
 return !a||a->style!=offline_world::Pressure;
}
static bool arm_batch(){
 batch_fixture=false;strategy_fixture=false;active=false;
 if(access(DATA_ROOT "ai-batch-lab.enabled",F_OK)||access(DATA_ROOT "native-bot-lab.enabled",F_OK)||access(DATA_ROOT "ai-duel-lab.enabled",F_OK)||access(DATA_ROOT "ai-lab.enabled",F_OK))return false;
 FILE* f=fopen(DATA_ROOT "ai-batch-job.txt","r");if(!f)return false;ai_replay::Job j;bool ok=ai_replay::read(f,j);fclose(f);
 const auto* r=ok?local_match_maps::find(j.scenario):nullptr;
 if(!r||r->max_seats<j.count)return false;
 batch_job=j;batch_fixture=true;return true;
}
static bool configure_fixture(int chapter,bool player_match){
 strategy_fixture=false;if(player_match){batch_fixture=false;return true;}active=false;
 if(batch_fixture){
  if(chapter!=batch_job.scenario)return false;
  const auto* route=local_match_maps::find(chapter);if(!route)return false;
  for(int i=0;i<batch_job.count;++i){auto& a=fixture_actors[i];a={};a.faction=i%2?batch_job.opponent:route->faction;a.style=batch_job.seats[i].style;a.tier=batch_job.seats[i].tier;fixture_tiers[i]=a.tier;}
  strategy_fixture=true;LOG("AI batch accepted id=%s scenario=%d mode=%d seats=%d native_seed_controlled=0 career_writes=0",batch_job.id,chapter,batch_job.mode,batch_job.count);return true;
 }
 if(access(DATA_ROOT "commander-ai-lab.enabled",F_OK))return true;
 if(chapter!=201||access(DATA_ROOT "native-bot-lab.enabled",F_OK)||access(DATA_ROOT "ai-duel-lab.enabled",F_OK)||access(DATA_ROOT "ai-lab.enabled",F_OK))return false;
 FILE* f=fopen(DATA_ROOT "commander-ai-tiers.txt","r");if(!f)return false;int a=-1,b=-1;bool ok=fscanf(f,"%d %d",&a,&b)==2;fclose(f);
 if(!ok||a<0||a>2||b<0||b>2)return false;fixture_tiers[0]=a;fixture_tiers[1]=b;strategy_fixture=true;
 for(int i=0;i<2;++i){fixture_actors[i]={};fixture_actors[i].faction=2;fixture_actors[i].tier=fixture_tiers[i];fixture_actors[i].style=offline_world::Balanced;}
 LOG("Local private AI calibration tiers=%d,%d original_economy=1 player_runs=0",a,b);return true;
}
static bool matches(const offline_world::Snapshot& s,const local_match_store::State& m){
 return s.run==m.next&&s.faction==uint32_t(local_faction::active)&&s.mode==uint32_t(m.mode)&&s.map==uint32_t(m.map)&&s.opponent==uint32_t(m.opponent)&&bool(s.formal)==local_match_store::formal(m);
}
static uint32_t result_digest(const local_match_store::State& m){auto copy=m;copy.generation=0;return local_match_store::hash(copy);}
static bool sync(const local_match_store::State& m){
 if(!m.settled||m.next!=m.settled)return true;const char* root=local_faction::root();
 char path[760];if(!offline_world_store::path(root,m.settled,path,sizeof(path)))return false;
 if(access(path,F_OK)){return errno==ENOENT;}
 offline_world::Snapshot s;offline_world::State state;
 if(!offline_world_store::snapshot(root,m.settled,s)||!matches(s,m)||offline_world_store::load(root,local_faction::active,state)!=1)return false;
 auto before=state;
 if(!offline_world::settle(state,s,m.outcome,m.reason,result_digest(m)))return false;
 if(!memcmp(&before,&state,sizeof(state)))return true;
 bool ok=offline_world_store::write(root,state);
 LOG("Local strength result saved=%d run=%llu mode=%d formal=%u outcome=%d rating=%d->%d games=%u wallet_writes=0",ok,(unsigned long long)m.settled,m.mode,s.formal,m.outcome,before.lanes[offline_world::lane(m.mode)].rating,state.lanes[offline_world::lane(m.mode)].rating,state.lanes[offline_world::lane(m.mode)].games);
 return ok;
}
static bool preview(const local_match_store::State& m,offline_world::Snapshot& out){
 offline_world::State state;int status=offline_world_store::load(local_faction::root(),local_faction::active,state);
 if(status<0||m.next==UINT64_MAX)return false;if(!status)state=offline_world::initial(local_faction::active,m.next+1);
 return offline_world::assign(state,m.next+1,m.mode,m.map,m.opponent,false,out);
}
static bool freeze(const local_match_store::State& m){
 active=false;const char* root=local_faction::root();offline_world::State state;int status=offline_world_store::load(root,local_faction::active,state);
 if(status<0)return false;if(!status){state=offline_world::initial(local_faction::active,m.next);if(!offline_world_store::write(root,state))return false;}
 char path[760];if(!offline_world_store::path(root,m.next,path,sizeof(path)))return false;
 if(access(path,F_OK)==0){if(!offline_world_store::snapshot(root,m.next,session)||!matches(session,m))return false;}
 else if(errno!=ENOENT||!offline_world::assign(state,m.next,m.mode,m.map,m.opponent,local_match_store::formal(m),session)||!offline_world_store::retain(root,session))return false;
 active=true;
 LOG("Local strength frozen run=%llu mode=%d formal=%u player_rating=%d enemy_rating=%d actors=%u",(unsigned long long)session.run,m.mode,session.formal,session.player_rating,offline_world::enemy_rating(session),session.actor_count);
 return true;
}
static bool freeze_cooperative(uint64_t run){
 active=false;auto state=offline_world::initial(2,1);
 if(local_faction::active!=2||!offline_world::assign(state,run,3,558,1,false,session)||!offline_world_store::retain(local_faction::root(),session))return false;
 active=true;return true;
}
static const offline_world::Actor* actor(int side){if(strategy_fixture&&side>=1&&side<=(batch_fixture?batch_job.count:2))return &fixture_actors[side-1];return active&&side>=1&&side<=int(session.actor_count)?&session.actors[side-1]:nullptr;}
static offline_world::Tuning tuning(int side){if(strategy_fixture&&side>0&&side<=8)return offline_world::tuning(fixture_tiers[side-1],batch_fixture?batch_job.seats[side-1].style:offline_world::Balanced);
 const auto* a=local_match::playing?actor(side):nullptr;return offline_world::tuning(a?a->tier:1,a?a->style:offline_world::Balanced);}
static bool historical(int owner,const local_match_store::State& m,offline_world::Snapshot& out){
 return offline_world_store::snapshot(local_faction::root(owner),m.settled,out,m.mode==3)&&out.faction==uint32_t(owner)&&out.mode==uint32_t(m.mode)&&out.map==uint32_t(m.map);
}
static void result_text(int owner,const local_match_store::State& m,char* text,size_t size){
 text[0]=0;offline_world::Snapshot s;if(!historical(owner,m,s))return;
 const auto& enemy=s.actors[1];char strength[96];
 bool scored=s.formal&&((m.reason==1&&(m.outcome==1||m.outcome==2||m.outcome==4))||(m.reason==6&&m.outcome==3));
 if(!scored)snprintf(strength,sizeof(strength),"%s · 不计实力",s.formal?"中断/异常":"训练军演");
 else {offline_world::State w;bool committed=offline_world_store::load(local_faction::root(owner),owner,w)==1&&w.last_run>=s.run;
  // Version 1 snapshots freeze every input of the version 1 rating formula.
  double score=m.outcome==1?1.:m.outcome==4?.5:0.;
  int after=std::clamp(s.player_rating+offline_world::rating_delta(s.player_rating,offline_world::enemy_rating(s),score,s.player_games),400,2400);
  if(committed)snprintf(strength,sizeof(strength),"实力 %d → %d",s.player_rating,after);
  else snprintf(strength,sizeof(strength),"实力 %d · 评分待恢复",s.player_rating);
 }
 snprintf(text,size,"%s · 对手 %s（%s）",strength,enemy.name,offline_world::tier_name(enemy.tier));
}
static void ref(void* o,const char* name,void* value){field_set(o,class_field_from_name(object_class(o),name),value);}
static void* empty_list(void* object,const char* name,local_faction::Roots& roots){
 void* field=class_field_from_name(object_class(object),name),*type=field?class_from_type(field_type(field)):nullptr;
 void* list=type?object_new(type):nullptr;if(!roots.keep(list))return nullptr;
 void* ex=nullptr;invoke_named(list,".ctor",0,nullptr,&ex);if(ex)return nullptr;field_set(object,field,list);return list;
}
static void* avatar(void* source,int index,local_faction::Roots& roots){
 void* original=local_faction::commander_avatar_seed;
 void* copy=original?local_faction::clone(original,roots):nullptr;
 void* values=local_career::list("SHARED","avatarElements");int n=values?*(int*)((char*)values+0x18):0;
 if(!copy||!values||n<1||n>4096)return original;
 for(const auto& part:{std::pair<const char*,int>{"head",0x10},{"background",0x18},{"frame",0x20}}){
  void* current=*(void**)((char*)original+part.second);if(!current)continue;int type=*(int*)((char*)current+0x14);void* choices[256];int used=0;
  for(int i=0;i<n&&used<256;++i){void* e=list_item(values,i);if(e&&*(int*)((char*)e+0x14)==type&&*(int*)((char*)e+0x24)<=0&&*(int*)((char*)e+0x20)==*(int*)((char*)current+0x20))choices[used++]=e;}
  if(used)ref(copy,part.first,choices[(index+part.second)%used]);
 }return copy;
}
static void* profile(void* source,const offline_world::Actor& a,local_faction::Roots& roots){
 if(!source)return nullptr;if(!a.id)return source;
 void* copy=object_new(find_class("com.geargames.aow.entities","PublicProfile"));if(!roots.keep(copy))return nullptr;
 void* ex=nullptr;invoke_named(copy,".ctor",0,nullptr,&ex);if(ex)return nullptr;
 ref(copy,"name",string_new(a.name));ref(copy,"nameAlias",string_new(""));ref(copy,"clan",nullptr);
 ref(copy,"fraction",local_faction::fraction(a.faction));
 ref(copy,"country",local_faction::commander_country_seed);
 if(a.id){
  *(int*)((char*)copy+0x10)=a.id;
  void* rank=local_career::lookup("SHARED","profileRanks",1+a.tier*3);
  // This retained version has no SHARED account-level curve. Use its original
  // nullable level seed; military rank and local strength remain independent.
  if(!rank){LOG("Local commander missing original rank tier=%d",a.tier);return nullptr;}
  ref(copy,"rank",rank);ref(copy,"level",local_faction::commander_level_seed);
  *(int*)((char*)copy+0x68)=*(int*)((char*)rank+0x14);
  *(int*)((char*)copy+0x30)=0;
  ref(copy,"avatar",avatar(source,offline_world::actor_index(a.id),roots));
  *(int8_t*)((char*)copy+0x6c)=*(int8_t*)((char*)copy+0x6d)=50;
  for(const char* name:{"medals","communityLinkList"}){
   void* field=class_field_from_name(object_class(copy),name);void* type=field?class_from_type(field_type(field)):nullptr;
   void* list=type?object_new(type):nullptr;if(!roots.keep(list))return nullptr;
   invoke_named(list,".ctor",0,nullptr,&ex);if(ex)return nullptr;field_set(copy,field,list);
  }
  ref(copy,"communityLinks",nullptr);*(bool*)((char*)copy+0x58)=false;
 }return copy;
}
static bool publish_profile(void* profile){
 int id=profile?*(int*)((char*)profile+0x10):0,index=offline_world::actor_index(id);bool legacy=id>=-106&&id<=-101;
 if(index<0&&!legacy)return false;
 offline_world::State state;offline_world::Npc stats;
 if(index>=0){int status=offline_world_store::load(local_faction::root(),local_faction::active,state);if(status<0)return true;if(status>0)stats=state.npcs[index];}
 local_faction::Roots roots;void* answer=object_new(find_class("com.geargames.aow.messages","ProfileStatisticsAnswer"));if(!roots.keep(answer))return true;
 void* ex=nullptr;invoke_named(answer,".ctor",0,nullptr,&ex);if(ex)return true;
 ref(answer,"profile",profile);ref(answer,"rank",*(void**)((char*)profile+0x60));
 *(int*)((char*)answer+0x34)=*(int*)((char*)profile+0x30);*(int*)((char*)answer+0x38)=*(int*)((char*)profile+0x68);
 *(int*)((char*)answer+0x7c)=std::min<uint32_t>(stats.games,INT_MAX);*(int*)((char*)answer+0x80)=std::min<uint32_t>(stats.wins,INT_MAX);
 profile_statistics_policy::Snapshot view;view.total_ratio=stats.games?int(uint64_t(stats.wins)*100/stats.games):50;view.current_known=false;
 char note[320];if(index>=0)snprintf(note,sizeof(note),"本地指挥官 · %s · %s\n实力 %d；本阵营实际同场正式对战 %u 场，胜 %u、负 %u、平 %u。履历只记实际对战。",offline_world::tier_name(offline_world::definitions[index].tier),offline_world::style_name(offline_world::definitions[index].style),stats.games?stats.rating:700+offline_world::definitions[index].tier*300+(offline_world::definitions[index].style-1)*20,stats.games,stats.wins,stats.losses,stats.draws);
 else snprintf(note,sizeof(note),"旧记录未保存独立指挥官身份或战绩；这里不补造履历。");
 *(int*)((char*)answer+0x48)=*(int*)((char*)answer+0x4c)=view.total_ratio;
 if(!empty_list(answer,"rewards",roots)||!empty_list(answer,"inviteInfos",roots))return true;
 void* plates=empty_list(answer,"plates",roots);if(!plates)return true;
 void* plate=object_new(find_class("com.geargames.aow.entities.plates","PlateStatistics"));if(!roots.keep(plate))return true;
 invoke_named(plate,".ctor",0,nullptr,&ex);if(ex)return true;
 for(const char* name:{"icon","atlas"})ref(plate,name,string_new(""));
 ref(plate,"header",string_new("本地指挥官"));ref(plate,"hint",string_new(note));
 void* elements=empty_list(plate,"elements",roots);if(!elements)return true;
 const char* labels[]={"实力 / 档位","本阵营实际正式同场","胜 / 负 / 平"};
 char values[3][96];snprintf(values[0],96,"%d · %s",index>=0?(stats.games?stats.rating:700+offline_world::definitions[index].tier*300+(offline_world::definitions[index].style-1)*20):0,index>=0?offline_world::tier_name(offline_world::definitions[index].tier):"旧记录");
 snprintf(values[1],96,"%u",stats.games);snprintf(values[2],96,"%u / %u / %u",stats.wins,stats.losses,stats.draws);
 for(int i=0;i<3;++i){void* element=object_new(find_class("com.geargames.aow.entities.plates","PlateStatisticsElement"));if(!roots.keep(element))return true;
  invoke_named(element,".ctor",0,nullptr,&ex);if(ex)return true;
  for(const char* name:{"icon","atlas"})ref(element,name,string_new(""));
  ref(element,"desc",string_new(labels[i]));ref(element,"value",string_new(values[i]));ref(element,"hint",string_new(note));
  void* add[]={element};invoke_named(elements,"Add",1,add,&ex);if(ex)return true;
 }
 void* add[]={plate};invoke_named(plates,"Add",1,add,&ex);if(ex)return true;
 *(int*)((char*)answer+0xd8)=2;ref(answer,"inviteButtonHint",string_new("单机指挥官"));
 profile_statistics_runtime::remember_commander(profile,view,note);
 void* dispatcher=offline_network?*(void**)((char*)offline_network+0x88):nullptr;void* args[]={answer};if(dispatcher)invoke_named(dispatcher,"Dispatch",1,args,&ex);
 LOG("Local commander profile id=%d legacy=%d games=%u published=%d read_only=1",id,legacy,stats.games,dispatcher&&!ex);return true;
}
static void (*card_request_original)(void*,int,void*);
static void card_request(void* self,int id,void* method){
 card_request_original(self,id,method);int index=offline_world::actor_index(id);if(!offline_network||index<0)return;
 const auto& d=offline_world::definitions[index];offline_world::Actor actor;actor.id=id;actor.faction=d.faction;actor.tier=d.tier;actor.style=d.style;snprintf(actor.name,sizeof(actor.name),"%s",d.name);
 void* user=saved("UserConfiguration"),*source=user?*(void**)((char*)user+0x28):nullptr;local_faction::Roots roots;
 void* value=profile(source,actor,roots);if(value)publish_profile(value);
}
static void install(){const uint32_t signature[]={0xf81c0ffe,0xa9015ff8,0xa90257f6,0xa9034ff4};
 card_request_original=(decltype(card_request_original))install_hook(0x7bc58f0,signature,(void*)card_request);
 LOG("Local commander original player-card hook=%d",card_request_original!=nullptr);
}
static bool decorate_alliances(void* alliances){
 if(!active)return true;void* user=saved("UserConfiguration"),*source=user?*(void**)((char*)user+0x28):nullptr;
 if(!source||!alliances)return false;int count=*(int*)((char*)alliances+0x18),seen=0;
 for(int i=0;i<count;++i){void* alliance=list_item(alliances,i),*players=alliance?*(void**)((char*)alliance+0x18):nullptr;int n=players?*(int*)((char*)players+0x18):0;
  for(int j=0;j<n;++j){void* player=list_item(players,j);int side=player?*(int*)((char*)player+0x10):0;const auto* a=actor(side);if(!a)return false;
   local_faction::Roots roots;void* p=profile(source,*a,roots);if(!p)return false;ref(player,"m_profile",p);++seen;
  }}return seen==int(session.actor_count);
}
}
