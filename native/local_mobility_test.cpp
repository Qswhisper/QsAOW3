#include "local_mobility_policy.h"
#include "unit_stat_effects.h"
#include <cassert>
#include <set>
#include <utility>
int main(){
 std::set<std::pair<int,int>> levels,families;int rows=0,scalar=0,whole_refusals=0;
 for(const auto& r:local_mobility::rows){char remaining[4096];
  assert(local_mobility::strip(r,r.description,remaining,sizeof(remaining)));
  research_effects::UnitStats delta;int unknown=0;
  if(!*remaining||research_effects::unit_stats_parse(remaining,delta,unknown)==research_effects::UnitParse::ok)++scalar;
  else{assert(r.faction==2&&r.owner==127&&(r.level==12810||r.level==12811)&&strstr(remaining,"{#20032<0,2>}")&&strstr(remaining,"{#20033<0,1>}"));++whole_refusals;}
  assert(levels.insert({r.faction,r.level}).second);families.insert({r.faction,r.owner});++rows;
 }
 assert(rows==126&&families.size()==24&&scalar==124&&whole_refusals==2);
 char out[128];assert(!local_mobility::strip(local_mobility::rows[0],"{#20013<99>}",out,sizeof(out)));
 assert(local_mobility::apply(10,1)==11&&local_mobility::apply(10,100)==16);
 assert(local_mobility::apply(0,6)==0&&local_mobility::apply(INT16_MAX-1,6)==INT16_MAX);
 puts("PASS:126 original identities/24 families;124 complete scalar+local-turn levels;2 submarine mixed levels retain whole refusal for unknown20032/33; altered-description refusal,cap6,zero-state preservation,int16 clamp");
}
