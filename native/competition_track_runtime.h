// Original cooperative battle-pass view and callbacks; no online purchase or
// account VIP state. Only a registered, normally won cooperative run advances it.
#pragma once
#include "competition_track_store.h"
namespace local_competition_tracks {
using Roots=local_faction::Roots;
using competition_track_content::Reward;
static constexpr const char* ns="com.geargames.aow.entities.tournament.tournamentNew";
static bool installed,busy,content_ready;
static void* current_view;static uintptr_t view_root;
// Explicit development-only original-window fixture. Its wallet lives in a
// unique directory and is never projected into the player's account/profile.
static local_progress::State* private_state;static char private_root[640];
static const local_progress::State& view_state(){return private_state?*private_state:progress;}
static void ref(void* p,const char* field,void* value){local_supply::ref(p,field,value);}
static void* get(void* p,int offset){return local_supply::get(p,offset);}
static void* create(const char* space,const char* name,Roots& roots){
    void* c=find_class(space,name),*ex=nullptr;void* v=c?object_new(c):nullptr;if(!roots.keep(v))return nullptr;
    const void* ctor=class_method(c,".ctor",0);if(ctor)runtime_invoke(ctor,v,nullptr,&ex);return ex?nullptr:v;
}
static void* list(void* owner,const char* field,Roots& roots){
    void* f=owner?class_field_from_name(object_class(owner),field):nullptr;
    void* result=f?object_new(class_from_type(field_type(f))):nullptr,*ex=nullptr;if(!roots.keep(result))return nullptr;
    invoke_named(result,".ctor",0,nullptr,&ex);if(ex)return nullptr;field_set(owner,f,result);return result;
}
static bool content(){
    if(content_ready)return true;
    for(const auto& d:competition_track_content::boxes){
        void* b=local_supply::dictionary_list(get(saved("SHARED"),0x130),d.id);
        if(!b||strcmp(class_name(object_class(b)),"SupplyBox")||*(int*)((char*)b+0x14)!=12||
           *(int*)((char*)b+0x18)!=d.rank||*(int*)((char*)b+0x1c)!=d.points||*(int8_t*)((char*)b+0x20)!=d.active||
           *(int8_t*)((char*)b+0x40)!=d.minimum||*(int8_t*)((char*)b+0x41)!=d.maximum)return false;
        void* rows=local_supply::dictionary_list(get(saved("SHARED"),0xa8),d.id);
        if(!rows||*(int*)((char*)rows+0x18)!=d.count)return false;
        for(int i=0;i<d.count;++i){const auto& x=competition_track_content::items[d.first+i];void* v=local_supply::original_item(d.id,x.id);
            if(!v||*(int*)((char*)v+0x14)!=d.id||*(int*)((char*)v+0x18)!=x.prob||*(int*)((char*)v+0x1c)!=x.prob_txt||
               *(int*)((char*)v+0x20)!=x.type||*(int*)((char*)v+0x24)!=x.type_id||*(int*)((char*)v+0x40)!=x.minimum||*(int*)((char*)v+0x44)!=x.maximum)return false;}
    }
    content_ready=true;LOG("Local competition track original boxes matched boxes=8 canonical_pools_preserved=1");return true;
}
static bool obsolete(const Reward& r){return obsolete_reward_policy::speedup(r.type,r.id)||(r.type==11&&r.id==0)||(r.type==16&&r.id==20000001);}
static bool supported(const Reward& r,const local_progress::State& s){
    if(r.amount<=0)return false;if(obsolete(r))return true;
    if(r.type==14){const auto* b=competition_track_content::box(r.id);return r.amount==1&&b&&content()&&local_skill::rank(s)>=b->rank&&local_supply_rewards::supported_pool(*b,competition_track_content::items,s,2);}
    return r.type<=9&&local_career::supported(saved("UserConfiguration"),{r.type,r.id,r.amount},s.stars,2);
}
static void* reward(const Reward& r,Roots& roots,bool available=true){
    void* item=create("com.geargames.aow.messages","RewardItem",roots);if(!item)return nullptr;
    *(int8_t*)((char*)item+0x10)=r.type;*(int*)((char*)item+0x14)=r.id;*(int*)((char*)item+0x40)=r.amount;
    *(bool*)((char*)item+0x44)=r.random;*(int*)((char*)item+0x48)=r.limited;
    const char* why=r.type==16?"单机无需节日币":r.type==11?"单机无需重抽钥匙":r.type==7?"单机研究已即时完成":nullptr;
    const char* names[]={r.name,why?why:!available?"奖励暂不可用，成绩保留。":r.hint,r.icon,why?why:r.description};
    const char* fields[]={"name","hint","icon","desc"};for(int i=0;i<4;++i)ref(item,fields[i],names[i]?string_new(names[i]):nullptr);
    if(r.type==4){void* boost=local_career::lookup("BATTLE","boosts",r.id);if(!boost)return nullptr;*(int*)((char*)item+0x18)=*(int8_t*)((char*)boost+0x50);}
    return item;
}
static void* model(const local_progress::State& s,Roots& roots){
    if(!installed||!content())return nullptr;
    void* profile=create(ns,"BattlePassProfileInfo",roots),*info=create(ns,"BattlePassInfo",roots),*scale=create(ns,"BattlePassScale",roots),*points=create(ns,"CompetitionPoints",roots);
    if(!profile||!info||!scale||!points)return nullptr;
    *(int*)((char*)info+0x10)=competition_track_content::definition;
    *(int*)((char*)scale+0x10)=20;*(int*)((char*)scale+0x28)=5;
    *(int*)((char*)points+0x10)=10;*(int8_t*)((char*)points+0x14)=2;
    ref(points,"name",string_new("{#7038}"));ref(points,"desc",string_new("军衔7开放奖励轨道。新合作局正常胜利每次1点；两排各领一次，无需会员。"));ref(points,"icon",string_new("ico_competition_points"));
    void* thresholds=array_new(find_class("System","Int32"),20),*params=array_new(find_class("System","Int32"),1);
    if(!roots.keep(thresholds)||!roots.keep(params))return nullptr;
    memcpy((char*)thresholds+32,competition_track_content::thresholds,80);*(int*)((char*)params+32)=1;
    ref(points,"params",params);ref(scale,"pointsType",points);ref(scale,"pointsValue",thresholds);ref(info,"battlePassScale",scale);ref(profile,"battlePassInfo",info);
    int index=competition_track_policy::index(s.competition_tracks,competition_track_content::competition);
    const auto* t=index<0?nullptr:s.competition_tracks.tracks+index;
    if(t&&t->definition!=competition_track_content::definition)return nullptr;
    *(int*)((char*)profile+0x2c)=t?t->points:0;
    *(bool*)((char*)profile+0x28)=true; // Only this view; never account membership.
    for(int premium=0;premium<2;++premium){
        void* taken=array_new(find_class("System","SByte"),3);if(!roots.keep(taken))return nullptr;
        for(int i=0;i<3;++i)*((uint8_t*)taken+32+i)=t?uint8_t(t->taken[premium]>>(i*8)):0;
        ref(profile,premium?"rewardsTakenPremium":"rewardsTaken",taken);
        void* rows=list(info,premium?"rewardsPremium":"rewards",roots),*extras=list(info,premium?"extraRewardsPremium":"extraRewards",roots),*currencies=list(info,premium?"eventCurrenciesPremium":"eventCurrencies",roots);
        if(!rows||!extras||!currencies)return nullptr;void* ex=nullptr;
        for(int i=0;i<20;++i){Roots one;const auto& r=competition_track_content::rewards[premium][i];
            void* item=reward(r,one,supported(r,s)),*currency=reward(competition_track_content::currencies[premium][i],one);
            if(!item||!currency)return nullptr;void* a[]={item},*b[]={currency},*empty[]={nullptr};
            invoke_named(rows,"Add",1,a,&ex);if(!ex)invoke_named(extras,"Add",1,empty,&ex);if(!ex)invoke_named(currencies,"Add",1,b,&ex);if(ex)return nullptr;
        }
    }
    return profile;
}
static void* model_for_view(Roots& roots){
    if(FILE* request=fopen(DATA_ROOT "competition-track-ui-audit.request","r")){
        char mode[16]={};bool resume=fscanf(request,"%15s",mode)==1&&!strcmp(mode,"restore");fclose(request);unlink(DATA_ROOT "competition-track-ui-audit.request");
        local_progress::StateBuffer copy;bool ok=copy.data&&!private_state;
        if(ok&&resume){FILE* p=fopen(DATA_ROOT "competition-track-ui-fixture.txt","r");ok=p&&fscanf(p,"%639s",private_root)==1;if(p)fclose(p);
            const char* prefix=DATA_ROOT "competition-track-ui-";ok=ok&&!strncmp(private_root,prefix,strlen(prefix))&&!strstr(private_root,"..")&&!strchr(private_root+strlen(prefix),'/')&&local_progress::load(private_root,*copy.data)==1;}
        else if(ok){snprintf(private_root,sizeof(private_root),DATA_ROOT "competition-track-ui-XXXXXX");ok=mkdtemp(private_root)!=nullptr;*copy.data=local_progress::seed(1000,30,0,110);bool changed=false;
            for(uint64_t i=1;ok&&i<=2;++i)ok=competition_track_policy::begin(copy.data->competition_tracks,10140,101406,i)&&competition_track_policy::settle(copy.data->competition_tracks,10140,i,true,20,changed)&&changed;
            if(ok)ok=local_progress::write(private_root,*copy.data);
            if(ok){FILE* p=fopen(DATA_ROOT "competition-track-ui-fixture.txt","w");ok=p&&fprintf(p,"%s\n",private_root)>0;if(p)fclose(p);}}
        if(ok){private_state=(local_progress::State*)malloc(sizeof(progress));if(private_state)*private_state=*copy.data;else ok=false;}
        LOG("Local competition track UI audit activated=%d restore=%d player_save_writes=0 private_root=%s",ok,resume,private_root);
    }
    return model(view_state(),roots);
}
static bool prepare(const local_progress::State& s,int row,bool premium,supply_types::Grant* grants,int& count){
    count=0;if(row<0||row>=20||local_faction::active!=2||local_skill::rank(s)<competition_track_content::rank)return false;
    const auto& r=competition_track_content::rewards[premium?1:0][row];if(!supported(r,s))return false;if(obsolete(r))return true;
    if(r.type==14){supply_policy::Draw d;return local_supply_rewards::prepare_pool(*competition_track_content::box(r.id),competition_track_content::items,s.competition_tracks.claims,s,2,d,grants,128,count);}
    local_progress::CareerReward resolved[128];int n=0;
    if(r.type==8||r.type==9){bool ok=r.type==8?local_career::resolve_skill_reward(competition_track_content::definition,row+(premium?20:0),r.id,r.amount,2,resolved,128,n):
            local_career::resolve_hero_reward(competition_track_content::definition,row+(premium?20:0),r.id,r.amount,s.stars,2,resolved,128,n);
        if(!ok)return false;for(int i=0;i<n;++i)grants[count++]={resolved[i].type,resolved[i].id,resolved[i].amount};return true;}
    return local_supply_rewards::grant(grants,128,count,r.type,r.id,r.amount);
}
static const supply_content::Item* grant_source(int box,const supply_types::Grant& grant,const local_progress::State& s){
    const auto* b=competition_track_content::box(box);if(!b)return nullptr;
    for(int i=0;i<b->count;++i){const auto& item=competition_track_content::items[b->first+i];if(item.prob<=0||effective_box_policy::removed(item.type,item.type_id))continue;
        local_supply_rewards::Candidates candidates;if(local_supply_rewards::candidates(item,s,2,candidates)!=supply_policy::Eligible||candidates.type!=grant.type)continue;
        for(int j=0;j<candidates.count;++j)if(candidates.ids[j]==grant.id)return &item;}
    return nullptr;
}
static void* message(const local_progress::State& s,Roots& roots){
    const auto& ledger=s.competition_tracks;if(!ledger.pending||!competition_track_types::valid(ledger))return nullptr;
    const auto& t=ledger.tracks[ledger.pending-1];if(t.competition!=competition_track_content::competition||t.definition!=competition_track_content::definition)return nullptr;
    const auto& r=competition_track_content::rewards[ledger.pending_premium][ledger.pending_index];bool box=r.type==14;
    void* msg=create("com.geargames.aow.messages",box?"BoxRewardMessage":"RewardNotificationMessage",roots);
    void* rows=msg?list(msg,"items",roots):nullptr;if(!msg||!rows)return nullptr;
    void* ex=nullptr;
    for(uint32_t i=0;i<ledger.grant_count;++i){Roots one;const auto& g=ledger.grants[i];Reward text=r;
        void* source=nullptr;
        if(box){const auto* entry=grant_source(r.id,g,s);source=entry?local_supply::original_item(r.id,entry->id):nullptr;if(!source)return nullptr;
            text={g.type,g.id,g.amount,g.amount,false,nullptr,nullptr,nullptr,nullptr};}
        else{text.type=g.type;text.id=g.id;text.amount=text.limited=g.amount;text.random=false;}
        void* item=reward(text,one);if(!item)return nullptr;
        if(box){*(int*)((char*)item+0x18)=*(int8_t*)((char*)source+0x28);
            ref(item,"name",get(source,0x38));ref(item,"hint",get(source,0x38));ref(item,"desc",get(source,0x38));ref(item,"icon",get(source,0x30));}
        void* d=g.type==4?local_career::lookup("BATTLE","boosts",g.id):g.type==8?local_career::skill_card(g.id):g.type==9?local_career::lookup("SHARED","heroCards",g.id):nullptr;
        if(d){ref(item,"name",get(d,0x18));ref(item,"hint",get(d,0x20));ref(item,"desc",get(d,0x20));ref(item,"icon",get(d,g.type==4?0x48:g.type==8?0x30:0x28));
            if(g.type==4)*(int*)((char*)item+0x18)=*(int8_t*)((char*)d+0x50);}
        void* a[]={item};invoke_named(rows,"Add",1,a,&ex);if(ex)return nullptr;
    }
    if(!ledger.grant_count){Roots one;void* item=reward(r,one);if(!item)return nullptr;void* a[]={item};invoke_named(rows,"Add",1,a,&ex);if(ex)return nullptr;}
    if(box){void* b=local_faction::clone(local_supply::dictionary_list(get(saved("SHARED"),0x130),r.id),roots);if(!b)return nullptr;
        *(int*)((char*)b+0x58)=0;ref(msg,"box",b);ref(msg,"header",get(b,0x28));*(bool*)((char*)msg+0x45)=true;*(int*)((char*)msg+0x5c)=0;}
    else ref(msg,"headerStr",string_new("合作奖励"));
    return msg;
}
static bool annotate(void* view){
    void* pool=get(view,0xc0),*field=pool?class_field_from_name(object_class(pool),"m_poolObjects"):nullptr;
    void* rows=field?get(pool,field_offset(field)):nullptr;int count=rows?*(int*)((char*)rows+0x18):0;if(count!=20)return false;
    void* ex=nullptr;
    for(int i=0;i<count;++i)for(int premium=0;premium<2;++premium){void* milestone=list_item(rows,i),*cell=get(milestone,premium?0x30:0x28);
        void* event=get(cell,0x50),*value=get(event,0x28);if(!value)return false;
        // Keep the original gift slot/icon/amount in the immutable model. Its
        // existing numeric label now states that the local gift is cancelled;
        // the original info button supplies the full reason from RewardItem.
        // The original resource indicator has separate current/max text
        // channels. Its gift layout displays the latter; annotate both and
        // stop their numeric animation so +N cannot overwrite the notice.
        for(int offset:{0x40,0x48})if(void* text=get(value,offset)){
            invoke_named(text,"StopIncreasing",0,nullptr,&ex);if(ex)return false;
            ref(text,"m_prefix",string_new("无需"));void* a[]={string_new("无需")};invoke_named(text,"SetValueString",1,a,&ex);if(ex)return false;}
        const auto& d=competition_track_content::rewards[premium][i];
        if(!supported(d,view_state())){int unavailable=1;void* b[]={&unavailable};invoke_named(cell,"SetState",1,b,&ex);if(ex)return false;}
        if(obsolete(d)){void* label=get(cell,0x68);void* b[]={string_new(d.type==7?"无需加速":"无需钥匙")};if(label)invoke_named(label,"set_text",1,b,&ex);if(ex)return false;}
    }
    return true;
}
static bool refresh(void* view){
    if(!view||local_faction::active!=2)return false;Roots roots;void* p=model(view_state(),roots),*ex=nullptr;if(!p)return false;
    int block=0;bool force=true;void* a[]={p,&block,&force};
    runtime_invoke(class_method(object_class(view),"UpdateInfo",3),view,a,&ex);return !ex&&annotate(view);
}
static bool commit(local_progress::State& s,const char* root,int row,bool premium){
    supply_types::Grant grants[128];int count=0;local_progress::StateBuffer copy;if(!copy.data)return false;*copy.data=s;
    if(!prepare(s,row,premium,grants,count)||!competition_track_store::claim(*copy.data,row,premium,grants,count))return false;
    Roots roots;if(!message(*copy.data,roots)||!local_progress::write(root,*copy.data))return false;s=*copy.data;return true;
}
static bool publish_private(){
    if(!private_state||!private_state->competition_tracks.pending)return false;Roots roots;
    void* msg=message(*private_state,roots),*dispatcher=get(offline_network,0x88),*ex=nullptr;if(!msg||!dispatcher)return false;
    void* a[]={msg};invoke_named(dispatcher,"Dispatch",1,a,&ex);if(ex)return false;
    local_progress::StateBuffer copy;if(!copy.data)return false;*copy.data=*private_state;
    if(!competition_track_policy::dismiss(copy.data->competition_tracks)||!local_progress::write(private_root,*copy.data))return false;
    *private_state=*copy.data;return true;
}
static bool publish_pending(){
    if(!progress_ready||local_faction::active!=2||!progress.competition_tracks.pending)return true;
    Roots roots;void* msg=message(progress,roots),*dispatcher=get(offline_network,0x88);
    if(!msg||!dispatcher||!apply_local_progress()||!local_items::publish()||!local_modifications::publish()||!local_supply::refresh())return false;
    int gold=0;if(!local_faction::account_gold(progress,gold))return false;
    int values[]={progress.credits,gold,progress.supply.tokens};const char* names[]={"SetProfileCredits","SetAccountGold","SetProfileTokens"};void* ex=nullptr;
    for(int i=0;i<3&&!ex;++i){void* update=create("com.geargames.aow.messages",names[i],roots);if(!update)return false;
        void* a[]={values+i};invoke_named(update,".ctor",1,a,&ex);void* b[]={update};if(!ex)invoke_named(dispatcher,"Dispatch",1,b,&ex);}
    void* a[]={msg};if(!ex)invoke_named(dispatcher,"Dispatch",1,a,&ex);if(ex)return false;
    local_progress::StateBuffer copy;if(!copy.data)return false;*copy.data=progress;
    if(!competition_track_policy::dismiss(copy.data->competition_tracks)||!local_progress::write(local_faction::root(),*copy.data))return false;
    progress=*copy.data;LOG("Local competition track original reward published receipt_ack=1 claims=%llu",(unsigned long long)progress.competition_tracks.claims);return true;
}
static void (*claim_original)(void*,int,bool,void*);
static void claim(void* self,int row,bool premium,void* method){
    if(!offline_network){claim_original(self,row,premium,method);return;}
    if(!installed||busy||!progress_ready||local_faction::active!=2||!self||self!=current_view||*(int*)((char*)self+0xec)!=competition_track_content::competition)return;
    busy=true;struct Release{~Release(){busy=false;}} release;
    auto& state=private_state?*private_state:progress;
    if(state.competition_tracks.pending){if(private_state)publish_private();else publish_pending();refresh(self);return;}
    if(state.supply.pending||state.shop.pending||state.contract_boxes.pending||row<0||row>=20||
       !competition_track_policy::can_claim(state.competition_tracks,competition_track_content::competition,competition_track_content::definition,row,premium,competition_track_content::thresholds[row]))return;
    if(!commit(state,private_state?private_root:local_faction::root(),row,premium))return;
    LOG("Local competition track claim committed competition=10140 row=%d premium=%d grants=%u claims=%llu private_ui=%d",row,premium,state.competition_tracks.grant_count,(unsigned long long)state.competition_tracks.claims,private_state!=nullptr);
    refresh(self);if(private_state)publish_private();else publish_pending();
}
static bool initiate(void* view,void* profile,void* memory){
    if(!installed||!view||!profile)return false;void* ex=nullptr;
    int block=0,id=competition_track_content::competition;bool allowed=true;
    void* a[]={profile,&block,&id,&allowed,nullptr};invoke_named(view,"Initiate",5,a,&ex);if(ex)return false;
    uintptr_t handle=gchandle_new(view,true);if(!handle)return false;if(view_root)gchandle_free(view_root);view_root=handle;current_view=view;
    // Both original rows are unlocked in the model; its purchase control has no
    // local operation, and no account VIP flag or membership gift is created.
    local_supply::hide(get(view,0x30));return annotate(view);
}
static void install(){
    const uint32_t signature[]={0xa9bc67fe,0xa9015ff8,0xa90257f6,0xa9034ff4};
    claim_original=(decltype(claim_original))install_hook(0x7c55930,signature,(void*)claim);installed=claim_original;
    LOG("Local competition track hook=%d original_rows=2 free_manual_once=1 season_exchange=0",installed);
}
}
