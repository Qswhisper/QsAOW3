// Original quest UI/answer bridge with the explicit-goal rules .
#pragma once
namespace local_quests {
using Roots=local_faction::Roots;
struct Provider {
    bool (*available)(const local_progress::State&,void*)=nullptr;
    bool (*complete)(const local_progress::State&,void*)=nullptr;
    void* (*list)(void*,const local_progress::State&,Roots&)=nullptr;
};
static Provider provider;
static void (*after_change)();
static bool (*policy_approved)();
static bool active(){return policy_approved&&policy_approved()&&provider.available&&provider.complete&&provider.list;}
static void* get(void* p,int o){return p?*(void**)((char*)p+o):nullptr;}
static void ref(void* p,const char* name,void* v){field_set(p,class_field_from_name(object_class(p),name),v);}
static void* canonical(int id){return local_career::lookup(local_career::key(local_faction::active),"quests",id);}
static void* service(){void* ex=nullptr;void* screen=static_object("com.geargames.aow.scenes.main","ClientMainContext","<Screen>k__BackingField");return screen?invoke_named(screen,"get_QuestsService",0,nullptr,&ex):nullptr;}
static void* quest(void* description,int state,Roots& roots){
    void* v=object_new(find_class("com.geargames.aow.entities","AvailableQuest"));if(!roots.keep(v))return nullptr;
    void* ex=nullptr;void* args[]={description,&state};invoke_named(v,".ctor",2,args,&ex);return ex?nullptr:v;
}
#include "quest_rules.h"
static bool approved(){return true;} // Offline quest policy is enabled.
static bool reconcile(void* s,void* rows){
    void* models=get(s,0x10),*ex=nullptr;if(!models||!rows)return false;
    void* boxed=invoke_named(models,"get_Count",0,nullptr,&ex);int count=boxed&&!ex?*(int*)((char*)boxed+16):-1;
    if(count<0||count>512)return false;
    for(int i=0;i<*(int*)((char*)rows+0x18)&&!ex;++i){void* q=list_item(rows,i),*d=get(q,0x10),*match=nullptr;if(!d)return false;
        for(int j=0;j<count&&!ex;++j){void* a[]={&j};void* m=invoke_named(models,"get_Item",1,a,&ex);if(get(get(m,0x40),0x10)==d){match=m;break;}}
        void* a[]={q};if(ex)return false;
        if(match){invoke_named(match,"SetData",1,a,&ex);if(!ex)invoke_named(match,"UpdateQuest",0,nullptr,&ex);}
        else invoke_named(s,"CreateQuestModel",1,a,&ex);
    }
    if(!ex)invoke_named(s,"OnUpdatedQuest",0,nullptr,&ex);
    LOG("Local quest original publication success=%d rows=%d exception=%s",!ex,*(int*)((char*)rows+0x18),ex?class_name(object_class(ex)):"none");return !ex;
}
static bool refresh(){
    if(!offline_network||!progress_ready||!active())return false;
    Roots roots;void* user=saved("UserConfiguration"),*profile=get(user,0x28),*s=service();
    if(!s||!profile||get(s,0x30)!=profile)return false;
    void* rows=provider.list(user,progress,roots);if(!rows)return false;ref(profile,"quests",rows);return reconcile(s,rows);
}
static bool prepare_profile(void* user){
    if(!active())return true;Roots roots;void* rows=provider.list(user,progress,roots),*profile=get(user,0x28);
    if(!rows||!profile)return false;ref(profile,"quests",rows);
    LOG("Local quest restored faction=%d rows=%d receipts=%u saved_generation=%llu",local_faction::active,*(int*)((char*)rows+0x18),progress.quest_count,(unsigned long long)progress.generation);return true;
}
static void (*read_original)(void*,void*);
static void (*finish_original)(void*,void*);
static bool live(void* self,void*& description){
    void* q=get(self,0x40);description=get(q,0x10);if(!description)return false;
    int id=*(int*)((char*)description+0x10);return canonical(id)==description&&provider.available(progress,description)&&!local_progress::has_quest(progress,id);
}
static void read(void* self,void* method){
    if(!offline_network||!active()){read_original(self,method);return;}if(!progress_ready)return;
    void* description=nullptr;if(!live(self,description))return;void* q=get(self,0x40);
    if(*(int*)((char*)q+0x18)!=4)return;int id=*(int*)((char*)description+0x10);
    auto next=progress;
    if(!local_progress::quest_state(next,id)&&(!local_progress::read_quest(next,id)||!local_progress::write(local_faction::root(),next))){LOG("Local quest read save refused id=%d",id);return;}
    progress=next;int state=8;void* args[]={&state};void* ex=nullptr;invoke_named(q,"set_State",1,args,&ex);
    if(!ex)invoke_named(self,"UpdateQuest",0,nullptr,&ex);
    LOG("Local quest read saved=1 id=%d published=%d",id,!ex);
}
static void finish(void* self,void* method){
    if(!offline_network||!active()){finish_original(self,method);return;}if(!progress_ready)return;
    void* description=nullptr;if(!live(self,description)||!provider.complete(progress,description))return;
    int id=*(int*)((char*)description+0x10),gold=*(int*)((char*)description+0x60),credits=*(int*)((char*)description+0x64);
    int tokens=*(int*)((char*)description+0x68),xp=*(int*)((char*)description+0x6c),points=*(int*)((char*)description+0x70);
    // Tokens have no supported local wallet yet. Never grant a partial reward.
    if(tokens){LOG("Local quest whole reward deferred id=%d unsupported_tokens=%d",id,tokens);return;}
    auto next=progress;if(!local_progress::claim_quest(next,id,credits,gold,xp,points))return;
    Roots roots;void* user=saved("UserConfiguration"),*profile=get(user,0x28),*quests=provider.list(user,next,roots);
    void* s=service(),*dispatcher=get(offline_network,0x88);
    if(!profile||!quests||!s||get(s,0x30)!=profile||!dispatcher||!roots.keep(self))return;
    int account_gold=0;if(!local_faction::account_gold(next,account_gold))return;
    void* messages[3]={};int values[]={next.credits,account_gold,next.experience};const char* names[]={"SetProfileCredits","SetAccountGold","SetExperience"};
    void* ex=nullptr;
    for(int i=0;i<3;++i){messages[i]=object_new(find_class("com.geargames.aow.messages",names[i]));if(!roots.keep(messages[i]))return;void* args[]={values+i};invoke_named(messages[i],".ctor",1,args,&ex);if(ex)return;}
    void* answer=object_new(find_class("com.geargames.aow.messages.quests","QuestFinishedAnswer"));if(!roots.keep(answer))return;
    invoke_named(answer,".ctor",0,nullptr,&ex);if(ex)return;ref(answer,"questDescription",description);
    const void* receiver=class_method(object_class(self),"OnMessageQuestFinishedAnswerReceived",1);
    if(!receiver||!class_method(object_class(s),"UpdateAvailableQuests",0))return;
    if(!local_progress::write(local_faction::root(),next)){LOG("Local quest reward save refused id=%d",id);return;}
    progress=next;ref(profile,"quests",quests);
    for(int i=0;i<3&&!ex;++i){void* args[]={messages[i]};invoke_named(dispatcher,"Dispatch",1,args,&ex);}
    void* args[]={answer};if(!ex)runtime_invoke(receiver,self,args,&ex);
    bool published=!ex&&reconcile(s,quests);
    LOG("Local quest claim saved=1 published=%d id=%d credits=%d gold=%d xp=%d legacy_points=%d receipts=%u exception=%s",published,id,credits,gold,xp,points,progress.quest_count,ex?class_name(object_class(ex)):"none");
    if(published&&after_change)after_change();
}
static void rule_audit(){
    if(access(DATA_ROOT "quest-rules-audit.request",F_OK))return;unlink(DATA_ROOT "quest-rules-audit.request");
    auto before=progress;bool ok=true;FILE* f=fopen(DATA_ROOT "quest-rules-audit.jsonl","w");if(!f)return;
    for(int faction:{1,2}){Rules g;bool good=rules(g,faction);int battle=0,research=0,deferred=0,edges_missing=0;
        auto s=local_progress::seed(0,0,0,0);bool initial[512]={},read[512]={},after[512]={};
        if(good){int root=g.index(g.root);reach(g,s,initial);good=root>=0&&initial[root];
            local_progress::read_quest(s,g.root);reach(g,s,read);
            for(int i=0;i<g.count;++i)good=good&&initial[i]==read[i];
            good=good&&local_progress::claim_quest(s,g.root,0,0,0,0);reach(g,s,after);
            for(int i=0;i<g.count;++i){const auto& r=g.nodes[i];bool sample=true;
                if(r.kind==1){++battle;auto x=local_progress::seed(0,0,0,0);sample=!accomplished(x,r);
                    auto* m=local_progress::mission(x,r.target);if(m){m->losses=1;sample=sample&&!accomplished(x,r);m->wins=1;sample=sample&&accomplished(x,r);}else sample=false;}
                else if(r.kind==2){++research;auto x=local_progress::seed(0,0,0,0);sample=!accomplished(x,r)&&local_progress::buy_upgrade(x,r.target,0,0)&&accomplished(x,r);}
                else ++deferred;
                if(r.rank>local_skill::rank(s)&&after[i])sample=false;
                void* edges=get(r.description,0x80);for(size_t j=0;edges&&j<array_length(edges);++j){void* d=((void**)((char*)edges+32))[j];if(!d||g.index(*(int*)((char*)d+0x10))<0)++edges_missing;}
                fprintf(f,"{\"faction\":%d,\"quest\":%d,\"kind\":%d,\"target\":%d,\"rank\":%d,\"reward_supported\":%s,\"private_goal_sample\":%s}\n",faction,r.id,r.kind,r.target,r.rank,reward_supported(r.description)?"true":"false",sample?"true":"false");good=good&&sample;
            }
        }
        fprintf(f,"{\"faction\":%d,\"phase\":\"rules_summary\",\"pass\":%s,\"quests\":%d,\"battle_goals\":%d,\"research_goals\":%d,\"deferred_goals\":%d,\"missing_edges\":%d,\"read_does_not_unlock\":true}\n",faction,good?"true":"false",g.count,battle,research,deferred,edges_missing);ok=ok&&good;
    }
    bool unchanged=!memcmp(&before,&progress,sizeof(progress));fprintf(f,"{\"phase\":\"final\",\"pass\":%s,\"player_state_unchanged\":%s,\"player_save_writes\":0}\n",ok&&unchanged?"true":"false",unchanged?"true":"false");fclose(f);chmod(DATA_ROOT "quest-rules-audit.jsonl",0644);
}
static void audit(){
    rule_audit();
    if(access(DATA_ROOT "quest-interface-audit.request",F_OK))return;
    void* s=service();if(!s)return;unlink(DATA_ROOT "quest-interface-audit.request");auto before=progress;
    Roots roots;void* user=saved("UserConfiguration"),*profile=get(user,0x28),*old=get(profile,0xa8);
    void* base=local_faction::fraction(local_faction::active);int id=base?*(int*)((char*)base+0x108):-1;
    void* description=canonical(id);void* q=description?quest(description,16,roots):nullptr;
    void* factory=get(s,0x28),*model=nullptr,*answer=nullptr,*ex=nullptr;
    bool ok=profile&&old&&q&&factory;int previous_credits=profile?*(int*)((char*)profile+0xd0):-1;
    if(ok){void* args[]={q};model=invoke_named(factory,"Create",1,args,&ex);ok=!ex&&roots.keep(model)&&get(model,0x40)==q;}
    if(ok){answer=object_new(find_class("com.geargames.aow.messages.quests","QuestFinishedAnswer"));ok=roots.keep(answer);
        if(ok){invoke_named(answer,".ctor",0,nullptr,&ex);if(!ex)ref(answer,"questDescription",description);void* args[]={answer};if(!ex)invoke_named(model,"OnMessageQuestFinishedAnswerReceived",1,args,&ex);ok=!ex&&*(int*)((char*)q+0x18)==32;}}
    if(model){void* dispose_ex=nullptr;invoke_named(model,"Dispose",0,nullptr,&dispose_ex);if(dispose_ex)ok=false;}
    bool unchanged=profile&&!memcmp(&before,&progress,sizeof(progress))&&get(profile,0xa8)==old&&*(int*)((char*)profile+0xd0)==previous_credits;
    FILE* f=fopen(DATA_ROOT "quest-interface-audit.json","w");if(f){fprintf(f,"{\"pass\":%s,\"quest\":%d,\"private_original_model\":%s,\"original_completed_state\":16,\"original_claimed_state\":%d,\"policy_active\":%s,\"player_state_unchanged\":%s,\"player_save_writes\":0,\"exception\":\"%s\"}\n",ok&&unchanged?"true":"false",id,model?"true":"false",q?*(int*)((char*)q+0x18):-1,active()?"true":"false",unchanged?"true":"false",ex?class_name(object_class(ex)):"none");fclose(f);chmod(DATA_ROOT "quest-interface-audit.json",0644);}
}
static void install(){const uint32_t sig[]={0xf81d0ffe,0xa90157f6,0xa9024ff4,0xb000edd4};
    provider={rule_available,rule_complete,rule_list};policy_approved=approved;
    read_original=(decltype(read_original))install_hook(0x7ffa588,sig,(void*)read);
    finish_original=(decltype(finish_original))install_hook(0x7ffac74,sig,(void*)finish);
    LOG("Local quest original interface hooks read=%d finish=%d policy_active=%d",read_original!=nullptr,finish_original!=nullptr,active());
}
}
