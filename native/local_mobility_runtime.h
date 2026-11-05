// The description annotation is presentation only. The reviewed original raw
// text remains available to whole-level validation for these exact objects.
#pragma once
#include "local_mobility_policy.h"
namespace local_mobility_runtime {
struct Annotation {void* text;uintptr_t root;int faction,owner,level;};
static Annotation annotations[252];static int annotation_count;
static const local_mobility::Row* row(void* level){
 void* upgrade=level?*(void**)((char*)level+0x48):nullptr;
 if(!upgrade||strcmp(class_name(object_class(upgrade)),"UpgradeUnit"))return nullptr;
 void* type=*(void**)((char*)upgrade+0x48);if(!type||strcmp(class_name(object_class(type)),"UnitType"))return nullptr;
 void* f=*(void**)((char*)type+0x20);
 return f?local_mobility::find(*(int*)((char*)f+0x10),*(int*)((char*)type+0x10),*(int*)((char*)level+0x10)):nullptr;
}
static const char* original(void* level){const auto* r=row(level);if(!r)return nullptr;void* text=*(void**)((char*)level+0x18);
 for(int i=0;i<annotation_count;++i){const auto& a=annotations[i];if(a.text==text&&a.faction==r->faction&&a.owner==r->owner&&a.level==r->level)return r->description;}return nullptr;}
static bool annotate(){
 // New main-scene loads deserialize fresh dictionary objects. Retain only
 // annotations still referenced by those current objects, rather than filling
 // a lifetime registry after several battles or faction switches.
 void* current[252];int used=0;
 for(int faction:{1,2}){void* levels=local_career::list(local_career::key(faction),"upgradeLevels");int n=levels?*(int*)((char*)levels+0x18):0;if(n<1||n>10000)return false;
  for(int i=0;i<n;++i){void* level=list_item(levels,i);if(row(level)){if(used==252)return false;current[used++]=*(void**)((char*)level+0x18);}}}
 int kept=0;for(int i=0;i<annotation_count;++i){bool live=false;for(int j=0;j<used;++j)live|=annotations[i].text==current[j];
  if(live)annotations[kept++]=annotations[i];else gchandle_free(annotations[i].root);
 }annotation_count=kept;
 int count=0;for(int faction:{1,2}){void* levels=local_career::list(local_career::key(faction),"upgradeLevels");int n=levels?*(int*)((char*)levels+0x18):0;if(n<1||n>10000)return false;
  for(int i=0;i<n;++i){void* level=list_item(levels,i);const auto* r=row(level);if(!r)continue;++count;
   if(original(level))continue;char raw[4096],other[4096];
   if(annotation_count==252||!local_research_runtime::ascii(*(void**)((char*)level+0x18),raw,sizeof(raw))||!local_mobility::strip(*r,raw,other,sizeof(other)))return false;
   char label[4352];snprintf(label,sizeof(label),"%s%s<color=#15e6e6>• 转向 +1（单机转向适配，累计上限 +6）</color>",other,*other?"\n":"");
   void* text=string_new(label);uintptr_t root=gchandle_new(text,true);if(!root)return false;
   void* field=class_field_from_name(object_class(level),"desc");if(!field||field_offset(field)!=0x18){gchandle_free(root);return false;}
   annotations[annotation_count++]={text,root,r->faction,r->owner,r->level};field_set(level,field,text);
  }}LOG("Local mobility annotated=%d expected=126 original_prices_and_requirements_untouched=1",count);return count==126;
}
}
