// Permanent local collection through the original configuration/model/messages.
#pragma once
namespace local_modifications {
using Roots=local_faction::Roots;
static bool installed,ready;
static void* dictionary_class,*list_class;
static uint64_t checked_generation[14];static int checked_faction[14];static bool checked_result[14],checked[14];
static void* get(void* p,int o){return p?*(void**)((char*)p+o):nullptr;}
static void ref(void* p,const char* name,void* value){field_set(p,class_field_from_name(object_class(p),name),value);}
static void* generic(const char* name,void** arguments,int count){
    if(!class_from_system_type||count<1||count>2)return nullptr;Roots roots;
    void* k=find_class("System.Collections.Generic",name),*type=k?type_object(class_type(k)):nullptr,*types=array_new(find_class("System","Type"),count);
    if(!roots.keep(type)||!roots.keep(types))return nullptr;
    for(int i=0;i<count;++i){void* t=arguments[i]?type_object(class_type(arguments[i])):nullptr;if(!roots.keep(t))return nullptr;((void**)((char*)types+32))[i]=t;}
    void* ex=nullptr;void* args[]={types};void* made=invoke_named(type,"MakeGenericType",1,args,&ex);return !ex&&made?class_from_system_type(made):nullptr;
}
static bool classes(){
    if(dictionary_class&&list_class)return true;
    void* a[]={find_class("System","Int32"),find_class("com.geargames.aow.entities.modification","UnitModificationSet")};dictionary_class=generic("Dictionary`2",a,2);
    void* b[]={find_class("com.geargames.aow.entities","UnitModification")};list_class=generic("List`1",b,1);return dictionary_class&&list_class;
}
static int gate_stars(int faction,int kind){
    int minimum=INT_MAX;void* rows=local_career::list(local_career::key(faction),"careerStones");
    for(int i=0;rows&&i<*(int*)((char*)rows+0x18);++i){void* stone=list_item(rows,i),*rewards=get(stone,0x18);
        for(int j=0;rewards&&j<*(int*)((char*)rewards+0x18);++j){void* r=list_item(rewards,j);if(*(int*)((char*)r+0x14)!=18)continue;
            void* m=local_career::lookup(local_career::key(faction),"gameMechanics",*(int*)((char*)r+0x18));int stars=*(int*)((char*)stone+0x20);
            if(m&&*(int*)((char*)m+0x38)==kind&&stars<minimum)minimum=stars;}}
    return minimum;
}
static int gate_rank(int faction,int kind){
    int stars=gate_stars(faction,kind);void* rows=local_career::list("SHARED","profileRanks");
    for(int i=0;rows&&i<*(int*)((char*)rows+0x18);++i){void* r=list_item(rows,i);if(*(int*)((char*)r+0x14)==stars)return *(int*)((char*)r+0x10);}return 0;
}
static bool mechanic(const local_progress::State& s,int faction,int kind){
    void* rows=local_career::list(local_career::key(faction),"careerStones");
    for(int i=0;rows&&i<*(int*)((char*)rows+0x18);++i){void* stone=list_item(rows,i);if(*(int*)((char*)stone+0x20)>s.stars||!local_progress::has_career(s,*(int*)((char*)stone+0x10)))continue;void* rewards=get(stone,0x18);
        for(int j=0;rewards&&j<*(int*)((char*)rewards+0x18);++j){void* r=list_item(rewards,j);if(*(int*)((char*)r+0x14)!=18)continue;void* m=local_career::lookup(local_career::key(faction),"gameMechanics",*(int*)((char*)r+0x18));if(m&&*(int*)((char*)m+0x38)==kind)return true;}}
    return false;
}
static bool unit_available(const local_progress::State& s,int faction,int unit){
    void* rows=local_career::list(local_career::key(faction),"careerStones");
    for(int i=0;rows&&i<*(int*)((char*)rows+0x18);++i){void* stone=list_item(rows,i);if(*(int*)((char*)stone+0x20)>s.stars||!local_progress::has_career(s,*(int*)((char*)stone+0x10)))continue;void* rewards=get(stone,0x18);
        for(int j=0;rewards&&j<*(int*)((char*)rewards+0x18);++j){void* r=list_item(rewards,j);if(*(int*)((char*)r+0x14)==21&&*(int*)((char*)r+0x18)==unit)return true;}}
    return false;
}
static void* researched_type(const modification_content::Definition& d,local_research_runtime::Roots& roots){
    void* native=local_mod_effects::original(d.id),*type=get(native,0x38);if(!type)return nullptr;
    if(d.faction!=local_faction::active)return type;
    auto* plan=new(std::nothrow) generic_unit_research::Plan{};if(!plan)return nullptr;bool ok=generic_unit_research::collect(*plan);void* result=type;
    for(int i=0;ok&&i<plan->count;++i){const auto& effect=plan->families[i];if(*(int*)((char*)effect.owner+0x10)==d.unit){result=generic_unit_research::copy_type(type,effect,roots);break;}}
    delete plan;return ok?result:nullptr;
}
static bool preflight(int id){
    const auto* d=local_mod_effects::definition(id);if(!d||!progress_ready)return false;int index=int(d-modification_content::definitions);
    if(checked[index]&&checked_generation[index]==progress.generation&&checked_faction[index]==local_faction::active)return checked_result[index];
    local_research_runtime::Roots roots;void* type=researched_type(*d,roots);int fields=0,globals=0;void* copy=type?local_mod_effects::copy_unit(type,id,roots,fields,globals):nullptr;auto* c=local_mod_effects::compiled(id);bool ok=copy&&c;
    for(int i=0;ok&&i<c->count;++i){const auto& g=c->groups[i];if(g.key.kind<3)continue;void* source=g.key.kind==3?get(type,0x20):local_career::lookup("BATTLE","mineTypes",g.key.entity);int count=0;ok=local_mod_effects::copy_scope(source,g.key.kind,*c,roots,count)!=nullptr;}
    checked[index]=true;checked_generation[index]=progress.generation;checked_faction[index]=local_faction::active;checked_result[index]=ok;return ok;
}
static bool supported(const local_progress::CareerReward& r,int faction){const auto* d=local_mod_effects::definition(r.id);return installed&&ready&&r.type==17&&r.amount>0&&d&&d->faction==faction&&!d->feature&&!d->event&&preflight(r.id);}
static supply_policy::Eligibility candidates(const supply_content::Item& item,const local_progress::State& s,int faction,local_supply_rewards::Candidates& out){
    if(!installed||!ready||item.type!=26)return supply_policy::Unsupported;local_supply_rewards::Candidates pool;pool.type=17;int rarity=item.type_id>=5000&&item.type_id<=5002?item.type_id-4999:0;
    if(!rarity&&!local_mod_effects::definition(item.type_id))return supply_policy::Unsupported;
    for(const auto& d:modification_content::definitions){if(d.faction!=faction||(rarity?d.rarity!=rarity:d.id!=item.type_id)||!unit_available(s,faction,d.unit))continue;
        if(d.feature||d.event||!preflight(d.id)||!local_supply_rewards::add(pool,d.id))return supply_policy::Unsupported;}
    out=pool;return pool.count?supply_policy::Eligible:supply_policy::Ineligible;
}
static void* construct(void* klass,Roots& roots){void* v=klass?object_new(klass):nullptr;if(!roots.keep(v))return nullptr;void* ex=nullptr;invoke_named(v,".ctor",0,nullptr,&ex);return ex?nullptr:v;}
static void* configuration(const local_progress::State& s,int faction,Roots& roots){
    int visual=gate_rank(faction,5),available=gate_rank(faction,6);if(!visual||!available||!classes())return nullptr;
    for(uint32_t i=0;i<s.stock_count;++i)if(s.stocks[i].type==17){const auto* d=local_mod_effects::definition(s.stocks[i].id);if(!d||d->faction!=faction||s.stocks[i].amount<=0)return nullptr;}
    void* cfg=construct(find_class("com.geargames.aow.messages.modification","UnitModificationConfiguration"),roots),*dictionary=construct(dictionary_class,roots),*expired=construct(list_class,roots);if(!cfg||!dictionary||!expired)return nullptr;
    ref(cfg,"modifications",dictionary);ref(cfg,"expiredModifications",expired);*(int*)((char*)cfg+0x20)=visual;*(int*)((char*)cfg+0x24)=available;
    int units[14]={},count=0;for(const auto& d:modification_content::definitions)if(d.faction==faction&&local_progress::stock_amount(s,17,d.id)>0){bool known=false;for(int j=0;j<count;++j)if(units[j]==d.unit)known=true;if(!known)units[count++]=d.unit;}
    for(int i=0;i<count;++i){Roots set_roots;void* set=construct(find_class("com.geargames.aow.entities.modification","UnitModificationSet"),set_roots),*list=construct(list_class,set_roots);if(!set||!list)return nullptr;ref(set,"modifications",list);void* put_on=nullptr;
        for(const auto& d:modification_content::definitions){int amount=local_progress::stock_amount(s,17,d.id);if(d.faction!=faction||d.unit!=units[i]||amount<=0)continue;Roots item_roots;void* native=local_mod_effects::original(d.id),*item=construct(find_class("com.geargames.aow.entities","UnitModification"),item_roots);if(!native||!item)return nullptr;
            ref(item,"modification",native);*(uint64_t*)((char*)item+0x18)=3155378975999999999ULL;*(int*)((char*)item+0x20)=amount;void* args[]={item};void* ex=nullptr;invoke_named(list,"Add",1,args,&ex);if(ex)return nullptr;
            if(local_progress::equipped_modification(s,units[i])==d.id)put_on=item;}
        if(local_progress::equipped_modification(s,units[i])&&!put_on)return nullptr;ref(set,"putOnModification",put_on);void* args[]={units+i,set};void* ex=nullptr;invoke_named(dictionary,"Add",2,args,&ex);if(ex)return nullptr;
    }
    for(uint32_t i=0;i<s.modification_equipment_count;++i){const auto& e=s.modification_equipment[i];const auto* d=local_mod_effects::definition(e.modification);if(!d||d->unit!=e.unit||d->faction!=faction||local_progress::stock_amount(s,17,e.modification)<=0)return nullptr;}
    return cfg;
}
static bool prepare_profile(void* user){
    if(!progress_ready||!user)return false;if(!installed)return !local_progress::stock_types(progress,17)&&!progress.modification_equipment_count;
    Roots roots;void* cfg=configuration(progress,local_faction::active,roots);if(!cfg){LOG("Local modification original configuration unavailable; no grants enabled");ready=false;return !local_progress::stock_types(progress,17)&&!progress.modification_equipment_count;}
    bool became_ready=!ready;ready=true;ref(user,"modificationConfiguration",cfg);if(became_ready&&!local_career::apply(user))return false;return true;
}
static void* service(){
    void* screen=static_object("com.geargames.aow.scenes.main","ClientMainContext","<Screen>k__BackingField"),*repository=get(screen,0x290),*dictionary=get(repository,0x20),*entries=get(dictionary,0x18);int n=dictionary?*(int*)((char*)dictionary+0x20):0;if(!entries||n<0||n>128)return nullptr;
    for(int i=0;i<n;++i){char* row=(char*)entries+32+i*24;if(*(int*)row<0)continue;void* value=*(void**)(row+16);if(value&&!strcmp(class_name(object_class(value)),"UnitModificationsConfigurationDataSource"))return value;}return nullptr;
}
static bool publish(){
    if(!installed||!ready)return !local_progress::stock_types(progress,17)&&!progress.modification_equipment_count;
    void* user=saved("UserConfiguration"),*dispatcher=get(offline_network,0x88),*source=service(),*cfg=get(user,0x170);if(!cfg||!dispatcher||!source)return false;
    Roots roots;void* message=object_new(find_class("com.geargames.aow.messages.modification","UnitModificationConfigurationUpdate"));if(!roots.keep(message))return false;void* ex=nullptr;void* args[]={cfg};invoke_named(message,".ctor",1,args,&ex);void* dispatch[]={message};if(!ex)invoke_named(dispatcher,"Dispatch",1,dispatch,&ex);
    if(!ex&&get(source,0x48)!=cfg)invoke_named(source,"SetModificationConfiguration",1,args,&ex);
    bool ok=!ex&&get(source,0x48)==cfg;LOG("Local original modification configuration published=%d owned=%u equipped=%u permanent=1",ok,local_progress::stock_types(progress,17),progress.modification_equipment_count);return ok;
}
static void (*equip_original)(void*,void*,void*);
static void equip(void* self,void* model,void* method){
    if(!offline_network){equip_original(self,model,method);return;}
    if(!installed||!ready||!self||!model||!mechanic(progress,local_faction::active,6))return;
    Roots roots;if(!roots.keep(self)||!roots.keep(model))return;void* ex=nullptr,*id_box=invoke_named(model,"get_Id",0,nullptr,&ex),*unit_box=!ex?invoke_named(model,"get_PrototypeId",0,nullptr,&ex):nullptr;
    int id=id_box?*(int*)((char*)id_box+16):INT_MIN,unit=unit_box?*(int*)((char*)unit_box+16):-1;void* prototype=get(self,0x20);
    if(ex||!id_box||!unit_box||!prototype||*(int*)((char*)prototype+0x10)!=unit||!unit_available(progress,local_faction::active,unit))return;
    if(id==-1)id=0;const auto* d=id?local_mod_effects::definition(id):nullptr;
    if(id&&( !d||d->unit!=unit||!supported({17,id,1},local_faction::active)||local_progress::stock_amount(progress,17,id)<=0))return;
    local_progress::StateBuffer storage;if(!storage.data)return;auto& next=*storage.data;next=progress;
    if(!local_progress::equip_modification(next,unit,id))return;void* cfg=configuration(next,local_faction::active,roots);if(!cfg||!local_progress::write(local_faction::root(),next))return;progress=next;ref(saved("UserConfiguration"),"modificationConfiguration",cfg);
    void* arg[]={model};invoke_named(get(self,0x50),"set_Value",1,arg,&ex);if(!ex)invoke_named(get(self,0x58),"OnNext",1,arg,&ex);bool published=publish();
    LOG("Local modification equipped saved=1 unit=%d id=%d ui=%d publication=%d cost=0 permanent_stock_retained=1",unit,id,!ex,published);
}
static void* (*amount_original)(int,int,void*,void*);
static void* amount(int type,int value,void* fallback,void* method){
    if(offline_network&&installed){if(type==17)return string_new("永久");if(type==15){char text[128];snprintf(text,sizeof(text),"×%d 单机无需赛事入场券",value);return string_new(text);}if(type==6&&local_career::obsolete_premium_gifts)return string_new("单机无需会员");
        if(type==7&&local_career::obsolete_speedup_gifts){char text[128];snprintf(text,sizeof(text),"×%d 单机研究已即时完成",value);return string_new(text);}}
    return amount_original(type,value,fallback,method);
}
static void* (*box_amount_original)(void*,void*);
static void* box_amount(void* item,void* method){
    if(offline_network&&installed&&item){int type=*(int*)((char*)item+0x20);
        if(type==26)return string_new("永久");
        if(type==25&&*(int*)((char*)item+0x24)==1&&*(int*)((char*)item+0x10)==12019)return string_new("×1 单机无需赛事入场券");
        if(local_career::obsolete_speedup_gifts&&local_items::speedup_id(type)){
            int low=*(int*)((char*)item+0x40),high=*(int*)((char*)item+0x44);char text[160];
            if(low==high)snprintf(text,sizeof(text),"×%d 单机研究已即时完成",low);
            else snprintf(text,sizeof(text),"×%d～%d 单机研究已即时完成",low,high);return string_new(text);}}
    return box_amount_original(item,method);
}
static bool enabled_mechanic(int type){return installed&&ready&&(type==5||type==6);}
static void install(){
    const uint32_t a[]={0xf81c0ffe,0xa9015ff8,0xa90257f6,0xa9034ff4};const uint32_t b[]={0xf81e0ffe,0xa9014ff4,0x7100601f,0x2a0103f3};const uint32_t c[]={0xf81e0ffe,0xa9014ff4,0xb4000320,0xf9400008};
    equip_original=(decltype(equip_original))install_hook(0x7ce270c,a,(void*)equip);amount_original=(decltype(amount_original))install_hook(0x7f258c0,b,(void*)amount);box_amount_original=(decltype(box_amount_original))install_hook(0x7f259dc,c,(void*)box_amount);
    installed=equip_original&&amount_original&&box_amount_original;local_career::modification_reward_supported=supported;local_career::extra_mechanic_supported=enabled_mechanic;local_supply_rewards::extra_candidates=candidates;
    local_career::obsolete_premium_gifts=installed;
    local_career::obsolete_speedup_gifts=installed;
    LOG("Local original modification service hooks=%d original_gates=1 permanent_collection=1",installed);
}
}
