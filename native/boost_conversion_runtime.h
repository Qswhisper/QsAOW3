// Original warehouse conversion button, retained recipes and atomic stock wallet.
#pragma once
#include "boost_conversion_policy.h"
namespace local_boost_conversion {
using Roots=local_faction::Roots;
static void* get(void* p,int o){return p?*(void**)((char*)p+o):nullptr;}
static bool same_text(void* a,void* b){
    if(!a||!b)return a==b;int n=*(int*)((char*)a+16),m=*(int*)((char*)b+16);
    return n>=0&&n<=256&&n==m&&!memcmp((char*)a+20,(char*)b+20,n*2);
}
static bool same_actions(void* a,void* b){
    if(!a||!b)return a==b;size_t n=array_length(a),m=array_length(b);
    return n<=64&&n==m&&!memcmp((char*)a+32,(char*)b+32,n);
}
static void* target(void* source){
    if(!source)return nullptr;int category=*(int8_t*)((char*)source+0x2a);
    if(category<0||category>1)return nullptr;void* result=nullptr;
    void* rows=local_career::list("BATTLE","boosts");int n=rows?*(int*)((char*)rows+0x18):0;
    for(int i=0;i<n;++i){void* candidate=list_item(rows,i);
        if(!candidate||*(int8_t*)((char*)candidate+0x2a)!=category+1||
           *(int8_t*)((char*)candidate+0x28)!=*(int8_t*)((char*)source+0x28)||
           *(int8_t*)((char*)candidate+0x29)!=*(int8_t*)((char*)source+0x29)||
           *(int8_t*)((char*)candidate+0x2b)!=*(int8_t*)((char*)source+0x2b)||
           !same_text(get(candidate,0x18),get(source,0x18))||!same_actions(get(candidate,0x30),get(source,0x30)))continue;
        if(result)return nullptr;result=candidate;
    }return result;
}
static bool unit_price(void* source,const local_progress::State& state,int& cost){
    if(!source||!local_skill::initialize())return false;
    int category=*(int8_t*)((char*)source+0x2a);if(category<0||category>1||!target(source))return false;
    void* misc=get(saved("UserConfiguration"),0x178),*card=local_career::skill_card(30);
    skill_tree_policy::Summary summary;if(!misc||!card||!local_skill::graph.summarize(state,summary))return false;
    int base=*(int*)((char*)misc+0x10+category*4),effect=*(int*)((char*)card+0x38);
    return boost_conversion_policy::price(base,effect,summary.level[30],cost);
}
static bool plan(const local_progress::State& state,void* source,int input,local_progress::State& next,int& cost,void*& output){
    output=target(source);if(!output||!unit_price(source,state,cost))return false;
    int id=*(int*)((char*)source+0x10),out=*(int*)((char*)output+0x10);
    if(source!=local_career::lookup("BATTLE","boosts",id)||
       *(int8_t*)((char*)source+0x2c)>local_skill::rank(state)||*(int8_t*)((char*)output+0x2c)>local_skill::rank(state))return false;
    next=state;return boost_conversion_policy::convert(next,id,out,input,cost);
}
struct Publication {void* remove=nullptr,*add=nullptr;};
static bool messages(void* source,void* output,int input,int count,Roots& roots,Publication& p){
    p.remove=object_new(find_class("com.geargames.aow.messages","DeleteBoosts"));
    p.add=object_new(find_class("com.geargames.aow.messages","AddBoosts"));
    void* boost=object_new(find_class("com.geargames.aow.entities.battle","Boost"));
    if(!roots.keep(p.remove)||!roots.keep(p.add)||!roots.keep(boost))return false;
    void* ex=nullptr;invoke_named(p.remove,".ctor",0,nullptr,&ex);if(!ex)invoke_named(p.add,".ctor",0,nullptr,&ex);
    void* a[]={source,&input};if(!ex)invoke_named(boost,".ctor",2,a,&ex);
    void* field=class_field_from_name(object_class(p.remove),"values");
    void* list=field?object_new(class_from_type(field_type(field))):nullptr;
    void* user=saved("UserConfiguration"),*profile=get(user,0x28);
    void* stock_field=profile?class_field_from_name(object_class(profile),"boosts"):nullptr;
    void* old=stock_field?get(profile,field_offset(stock_field)):nullptr;
    void* dictionary=old?object_new(object_class(old)):nullptr;
    if(ex||!roots.keep(list)||!roots.keep(dictionary))return false;
    invoke_named(list,".ctor",0,nullptr,&ex);if(!ex)invoke_named(dictionary,".ctor",0,nullptr,&ex);
    void* b[]={boost};if(!ex)invoke_named(list,"Add",1,b,&ex);
    void* c[]={output,&count};if(!ex)invoke_named(dictionary,"Add",2,c,&ex);
    if(ex)return false;local_skill::ref(p.remove,"values",list);local_skill::ref(p.add,"values",dictionary);return true;
}
static void (*modifier_original)(void*,void*),(*click_original)(void*,void*),(*shop_original)(void*,void*);
static void shop(void* self,void* method){if(!offline_network||installed)shop_original(self,method);}
static void muted(void*,void*){LOG("Local warehouse unsupported commercial/skill exchange action ignored");}
static void modifier(void* self,void* method){
    modifier_original(self,method);if(!offline_network||!progress_ready)return;
    int cost=0,multiple=*(int*)((char*)self+0x80);int64_t total=0;
    void* source=get(self,0x60),*output=target(source);
    bool allowed=output&&*(int8_t*)((char*)source+0x2c)<=local_skill::rank(progress)&&
        *(int8_t*)((char*)output+0x2c)<=local_skill::rank(progress);
    if(allowed&&unit_price(source,progress,cost)&&multiple>0)total=int64_t(cost)*multiple;
    if(total>0&&total<=INT_MAX)*(int*)((char*)self+0x30)=(int)total;
    else *(int*)((char*)self+0x68)=-1; // Original Initiate then disables its button; retain its price label.
}
static void click(void* self,void* method){
    if(!offline_network){click_original(self,method);return;}
    if(!installed||!progress_ready)return;modifier(self,nullptr);
    void* source=get(self,0x60),*output=nullptr;int input=*(int*)((char*)self+0x30),cost=0;
    local_progress::State next;
    if(!plan(progress,source,input,next,cost,output)){LOG("Local boost conversion refused before debit input=%d",input);return;}
    void* dispatcher=get(offline_network,0x88);if(!dispatcher||!class_method(object_class(dispatcher),"Dispatch",1))return;
    Roots roots;Publication publication;if(!messages(source,output,input,input/cost,roots,publication))return;
    if(!local_progress::write(local_faction::root(),next)){LOG("Local boost conversion save refused");return;}
    progress=next;void* ex=nullptr;void* a[]={publication.remove};invoke_named(dispatcher,"Dispatch",1,a,&ex);
    void* b[]={publication.add};if(!ex)invoke_named(dispatcher,"Dispatch",1,b,&ex);
    LOG("Local boost conversion saved=1 published=%d from=%d input=%d to=%d output=%d exception=%s",!ex,
        *(int*)((char*)source+0x10),input,*(int*)((char*)output+0x10),input/cost,ex?class_name(object_class(ex)):"none");
}
static void (*shop_open_handler)(void*,int,void*);
static void (*shop_open_original)(void*,int,void*);
static void shop_open(void* self,int result,void* method){
    if(!offline_network){shop_open_original(self,result,method);return;}
    if(shop_open_handler)shop_open_handler(self,result,method);
}
static void* set_shop_handler(void(*handler)(void*,int,void*)){
    if(!shop_open_original)return nullptr;shop_open_handler=handler;return (void*)shop_open_original;
}
static void install(){
    const uint32_t modifier_sig[]={0xa9be57fe,0xa9014ff4,0xf000f634,0xaa0003f3};
    const uint32_t click_sig[]={0xf81d0ffe,0xa90157f6,0xa9024ff4,0xf000f635};
    modifier_original=(decltype(modifier_original))install_hook(0x7eebd8c,modifier_sig,(void*)modifier);
    click_original=(decltype(click_original))install_hook(0x7eeb9b0,click_sig,(void*)click);
    bool muted_ok=true;
    {const uint32_t sig[]={0xf81e0ffe,0xa9014ff4,0xd000d8b4,0xaa0003f3};shop_original=(decltype(shop_original))install_hook(0x829eba8,sig,(void*)shop);}
    // Goods.OnClick selects the original detail/conversion panel, not a buy.
    // Keep that navigation; stop unsupported commerce at its actual buy boundary.
    {const uint32_t sig[]={0xa9bc67fe,0xa9015ff8,0xa90257f6,0xa9034ff4};muted_ok=install_hook(0x7be0230,sig,(void*)muted)!=nullptr&&muted_ok;}
    {const uint32_t sig[]={0xf81b0ffe,0xa90167fa,0xa9025ff8,0xa90357f6};muted_ok=install_hook(0x7be0324,sig,(void*)muted)!=nullptr&&muted_ok;}
    {const uint32_t sig[]={0xa9be57fe,0xa9014ff4,0xd000f775,0x9000c314};muted_ok=install_hook(0x7ec4950,sig,(void*)muted)!=nullptr&&muted_ok;}
    {const uint32_t sig[]={0xf81b0ffe,0xa90167fa,0xa9025ff8,0xa90357f6};shop_open_original=(decltype(shop_open_original))install_hook(0x7ec4aac,sig,(void*)shop_open);muted_ok=shop_open_original&&muted_ok;}
    {const uint32_t sig[]={0xa9bd5ffe,0xa90157f6,0xa9024ff4,0xd000f775};muted_ok=install_hook(0x7ec4d94,sig,(void*)muted)!=nullptr&&muted_ok;}
    {const uint32_t sig[]={0xf81d0ffe,0xa90157f6,0xa9024ff4,0xf000f755};muted_ok=install_hook(0x7ec7488,sig,(void*)muted)!=nullptr&&muted_ok;}
    {const uint32_t sig[]={0xf81d0ffe,0xa90157f6,0xa9024ff4,0x9000f755};muted_ok=install_hook(0x7eca374,sig,(void*)muted)!=nullptr&&muted_ok;}
    {const uint32_t sig[]={0xf81d0ffe,0xa90157f6,0xa9024ff4,0xd000f7b5};muted_ok=install_hook(0x7ebc898,sig,(void*)muted)!=nullptr&&muted_ok;}
    {const uint32_t sig[]={0xf81d0ffe,0xa90157f6,0xa9024ff4,0xd000f635};muted_ok=install_hook(0x7eec15c,sig,(void*)muted)!=nullptr&&muted_ok;}
    {const uint32_t sig[]={0xf81c0ffe,0xa9015ff8,0xa90257f6,0xa9034ff4};muted_ok=install_hook(0x7eec574,sig,(void*)muted)!=nullptr&&muted_ok;}
    installed=modifier_original&&click_original&&shop_original&&muted_ok;LOG("Local original boost conversion hooks=%d warehouse_entry=1 commercial_actions_muted=%d",installed,muted_ok);
}
}
