// Read-only research projection at the original workshop stat factories.
// Purchasing, availability, rank gates and the canonical dictionary are untouched.
#pragma once
namespace research_view {
using Roots=local_research_runtime::Roots;
static void* get(void* p,int offset){return p?*(void**)((char*)p+offset):nullptr;}
static int count(void* p){return p?*(int*)((char*)p+0x18):0;}
static void ref(void* p,const char* name,void* value){field_set(p,class_field_from_name(object_class(p),name),value);}
struct Cache {void* side=nullptr;uintptr_t root=0;uint64_t generation=0;int faction=0;bool attempted=false;};
static Cache cache;
struct Model {void* value;uintptr_t root;int id,faction;bool building;};
static Model models[256];static int model_count;
static void* presenters[8];static uintptr_t presenter_roots[8];static int presenter_count;
static void invalidate(){if(cache.root)gchandle_free(cache.root);cache={};}
static void reset(){
    invalidate();while(model_count){auto& m=models[--model_count];if(m.root)gchandle_free(m.root);m={};}
    while(presenter_count){--presenter_count;gchandle_free(presenter_roots[presenter_count]);presenter_roots[presenter_count]=0;presenters[presenter_count]=nullptr;}
}
static void observe_presenter(void* self){
    for(int i=0;i<presenter_count;++i)if(presenters[i]==self)return;
    if(presenter_count==8)return;uintptr_t root=gchandle_new(self,true);
    if(root){presenters[presenter_count]=self;presenter_roots[presenter_count++]=root;}
}
static void observe(void* owner,void* source,bool building){
    if(!owner||!source||strcmp(class_name(object_class(owner)),"ArmyPrototypeModel"))return;
    void* f=get(source,0x20);if(!f)return;int faction=*(int*)((char*)f+0x10),id=*(int*)((char*)source+0x10);
    int slot=-1;for(int i=0;i<model_count;++i){auto& m=models[i];if(m.value==owner)return;if(m.id==id&&m.faction==faction&&m.building==building)slot=i;}
    if(slot<0){if(model_count==256)return;slot=model_count;}
    uintptr_t root=gchandle_new(owner,true);if(!root)return;
    if(slot==model_count)++model_count;
    if(models[slot].root)gchandle_free(models[slot].root);models[slot]={owner,root,id,faction,building};
}
static void* dictionary(void* side,const char* field,const char* source,Roots& roots){
    // Side fields are IDictionary interfaces, not instantiable dictionary types.
    void* types[]={find_class("System","Int32"),find_class("com.geargames.aow.entities",!strcmp(source,"unitTypes")?"UnitType":!strcmp(source,"mineTypes")?"MineType":"BuildingType")};
    void* klass=local_modifications::generic("Dictionary`2",types,2);
    void* result=klass?object_new(klass):nullptr;
    void* list=local_career::list("BATTLE",source),*ex=nullptr;
    if(!roots.keep(result)||!list||count(list)<1||count(list)>512){LOG("Local research view dictionary failed source=%s concrete=%d rows=%d",source,!!klass,count(list));return nullptr;}
    invoke_named(result,".ctor",0,nullptr,&ex);
    for(int i=0;i<count(list)&&!ex;++i){void* item=list_item(list,i);if(!item)return nullptr;
        int id=*(int*)((char*)item+0x10);void* args[]={&id,item};invoke_named(result,"Add",2,args,&ex);}
    if(ex)LOG("Local research view dictionary failed source=%s exception=%s",source,class_name(object_class(ex)));
    return ex?nullptr:result;
}
// Keep the large shared research plans out of initialize_offline_main's frame.
// Metadata and allocations run only when the original UI asks for attributes.
static __attribute__((noinline)) bool build(){
    Roots roots;void* side=object_new(find_class("com.geargames.aow.entities.battle","BattleSide"));
    if(!roots.keep(side))return false;
    void* units=dictionary(side,"$gg","unitTypes",roots),*buildings=dictionary(side,"$Gg","buildingTypes",roots);
    research_effects::Delta delta;void* hq=nullptr;
    if(!units||!buildings||!local_research_runtime::gather(delta,hq))return false;
    void* fraction=local_research_runtime::clone(get(hq,0x20),roots);if(!fraction)return false;
    ref(side,"$gg",units);ref(side,"$Gg",buildings);ref(side,"$Eg",fraction);
    void* mines=dictionary(side,"$hg","mineTypes",roots);if(!mines)return false;ref(side,"$hg",mines);
    ref(side,"$vG",local_career::list("BATTLE","heroParam"));*(int*)((char*)side+0x10)=1;
    bool ok=local_research_runtime::apply(nullptr,1,delta,hq,side)&&
        building_research::prepare_saved(side)&&generic_unit_research::prepare_saved(side)&&
        building_weapon_research::prepare_saved(side)&&mine_research::prepare_saved(side)&&mine_growth::prepare_saved(side)&&support_ability::prepare_saved(side);
    if(!ok)return false;
    fraction=get(side,0xd8);if(!fraction)return false;
    // The original resource/capacity stat calculators consume Fraction, not Side.
    *(int*)((char*)fraction+0x28)=*(int*)((char*)side+0x17c);
    *(int*)((char*)fraction+0x2c)=*(int*)((char*)side+0x180);
    *(int8_t*)((char*)fraction+0x38)=*(int8_t*)((char*)side+0x1a9);
    *(int8_t*)((char*)fraction+0x39)=*(int8_t*)((char*)side+0x1aa);
    uintptr_t root=gchandle_new(side,true);if(!root)return false;
    if(cache.root)gchandle_free(cache.root);cache.side=side;cache.root=root;
    LOG("Local research view prepared=1 faction=%d generation=%llu canonical_and_purchase_models_unchanged=1",local_faction::active,(unsigned long long)progress.generation);
    return true;
}
static bool ready(){
    if(!offline_network||!progress_ready)return false;
    if(cache.attempted&&cache.generation==progress.generation&&cache.faction==local_faction::active)return cache.side;
    invalidate();cache.attempted=true;cache.generation=progress.generation;cache.faction=local_faction::active;
    if(!build())LOG("Local research view projection failed faction=%d generation=%llu",cache.faction,(unsigned long long)cache.generation);
    return cache.side;
}
// Exact pointer identity excludes already projected heroes, synthetic maxima,
// scenario/captured prototypes and the live battle's private research types.
static void* projected(void* source,bool building,void* owner=nullptr){
    if(!offline_network||!progress_ready||!source)return source;
    void* f=get(source,0x20);if(!f||*(int*)((char*)f+0x10)!=local_faction::active)return source;
    int id=*(int*)((char*)source+0x10);
    observe(owner,source,building);
    if(local_career::lookup("BATTLE",building?"buildingTypes":"unitTypes",id)!=source)return source;
    if(!ready())return source;
    void* result=local_dictionary_item(get(cache.side,building?0xf8:0xf0),id);
    return result?result:source;
}
static void* fraction(void* original,void* source,void* copy){
    return copy!=source&&cache.side&&original==get(source,0x20)?get(cache.side,0xd8):original;
}
static void* (*building_base_original)(void*,int,void*,void*,void*,void*,void*);
static void* building_base(void* b,int level,void* max,void* logger,void* creator,void* owner,void* method){
    return building_base_original(projected(b,true,owner),level,max,logger,creator,owner,method);
}
static void* (*building_cost_original)(void*,int,void*,void*,void*);
static void* building_cost(void* b,int level,void* max,void* owner,void* method){return building_cost_original(projected(b,true,owner),level,max,owner,method);}
static void* (*building_produce_original)(void*,int,void*,void*,void*,void*);
static void* building_produce(void* b,int level,void* f,void* max,void* owner,void* method){
    void* copy=projected(b,true,owner);return building_produce_original(copy,level,fraction(f,b,copy),max,owner,method);
}
static void* (*building_special_original)(void*,int,void*,void*,void*,void*);
static void* building_special(void* b,int level,void* max,void* f,void* owner,void* method){
    void* copy=projected(b,true,owner);return building_special_original(copy,level,max,fraction(f,b,copy),owner,method);
}
static void* (*unit_base_original)(void*,void*,void*,void*,void*,void*,void*);
static void* unit_base(void* u,void* max,void* hero,void* logger,void* creator,void* owner,void* method){
    return unit_base_original(projected(u,false,owner),max,hero,logger,creator,owner,method);
}
static void* (*unit_cost_original)(void*,void*,void*,void*);
static void* unit_cost(void* u,void* max,void* owner,void* method){return unit_cost_original(projected(u,false,owner),max,owner,method);}
static void* (*unit_special_original)(void*,void*,void*,void*,void*,void*);
static void* unit_special(void* u,void* max,void* f,void* hero,void* owner,void* method){
    void* copy=projected(u,false,owner);return unit_special_original(copy,max,fraction(f,u,copy),hero,owner,method);
}
// Preserve aliases by following the actual supplied weapon reference through
// each original state/tier slot. Names/types alone do not identify a weapon.
static void* mapped_weapon(void* source,void* copy,void* weapon){
    if(copy==source)return weapon;
    void* a=get(source,0x18),*b=get(copy,0x18),*result=nullptr;
    if(count(a)!=count(b))return nullptr;
    for(int i=0;i<count(a);++i){void* aw=get(list_item(a,i),0x18),*bw=get(list_item(b,i),0x18);
        if(count(aw)!=count(bw))return nullptr;
        for(int j=0;j<count(aw);++j)if(list_item(aw,j)==weapon){void* candidate=list_item(bw,j);if(!candidate||(result&&result!=candidate))return nullptr;result=candidate;}}
    return result;
}
static void* (*weapon_original)(void*,void*,void*,void*,void*);
static void* weapon(void* self,void* w,void* parent,void* owner,void* method){
    bool building=parent&&!strcmp(class_name(object_class(parent)),"BuildingType");
    bool unit=parent&&!strcmp(class_name(object_class(parent)),"UnitType");
    void* copy=building||unit?projected(parent,building,owner):parent;
    void* mapped=copy!=parent?mapped_weapon(parent,copy,w):w;
    if(!mapped){LOG("Local research view weapon mapping refused id=%d",parent?*(int*)((char*)parent+0x10):-1);copy=parent;mapped=w;}
    return weapon_original(self,mapped,copy,owner,method);
}
static void* mapped_mine(void* mine,void* parent,void* owner=nullptr){
    if(!offline_network||!progress_ready||!mine||!parent||strcmp(class_name(object_class(parent)),"UnitType"))return mine;
    void* f=get(parent,0x20);int id=*(int*)((char*)parent+0x10),key=*(int*)((char*)mine+0x10);
    if(!f||*(int*)((char*)f+0x10)!=local_faction::active||(id!=111&&id!=125)||
       local_career::lookup("BATTLE","unitTypes",id)!=parent||local_career::lookup("BATTLE","mineTypes",key)!=mine)return mine;
    observe(owner,parent,false);if(!ready())return mine;
    void* copy=local_dictionary_item(get(cache.side,0x100),key);return copy?copy:mine;
}
static void* (*mine_original)(void*,void*,void*,void*,void*);
static void* mine(void* self,void* m,void* parent,void* owner,void* method){
    return mine_original(self,mapped_mine(m,parent,owner),parent,owner,method);
}
static void (*presenter_original)(void*,void*,int,void*,void*);
static bool rebind(void* presenter){
    void* ex=nullptr;void* p=get(presenter,0x38),*l=get(presenter,0x40),*mods=get(presenter,0x48);
    void* owner=p?invoke_named(p,"get_Value",0,nullptr,&ex):nullptr;
    void* level=l&&!ex?invoke_named(l,"get_Value",0,nullptr,&ex):nullptr;
    if(ex||!owner||!level||!mods)return false;
    // SetPrototype normally returns early for the same owner/tier/mod collection.
    // Rebind its existing collections after invalidating only its comparison key.
    ref(presenter,"m_currentModifications",nullptr);
    presenter_original(presenter,owner,*(int*)((char*)level+16),mods,nullptr);return true;
}
static void* first_stat(void* values){
    void* ex=nullptr;void* boxed=values?invoke_named(values,"get_Count",0,nullptr,&ex):nullptr;
    if(!boxed||ex||*(int*)((char*)boxed+16)<1)return nullptr;
    int zero=0;void* args[]={&zero};void* value=invoke_named(values,"get_Item",1,args,&ex);return ex?nullptr:value;
}
static void refresh(){
    invalidate();void* ex=nullptr;int updated=0;
    int n=model_count;for(int i=0;i<n;++i){auto m=models[i];if(m.faction!=local_faction::active)continue;
        void* stats=get(m.value,0x80);if(stats)invoke_named(stats,"Clear",0,nullptr,&ex);
        if(ex)break;
        ref(m.value,"m_weaponListCache",nullptr);
        invoke_named(m.value,"InvokeDataInternalUpdated",0,nullptr,&ex);if(ex)break;++updated;
    }
    int rebound=0;if(!ex)for(int i=0;i<presenter_count;++i)rebound+=rebind(presenters[i]);
    LOG("Local research view refresh applied=%d models=%d original_presenters=%d save_written=0",!ex,updated,rebound);
}
static __attribute__((noinline)) void audit(void* presenter){
    if(!progress_ready||access(DATA_ROOT "research-view-audit.request",F_OK)||!first_stat(get(presenter,0x70)))return;
    unlink(DATA_ROOT "research-view-audit.request");auto before=progress;Roots roots;
    if(!ready()){
        FILE* file=fopen(DATA_ROOT "research-view-audit.jsonl","w");
        if(file){fprintf(file,"{\"summary\":true,\"pass\":false,\"phase\":\"projection_prepare\"}\n");fclose(file);chmod(DATA_ROOT "research-view-audit.jsonl",0644);}return;
    }
    bool ok=true;int own=0,foreign=0,synthetic=0,aliases=0;int f=local_faction::active;
    void* rifle=local_career::lookup("BATTLE","unitTypes",f==1?0:100),*hq=local_career::lookup("BATTLE","buildingTypes",f==1?0:100);
    int hp=*(int*)((char*)rifle+0x34),base=*(int*)((char*)get(hq,0x20)+0x28),capacity=*(int8_t*)((char*)get(hq,0x20)+0x38);
    void* private_rifle=projected(rifle,false),*private_hq=projected(hq,true);
    ok=private_rifle&&private_hq&&private_rifle!=rifle&&private_hq!=hq;
    void* first_side=cache.side;ok=ok&&ready()&&cache.side==first_side;
    int private_hp=*(int*)((char*)private_rifle+0x34),hq_hp=*(int*)((char*)list_item(get(private_hq,0x18),1)+0x34);
    int private_base=*(int*)((char*)get(cache.side,0xd8)+0x28),private_inc=*(int*)((char*)get(cache.side,0xd8)+0x2c),private_capacity=*(int8_t*)((char*)get(cache.side,0xd8)+0x38);
    for(bool building:{false,true}){void* list=local_career::list("BATTLE",building?"buildingTypes":"unitTypes");
        for(int i=0;i<count(list);++i){void* source=list_item(list,i),*fraction=get(source,0x20),*copy=projected(source,building);
            bool ours=fraction&&*(int*)((char*)fraction+0x10)==f;
            if(!ours){ok=ok&&source==copy;++foreign;continue;}++own;
            void* clone=local_research_runtime::clone(source,roots);if(!clone){ok=false;break;}
            ok=ok&&projected(clone,building)==clone;++synthetic;
            // Each weapon reference must follow its exact original slot.
            void* rows=get(source,0x18);for(int r=0;r<count(rows);++r){void* ws=get(list_item(rows,r),0x18);
                for(int w=0;w<count(ws);++w){void* original=list_item(ws,w),*mapped=mapped_weapon(source,copy,original);
                    ok=ok&&mapped==list_item(get(list_item(get(copy,0x18),r),0x18),w);++aliases;}}
            // Keep at most one temporary source clone rooted at a time.
            if(roots.count)gchandle_free(roots.handles[--roots.count]);
        }}
    void* stats=get(presenter,0x70),*old_first=first_stat(stats);if(old_first)roots.keep(old_first);
    void* observed=get(presenter,0x38),*ex=nullptr;void* model=observed?invoke_named(observed,"get_Value",0,nullptr,&ex):nullptr;
    void* data=get(model,0x90),*canonical=get(data,0x18);
    // Exercise profile-cache teardown/recreation without switching or rewriting
    // either player profile. Original UI still owns the same model and view.
    int resets=0;auto target=(void*(*)(uint32_t))dlsym(il2cpp,"il2cpp_gchandle_get_target");
    ok=ok&&roots.keep(presenter)&&model&&canonical&&target;
    for(int i=0;i<2&&ok;++i){reset();observe(model,canonical,!strcmp(class_name(object_class(canonical)),"BuildingType"));
        bool alive=model_count==1&&models[0].root&&target((uint32_t)models[0].root)==model;ok=ok&&alive;resets+=alive;}
    observe_presenter(presenter);
    refresh();void* fresh_first=first_stat(stats);
    bool refreshed=!ex&&old_first&&fresh_first&&old_first!=fresh_first&&get(data,0x18)==canonical;
    bool isolated=!memcmp(&before,&progress,sizeof(progress))&&*(int*)((char*)rifle+0x34)==hp&&
        *(int*)((char*)get(hq,0x20)+0x28)==base&&*(int8_t*)((char*)get(hq,0x20)+0x38)==capacity;
    FILE* file=fopen(DATA_ROOT "research-view-audit.jsonl","w");
    if(file){fprintf(file,"{\"summary\":true,\"pass\":%s,\"own_types\":%d,\"foreign_unchanged\":%d,\"synthetic_unchanged\":%d,\"weapon_slots\":%d,\"rifle_source_hp\":%d,\"rifle_projected_hp\":%d,\"hq_tier1_hp\":%d,\"income_base\":%d,\"income_increment\":%d,\"capacity_base\":%d,\"cache_reset_cycles\":%d,\"original_presenter_rebound\":%s,\"canonical_and_player_state_unchanged\":%s,\"player_save_writes\":0,\"battle_runs\":0}\n",ok&&isolated&&refreshed?"true":"false",own,foreign,synthetic,aliases,hp,private_hp,hq_hp,private_base,private_inc,private_capacity,resets,refreshed?"true":"false",isolated?"true":"false");fclose(file);chmod(DATA_ROOT "research-view-audit.jsonl",0644);}
    LOG("Local research view audit passed=%d own=%d foreign=%d weapon_slots=%d original_presenter_rebound=%d isolated=%d",ok&&isolated&&refreshed,own,foreign,aliases,refreshed,isolated);
}
static void presenter(void* self,void* owner,int level,void* mods,void* method){
    presenter_original(self,owner,level,mods,method);
    if(!offline_network||!progress_ready)return;
    observe_presenter(self);
    audit(self);
}
static void install(){
    const uint32_t a[]={0xf81b0ffe,0xa90167fa,0xa9025ff8,0xa90357f6};
    const uint32_t b[]={0xf81c0ffe,0xa9015ff8,0xa90257f6,0xa9034ff4};
    const uint32_t c[]={0xa9bc67fe,0xa9015ff8,0xa90257f6,0xa9034ff4};
    const uint32_t d[]={0xa9bd5ffe,0xa90157f6,0xa9024ff4,0x90010456};
    const uint32_t e[]={0xa9bc67fe,0xa9015ff8,0xa90257f6,0xa9034ff4};
    const uint32_t w[]={0xa9ba7bfd,0xa9016ffc,0xa90267fa,0xa9035ff8};
    const uint32_t p[]={0xd10203ff,0xa9027bfd,0xa9036ffc,0xa90467fa};
    const uint32_t m[]={0xf81a0ffe,0xa9016ffc,0xa90267fa,0xa9035ff8};
    building_base_original=(decltype(building_base_original))install_hook(0x7d27534,a,(void*)building_base);
    building_cost_original=(decltype(building_cost_original))install_hook(0x7d27a6c,b,(void*)building_cost);
    building_produce_original=(decltype(building_produce_original))install_hook(0x7d27e64,c,(void*)building_produce);
    building_special_original=(decltype(building_special_original))install_hook(0x7d28274,b,(void*)building_special);
    unit_base_original=(decltype(unit_base_original))install_hook(0x7d29334,a,(void*)unit_base);
    unit_cost_original=(decltype(unit_cost_original))install_hook(0x7d29430,d,(void*)unit_cost);
    unit_special_original=(decltype(unit_special_original))install_hook(0x7d294e0,e,(void*)unit_special);
    weapon_original=(decltype(weapon_original))install_hook(0x803b6b8,w,(void*)weapon);
    mine_original=(decltype(mine_original))install_hook(0x803ba38,m,(void*)mine);
    presenter_original=(decltype(presenter_original))install_hook(0x7cd86c8,p,(void*)presenter);
    local_research_view_refresh=refresh;local_hero::clear_extra_displays=reset;
    LOG("Local research view hooks=%d%d%d%d%d%d%d%d%d mine=%d original_purchase_models=1",!!building_base_original,!!building_cost_original,!!building_produce_original,!!building_special_original,!!unit_base_original,!!unit_cost_original,!!unit_special_original,!!weapon_original,!!presenter_original,!!mine_original);
}
}
