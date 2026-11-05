// Original shop button, reward window and the shared box RNG/wallet services.
#pragma once
namespace local_shop_boxes {
using Roots=local_faction::Roots;
static bool installed,busy,content_ready;
static void (*update_original)(void*,void*);
static void (*open_original)(void*,int,void*);
static bool initialize(){
    if(content_ready)return true;void* b=local_supply::original_box(12000);const auto& expected=shop_box_content::box;
    if(!b||*(int*)((char*)b+0x18)!=expected.rank||*(int8_t*)((char*)b+0x40)!=2||*(int8_t*)((char*)b+0x41)!=3)return false;
    void* rows=local_supply::dictionary_list(local_supply::get(saved("SHARED"),0xa8),12000);
    if(!rows||*(int*)((char*)rows+0x18)!=33)return false;
    for(const auto& i:shop_box_content::items){void* v=local_supply::original_item(12000,i.id);
        if(!v||*(int*)((char*)v+0x14)!=12000||*(int*)((char*)v+0x18)!=i.prob||*(int*)((char*)v+0x1c)!=i.prob_txt||
           *(int*)((char*)v+0x20)!=i.type||*(int*)((char*)v+0x24)!=i.type_id||*(int*)((char*)v+0x40)!=i.minimum||*(int*)((char*)v+0x44)!=i.maximum)return false;
    }content_ready=true;LOG("Local original free shop content matched box=12000 items=33");return true;
}
static bool supported(const local_progress::State& s,int faction){return installed&&initialize()&&local_skill::rank(s)>=1&&
    local_supply_rewards::supported_pool(shop_box_content::box,shop_box_content::items,s,faction);}
static bool prepare(const local_progress::State& s,int faction,supply_policy::Draw& d,supply_types::Grant* g,int& count){
    return supported(s,faction)&&local_supply_rewards::prepare_pool(shop_box_content::box,shop_box_content::items,s.shop.claims,s,faction,d,g,128,count);
}
static void update(void* self,void* method){
    if(!offline_network){update_original(self,method);return;}if(!self||!progress_ready)return;
    void* box=local_supply::original_box(12000);if(box)*(void**)((char*)self+0x98)=box;
    *(uint64_t*)((char*)self+0xa8)=0; // Original Show installs its original free-click delegate.
    update_original(self,method);
    bool usable=supported(progress,local_faction::active),ready=usable&&shop_box_policy::ready(progress.shop)&&!progress.supply.pending;
    void* button=local_supply::get(self,0x58),*glow=local_supply::get(self,0x70),*ex=nullptr;
    void* glow_args[]={&ready};if(glow)invoke_named(glow,"SetActive",1,glow_args,&ex);
    if(button){char text[160];if(!usable)snprintf(text,sizeof(text),"奖励暂不可用");
        else if(progress.shop.pending)snprintf(text,sizeof(text),"领取结果待恢复");
        else if(ready)snprintf(text,sizeof(text),"领取");else snprintf(text,sizeof(text),"完成对局 %u/3",progress.shop.games);
        int index=1;void* label=string_new(text);void* args[]={&index,label};invoke_named(button,"SetText",2,args,&ex);
        if(!ready){bool force=true;void* a[]={&force};invoke_named(button,"ShowHidden",1,a,&ex);}
    }
}
static void open(void* self,int ads_result,void* method){
    if(!offline_network){open_original(self,ads_result,method);return;}
    // Original OnBoxFreeButtonClick is an eight-byte wrapper passing -1.
    // Hook its full OpenBox callee, preserving adjacent method boundaries.
    if(ads_result!=-1||!installed||busy||!progress_ready||!self||strcmp(class_name(object_class(self)),"GUIMainStoreBoxesReconstructor"))return;
    busy=true;struct Release{~Release(){busy=false;}} release;
    if(progress.shop.pending){local_supply::publish_pending(true);update(self,nullptr);return;}
    if(progress.competition_tracks.pending||progress.contract_boxes.pending||progress.supply.pending||!shop_box_policy::ready(progress.shop)||local_supply::get(self,0x98)!=local_supply::original_box(12000))return;
    supply_policy::Draw draw;supply_types::Grant grants[128];int count=0;
    local_progress::StateBuffer storage;if(!storage.data)return;auto& next=*storage.data;next=progress;
    if(!local_contracts::prepare_state(progress,next)||!prepare(progress,local_faction::active,draw,grants,count)||!shop_box_store::claim(next,draw,grants,count))return;
    Roots roots;if(!local_supply::reward_message(next,roots,true)||!local_progress::write(local_faction::root(),next))return;
    progress=next;LOG("Local free shop claim committed box=12000 claim=%llu items=%u grants=%u",(unsigned long long)progress.shop.claims,progress.shop.draw_count,progress.shop.grant_count);
    // The durable shop receipt is the original category4 contract event. It
    // already committed with the box/wallet; UI refresh cannot add another open.
    if(!local_contracts::refresh())LOG("Local free shop contract view refresh deferred; durable receipt retained");
    local_supply::publish_pending(true);update(self,nullptr);
}
static void install(){
    const uint32_t u[]={0xf81d0ffe,0xa90157f6,0xa9024ff4,0xd000f775};
    update_original=(decltype(update_original))install_hook(0x7ec476c,u,(void*)update);
    open_original=(decltype(open_original))local_boost_conversion::set_shop_handler(open);
    installed=update_original&&open_original;LOG("Local free shop hooks=%d original_entry=1 first_plus_three=1 calendar_wait=0",installed);
}
}
