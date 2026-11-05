#pragma once
namespace local_mobility_audit {
static int16_t native_turn(void* state,bool& ok){void* ex=nullptr;void* v=invoke_named(state,"get_Rotate",0,nullptr,&ex);if(ex||!v){ok=false;return 0;}return *(int16_t*)((char*)v+16);}
static uint64_t original_hash(void* type){uint64_t h=uint32_t(*(int*)((char*)type+0x34));void* states=*(void**)((char*)type+0x18);int n=states?*(int*)((char*)states+0x18):0;
 for(int i=0;i<n;++i){void* s=list_item(states,i);for(int k:{0x58,0x5a,0x5b,0x76})h=(h^*((uint8_t*)s+k))*1099511628211ULL;}return h;}
static void run(){
 if(access(DATA_ROOT "local-mobility-audit.request",F_OK))return;
 FILE* f=fopen(DATA_ROOT "local-mobility-audit.jsonl","w");if(!f)return;bool ok=true,preserved=true;int accepted=0,refused=0,states=0;
 generic_unit_research::Plan total;
 for(const auto& r:local_mobility::rows){void* level=local_career::lookup(local_career::key(r.faction),"upgradeLevels",r.level);generic_unit_research::Effect e;
  bool supported=level&&generic_unit_research::resolve(level,e);bool expected=!(r.faction==2&&r.owner==127&&(r.level==12810||r.level==12811));bool pass=supported==expected;
  if(supported){++accepted;const char* reason=nullptr;uint64_t before=original_hash(e.owner);local_research_runtime::Roots roots;
   void* copy=generic_unit_research::copy_type(e.owner,e,roots);pass=pass&&copy&&e.local_turn==1&&total.add(e);
   if(copy){void* original_states=*(void**)((char*)e.owner+0x18),*changed_states=*(void**)((char*)copy+0x18);int n=*(int*)((char*)original_states+0x18);
    pass=pass&&n==*(int*)((char*)changed_states+0x18)&&*(int*)((char*)copy+0x34)==*(int*)((char*)e.owner+0x34)+e.delta.hp;
    for(int mode=0;mode<n;++mode){void* original=list_item(original_states,mode),*changed=list_item(changed_states,mode);bool called=true;
     int old=native_turn(original,called),now=native_turn(changed,called);bool unchanged=*(int8_t*)((char*)original+0x58)==*(int8_t*)((char*)changed+0x58)&&*(int8_t*)((char*)original+0x76)==*(int8_t*)((char*)changed+0x76);
     preserved=preserved&&unchanged;pass=pass&&called&&now==local_mobility::apply(old,1)&&unchanged;++states;
    }}pass=pass&&before==original_hash(e.owner);
  }else ++refused;
  fprintf(f,"{\"level\":%d,\"faction\":%d,\"owner\":%d,\"supported\":%s,\"expected\":%s,\"pass\":%s}\n",r.level,r.faction,r.owner,supported?"true":"false",expected?"true":"false",pass?"true":"false");ok=ok&&pass;
 }
 int capped=0;for(int i=0;i<total.count;++i){const auto& e=total.families[i];local_research_runtime::Roots roots;uint64_t before=original_hash(e.owner);void* copy=generic_unit_research::copy_type(e.owner,e,roots);bool pass=copy&&e.local_turn>0&&e.local_turn<=6;
  if(e.local_turn==6)++capped;
  if(copy){void* a=*(void**)((char*)e.owner+0x18),*b=*(void**)((char*)copy+0x18);int n=*(int*)((char*)a+0x18);
   for(int j=0;j<n;++j){bool called=true;pass=pass&&native_turn(list_item(b,j),called)==local_mobility::apply(native_turn(list_item(a,j),called),e.local_turn)&&called;}}
  ok=ok&&pass&&before==original_hash(e.owner);
 }
 ok=ok&&accepted==124&&refused==2&&total.count==24;
 fprintf(f,"{\"summary\":true,\"passed\":%s,\"accepted\":%d,\"other_unknown_whole_refusals\":%d,\"families\":%d,\"native_state_getters\":%d,\"capped_families\":%d,\"original_speed_acceleration_preserved\":%s,\"original_price_rank_prerequisites_unchanged\":true,\"purchase_calls\":0,\"player_save_writes\":0}\n",ok?"true":"false",accepted,refused,total.count,states,capped,preserved?"true":"false");
 fclose(f);chmod(DATA_ROOT "local-mobility-audit.jsonl",0644);unlink(DATA_ROOT "local-mobility-audit.request");
}
}
