// Read the actual purchase guard and original prerequisites in one batch.
// This is not a purchase fixture and never changes career state.
#pragma once
namespace growth_closure_audit {
static void run(){
 if(access(DATA_ROOT "growth-closure-audit.request",F_OK))return;
 char path[256];snprintf(path,sizeof(path),DATA_ROOT "growth-closure-f%d.jsonl",local_faction::active);
 FILE* f=fopen(path,"w");if(!f)return;int total=0,supported=0;bool valid=true;
 void* levels=local_career::list(local_career::key(local_faction::active),"upgradeLevels");int n=levels?*(int*)((char*)levels+0x18):0;
 if(n<1||n>20000)valid=false;
 for(int i=0;valid&&i<n;++i){void* level=list_item(levels,i),*up=level?*(void**)((char*)level+0x48):nullptr;if(!level||!up){valid=false;break;}
  const char* kind=class_name(object_class(up));bool unit=!strcmp(kind,"UpgradeUnit"),building=!strcmp(kind,"UpgradeBuilding");
  void* owner=(unit||building)?*(void**)((char*)up+0x48):nullptr;void* rank=*(void**)((char*)level+0x40);
  bool allowed=local_research_supported(level);++total;supported+=allowed;
  fprintf(f,"{\"id\":%d,\"faction\":%d,\"kind\":\"%s\",\"owner\":%d,\"supported\":%s,\"cost\":%d,\"xp\":%d,\"rank\":%d,\"card\":%d,\"cards\":%d,\"description\":",*(int*)((char*)level+0x10),local_faction::active,kind,owner?*(int*)((char*)owner+0x10):-1,allowed?"true":"false",*(int*)((char*)level+0x20),*(int*)((char*)level+0x28),rank?*(int*)((char*)rank+0x10):0,*(int*)((char*)level+0x50),*(int*)((char*)level+0x54));
  research_batch_audit::json_string(f,*(void**)((char*)level+0x18));fputs(",\"prerequisites\":[",f);
  void* req=*(void**)((char*)level+0x30);size_t count=req?array_length(req):0;if(count>512){valid=false;count=0;}
  for(size_t j=0;j<count;++j){void* r=((void**)((char*)req+32))[j];fprintf(f,"%s%d",j?",":"",r?*(int*)((char*)r+0x10):-1);}fputs("]}\n",f);
 }
 fprintf(f,"{\"summary\":true,\"valid\":%s,\"faction\":%d,\"total\":%d,\"supported\":%d,\"purchase_calls\":0,\"player_save_writes\":0}\n",valid?"true":"false",local_faction::active,total,supported);
 fclose(f);chmod(path,0644);unlink(DATA_ROOT "growth-closure-audit.request");
}
}
