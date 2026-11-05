// points-box service, using the existing model/configuration/messages.
#pragma once
#include "supply_rewards_runtime.h"
#include "shop_box_store.h"
#include "contract_box_store.h"
namespace local_supply {
using Roots=local_faction::Roots;
static bool installed,ready,busy;
static void* current_list;static uintptr_t list_root;
static int cache_faction,cache_rank,cache_group;static uint64_t cache_claims=UINT64_MAX,cache_cycle=UINT64_MAX;
static bool available[13];
static bool (*validate_original)(void*,void*,void*,void*,void**,void*);
static void* get(void* p,int offset){return p?*(void**)((char*)p+offset):nullptr;}
static void ref(void* p,const char* name,void* value){field_set(p,class_field_from_name(object_class(p),name),value);}
static bool unlocked(const local_progress::State& s,int faction){
    void* stones=local_career::list(local_career::key(faction),"careerStones");int n=stones?*(int*)((char*)stones+0x18):0;
    for(int i=0;i<n;++i){void* stone=list_item(stones,i);if(!stone||*(int*)((char*)stone+0x20)>s.stars||!local_progress::has_career(s,*(int*)((char*)stone+0x10)))continue;
        void* rewards=get(stone,0x18);int count=rewards?*(int*)((char*)rewards+0x18):0;
        for(int j=0;j<count;++j){void* reward=list_item(rewards,j);if(!reward||*(int*)((char*)reward+0x14)!=18)continue;
            void* mechanic=local_career::lookup(local_career::key(faction),"gameMechanics",*(int*)((char*)reward+0x18));
            if(mechanic&&*(int*)((char*)mechanic+0x38)==12)return true;}}
    return false;
}
static void* dictionary_list(void* dictionary,int key){
    void* result=nullptr;void* ex=nullptr;void* args[]={&key,&result};
    void* found=dictionary?invoke_named(dictionary,"TryGetValue",2,args,&ex):nullptr;
    return found&&!ex&&*(bool*)((char*)found+16)?result:nullptr;
}
static void* canonical_group(int rank,int* category=nullptr){
    void* dictionary=get(saved("SHARED"),0x88),*entries=get(dictionary,0x18);int count=dictionary?*(int*)((char*)dictionary+0x20):0;
    if(count<0||count>128||!entries||array_length(entries)<size_t(count))return nullptr;
    for(int i=0;i<count;++i){char* row=(char*)entries+32+i*24;if(*(int*)row<0)continue;void* list=*(void**)(row+16);
        int n=list?*(int*)((char*)list+0x18):0;void* first=n>0?list_item(list,0):nullptr;
        if(first&&*(int*)((char*)first+0x18)==rank){if(category)*category=*(int*)(row+8);return list;}}
    return nullptr;
}
static void* (*external_box)(int);
static void* original_box(int id){if(contract_box_content::box(id))return external_box?external_box(id):nullptr;if(id==12000){void* sale=get(saved("UserConfiguration"),0xf0),*b=get(sale,0x98);
    return b&&*(int*)((char*)b+0x10)==12000&&*(int*)((char*)b+0x14)==4?b:nullptr;}const auto* d=supply_policy::box(id);void* rows=d?canonical_group(d->rank):nullptr;int n=rows?*(int*)((char*)rows+0x18):0;
    for(int i=0;i<n;++i){void* b=list_item(rows,i);if(b&&*(int*)((char*)b+0x10)==id)return b;}return nullptr;
}
static void* original_item(int box,int id){void* rows=dictionary_list(get(saved("SHARED"),0xa8),box);int n=rows?*(int*)((char*)rows+0x18):0;
    for(int i=0;i<n;++i){void* item=list_item(rows,i);if(item&&*(int*)((char*)item+0x10)==id)return item;}return nullptr;
}
static bool initialize(){
    if(ready)return true;void* shared=saved("SHARED");if(!shared||strcmp(class_name(object_class(shared)),"SharedBaseDictionary"))return false;
    int boxes=0,items=0;
    for(const auto& d:supply_content::boxes){void* b=original_box(d.id);if(!b||*(int*)((char*)b+0x14)!=1||*(int*)((char*)b+0x18)!=d.rank||*(int*)((char*)b+0x1c)!=d.points||
        *(int8_t*)((char*)b+0x20)!=d.active||*(int8_t*)((char*)b+0x40)!=d.minimum||*(int8_t*)((char*)b+0x41)!=d.maximum)return false;
        void* rows=dictionary_list(get(shared,0xa8),d.id);int n=rows?*(int*)((char*)rows+0x18):0;if(n!=d.count)return false;
        for(int i=0;i<d.count;++i){const auto& item=supply_content::items[d.first+i];void* v=original_item(d.id,item.id);
            if(!v||*(int*)((char*)v+0x14)!=d.id||*(int*)((char*)v+0x18)!=item.prob||*(int*)((char*)v+0x1c)!=item.prob_txt||*(int*)((char*)v+0x20)!=item.type||
               *(int*)((char*)v+0x24)!=item.type_id||*(int*)((char*)v+0x40)!=item.minimum||*(int*)((char*)v+0x44)!=item.maximum)return false;++items;}
        ++boxes;
    }
    ready=true;LOG("Local original supply content matched boxes=%d items=%d rank_groups=16",boxes,items);return true;
}
static int cycle_points(const local_progress::State& s){
    const auto* group=supply_policy::active_group(s.supply,local_skill::rank(s));const auto* previous=group&&s.supply.step?supply_policy::box(group->boxes[s.supply.step-1]):nullptr;
    int64_t value=int64_t(s.quest_warpoints)+(previous?previous->points:0);return value>INT_MAX?INT_MAX:int(value);
}
static int points(){return progress_ready?cycle_points(progress):0;}
static void* list_for(const local_progress::State& s,int faction){
    if(!initialize())return nullptr;int rank=local_skill::rank(s);const auto* group=supply_policy::active_group(s.supply,rank);if(!group)return nullptr;
    if(current_list&&cache_faction==faction&&cache_rank==rank&&cache_group==group->rank&&cache_claims==s.supply.claims&&cache_cycle==s.supply.cycle)return current_list;
    void* original=canonical_group(group->rank);int n=original?*(int*)((char*)original+0x18):0;if(n<1||n>13)return nullptr;
    Roots roots;void* copy=object_new(object_class(original));if(!roots.keep(copy))return nullptr;void* ex=nullptr;invoke_named(copy,".ctor",0,nullptr,&ex);
    bool supported[13]={};
    for(int i=0;i<n&&!ex;++i){Roots row_roots;void* row=local_faction::clone(list_item(original,i),row_roots);if(!row)return nullptr;
        const auto* def=supply_policy::box(*(int*)((char*)row+0x10));if(!def)return nullptr;
        for(int step=0;step<group->count;++step)if(group->boxes[step]==def->id)supported[step]=local_supply_rewards::supported(*def,s,faction);
        bool service=def->points>0&&def->active!=2&&local_supply_rewards::supported(*def,s,faction);
        if(def->points>0&&def->active!=2&&!service)ref(row,"hint",string_new("此补给奖励暂不可用，战争点会保留。"));
        *(int*)((char*)row+0x58)=0;void* args[]={row};invoke_named(copy,"Add",1,args,&ex);
    }
    uintptr_t handle=!ex?gchandle_new(copy,true):0;if(!handle)return nullptr;
    if(list_root)gchandle_free(list_root);list_root=handle;current_list=copy;
    cache_faction=faction;cache_rank=rank;cache_group=group->rank;cache_claims=s.supply.claims;cache_cycle=s.supply.cycle;
    memcpy(available,supported,sizeof(available));return copy;
}
static void* configuration(void* user,const local_progress::State& s,Roots& roots){
    const auto* group=supply_policy::active_group(s.supply,local_skill::rank(s));if(!group||!list_for(s,local_faction::active))return nullptr;
    void* c=object_new(find_class("com.geargames.aow.entities","SupplyConfiguration"));if(!roots.keep(c))return nullptr;
    void* ex=nullptr;invoke_named(c,".ctor",0,nullptr,&ex);if(ex)return nullptr;
    int category=0;if(!canonical_group(group->rank,&category)||category<=0)return nullptr;
    *(bool*)((char*)c+0x10)=unlocked(s,local_faction::active);*(int*)((char*)c+0x14)=category;*(int*)((char*)c+0x18)=cycle_points(s);
    *(int*)((char*)c+0x2c)=s.supply.step?group->boxes[s.supply.step-1]:0;
    *(uint64_t*)((char*)c+0x30)=3155378975999999999ULL;
    bool secret=s.supply.step==group->count-1;*(bool*)((char*)c+0x38)=secret;*(bool*)((char*)c+0x39)=secret;
    *(int*)((char*)c+0x3c)=local_progress::stock_amount(s,11,0);
    return c;
}
static bool prepare_profile(void* user){
    if(!progress_ready||!installed)return false;Roots roots;void* cfg=configuration(user,progress,roots),*profile=get(user,0x28);if(!cfg||!profile)return false;
    ref(user,"supplyConfiguration",cfg);*(int*)((char*)profile+0xd4)=progress.supply.tokens;return true;
}
static bool validate(void* self,void* cfg,void* shared,void* logger,void** out,void* method){
    if(!offline_network||!progress_ready)return validate_original(self,cfg,shared,logger,out,method);
    if(!out||!cfg||!initialize())return false;const auto* g=supply_policy::active_group(progress.supply,local_skill::rank(progress));
    int category=0;if(!g||!canonical_group(g->rank,&category)||*(int*)((char*)cfg+0x14)!=category)return false;
    void* list=list_for(progress,local_faction::active);if(!list)return false;*out=list;return true;
}
static void* actual(void* self,void*){
    if(!progress_ready||!initialize())return nullptr;const auto* g=supply_policy::active_group(progress.supply,local_skill::rank(progress));
    if(!g||progress.supply.step>=g->count)return nullptr;void* rows=get(self,0x78);int n=rows?*(int*)((char*)rows+0x18):0;
    for(int i=0;i<n;++i){void* b=list_item(rows,i);if(b&&*(int*)((char*)b+0x10)==g->boxes[progress.supply.step])return b;}return nullptr;
}
static int box_state(void* self,void* box,void*){
    if(!progress_ready||!box||!unlocked(progress,local_faction::active)||!list_for(progress,local_faction::active))return 0;const auto* g=supply_policy::active_group(progress.supply,local_skill::rank(progress));
    int id=*(int*)((char*)box+0x10);if(!g)return 0;
    for(int i=0;i<g->count;++i)if(g->boxes[i]==id){if(i<progress.supply.step)return 1;
        const supply_content::Box* next=nullptr;int cost=0;
        return i==progress.supply.step&&available[i]&&supply_policy::next(progress.supply,local_skill::rank(progress),next,cost)&&progress.quest_warpoints>=cost?2:0;}
    return 0;
}
static bool no_restart(void*,void*){return false;}
static void no_timer(void*,void*,int,void*,void*,uint64_t,void*){}
static void no_request(void*,void*){LOG("Local supply uses saved gameplay points; calendar request disabled");}
static void* model(){void* screen=static_object("com.geargames.aow.scenes.main","ClientMainContext","<Screen>k__BackingField");void* ex=nullptr;
    return screen?invoke_named(screen,"get_PlayerSupplyModel",0,nullptr,&ex):nullptr;
}
static bool refresh(){
    void* user=saved("UserConfiguration"),*live=model();if(!live||!prepare_profile(user))return false;Roots roots;
    void* message=object_new(find_class("com.geargames.aow.messages","SupplyConfigurationUpdate"));if(!roots.keep(message))return false;
    void* cfg=get(user,0x138),*ex=nullptr;void* args[]={cfg};invoke_named(message,".ctor",1,args,&ex);
    void* a[]={message};if(!ex)invoke_named(live,"OnSupplyConfigurationUpdateReceived",1,a,&ex);
    LOG("Local supply original configuration published=%d group=%d step=%d cycle=%llu balance=%d",!ex,cache_group,progress.supply.step,(unsigned long long)progress.supply.cycle,progress.quest_warpoints);return !ex;
}
static const supply_content::Item* grant_item(const local_progress::State& state,const supply_types::Grant& grant,bool shop=false,bool contract=false){
    const auto* pool=contract?contract_box_content::items:shop?shop_box_content::items:supply_content::items;int size=contract?77:shop?33:int(sizeof(supply_content::items)/sizeof(supply_content::items[0]));
    auto* draws=contract?state.contract_boxes.draws:shop?state.shop.draws:state.supply.draws;uint32_t count=contract?state.contract_boxes.draw_count:shop?state.shop.draw_count:state.supply.draw_count;
    for(uint32_t i=0;i<count;++i)for(int k=0;k<size;++k){const auto& item=pool[k];if(item.id!=draws[i].item)continue;
        if(grant.type==15&&item.type==25&&item.type_id==grant.id)return &item;
        if((grant.type==0&&item.type==1)||(grant.type==1&&item.type==2)||(grant.type==2&&item.type==7))return &item;
        if((grant.type==7&&local_items::speedup_id(item.type)==grant.id)||(grant.type==10&&item.type==grant.id+16)||(grant.type==11&&item.type==21))return &item;
        if(grant.type==17&&item.type==26){void* d=local_career::lookup("SHARED","unitModificationTypes",item.type_id);
            void* concrete=nullptr,*reader=saved("@dictionary-reader"),*context=get(reader,0x10),*ex=nullptr;void* name=string_new("com.geargames.aow.entities.UnitModificationType");int id=grant.id;void* args[]={name,&id};if(context)concrete=invoke_named(context,"getProcessed",2,args,&ex);
            if(!ex&&concrete&&(item.type_id==grant.id||(d&&*(int*)((char*)d+0x48)==*(int*)((char*)concrete+0x48))))return &item;}
        void* card=grant.type==4?local_career::lookup("BATTLE","boosts",grant.id):grant.type==8?local_career::skill_card(grant.id):grant.type==9?local_career::lookup("SHARED","heroCards",grant.id):nullptr;
        if(!card)continue;
        if((grant.type==4&&item.type==3+*(int8_t*)((char*)card+0x2a))||
           (grant.type==8&&item.type==10+*(int*)((char*)card+0x28))||
           (grant.type==9&&item.type==13+*(int8_t*)((char*)card+0x30)))return &item;
    }return nullptr;
}
static void* reward_message(const local_progress::State& state,Roots& roots,bool shop=false,bool contract=false){
    if(contract?(!state.contract_boxes.claims||!contract_box_types::valid(state.contract_boxes,state.contract_points)):shop?(!state.shop.claims||!shop_box_policy::valid(state.shop)):(!state.supply.claims||!supply_types::valid(state.supply)))return nullptr;
    int box_id=contract?state.contract_boxes.last_box:shop?12000:state.supply.last_box;uint32_t grant_count=contract?state.contract_boxes.grant_count:shop?state.shop.grant_count:state.supply.grant_count;
    auto* grants=contract?state.contract_boxes.grants:shop?state.shop.grants:state.supply.grants;
    void* message=object_new(find_class("com.geargames.aow.messages","BoxRewardMessage"));if(!roots.keep(message))return nullptr;
    void* ex=nullptr;invoke_named(message,".ctor",0,nullptr,&ex);
    void* field=class_field_from_name(object_class(message),"items");void* list=field?object_new(class_from_type(field_type(field))):nullptr;
    void* box=local_faction::clone(original_box(box_id),roots);if(!roots.keep(list)||!box)return nullptr;
    invoke_named(list,".ctor",0,nullptr,&ex);*(int*)((char*)box+0x58)=0;
    for(uint32_t i=0;i<grant_count&&!ex;++i){Roots row_roots;const auto& g=grants[i];const auto* selected=grant_item(state,g,shop,contract);
        void* source=selected?original_item(box_id,selected->id):nullptr;if(!source)return nullptr;
        void* item=object_new(find_class("com.geargames.aow.messages","RewardItem"));if(!row_roots.keep(item))return nullptr;
        int8_t type=g.type;int atlas=*(int8_t*)((char*)source+0x28);int amount=g.amount,id=g.id;bool random=false;
        void* a[]={&type,&atlas,&amount,&id,&random};invoke_named(item,".ctor",5,a,&ex);
        void* name=get(source,0x38),*description=get(source,0x38),*icon=get(source,0x30);
        void* definition=g.type==4?local_career::lookup("BATTLE","boosts",g.id):g.type==8?local_career::skill_card(g.id):g.type==9?local_career::lookup("SHARED","heroCards",g.id):nullptr;
        if(definition){name=get(definition,0x18);description=get(definition,0x20);icon=get(definition,g.type==4?0x48:g.type==8?0x30:0x28);
            if(g.type==4)*(int*)((char*)item+0x18)=*(int8_t*)((char*)definition+0x50);}
        if(g.type==7){void* d=local_items::speedup(g.id);if(!d)return nullptr;name=get(d,0x18);description=string_new("单机研究已即时完成");icon=get(d,0x28);}
        if(g.type==10){void* d=local_items::battery(g.id);if(!d)return nullptr;name=get(d,0x20);description=get(d,0x28);icon=get(d,0x30);}
        if(g.type==15){name=string_new("赛事入场券");description=string_new("单机无需赛事入场券");}
        if(g.type==17){void* reader=saved("@dictionary-reader"),*context=get(reader,0x10),*e=nullptr;void* type=string_new("com.geargames.aow.entities.UnitModificationType");int id=g.id;void* args[]={type,&id};void* d=context?invoke_named(context,"getProcessed",2,args,&e):nullptr;
            if(e||!d)return nullptr;name=get(d,0x18);description=get(d,0x20);icon=get(d,0x30);}
        ref(item,"name",name);ref(item,"hint",description);ref(item,"desc",description);ref(item,"icon",icon);
        void* args[]={item};if(!ex)invoke_named(list,"Add",1,args,&ex);
    }
    if(ex)return nullptr;ref(message,"box",box);ref(message,"items",list);ref(message,"header",get(box,0x28));
    *(bool*)((char*)message+0x45)=true; // The atomic local wallet already owns every grant.
    *(int*)((char*)message+0x5c)=0;return message;
}
static bool publish_pending(bool shop=false,bool contract=false){
    if(!(contract?progress.contract_boxes.pending:shop?progress.shop.pending:progress.supply.pending))return true;Roots roots;void* message=reward_message(progress,roots,shop,contract),*dispatcher=get(offline_network,0x88);
    if(!message||!dispatcher||!apply_local_progress()||!local_items::publish()||!local_modifications::publish()||!refresh())return false;
    int gold=0;if(!local_faction::account_gold(progress,gold))return false;
    int values[]={progress.credits,gold,progress.supply.tokens};const char* names[]={"SetProfileCredits","SetAccountGold","SetProfileTokens"};void* ex=nullptr;
    for(int i=0;i<3&&!ex;++i){void* update=object_new(find_class("com.geargames.aow.messages",names[i]));if(!roots.keep(update))return false;
        void* a[]={values+i};invoke_named(update,".ctor",1,a,&ex);void* args[]={update};if(!ex)invoke_named(dispatcher,"Dispatch",1,args,&ex);}
    void* args[]={message};if(!ex)invoke_named(dispatcher,"Dispatch",1,args,&ex);if(ex)return false;
    auto next=progress;if(contract)next.contract_boxes.pending=0;else if(shop)next.shop.pending=0;else next.supply.pending=0;bool acknowledged=local_progress::write(local_faction::root(),next);if(acknowledged)progress=next;
    LOG("Local supply original reward published=1 receipt_ack=%d claim=%llu box=%d grants=%u",acknowledged,(unsigned long long)progress.supply.claims,progress.supply.last_box,progress.supply.grant_count);
    if(acknowledged){refresh();if(contract)local_contracts::refresh();}return acknowledged;
}
static void hide(void* component){
    if(!component)return;void* ex=nullptr;void* go=!strcmp(class_name(object_class(component)),"GameObject")?component:invoke_named(component,"get_gameObject",0,nullptr,&ex);
    bool no=false;void* args[]={&no};if(go&&!ex)invoke_named(go,"SetActive",1,args,&ex);
}
static bool ours(void* message){void* box=get(message,0x20);return box&&(*(int*)((char*)box+0x10)==12000||contract_box_content::box(*(int*)((char*)box+0x10))||supply_policy::box(*(int*)((char*)box+0x10))||competition_track_content::box(*(int*)((char*)box+0x10)));}
static void (*result_show_original)(void*,void*,void*);
static void result_show(void* self,void* message,void* method){
    result_show_original(self,message,method);if(!ours(message))return;
    for(int offset:{0x78,0x80,0x88,0xa0,0xc0})hide(get(self,offset));
    void* final=get(self,0xd8);hide(get(final,0x30));
}
static void (*reroll_show_original)(void*,void*);
static void (*reroll_original)(void*,void*);
static void reroll_show(void* self,void* method){
    void* window=get(self,0x48);if(ours(get(window,0x118))){void* ex=nullptr;invoke_named(self,"Hide",0,nullptr,&ex);return;}
    reroll_show_original(self,method);
}
static void reroll(void* self,void* method){
    void* window=get(self,0x48);if(ours(get(window,0x118))){LOG("Local supply reroll unavailable; saved draw retained");return;}reroll_original(self,method);
}
static void (*window_localize_original)(void*,void*);
static void window_localize(void* self,void* method){
    window_localize_original(self,method);if(!offline_network)return;
    for(int offset:{0x38,0x58,0x60,0x68,0x78})hide(get(self,offset));
}
static bool live_available(){
    void* live=model(),*ex=nullptr;void* state=live?invoke_named(live,"get_State",0,nullptr,&ex):nullptr;
    void* value=state&&!ex?invoke_named(state,"get_Value",0,nullptr,&ex):nullptr;return value&&!ex&&!*(int*)((char*)value+16);
}
static void collect(void* self,void* box,void*){
    if(progress.competition_tracks.pending||progress.shop.pending||progress.contract_boxes.pending||busy||!installed||!offline_network||!progress_ready||!unlocked(progress,local_faction::active)||!live_available()||!self||self!=model()||!box)return;
    busy=true;struct Release{~Release(){busy=false;}} release;
    if(progress.supply.pending){publish_pending();return;}
    void* rows=get(self,0x78);int n=rows?*(int*)((char*)rows+0x18):0;bool bound=false;for(int i=0;i<n;++i)if(list_item(rows,i)==box)bound=true;
    if(!bound||box_state(self,box,nullptr)!=2)return;
    const auto* d=supply_policy::box(*(int*)((char*)box+0x10));supply_policy::Draw drawn;supply_types::Grant grants[128];int count=0;
    if(!d||!local_supply_rewards::prepare(*d,progress,local_faction::active,drawn,grants,128,count))return;
    auto next=progress;if(!supply_store::claim(next,local_skill::rank(progress),d->id,drawn,grants,count))return;
    Roots roots;if(!reward_message(next,roots)){LOG("Local supply claim refused: original reward message could not be prepared");return;}
    if(!local_progress::write(local_faction::root(),next)){LOG("Local supply claim save failed box=%d; balance retained",d->id);return;}
    progress=next;LOG("Local supply claim committed box=%d cost=%d claim=%llu balance=%d items=%u grants=%u",d->id,next.supply.last_cost,(unsigned long long)next.supply.claims,next.quest_warpoints,next.supply.draw_count,next.supply.grant_count);
    if(!publish_pending())LOG("Local supply committed receipt awaits original UI publication; no second draw/debit");
}
static bool install(){
    const uint32_t vs[]={0xf81c0ffe,0xa9015ff8,0xa90257f6,0xa9034ff4};
    const uint32_t as[]={0xa9bd5ffe,0xa90157f6,0xa9024ff4,0xd0010d74};
    const uint32_t bs[]={0xa9bd5ffe,0xa90157f6,0xa9024ff4,0xb0010d75};
    const uint32_t rs[]={0xf81f0ffe,0xf9402000,0xb4000180,0xf9400008};
    const uint32_t ts[]={0xa9bc67fe,0xa9015ff8,0xa90257f6,0xa9034ff4};
    const uint32_t qs[]={0xa9bd5ffe,0xa90157f6,0xa9024ff4,0xf0010d55};
    const uint32_t show_s[]={0xa9be57fe,0xa9014ff4,0xb000ed55,0xaa0103f3};
    const uint32_t reroll_show_s[]={0xd10183ff,0xa9016ffe,0xa90267fa,0xa9035ff8};
    const uint32_t reroll_s[]={0xa9be57fe,0xa9014ff4,0xd000ed55,0xf000b8d4};
    const uint32_t localize_s[]={0xa9be57fe,0xa9014ff4,0xd000f3f5,0xf000bf74};
    validate_original=(decltype(validate_original))install_hook(0x7c04794,vs,(void*)validate);
    bool a=install_hook(0x7c028cc,as,(void*)actual)!=nullptr,b=install_hook(0x7c03e3c,bs,(void*)box_state)!=nullptr;
    bool r=install_hook(0x7c02880,rs,(void*)no_restart)!=nullptr,t=install_hook(0x7c04a04,ts,(void*)no_timer)!=nullptr,q=install_hook(0x7c05130,qs,(void*)no_request)!=nullptr;
    result_show_original=(decltype(result_show_original))install_hook(0x800a310,show_s,(void*)result_show);
    reroll_show_original=(decltype(reroll_show_original))install_hook(0x8008514,reroll_show_s,(void*)reroll_show);
    reroll_original=(decltype(reroll_original))install_hook(0x800906c,reroll_s,(void*)reroll);
    window_localize_original=(decltype(window_localize_original))install_hook(0x7f3434c,localize_s,(void*)window_localize);
    installed=validate_original&&a&&b&&r&&t&&q&&result_show_original&&reroll_show_original&&reroll_original&&window_localize_original;
    if(installed){local_warpoints::points_projection=points;local_warpoints::claim_handler=collect;}
    LOG("Local supply service hooks installed=%d original_gates_preserved=1 no_calendar=1",installed);return installed;
}
}
