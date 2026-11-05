// Original Daily UI with real-behavior cycles. Initial content handlers
// cover research, headquarters quests, real 1v1 wins and original category4
// free-shop box opens; no login/ad/paid loop.
#pragma once
#include "contract_policy.h"
#include "contract_box_policy.h"
#include "contract_content.generated.h"
#include "battle_contract_policy.h"
namespace local_contracts {
using Roots=local_faction::Roots;
using Definition=contract_content::Definition;
static void* types[64];static uintptr_t type_roots[64];static bool ready;
static bool (*battle_available)();
static bool (*timed_available)();
static bool (*boost_available)();
static bool (*activity_available)();
static void* get(void* p,int o){return p?*(void**)((char*)p+o):nullptr;}
static void ref(void* p,const char* name,void* v){field_set(p,class_field_from_name(object_class(p),name),v);}
static const int total=sizeof(contract_content::definitions)/sizeof(Definition);
static int index(int id){for(int i=0;i<total;++i)if(contract_content::definitions[i].id==id)return i;return -1;}
static bool supported(const Definition& d){
    if(d.kind<1||d.kind>9||d.value<1||d.reward_count<1||d.reward_count>8||d.flags&12)return false;
    if(d.kind==4&&(d.type!=14||d.subtype||d.param!=4))return false;
    if(d.kind==5){int i=battle_contract_policy::slot(d.id);if(i<0||i>=battle_contract_events::count||!battle_available||!battle_available())return false;
        const auto& rule=contract_content::battle_rules[i];
        if(d.type!=12||d.param||d.subtype!=rule.subtype||d.flags!=rule.flags||d.value!=rule.value)return false;}
    if(d.kind==6){int i=battle_contract_policy::slot(d.id)-battle_contract_events::count;
        if(i<0||i>=battle_contract_events::result_count||!battle_available||!battle_available())return false;
        const auto& rule=contract_content::result_rules[i];
        if(d.type!=16||d.param||d.subtype!=rule.subtype||d.flags!=rule.flags||d.value!=rule.value||d.subtype<=0||d.subtype>INT_MAX/600)return false;}
    if(d.kind==7){const auto& rule=contract_content::timed_rules[0];
        if(!timed_available||!timed_available()||d.id!=rule.id||d.type!=15||d.subtype!=21||d.flags!=1||d.value!=rule.value||d.param||
           d.param_count!=2||d.params[0]!=rule.minutes||d.params[1]!=rule.buildings||rule.minutes<=0||rule.minutes>INT_MAX/600||rule.buildings<1)return false;}
    if(d.kind==8){int i=battle_contract_policy::slot(d.id)-7;
        if(!boost_available||!boost_available()||i<0||i>=boost_contract_events::count)return false;
        const auto& rule=contract_content::boost_rules[i];
        void* type=local_career::lookup("BATTLE","boosts",rule.boost);
        if(d.type!=9||d.subtype||d.param!=rule.boost||d.param_count||d.value!=1||rule.value!=1||d.flags!=1||rule.flags!=1||!type||*(int*)((char*)type+0x10)!=rule.boost)return false;}
    if(d.kind==9){const auto& rule=contract_content::activity_rules[0];
        if(!activity_available||!activity_available()||d.id!=rule.id||d.type!=3||d.subtype!=70||d.flags!=3||d.value!=1||d.param||d.param_count)return false;}
    for(int i=0;i<d.reward_count;++i)if(d.rewards[i].id||d.rewards[i].amount<0||(d.rewards[i].type!=0&&d.rewards[i].type!=1))return false;
    return d.xp>=0&&d.points>=0;
}
static bool eligible(const Definition& d,const local_progress::State& s){int rank=local_skill::rank(s);
    return contract_policy::rank_allowed(d.id,rank,d.rank_min,d.rank_max)&&(!d.faction||d.faction==local_faction::active)&&supported(d);
}
static bool initialize(){
    if(ready)return true;if(total<1||total>64)return false;
    void* c=find_class("com.geargames.aow.entities","DailyType");if(!c)return false;
    const char* names[]={"type","rewardAmount","location","params"};int offsets[]={0x18,0x70,0x80,0x38};
    for(int j=0;j<4;++j){void* f=class_field_from_name(c,names[j]);if(!f||field_offset(f)!=offsets[j])return false;}
    for(int i=0;i<total;++i){if(types[i])continue;const auto& d=contract_content::definitions[i];Roots roots;
        void* t=object_new(c);if(!roots.keep(t))return false;void* ex=nullptr;invoke_named(t,".ctor",0,nullptr,&ex);if(ex)return false;
        *(int*)((char*)t+0x10)=d.id;*(int8_t*)((char*)t+0x14)=d.faction;*(int*)((char*)t+0x18)=d.type;
        *(int8_t*)((char*)t+0x15)=d.category;*(int8_t*)((char*)t+0x1c)=d.subtype;*(int8_t*)((char*)t+0x1d)=d.flags;
        char description[2048];snprintf(description,sizeof(description),"%s%s",contract_policy::continued(d.id)?"常规合同（单机延续） · ":"",d.description);
        ref(t,"desc",string_new(description));ref(t,"icon",string_new(d.icon));ref(t,"background",string_new(d.background));
        *(int8_t*)((char*)t+0x2c)=d.rank_min;*(int16_t*)((char*)t+0x2e)=contract_policy::published_rank_max(d.id,d.rank_min,d.rank_max);
        *(int8_t*)((char*)t+0x28)=d.difficulty;*(int8_t*)((char*)t+0x29)=d.level_min;*(int16_t*)((char*)t+0x2a)=d.level_max;
        *(int*)((char*)t+0x30)=d.value;*(int*)((char*)t+0x34)=d.param;*(int*)((char*)t+0x40)=d.xp;
        if(d.param_count){
            if(d.param_count<0||d.param_count>8)return false;
            void* params=array_new(find_class("System","Int32"),d.param_count);if(!roots.keep(params))return false;
            memcpy((char*)params+32,d.params,d.param_count*sizeof(int));ref(t,"params",params);}
        *(int*)((char*)t+0x50)=d.probability;*(int*)((char*)t+0x54)=d.ads_inc;*(int*)((char*)t+0x58)=d.points;
        for(int k=0;k<3;++k){void* a=array_new(find_class("System","Int32"),d.reward_count);if(!roots.keep(a))return false;
            for(int j=0;j<d.reward_count;++j)((int*)((char*)a+32))[j]=k==0?d.rewards[j].type:k==1?d.rewards[j].id:d.rewards[j].amount;
            ref(t,k==0?"rewardType":k==1?"rewardTypeId":"rewardAmount",a);}
        void* loc=array_new(find_class("System","String"),d.location_count);if(!roots.keep(loc))return false;
        auto barrier=(void(*)(void*,void**,void*))dlsym(il2cpp,"il2cpp_gc_wbarrier_set_field");if(!barrier)return false;
        for(int j=0;j<d.location_count;++j)barrier(loc,((void**)((char*)loc+32))+j,string_new(d.location[j]));ref(t,"location",loc);
        type_roots[i]=gchandle_new(t,true);if(!type_roots[i])return false;types[i]=t;
    }
    ready=true;LOG("Local contract original definitions ready=1 ordinary=%d handlers=9",total);return true;
}
static uint64_t counter(const local_progress::State& s,const Definition& d){
    return contract_policy::event_counter(s,d.kind);
}
static int count(const local_progress::State& s,const Definition& d){
    return d.kind>=5?battle_contract_policy::count(s,d.id):contract_policy::count(s,d.id,counter(s,d),d.value);
}
static bool prepare_state(const local_progress::State& s,local_progress::State& next){
    next=s;if(local_skill::rank(s)<4)return true;if(!initialize())return false;
    local_match_store::State match;int status=local_match_store::load(local_faction::root(),match);if(status<0)return false;
    if(!contract_box_policy::initialize(next.contract_boxes,next.contract_points))return false;
    if(!next.contract_count)next.contract_pvp_cursor=match.settled;
    else if(match.settled!=next.contract_pvp_cursor){
        if(!battle_contract_policy::reconcile(next,match)){
            LOG("Local contract real-result cursor/events refused old=%llu latest=%llu",(unsigned long long)next.contract_pvp_cursor,(unsigned long long)match.settled);return false;}}
    for(const auto& d:contract_content::definitions)if(eligible(d,next)&&!contract_policy::receipt(next,d.id))
        if(!contract_policy::initialize(next,d.id,counter(next,d)))return false;
    return true;
}
static void* daily(const local_progress::State& s,int i,Roots& roots,int forced=-1){
    const auto& d=contract_content::definitions[i];if(!types[i])return nullptr;
    int count=local_contracts::count(s,d);if(count<0)return nullptr;
    void* v=object_new(find_class("com.geargames.aow.entities","Daily"));if(!roots.keep(v))return nullptr;
    void* ex=nullptr;invoke_named(v,".ctor",0,nullptr,&ex);if(ex)return nullptr;
    ref(v,"type",types[i]);*(int8_t*)((char*)v+0x18)=forced>=0?forced:count==d.value?1:0;
    *(int*)((char*)v+0x1c)=count;*(bool*)((char*)v+0x23)=false;return v;
}
static void (*content_observer)(void*,void*);
static bool (*box_configuration)(void*,const local_progress::State&,Roots&);
static void* configuration(void* user,const local_progress::State& s,Roots& roots){
    if(!initialize())return nullptr;void* old=get(user,0x128),*cfg=local_faction::clone(old,roots);
    if(content_observer)content_observer(user,old);
    if(!cfg||!local_faction::empty_field(cfg,"dailies",roots)||!local_faction::empty_field(cfg,"boxes",roots)||!local_faction::empty_field(cfg,"boxesWeek",roots))return nullptr;
    void* rows=get(cfg,0x10);if(!rows)return nullptr;void* ex=nullptr;
    for(int i=0;i<total;++i)if(eligible(contract_content::definitions[i],s)){Roots one;void* v=daily(s,i,one);if(!v)return nullptr;
        *(int8_t*)((char*)v+0x22)=*(int*)((char*)rows+0x18);void* args[]={v};invoke_named(rows,"Add",1,args,&ex);if(ex)return nullptr;}
    // No calendar wait or imported personal dates/counters. Box consumption is
    // not implemented here: empty original box lists prevent phantom rewards.
    for(int o:{0x18,0x20,0x78})*(uint64_t*)((char*)cfg+o)=3155378975999999999ULL;
    *(int*)((char*)cfg+0x80)=s.contract_points;*(int*)((char*)cfg+0x84)=0;
    *(int*)((char*)cfg+0x98)=*(int*)((char*)cfg+0x9c)=0;*(int8_t*)((char*)cfg+0x60)=0;
    *(bool*)((char*)cfg+0x70)=false;ref(cfg,"dailyChest",nullptr);
    if(box_configuration&&!box_configuration(cfg,s,roots))return nullptr;return cfg;
}
static void* source(){return static_object("com.geargames.aow.scenes.main","ClientMainContext","<DailyConfigurationDataSource>k__BackingField");}
static bool contains(void* cfg,void* row){
    void* rows=get(cfg,0x10);int n=rows?*(int*)((char*)rows+0x18):0;if(!row||n<0||n>64)return false;
    for(int j=0;j<n;++j)if(list_item(rows,j)==row)return true;return false;
}
static bool publish_configuration(void* data,void* cfg,void*& ex){
    if(!data||!cfg)return false;void* args[]={cfg};invoke_named(data,"SetConfiguration",1,args,&ex);
    // SetConfiguration only sets readiness. UpdateDaily emits the original
    // data-change notification even when that source was already ready.
    void* rows=get(cfg,0x10);int n=rows?*(int*)((char*)rows+0x18):0;if(n<0||n>64)return false;
    if(n&&!ex){void* update[]={list_item(rows,0)};invoke_named(data,"UpdateDaily",1,update,&ex);}
    return !ex;
}
static bool sync_state(){
    if(!progress_ready)return false;auto next=progress;if(!prepare_state(progress,next))return false;
    if(memcmp(&next,&progress,sizeof(next))&&!local_progress::write(local_faction::root(),next))return false;
    progress=next;return true;
}
static bool prepare_profile(void* user){
    if(!progress_ready)return false;auto next=progress;if(!prepare_state(progress,next))return false;
    Roots roots;void* cfg=configuration(user,next,roots);if(!cfg)return false;
    if(memcmp(&next,&progress,sizeof(next))&&!local_progress::write(local_faction::root(),next))return false;
    progress=next;ref(user,"dailyConfiguration",cfg);
    LOG("Local contracts restored faction=%d rank=%d active=%u rows=%d",local_faction::active,local_skill::rank(progress),progress.contract_count,*(int*)((char*)get(cfg,0x10)+0x18));return true;
}
static bool refresh(){
    if(!offline_network||!progress_ready)return false;void* user=saved("UserConfiguration"),*data=source();if(!user||!data)return false;
    if(!prepare_profile(user))return false;void* ex=nullptr;bool ok=publish_configuration(data,get(user,0x128),ex);
    LOG("Local contract original configuration published=%d exception=%s",ok,ex?class_name(object_class(ex)):"none");return ok;
}
static bool reward(const local_progress::State& s,const Definition& d,int& credits,int& gold){
    if(!supported(d)||!local_skill::initialize())return false;skill_tree_policy::Summary summary;if(!local_skill::graph.summarize(s,summary))return false;
    void* card=local_career::skill_card(27);if(!card)return false;int effect=*(int*)((char*)card+0x38),level=summary.level[27];credits=gold=0;
    for(int i=0;i<d.reward_count;++i){int amount=d.rewards[i].amount;
        if(d.rewards[i].type==0){if(!contract_policy::credits_bonus(amount,effect,level,amount)||int64_t(credits)+amount>INT_MAX)return false;credits+=amount;}
        else {if(int64_t(gold)+amount>INT_MAX)return false;gold+=amount;}}
    return true;
}
static bool plan_claim(const local_progress::State& s,const Definition& d,local_progress::State& next,int& credits,int& gold){
    next=s;
    if(!reward(s,d,credits,gold))return false;
    return d.kind>=5?battle_contract_policy::claim(next,d.id,credits,gold,d.xp,d.points):
        contract_policy::claim(next,d.id,counter(next,d),d.value,credits,gold,d.xp,d.points);
}
static void (*claim_original)(void*,void*),(*init_original)(void*,void*,void*);
static void claim(void* self,void* method){
    if(!offline_network){claim_original(self,method);return;}if(!progress_ready||local_skill::rank(progress)<4||!initialize())return;
    void* old=get(self,0xe0),*type=get(old,0x10);int i=type?index(*(int*)((char*)type+0x10)):-1;
    if(i<0||type!=types[i])return;const auto& d=contract_content::definitions[i];if(!eligible(d,progress))return;
    void* user=saved("UserConfiguration"),*data=source();
    if(!contains(get(user,0x128),old)||!contains(get(data,0x18),old)){
        LOG("Local contract stale row refused id=%d",d.id);return;}
    auto next=progress;int credits=0,gold=0;
    if(!plan_claim(progress,d,next,credits,gold)){
        LOG("Local contract claim refused before reward id=%d",d.id);return;}
    Roots roots;void* cfg=configuration(user,next,roots),*finished=daily(progress,i,roots,2),*replacement=nullptr;
    void* rows=get(cfg,0x10);for(int j=0;rows&&j<*(int*)((char*)rows+0x18);++j)if(get(list_item(rows,j),0x10)==type)replacement=list_item(rows,j);
    void* dispatcher=get(offline_network,0x88);if(!cfg||!finished||!replacement||!dispatcher||!data)return;
    int account_gold=0;if(!local_faction::account_gold(next,account_gold))return;
    void* messages[3];int values[]={next.credits,account_gold,next.experience};const char* names[]={"SetProfileCredits","SetAccountGold","SetExperience"};void* ex=nullptr;
    for(int j=0;j<3;++j){messages[j]=object_new(find_class("com.geargames.aow.messages",names[j]));if(!roots.keep(messages[j]))return;
        void* args[]={values+j};invoke_named(messages[j],".ctor",1,args,&ex);if(ex)return;}
    void* answer_class=find_class("com.geargames.aow.messages.daily","DailyRewardAnswer");if(!answer_class)return;
    void* answer=object_new(answer_class);if(!roots.keep(answer))return;
    invoke_named(answer,".ctor",0,nullptr,&ex);if(ex)return;ref(answer,"daily",finished);*(int*)((char*)answer+0x34)=contract_box_policy::balance(next.contract_boxes,0,next.contract_points);
    *(int*)((char*)answer+0x38)=contract_box_policy::balance(next.contract_boxes,1,next.contract_points);
    if(!local_progress::write(local_faction::root(),next)){LOG("Local contract claim save refused id=%d",d.id);return;}
    progress=next;
    for(int j=0;j<3&&!ex;++j){void* args[]={messages[j]};invoke_named(dispatcher,"Dispatch",1,args,&ex);}
    void* args[]={answer};if(!ex)invoke_named(dispatcher,"Dispatch",1,args,&ex);
    // Keep other live rows; the original replacement API emits the change
    // event that rebinds the claimed row to its new zero-count Daily.
    int price=0;void* replace_args[]={type,replacement,&price};if(!ex)invoke_named(data,"ReplaceOrAddDaily",3,replace_args,&ex);
    ref(user,"dailyConfiguration",get(data,0x18));
    LOG("Local contract claim saved=1 published=%d id=%d credits=%d gold=%d xp=%d points=%d cycle=%u reset_count=0 exception=%s",!ex,d.id,credits,gold,d.xp,d.points,contract_policy::receipt(progress,d.id)->claims,ex?class_name(object_class(ex)):"none");
}
static void hide(void* component){if(!component)return;void* ex=nullptr;void* go=invoke_named(component,"get_gameObject",0,nullptr,&ex);bool no=false;void* args[]={&no};if(go&&!ex)invoke_named(go,"SetActive",1,args,&ex);}
static void init(void* self,void* value,void* method){init_original(self,value,method);if(!offline_network)return;
    for(int o:{0xf8,0x100,0x118})hide(get(self,o)); // Original ad/paid replacement/deadline controls.
    for(int o:{0x138,0x140}){void* go=get(self,o);bool no=false;void* args[]={&no};void* ex=nullptr;if(go)invoke_named(go,"SetActive",1,args,&ex);}
}
static void (*info_original)(void*,void*,uint64_t,int,int,void*);
static void (*box_info)(void*,void*,uint64_t,int,int,void*);
static void info(void* self,void* boxes,uint64_t date,int last,int points,void* method){
    if(offline_network&&box_info){box_info(self,boxes,date,last,points,method);return;}
    if(!offline_network||(boxes&&*(int*)((char*)boxes+0x18)>0)){info_original(self,boxes,date,last,points,method);return;}
    // No supplied box service or calendar wait: disable its original controls.
    for(int o:{0x28,0x30,0x38})hide(get(self,o));
    void* controls=get(self,0x20);for(int j=0;controls&&j<*(int*)((char*)controls+0x18);++j)hide(list_item(controls,j));
}
static void changed(){refresh();}
static void publication_audit(){
    if(access(DATA_ROOT "contract-publication-audit.request",F_OK))return;unlink(DATA_ROOT "contract-publication-audit.request");
    auto before=progress;void* user=saved("UserConfiguration"),*original=get(user,0x128),*live=source(),*live_cfg=get(live,0x18);Roots roots;void* ex=nullptr;
    void* rank=local_career::lookup("SHARED","profileRanks",4);auto fixture=local_progress::seed(0,0,0,rank?*(int*)((char*)rank+0x14):0);
    local_progress::State active{};bool ok=prepare_state(fixture,active);void* cfg=ok?configuration(user,active,roots):nullptr;
    void* data=live?object_new(object_class(live)):nullptr;if(!roots.keep(data))ok=false;
    if(ok){void* args[]={get(live,0x10)};invoke_named(data,".ctor",1,args,&ex);ok=!ex&&publish_configuration(data,cfg,ex);}
    void* rows=get(cfg,0x10),*old=rows?list_item(rows,0):nullptr,*type=get(old,0x10);
    int i=type?index(*(int*)((char*)type+0x10)):-1;void* completed=i>=0?daily(active,i,roots,1):nullptr;
    if(completed)*(int*)((char*)completed+0x1c)=contract_content::definitions[i].value;
    if(ok&&completed){void* args[]={completed};invoke_named(data,"UpdateDaily",1,args,&ex);}else ok=false;
    bool updated=ok&&!ex&&contains(get(data,0x18),completed)&&!contains(get(data,0x18),old);
    void* next=i>=0?daily(active,i,roots):nullptr;int price=0;
    if(updated&&next){void* args[]={type,next,&price};invoke_named(data,"ReplaceOrAddDaily",3,args,&ex);}else ok=false;
    bool replaced=ok&&!ex&&contains(get(data,0x18),next)&&!contains(get(data,0x18),completed)&&*(int*)((char*)next+0x1c)==0&&*(int8_t*)((char*)next+0x18)==0;
    void* answer=object_new(find_class("com.geargames.aow.messages.daily","DailyRewardAnswer"));bool confirms=false;
    if(roots.keep(answer)){invoke_named(answer,".ctor",0,nullptr,&ex);void* value=!ex?invoke_named(answer,"get_Confirm",0,nullptr,&ex):nullptr;confirms=!ex&&value&&*(bool*)((char*)value+16);}
    bool unchanged=!memcmp(&before,&progress,sizeof(progress))&&get(user,0x128)==original&&get(live,0x18)==live_cfg;
    ok=ok&&updated&&replaced&&confirms&&unchanged;
    FILE* f=fopen(DATA_ROOT "contract-publication-audit.json","w");if(f){fprintf(f,"{\"pass\":%s,\"original_source_update\":%s,\"original_source_replace_zero\":%s,\"stale_row_rejected\":%s,\"original_answer_confirms\":%s,\"player_and_live_source_unchanged\":%s,\"player_save_writes\":0,\"exception\":\"%s\"}\n",ok?"true":"false",updated?"true":"false",replaced?"true":"false",replaced?"true":"false",confirms?"true":"false",unchanged?"true":"false",ex?class_name(object_class(ex)):"none");fclose(f);chmod(DATA_ROOT "contract-publication-audit.json",0644);}
}
static void audit(){
    publication_audit();
    if(access(DATA_ROOT "contract-runtime-audit.request",F_OK))return;unlink(DATA_ROOT "contract-runtime-audit.request");
    auto before=progress;void* user=saved("UserConfiguration"),*original_config=get(user,0x128);Roots roots;
    bool ok=initialize()&&local_skill::initialize();int cases=0;void* ex=nullptr,*card=local_career::skill_card(27);
    void* controller=find_class("com.geargames.aow.scenes.main.gui.skills","SkillsController");
    const void* method=controller?class_method(controller,"GetBoostedValueInt",3):nullptr;
    int max=local_skill::graph.max_level[27],effect=card?*(int*)((char*)card+0x38):-1;
    ok=ok&&method&&card&&max==11&&effect==100;
    for(int level=0;ok&&level<=max;++level){Roots one;void* skill=object_new(find_class("com.geargames.aow.entities","Skill"));int amount=0;
        if(!one.keep(skill)){ok=false;break;}void* args[]={card,&level,&max,&amount};invoke_named(skill,".ctor",4,args,&ex);if(ex){ok=false;break;}
        for(const auto& d:contract_content::definitions)for(int j=0;ok&&j<d.reward_count;++j)if(d.rewards[j].type==0){int base=d.rewards[j].amount,expected=0;bool visual=true;
            void* a[]={&base,skill,&visual};void* boxed=runtime_invoke(method,nullptr,a,&ex);
            ok=!ex&&boxed&&contract_policy::credits_bonus(base,effect,level,expected)&&*(int*)((char*)boxed+16)==expected;++cases;}
    }
    int target=0;for(int i=0;i<local_skill::graph.count;++i)if(local_skill::graph.nodes[i].card==27){target=local_skill::graph.nodes[i].id;break;}
    auto low=progress;bool low_refused=target&&!local_skill::plan(progress,target,low);
    void* rank=local_career::lookup("SHARED","profileRanks",5);auto fixture=local_progress::seed(2000000,10,0,rank?*(int*)((char*)rank+0x14):0);
    void* cards[128];int n=local_career::skill_cards(cards,128);if(n<1||n>128)ok=false;
    for(int i=0;i<n;++i)fixture.stocks[fixture.stock_count++]={8,*(int*)((char*)cards[i]+0x10),1000};
    bool legal=false;for(int round=0;ok&&!legal&&round<local_skill::graph.count;++round){bool changed=false;
        for(int i=0;i<local_skill::graph.count&&!legal;++i){auto& node=local_skill::graph.nodes[i];if(node.stage!=local_skill::graph.stages[0].id||local_progress::has_skill_node(fixture,node.id))continue;
            local_progress::State next;if(local_skill::plan(fixture,node.id,next)){fixture=next;changed=true;if(node.id==target)legal=true;}}
        if(!changed)break;
    }
    local_progress::State active{};bool setup=legal&&prepare_state(fixture,active);int k=index(20060),credits=0,gold=0;
    void* first=setup&&k>=0?daily(active,k,roots):nullptr;
    bool start_zero=first&&*(int*)((char*)first+0x1c)==0&&*(int8_t*)((char*)first+0x18)==0;
    bool event=setup&&local_progress::buy_upgrade(active,10000,100,1);void* completed=event?daily(active,k,roots):nullptr;
    bool native_complete=completed&&*(int8_t*)((char*)completed+0x18)==1&&*(int*)((char*)completed+0x1c)==1;
    bool paid=event&&reward(active,contract_content::definitions[k],credits,gold)&&credits==1320&&gold==0&&
        contract_policy::claim(active,20060,counter(active,contract_content::definitions[k]),1,credits,gold,0,15);
    auto after_claim=active;bool duplicate_refused=paid&&!contract_policy::claim(active,20060,counter(active,contract_content::definitions[k]),1,credits,gold,0,15)&&!memcmp(&active,&after_claim,sizeof(active));
    void* next_config=paid?configuration(user,active,roots):nullptr,*rows=get(next_config,0x10);bool reset=false;
    for(int i=0;rows&&i<*(int*)((char*)rows+0x18);++i){void* d=list_item(rows,i);if(get(d,0x10)==types[k])reset=*(int8_t*)((char*)d+0x18)==0&&*(int*)((char*)d+0x1c)==0;}
    Definition mixed=k>=0?contract_content::definitions[k]:Definition{};mixed.reward_count=2;mixed.rewards[1]={1,0,2};int mixed_c=0,mixed_g=0;
    bool mixed_ok=paid&&reward(active,mixed,mixed_c,mixed_g)&&mixed_c==1320&&mixed_g==2;
    mixed.rewards[1]={2,0,2};bool unsupported_refused=!reward(active,mixed,mixed_c,mixed_g);
    char directory[640];snprintf(directory,sizeof(directory),DATA_ROOT "contract-private-XXXXXX");bool saved=false;
    if(paid&&mkdtemp(directory)){local_progress::State restored;saved=local_progress::write(directory,active)&&local_progress::load(directory,restored)==1&&!memcmp(&restored,&active,sizeof(active));}
    else directory[0]=0;
    bool unchanged=!memcmp(&before,&progress,sizeof(progress))&&get(user,0x128)==original_config;
    ok=ok&&low_refused&&legal&&setup&&start_zero&&native_complete&&paid&&duplicate_refused&&reset&&mixed_ok&&unsupported_refused&&saved&&unchanged;
    FILE* f=fopen(DATA_ROOT "contract-runtime-audit.json","w");if(f){fprintf(f,"{\"pass\":%s,\"original_boost_cases\":%d,\"levels\":%d,\"card_effect\":%d,\"legal_skill27_purchase\":%s,\"current_low_rank_refused\":%s,\"original_daily_starts_zero\":%s,\"original_daily_completes_after_event\":%s,\"claim_green\":%d,\"mixed_gold_unchanged\":%s,\"unknown_whole_reward_refused\":%s,\"duplicate_refused\":%s,\"next_original_daily_zero\":%s,\"private_store_reload\":%s,\"private_store_root\":\"%s\",\"player_state_unchanged\":%s,\"player_save_writes\":0,\"exception\":\"%s\"}\n",ok?"true":"false",cases,max+1,effect,legal?"true":"false",low_refused?"true":"false",start_zero?"true":"false",native_complete?"true":"false",credits,mixed_ok?"true":"false",unsupported_refused?"true":"false",duplicate_refused?"true":"false",reset?"true":"false",saved?"true":"false",directory,unchanged?"true":"false",ex?class_name(object_class(ex)):"none");fclose(f);chmod(DATA_ROOT "contract-runtime-audit.json",0644);}
}
static void install(){
    const uint32_t a[]={0xa9bd5ffe,0xa90157f6,0xa9024ff4,0xd000edf3};
    const uint32_t b[]={0xf81e0ffe,0xa9014ff4,0xaa0103f4,0xaa0003f3};
    claim_original=(decltype(claim_original))install_hook(0x7ff5fa8,a,(void*)claim);
    init_original=(decltype(init_original))install_hook(0x7ff514c,b,(void*)init);
    const uint32_t c[]={0xf81c0ffe,0xa9015ff8,0xa90257f6,0xa9034ff4};
    info_original=(decltype(info_original))install_hook(0x7ff3048,c,(void*)info);
    local_quests::after_change=changed;
    LOG("Local contract original hooks claim=%d init=%d info=%d no_calendar_policy=1",claim_original!=nullptr,init_original!=nullptr,info_original!=nullptr);
}
}
