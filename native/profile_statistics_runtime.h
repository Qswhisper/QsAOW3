#pragma once
#include "profile_statistics_policy.h"
namespace profile_statistics_runtime {
struct Cached {void* profile=nullptr;profile_statistics_policy::Snapshot stats;char note[512]={};};
static Cached cache[2];
static Cached commander;
static uintptr_t commander_root;
static void (*updated_original)(void*,void*);
static bool available(){return updated_original!=nullptr;}
static void remember(void* profile,int faction,const profile_statistics_policy::Snapshot& s){
    if(faction>=1&&faction<=2){auto& c=cache[faction-1];c={profile,s};offline_world::State world;
        int status=offline_world_store::load(local_faction::root(faction),faction,world);
        if(status>0)snprintf(c.note,sizeof(c.note),"\n单机正式实力：1v1 %d · 2v2 %d · 3v3 %d。实力与成长军衔独立；训练不评分，正常胜负/平局及确认投降结算，异常/中断不评分。",world.lanes[0].rating,world.lanes[1].rating,world.lanes[2].rating);
    }
}
static void remember_commander(void* profile,const profile_statistics_policy::Snapshot& s,const char* note){
    uintptr_t root=gchandle_new(profile,true);if(!root)return;
    if(commander_root)gchandle_free(commander_root);commander_root=root;
    commander={profile,s};snprintf(commander.note,sizeof(commander.note),"%s",note);
}
static bool decorate(void* self){
    void* profile=self?*(void**)((char*)self+0x20):nullptr;
    const Cached* current=nullptr;for(const auto& c:cache)if(c.profile&&c.profile==profile){current=&c;break;}
    if(commander.profile==profile)current=&commander;
    if(!current)return true;
    void* label=*(void**)((char*)self+0x40),*hint=*(void**)((char*)self+0x90);
    void* field=hint?class_field_from_name(object_class(hint),"m_stringReplacer"):nullptr;
    if(!label||!field||field_offset(field)!=0x28)return false;
    const auto& s=current->stats;char text[48];
    if(s.current_known)snprintf(text,sizeof(text),"%d%%/%d%%",s.total_ratio,s.current_ratio);
    else snprintf(text,sizeof(text),"%d%%/—",s.total_ratio);
    void* ex=nullptr;void* a[]={string_new(text)};invoke_named(label,"set_text",1,a,&ex);
    char note[1024];snprintf(note,sizeof(note),"%s%s",current==&commander?"":"本地对战胜率（旧累计含训练），战役与合作不计。当前取最近50场；少于50场取全部实际战绩，无战绩时默认50%。缺少完整近期记录时显示“—”。胜利奖励按本地固定规则。",current->note);
    field_set(hint,field,string_new(note));
    return !ex;
}
static void updated(void* self,void* method){
    updated_original(self,method);if(offline_network&&!decorate(self))LOG("Local profile win-rate decoration failed; player saves unchanged");
}
static void install(){
    const uint32_t signature[]={0xd10183ff,0xf9000bfe,0xa90267fa,0xa9035ff8};
    updated_original=(decltype(updated_original))install_hook(0x7f61a38,signature,(void*)updated);
    LOG("Local profile statistics consumer hook=%d read_only=1",available());
}
}
