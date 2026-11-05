// One contract-point source, the two original reward rows, no calendar tasks.
#pragma once
#include "contract_box_store.h"
namespace local_contract_boxes {
using Roots=local_faction::Roots;
static void* boxes[7];static uintptr_t handles[7];static bool ready,installed,busy;
static bool initialize(){
    if(ready)return true;
    void* cls=find_class("com.geargames.aow.entities","SupplyBox");if(!cls)return false;
    for(int j=0;j<7;++j){const auto& b=contract_box_content::boxes[j];const auto& a=contract_box_content::appearances[j];
        void* rows=local_supply::dictionary_list(local_supply::get(saved("SHARED"),0xa8),b.id);int n=rows?*(int*)((char*)rows+0x18):0;if(n!=b.count)return false;
        for(int k=0;k<b.count;++k){const auto& d=contract_box_content::items[b.first+k];void* v=local_supply::original_item(b.id,d.id);
            if(!v||*(int*)((char*)v+0x14)!=b.id||*(int*)((char*)v+0x18)!=d.prob||*(int*)((char*)v+0x1c)!=d.prob_txt||*(int*)((char*)v+0x20)!=d.type||*(int*)((char*)v+0x24)!=d.type_id||*(int*)((char*)v+0x40)!=d.minimum||*(int*)((char*)v+0x44)!=d.maximum)return false;}
        if(boxes[j])continue;Roots roots;void* v=object_new(cls),*ex=nullptr;if(!roots.keep(v))return false;invoke_named(v,".ctor",0,nullptr,&ex);if(ex)return false;
        *(int*)((char*)v+0x10)=b.id;*(int*)((char*)v+0x14)=j<5?10:11;*(int*)((char*)v+0x18)=b.rank;*(int*)((char*)v+0x1c)=b.points;
        *(int8_t*)((char*)v+0x20)=b.active;*(int8_t*)((char*)v+0x40)=b.minimum;*(int8_t*)((char*)v+0x41)=b.maximum;
        for(int k=0;k<4;++k){const char* text=k==0?a.name:k==1?a.icon:k==2?a.hint:a.small;local_supply::ref(v,k==0?"name":k==1?"icon":k==2?"hint":"iconSmall",text?string_new(text):nullptr);}
        auto barrier=(void(*)(void*,void**,void*))dlsym(il2cpp,"il2cpp_gc_wbarrier_set_field");if(!barrier)return false;
        for(int k=0;k<2;++k){void* arr=array_new(find_class("System","String"),3);if(!roots.keep(arr))return false;
            for(int i=0;i<3;++i)barrier(arr,((void**)((char*)arr+32))+i,string_new(k?a.bar_values[i]:a.bar_icons[i]));
            local_supply::ref(v,k?"barValues":"barIcons",arr);}
        handles[j]=gchandle_new(v,true);if(!handles[j])return false;boxes[j]=v;
    }
    ready=true;LOG("Local contract boxes original definitions matched boxes=7 items=77 group=4");return true;
}
static void* original_box(int id){if(!initialize())return nullptr;for(int i=0;i<7;++i)if(contract_box_content::boxes[i].id==id)return boxes[i];return nullptr;}
static bool localize(){
    void* locale=static_object("com.geargames.aow.config","ClientContext","m_localizationContext");
    void* ex=nullptr;void* strings=locale?invoke_named(locale,"get_CurrentStrings",0,nullptr,&ex):nullptr;
    const void* setter=strings?class_method(object_class(strings),"set_Item",2):nullptr;if(ex||!setter)return false;
    const int keys[]={7953,7954,7956,7957};const char* values[]={"合同奖励","长期奖励","完成合同所得积分同时推进两排奖励；各排领完循环，不限日期。军衔4起，高军衔沿用现有箱组（单机延续）。","完成合同所得积分同时推进两排奖励；各排领完循环，不限日期。军衔4起，高军衔沿用现有箱组（单机延续）。"};
    for(int i=0;i<4;++i){int key=keys[i];void* text=string_new(values[i]);void* args[]={&key,text};runtime_invoke(setter,strings,args,&ex);if(ex)return false;}return true;
}
static bool configuration(void* cfg,const local_progress::State& s,Roots& roots){
    if(!installed)return false;
    if(local_skill::rank(s)<4)return true;
    if(!initialize()||!localize()||!contract_box_types::valid(s.contract_boxes,s.contract_points))return false;
    for(int track=0;track<2;++track){
        void* rows=local_supply::get(cfg,track?0x90:0x88),*ex=nullptr;
        if(!rows){void* field=class_field_from_name(object_class(cfg),track?"boxesWeek":"boxes");
            rows=field?object_new(class_from_type(field_type(field))):nullptr;if(!roots.keep(rows))return false;
            invoke_named(rows,".ctor",0,nullptr,&ex);if(ex)return false;field_set(cfg,field,rows);}
        if(contract_box_policy::group(s.contract_boxes,track,local_skill::rank(s))==4){
            int count=track?2:5;for(int i=0;i<count;++i){void* b=local_faction::clone(boxes[(track?5:0)+i],roots);if(!b)return false;
                const auto& d=contract_box_content::boxes[(track?5:0)+i];
                if(!local_supply_rewards::supported_pool(d,contract_box_content::items,s,local_faction::active))local_supply::ref(b,"hint",string_new("奖励暂不可用，合同积分会保留。"));
                void* args[]={b};invoke_named(rows,"Add",1,args,&ex);if(ex)return false;}
        }
        *(int*)((char*)cfg+(track?0x84:0x80))=contract_box_policy::balance(s.contract_boxes,track,s.contract_points);
        int step=s.contract_boxes.tracks[track].step;
        *(int*)((char*)cfg+(track?0x9c:0x98))=step?contract_box_content::boxes[(track?5:0)+step-1].id:0;
    }
    return true;
}
static void set_active(void* c,bool yes){if(!c)return;void* ex=nullptr;void* go=!strcmp(class_name(object_class(c)),"GameObject")?c:invoke_named(c,"get_gameObject",0,nullptr,&ex);void* args[]={&yes};if(go&&!ex)invoke_named(go,"SetActive",1,args,&ex);}
static void info(void* self,void* rows,uint64_t,int last,int points,void*){
    if(!self)return;local_contracts::hide(local_supply::get(self,0x28));local_contracts::hide(local_supply::get(self,0x38));
    int n=rows?*(int*)((char*)rows+0x18):0;void* controls=local_supply::get(self,0x20);int count=controls?*(int*)((char*)controls+0x18):0;
    if(n<0||n>5||count<0||count>16)return;bool week=*(bool*)((char*)self+0x48);int track=week?1:0;
    if(!n&&local_skill::rank(progress)>=4){void* message=local_supply::get(self,0x38);set_active(message,true);void* text=string_new("原奖励内容待补齐，积分保留。");void* args[]={text};void* error=nullptr;if(message)invoke_named(message,"set_text",1,args,&error);}
    void* label=local_supply::get(self,0x30);set_active(label,n>0);
    void* ex=nullptr;if(n>0&&label){int max=contract_box_content::boxes[week?6:4].points;
        void* a[]={rows};void* scale=invoke_named(self,"GetScaleProgress",1,a,&ex);void* args[]={&points,&max,scale};if(!ex)invoke_named(label,"Initiate",3,args,&ex);}
    for(int i=0;i<count&&!ex;++i){void* control=list_item(controls,i);set_active(control,i<n);if(i>=n)continue;
        void* b=list_item(rows,i);int id=b?*(int*)((char*)b+0x10):0;const auto* d=contract_box_content::box(id);if(!d)return;
        bool supported=local_supply_rewards::supported_pool(*d,contract_box_content::items,progress,local_faction::active);
        int state=id<=last?2:contract_box_policy::can_claim(progress.contract_boxes,progress.contract_points,track,local_skill::rank(progress),id)&&supported&&!progress.supply.pending&&!progress.shop.pending?3:0;
        void* hint=string_new("完成合同积累积分，达到条件后手动领取。"),*unavailable=string_new("奖励暂不可用，积分会保留。");
        void* args[]={b,&state,&points,hint,unavailable,hint};invoke_named(control,"Initiate",6,args,&ex);
    }
    if(ex)LOG("Local contract box original row publication exception=%s",class_name(object_class(ex)));
}
static void (*init_original)(void*,void*,uint64_t,int,int,void*);
static void initiate(void* self,void* rows,uint64_t date,int last,int points,void* method){
    if(!offline_network){init_original(self,rows,date,last,points,method);return;}info(self,rows,date,last,points,method);
}
static void (*open_original)(void*,void*,void*);
static void open(void* self,void* control,void* method){
    if(!offline_network){open_original(self,control,method);return;}
    if(!installed||busy||!progress_ready||local_skill::rank(progress)<4||!self||!control)return;
    busy=true;struct Release{~Release(){busy=false;}} release;
    if(progress.contract_boxes.pending){local_supply::publish_pending(false,true);return;}
    if(progress.competition_tracks.pending||progress.supply.pending||progress.shop.pending)return;
    void* controls=local_supply::get(self,0x20);bool bound=false;int n=controls?*(int*)((char*)controls+0x18):0;if(n<0||n>16)return;
    for(int i=0;i<n;++i)if(list_item(controls,i)==control)bound=true;if(!bound)return;
    int track=*(bool*)((char*)self+0x48)?1:0;void* box=local_supply::get(control,0x88);int id=box?*(int*)((char*)box+0x10):0;
    void* cfg=local_supply::get(saved("UserConfiguration"),0x128),*rows=local_supply::get(cfg,track?0x90:0x88);bool live=false;
    for(int i=0;rows&&i<*(int*)((char*)rows+0x18);++i)if(list_item(rows,i)==box)live=true;
    const auto* d=contract_box_policy::next(progress.contract_boxes,track,local_skill::rank(progress));
    if(!live||!d||!contract_box_policy::can_claim(progress.contract_boxes,progress.contract_points,track,local_skill::rank(progress),id))return;
    supply_policy::Draw draw;supply_types::Grant grants[128];int count=0;
    if(!local_supply_rewards::prepare_pool(*d,contract_box_content::items,progress.contract_boxes.claims,progress,local_faction::active,draw,grants,128,count))return;
    local_progress::StateBuffer storage;if(!storage.data)return;auto& next=*storage.data;next=progress;
    if(!contract_box_store::claim(next,track,local_skill::rank(progress),id,draw,grants,count))return;
    Roots roots;if(!local_supply::reward_message(next,roots,false,true)||!local_progress::write(local_faction::root(),next))return;
    progress=next;LOG("Local contract box committed id=%d track=%d claim=%llu points=%d",id,track,(unsigned long long)next.contract_boxes.claims,next.contract_points);
    local_supply::publish_pending(false,true);
}
static void install(){
    const uint32_t a[]={0xa9bc67fe,0xa9015ff8,0xa90257f6,0xa9034ff4};
    const uint32_t b[]={0xa9bd5ffe,0xa90157f6,0xa9024ff4,0x9000ee16};
    init_original=(decltype(init_original))install_hook(0x7ff2de4,a,(void*)initiate);
    open_original=(decltype(open_original))install_hook(0x7ff34a4,b,(void*)open);
    installed=init_original&&open_original;
    if(installed){local_supply::external_box=original_box;local_contracts::box_configuration=configuration;local_contracts::box_info=info;}
    LOG("Local contract boxes hooks=%d original_rows=2 shared_points=1 calendar=0",installed);
}
}
