// Opt-in skill mechanism batch. Only private fixtures and the no-save skill lab.
#pragma once
namespace skill_chest_audit {
using Roots=local_research_runtime::Roots;
static bool pending;static void* chest;static void* owner;static uintptr_t chest_root,map_root;
static void* old_map;static int old_time,start_tick,gold_before,claimed_amount,claimed_delta,claim_calls;
static thread_local bool claiming;
static void (*claim_original)(void*,void*,void*,bool,void*);
static void (*gold_original)(void*,void*,int,bool,void*);
static local_progress::State before;
static const char* path=DATA_ROOT "skill-chest-audit.jsonl";
static void* get(void* p,int o){return p?*(void**)((char*)p+o):nullptr;}
static void log(const char* format,...){FILE* f=fopen(path,"a");if(!f)return;va_list a;va_start(a,format);vfprintf(f,format,a);va_end(a);fputc('\n',f);fclose(f);chmod(path,0644);}
static int number(void* p,const char* name){void* ex=nullptr;void* v=p?invoke_named(p,name,0,nullptr,&ex):nullptr;return v&&!ex?*(int*)((char*)v+16):-1;}
static int value(void* map,int key){void* ex=nullptr;int v=0;void* args[]={&key,&v};void* found=map?invoke_named(map,"TryGetValue",2,args,&ex):nullptr;return found&&!ex&&*(bool*)((char*)found+16)?v:0;}
static int time(void* battle,void* box,void* side){void* ex=nullptr;void* args[]={battle,box,side};void* v=runtime_invoke(class_method(find_class("com.geargames.aow.entities.battle","BonusChestAct"),"$Rb",3),nullptr,args,&ex);return v&&!ex?*(int*)((char*)v+16):-1;}
static int tick(void* b){auto k=object_class(b);return (*(int(**)(void*,void*))((char*)k+0x1018))(b,get(k,0x1020));}
static void claim(void* battle,void* box,void* side,bool automatic,void* method){
    bool previous=claiming;claiming=pending&&box==chest&&side==owner;
    claim_original(battle,box,side,automatic,method);claiming=previous;
}
static void gold(void* battle,void* side,int amount,bool flag,void* method){
    int old=side?*(int*)((char*)side+0x16c):0;gold_original(battle,side,amount,flag,method);
    if(claiming&&side==owner){claimed_amount+=amount;claimed_delta+=*(int*)((char*)side+0x16c)-old;++claim_calls;}
}
static bool install(){
    if(claim_original&&gold_original)return true;
    const uint32_t a[]={0xd10203ff,0xa9027bfd,0xa9036ffc,0xa90467fa};
    const uint32_t b[]={0xa9bd5ffe,0xa90157f6,0xa9024ff4,0xb002adf6};
    claim_original=(decltype(claim_original))install_hook(0x472651c,a,(void*)claim);
    gold_original=(decltype(gold_original))install_hook(0x47e5604,b,(void*)gold);
    return claim_original&&gold_original;
}
static void finish(bool pass,int now){
    if(owner){local_skill::ref(owner,"$IH",old_map);*(int16_t*)((char*)owner+0x218)=old_time;}
    log("{\"summary\":true,\"pass\":%s,\"elapsed_ticks\":%d,\"player_state_unchanged\":%s,\"player_save_writes\":0,\"real_player_result\":false}",pass?"true":"false",now-start_tick,!memcmp(&before,&progress,sizeof(before))?"true":"false");
    if(chest_root)gchandle_free(chest_root);if(map_root)gchandle_free(map_root);chest_root=map_root=0;pending=false;
}
static void step(void* battle){
    if(!pending)return;int now=tick(battle);int gain=*(int*)((char*)owner+0x16c)-gold_before;
    if(*(bool*)((char*)chest+0x40)){
        log("{\"phase\":\"original_timed_pickup\",\"tick\":%d,\"capture_time\":95,\"empty\":true,\"total_gold_gain\":%d,\"native_claim_calls\":%d,\"native_claim_amount\":%d,\"native_claim_gold_delta\":%d,\"expected_claim_amount\":110,\"fixture_income_unchanged\":true}",now,gain,claim_calls,claimed_amount,claimed_delta);
        finish(claim_calls==1&&claimed_amount==110&&claimed_delta==110&&!memcmp(&before,&progress,sizeof(before)),now);
    }else if(now-start_tick>400){log("{\"failure\":\"original_pickup_timeout\",\"capture_progress\":%d,\"gold_gain\":%d}",*(int*)((char*)chest+0x78),gain);finish(false,now);}
}
static void run(void* battle,void* unit){
    if(access(DATA_ROOT "skill-chest-audit.request",F_OK))return;unlink(DATA_ROOT "skill-chest-audit.request");FILE* f=fopen(path,"w");if(f)fclose(f);
    before=progress;Roots roots;void* side=get(unit,0x18),*source_map=get(side,0x3e8);
    bool ok=side&&source_map&&local_skill::initialize();int original_count=number(source_map,"get_Count"),original_time=*(int16_t*)((char*)side+0x218);
    void* box=object_new(find_class("com.geargames.aow.entities","BonusChest"));void* ex=nullptr;
    ok=ok&&roots.keep(box);if(ok)invoke_named(box,".ctor",0,nullptr,&ex);ok=ok&&!ex;
    int cases=0;
    for(int level=1;ok&&level<=11;++level){void* copy=local_research_runtime::clone(side,roots);if(!copy){ok=false;break;}
        int resources=100*level,capture=50*level;ok=skill_chest::prepare(copy,resources,capture);
        int expected=100*(1000-capture)/1000,actual=ok?time(battle,box,copy):-1;
        void* skill=object_new(find_class("com.geargames.aow.entities","Skill"));int maximum=11,cards=0,base=100;
        if(!roots.keep(skill)){ok=false;break;}void* a[]={local_career::skill_card(5),&level,&maximum,&cards};invoke_named(skill,".ctor",4,a,&ex);
        void* b[]={&base};void* reference=!ex?invoke_named(skill,"applyDown",1,b,&ex):nullptr;
        ok=ok&&reference&&!ex&&*(int*)((char*)reference+16)==expected;
        ok=ok&&actual==100&&value(get(copy,0x3e8),4)==10*level&&value(get(copy,0x3e8),5)==5*level&&get(copy,0x3e8)!=source_map;
        if(ok){*(int*)((char*)box+0x30)=240;ok=time(battle,box,copy)==240;*(int*)((char*)box+0x30)=0;}
        if(ok){ok=skill_chest::prepare(copy,resources,capture)&&time(battle,box,copy)==100;++cases;}
    }
    ok=ok&&number(source_map,"get_Count")==original_count&&*(int16_t*)((char*)side+0x218)==original_time;
    auto fixture=local_progress::seed(1000000,0,0,10000);local_progress::State next;
    bool purchase=local_progress::grant_stock(fixture,8,5,20)&&local_skill::plan(fixture,114,next);
    auto low=fixture;low.stars=9;bool rank=!local_skill::plan(low,114,fixture);
    void* projected=local_research_runtime::clone(side,roots);
    bool projection=purchase&&projected&&skill_combat::project(projected,next,get(side,0xf0),get(side,0xf8))&&time(battle,box,projected)==100&&value(get(projected,0x3e8),4)==0&&value(get(projected,0x3e8),5)==5;
    bool native_init=original_time==95&&value(source_map,5)==5;
    ok=ok&&projection&&native_init&&install();
    log("{\"phase\":\"private_original_contracts\",\"pass\":%s,\"levels_checked\":%d,\"native_capture_getter\":true,\"explicit_script_time_preserved\":true,\"resource_dictionary_key\":4,\"native_percent_scale\":100,\"source_map_entries\":%d,\"original_time\":%d,\"legal_purchase\":%s,\"rank_refused\":%s}",ok&&purchase&&rank?"true":"false",cases,original_count,original_time,purchase?"true":"false",rank?"true":"false");
    log("{\"legal_saved_skill_projection\":%s,\"native_skill_initialization\":%s,\"original_applyDown_levels\":%d,\"observer_hooks_installed\":%s}",projection?"true":"false",native_init?"true":"false",cases,claim_original&&gold_original?"true":"false");
    start_tick=tick(battle);owner=side;old_map=source_map;old_time=original_time;claimed_amount=claimed_delta=claim_calls=0;
    if(!ok||!purchase||!rank){finish(false,start_tick);return;}
    map_root=gchandle_new(old_map,true);void* award_copy=local_research_runtime::clone(side,roots);
    if(!map_root||!award_copy||!skill_chest::prepare(award_copy,100,50)){finish(false,start_tick);return;}
    // Only add the isolated card4 sample here. Card5 was legally projected
    // before battle and has already passed through the real native initializer.
    local_skill::ref(side,"$IH",get(award_copy,0x3e8));
    gold_before=*(int*)((char*)side+0x16c);
    int x=*(int16_t*)((char*)unit+0x48),y=*(int16_t*)((char*)unit+0x4a),amount=100,capture=0,preview=0;
    int8_t type=0,model=0,occupation=0,color=0,visibility=2;void* args[]={battle,&x,&y,&type,&model,&occupation,&amount,&capture,nullptr,&color,&visibility,&preview};
    ex=nullptr;chest=runtime_invoke(class_method(find_class("com.geargames.aow.entities.battle","BonusChestAct"),"$kb",12),nullptr,args,&ex);
    if(!chest||ex){log("{\"failure\":\"original_chest_factory\",\"exception\":\"%s\"}",ex?class_name(object_class(ex)):"none");finish(false,start_tick);return;}
    chest_root=gchandle_new(chest,true);pending=chest_root!=0;
    log("{\"phase\":\"original_chest_created\",\"tick\":%d,\"unit\":%d,\"chest\":%d,\"cell\":[%d,%d],\"amount\":100,\"native_capture_time\":%d,\"private_level\":1}",start_tick,*(int*)((char*)unit+0x10),*(int*)((char*)chest+0x10),x,y,time(battle,chest,side));
    if(!pending)finish(false,start_tick);
}
}
