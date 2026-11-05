// Original skill tree, card wallet and original window/answer publication.
#pragma once
#include "skill_tree_policy.h"
namespace local_skill {
using Roots=local_faction::Roots;
static skill_tree_policy::Graph graph;static void* originals[512];static bool ready;static void* graph_source;
static void* presentation;static void* presentation_source;static uintptr_t presentation_root,presentation_source_root;
static void* waived_cards[100],*waived_owner;static uintptr_t waived_card_roots[100];
static bool (*effect_supported)(int);
static bool (*prepare_combat)(void*,const local_progress::State&);
static void* get(void* p,int o){return p?*(void**)((char*)p+o):nullptr;}
static void ref(void* p,const char* name,void* v){field_set(p,class_field_from_name(object_class(p),name),v);}
static bool append(void* list,void* item){void* ex=nullptr;void* args[]={item};invoke_named(list,"Add",1,args,&ex);return !ex;}
static void* displayed_card(void* card,Roots& roots){
    if(!card||!local_progress::waived_skill_card(*(int*)((char*)card+0x10)))return card;
    int id=*(int*)((char*)card+0x10);
    if(waived_owner!=saved("SHARED")){
        for(int i=0;i<100;++i){if(waived_card_roots[i])gchandle_free(waived_card_roots[i]);waived_card_roots[i]=0;waived_cards[i]=nullptr;}
        waived_owner=saved("SHARED");}
    if(waived_cards[id])return waived_cards[id];
    void* copy=local_faction::clone(card,roots);if(!copy)return nullptr;
    ref(copy,"desc",string_new(id==11?"单机战争点不设日上限；此节点免费通过。":id==12?"商城免费箱不采用旧倒计时；此节点免费通过。":id==22?"单机不设每日首胜赠奖；此节点免费通过。":"每局出战前铀能已补满；此节点免费通过。"));
    *(int*)((char*)copy+0x38)=0;*(int*)((char*)copy+0x3c)=0;
    uintptr_t root=gchandle_new(copy,true);if(!root)return nullptr;
    if(waived_card_roots[id])gchandle_free(waived_card_roots[id]);
    waived_card_roots[id]=root;waived_cards[id]=copy;return copy;
}
static bool initialize(){
    if(ready&&graph_source==saved("SHARED"))return true;ready=false;graph=skill_tree_policy::Graph{};
    void* stages=local_career::list("SHARED","milestones");int n=stages?*(int*)((char*)stages+0x18):0;
    if(n<1||n>32)return false;
    for(int i=0;i<n;++i){void* s=list_item(stages,i);if(!s)return false;
        graph.stages[graph.stage_count++]={*(int*)((char*)s+0x10),*(int*)((char*)s+0x14)};
        void* map=get(s,0x20);if(!map||array_length(map)>128)return false;
        for(size_t j=0;j<array_length(map);++j){void* e=((void**)((char*)map+32))[j],*card=get(e,0x20),*edges=get(e,0x30);
            if(!card||graph.count==512||(edges&&array_length(edges)>16))return false;
            auto& node=graph.nodes[graph.count];originals[graph.count++]=e;
            node.id=*(int*)((char*)e+0x10);node.card=*(int*)((char*)card+0x10);node.stage=*(int*)((char*)s+0x10);
            node.credits=*(int*)((char*)e+0x28);node.cards=*(int*)((char*)e+0x2c);
            node.edge_count=edges?(int)array_length(edges):0;
            for(int k=0;k<node.edge_count;++k){void* p=((void**)((char*)edges+32))[k];if(!p)return false;node.edges[k]=*(int*)((char*)p+0x10);}
        }
    }
    void* config=get(saved("UserConfiguration"),0x1e8),*skills=get(config,0x20);
    for(int i=0;skills&&i<*(int*)((char*)skills+0x18);++i){void* s=list_item(skills,i),*c=get(s,0x10);int id=c?*(int*)((char*)c+0x10):0;
        if(id<=0||id>=100||graph.max_level[id])return false;graph.max_level[id]=*(int*)((char*)s+0x1c);}
    ready=graph.valid();if(ready)graph_source=saved("SHARED");LOG("Local original skill graph ready=%d nodes=%d milestones=%d",ready,graph.count,graph.stage_count);return ready;
}
static void* runtime_shared(){
    void* source=saved("SHARED");if(presentation&&presentation_source==source)return presentation;
    if(!source||!initialize())return nullptr;Roots roots;
    auto barrier=(void(*)(void*,void**,void*))dlsym(il2cpp,"il2cpp_gc_wbarrier_set_field");
    void* shared=local_faction::clone(source,roots),*old=local_career::list("SHARED","milestones");
    void* all=array_new(find_class("com.geargames.aow.entities","SkillMapElement"),graph.count);
    void* stages=old?object_new(object_class(old)):nullptr;void* ex=nullptr;
    if(!barrier||!shared||!roots.keep(all)||!roots.keep(stages))return nullptr;
    invoke_named(stages,".ctor",0,nullptr,&ex);if(ex)return nullptr;
    auto put=[&](void* array,int i,void* value){barrier(array,((void**)((char*)array+32))+i,value);};
    // One shared view graph: tree, prerequisites, completed nodes and answers
    // must use the same objects, as the original models compare references.
    for(int i=0;i<graph.count;++i){Roots one;void* node=local_faction::clone(originals[i],one);if(!node)return nullptr;
        if(local_progress::waived_skill_card(graph.nodes[i].card)){
            void* c=displayed_card(get(node,0x20),one);if(!c)return nullptr;
            *(int*)((char*)node+0x28)=0;*(int*)((char*)node+0x2c)=0;ref(node,"skillCard",c);}
        put(all,i,node);}
    for(int i=0;i<graph.count;++i){Roots one;const auto& n=graph.nodes[i];
        void* edges=array_new(find_class("com.geargames.aow.entities","SkillMapElement"),n.edge_count);if(!one.keep(edges))return nullptr;
        for(int j=0;j<n.edge_count;++j){int k=graph.index(n.edges[j]);if(k<0)return nullptr;put(edges,j,((void**)((char*)all+32))[k]);}
        ref(((void**)((char*)all+32))[i],"contrib",edges);}
    for(int i=0;i<graph.stage_count;++i){Roots one;void* stage=local_faction::clone(list_item(old,i),one),*map=get(stage,0x20);
        size_t n=map?array_length(map):0;void* array=array_new(find_class("com.geargames.aow.entities","SkillMapElement"),n);
        if(!stage||!one.keep(array))return nullptr;
        for(size_t j=0;j<n;++j){void* original=((void**)((char*)map+32))[j];int k=original?graph.index(*(int*)((char*)original+0x10)):-1;
            if(k<0)return nullptr;put(array,j,((void**)((char*)all+32))[k]);}
        ref(stage,"skillMap",array);if(!append(stages,stage))return nullptr;}
    ref(shared,"milestones",stages);
    uintptr_t a=gchandle_new(shared,true),b=gchandle_new(source,true);if(!a||!b){if(a)gchandle_free(a);if(b)gchandle_free(b);return nullptr;}
    if(presentation_root)gchandle_free(presentation_root);if(presentation_source_root)gchandle_free(presentation_source_root);
    presentation=shared;presentation_source=source;presentation_root=a;presentation_source_root=b;return shared;
}
static void* presented_node(void* original){
    if(!original)return nullptr;void* shared=runtime_shared(),*list=get(shared,0xb8);int id=*(int*)((char*)original+0x10);
    for(int i=0;list&&i<*(int*)((char*)list+0x18);++i){void* nodes=get(list_item(list,i),0x20);
        for(size_t j=0;nodes&&j<array_length(nodes);++j){void* n=((void**)((char*)nodes+32))[j];if(n&&*(int*)((char*)n+0x10)==id)return n;}}
    return nullptr;
}
static void* info(void* user,const local_progress::State& state,Roots& roots){
    if(!initialize())return nullptr;skill_tree_policy::Summary summary;if(!graph.summarize(state,summary))return nullptr;
    for(uint32_t i=0;i<state.stock_count;++i)if(state.stocks[i].type==8&&!local_career::skill_card(state.stocks[i].id))return nullptr;
    void* source=get(user,0x1e8),*copy=local_faction::clone(source,roots);
    if(!copy||!local_faction::empty_field(copy,"skills",roots)||!local_faction::empty_field(copy,"skillMapFinished",roots)||!local_faction::empty_field(copy,"milestoneBoxOpened",roots))return nullptr;
    for(uint32_t i=0;i<state.skill_count;++i){int k=graph.index(state.skill_nodes[i]);void* node=k>=0?presented_node(originals[k]):nullptr;if(!node||!append(get(copy,0x10),node))return nullptr;}
    void* cards[128];int count=local_career::skill_cards(cards,128);
    for(int i=0;i<count;++i){Roots one;void* c=cards[i];int id=*(int*)((char*)c+0x10),level=summary.level[id],max=graph.max_level[id],amount=local_progress::stock_amount(state,8,id);
        void* s=object_new(find_class("com.geargames.aow.entities","Skill"));if(!one.keep(s))return nullptr;
        c=displayed_card(c,one);if(!c)return nullptr;
        void* ex=nullptr;void* args[]={c,&level,&max,&amount};invoke_named(s,".ctor",4,args,&ex);
        if(ex||!append(get(copy,0x20),s))return nullptr;
    }
    *(int*)((char*)copy+0x28)=summary.stage;
    // AvailableMilestones is the original tree ceiling, not earned progress.
    // LastMilestone and completed nodes determine actual purchase access.
    return copy;
}
static int rank(const local_progress::State& state){
    void* ranks=local_career::list("SHARED","profileRanks");int result=0;
    for(int i=0;ranks&&i<*(int*)((char*)ranks+0x18);++i){void* r=list_item(ranks,i);
        if(*(int*)((char*)r+0x14)<=state.stars&&*(int*)((char*)r+0x10)>result)result=*(int*)((char*)r+0x10);}
    return result;
}
static bool plan(const local_progress::State& state,int node,local_progress::State& next,bool require_rank=true){
    if(!initialize()||(require_rank&&rank(state)<5))return false;int i=graph.index(node);
    if(i<0||!effect_supported||!effect_supported(graph.nodes[i].card))return false;
    next=state;return graph.purchase(next,node);
}
static void (*upgrade_original)(void*,void*);
static void (*update_original)(void*,void*);
static void update(void* self,void* method){
    if(offline_network){void* node=get(self,0x110),*shown=presented_node(node);
        if(node&&!shown)return;if(shown&&shown!=node)ref(self,"m_mapElement",shown);}
    update_original(self,method);if(!offline_network||!effect_supported)return;
    void* node=get(self,0x110),*card=get(node,0x20),*button=get(self,0x98);
    if(card&&button&&!effect_supported(*(int*)((char*)card+0x10))){void* ex=nullptr;invoke_named(button,"ShowHidden",0,nullptr,&ex);}
}
static void upgrade(void* self,void* method){
    if(!offline_network){upgrade_original(self,method);return;}if(!progress_ready)return;
    void* live=get(self,0x110);int id=live?*(int*)((char*)live+0x10):0;int index=initialize()?graph.index(id):-1;
    if(index<0)return;const auto& node=graph.nodes[index];void* canonical=originals[index];
    if(*(int*)((char*)live+0x28)!=skill_tree_policy::credits(node)||*(int*)((char*)live+0x2c)!=skill_tree_policy::cards(node)||
        !get(live,0x20)||*(int*)((char*)get(live,0x20)+0x10)!=node.card)return;
    void* source=static_object("com.geargames.aow.scenes.main","ClientMainContext","<SkillInfoDataSource>k__BackingField");
    void* dispatcher=get(offline_network,0x88);
    const void* receiver=class_method(find_class("com.geargames.aow.scenes.main","MainManager"),"OnMessageSkillUpgradeAnswerReceived",1);
    if(!source||!dispatcher||!receiver||!class_method(object_class(dispatcher),"Dispatch",1)||!class_method(object_class(self),"UpdateInfo",0))return;
    local_progress::State next;Roots roots;
    if(!plan(progress,id,next)){LOG("Local skill upgrade refused before debit node=%d card=%d",id,node.card);return;}
    void* skills=info(saved("UserConfiguration"),next,roots);if(!skills)return;
    void* hero_config=nullptr,*hero_source=nullptr;
    if(node.card==28){
        hero_config=local_faction::clone(get(saved("UserConfiguration"),0x110),roots);
        hero_source=static_object("com.geargames.aow.scenes.main","ClientMainContext","<HeroConfigurationDataSource>k__BackingField");
        if(!hero_config||!local_hero::capacity_projector||!local_hero::capacity_projector(hero_config,next)||
           !hero_source||!class_method(object_class(hero_source),"SetConfiguration",1))return;
    }
    void* answer=object_new(find_class("com.geargames.aow.messages","SkillUpgradeAnswer"));
    void* balance=object_new(find_class("com.geargames.aow.messages","SetProfileCredits"));
    if(!roots.keep(answer)||!roots.keep(balance))return;void* ex=nullptr;
    invoke_named(answer,".ctor",0,nullptr,&ex);void* credit_args[]={&next.credits};if(!ex)invoke_named(balance,".ctor",1,credit_args,&ex);
    void* skill=nullptr,*rows=get(skills,0x20);for(int i=0;rows&&i<*(int*)((char*)rows+0x18);++i){void* s=list_item(rows,i);if(*(int*)((char*)get(s,0x10)+0x10)==node.card)skill=s;}
    void* shown=presented_node(canonical);
    if(ex||!skill||!shown)return;ref(answer,"skill",skill);ref(answer,"skillMapElement",shown);*(int*)((char*)answer+0x38)=*(int*)((char*)skills+0x28);
    if(!local_progress::write(local_faction::root(),next)){LOG("Local skill save refused node=%d",id);return;}
    progress=next;ref(saved("UserConfiguration"),"skillInfo",skills);
    if(hero_config){ref(saved("UserConfiguration"),"heroConfiguration",hero_config);
        void* a[]={hero_config};invoke_named(hero_source,"SetConfiguration",1,a,&ex);}
    void* args[]={balance};if(!ex)invoke_named(dispatcher,"Dispatch",1,args,&ex);
    void* update_args[]={answer};
    if(!ex)runtime_invoke(receiver,nullptr,update_args,&ex);
    // The existing view renders the authoritative updated model directly;
    // no waiting network request or replacement screen is introduced.
    if(!ex)invoke_named(self,"UpdateInfo",0,nullptr,&ex);
    LOG("Local skill saved=1 published=%d node=%d card=%d credits=%d receipts=%u exception=%s",!ex,id,node.card,progress.credits,progress.skill_count,ex?class_name(object_class(ex)):"none");
}
static void install(){
    const uint32_t sig[]={0xf81c0ffe,0xa9015ff8,0xa90257f6,0xa9034ff4};
    upgrade_original=(decltype(upgrade_original))install_hook(0x78c1a04,sig,(void*)upgrade);
    const uint32_t update_sig[]={0xf81a0ffe,0xa9016ffc,0xa90267fa,0xa9035ff8};
    update_original=(decltype(update_original))install_hook(0x78c12ac,update_sig,(void*)update);
    local_career::skill_projector=info;LOG("Local skill original upgrade hook=%d view_hook=%d",upgrade_original!=nullptr,update_original!=nullptr);
}
}
