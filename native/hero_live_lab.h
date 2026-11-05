// Opt-in original-engine sample. Temporary battle prototypes/buildings only;
// no card grants, career/run reservation, reward or player result.
#pragma once
namespace local_duel {static bool seed(void*,int);}
namespace ai_lab {static void* command(const char*,int,local_research_runtime::Roots&,int);static int perform(void*,void*);}
namespace hero_live_lab {
using local_research_runtime::Roots;
static bool active,frozen,stopped,bought;static int last=-100,bought_tick;
static local_progress::State before;
static bool skill_mode;static int skill_damage,skill_construction,skill_build_tick;
static const char* report=DATA_ROOT "hero-live-lab.jsonl";
static void* get(void* o,int n){return o?*(void**)((char*)o+n):nullptr;}
static int number(void* o,const char* name){void* ex=nullptr;void* v=o?invoke_named(o,name,0,nullptr,&ex):nullptr;return v&&!ex?*(int*)((char*)v+16):-1;}
static void log(const char* format,...){FILE* f=fopen(report,"a");if(!f)return;va_list a;va_start(a,format);vfprintf(f,format,a);va_end(a);fputc('\n',f);fclose(f);chmod(report,0644);}
static void fail(const char* phase,void* ex=nullptr){log("{\"failure\":\"%s\",\"exception\":\"%s\"}",phase,ex?class_name(object_class(ex)):"none");stopped=true;}
static void begin(bool skills=false){active=true;frozen=stopped=bought=false;last=-100;before=progress;skill_mode=skills;skill_build_tick=-1;
    report=skills?DATA_ROOT "skill-live-lab.jsonl":DATA_ROOT "hero-live-lab.jsonl";FILE* f=fopen(report,"w");if(f)fclose(f);
    log("{\"diagnostic_only\":true,\"private_skill_sample\":%s,\"temporary_hero_class\":%d,\"temporary_buildings\":true,\"career_writes\":false,\"player_result\":false}",skills?"true":"false",skills?0:2);}
static bool project(void* battle){
    void* side=local_dictionary_item(get(battle,0x178),1);generic_unit_research::Plan plan;int matched=0;
    if(skill_mode){auto fixture=local_progress::seed(10000,0,0,0);local_progress::State next;
        if(!local_progress::grant_stock(fixture,8,1,20)||!local_progress::grant_stock(fixture,8,3,20)||
            !local_skill::plan(fixture,111,next,false))return false;fixture=next;
        if(!local_skill::plan(fixture,121,next,false))return false;fixture=next;
        if(!access(DATA_ROOT "skill-chest-audit.request",F_OK)){
            if(!local_progress::grant_stock(fixture,8,5,20)||!local_skill::plan(fixture,114,next,false))return false;fixture=next;}
        if(!skill_flag_audit::fixture(fixture))return false;
        if(!skill_phase3_audit::fixture(fixture))return false;
        void* infantry=local_dictionary_item(get(side,0xf0),1100),*barracks=local_dictionary_item(get(side,0xf8),1105);
        void* weapon=list_item(get(list_item(get(infantry,0x18),0),0x18),0);int base_damage=weapon?*(int*)((char*)weapon+0x28):-1;
        int base_construction=barracks?*(int*)((char*)list_item(get(barracks,0x18),0)+0x64):-1;
        if(base_damage<0||base_construction<=0||!skill_combat::prepare(side,fixture)||!skill_economy_audit::prepare(side)||!skill_flag_audit::prepared(side)||!skill_phase3_audit::prepared(side))return false;
        skill_tree_policy::Summary summary;if(!local_skill::graph.summarize(fixture,summary))return false;
        skill_combat::scaled(base_damage,15*summary.level[1],false,skill_damage);skill_combat::scaled(base_construction,20*summary.level[3],true,skill_construction);
        log("{\"phase\":\"private_skill_input\",\"legal_private_nodes\":[111,121],\"base_damage\":%d,\"expected_damage\":%d,\"base_construction\":%d,\"expected_construction\":%d,\"player_rank_unchanged\":true}",base_damage,skill_damage,base_construction,skill_construction);return true;
    }
    for(int id:{33300,33301}){generic_unit_research::Effect effect;if(!generic_unit_research::resolve(local_upgrade_level(id),effect)||!plan.add(effect))return false;}
    return generic_unit_research::apply(side,get(side,0xf0),plan,matched)&&matched==1&&skill_hero_cooldown_audit::fixture(side);
}
static void* building(void* side,int category){void* a=get(side,0x148);for(size_t i=0;a&&i<array_length(a);++i){void* b=((void**)((char*)a+32))[i];void* t=get(b,0x38);if(t&&*(int8_t*)((char*)t+0x30)==category&&!*(bool*)((char*)b+0x139))return b;}return nullptr;}
static void script(void* battle,void* method,void(*original)(void*,void*)){
    // Uses the accepted independent two-seat seed, with captured GAI removed.
    // No tutorial actions and no AI actions are issued by this sample.
}
static void* seed(void* battle,void* side,void* hq,int category,int level,int construction_state=1){
    Roots roots;uintptr_t handle=0;void* a=local_dictionary_values(get(side,0xf8),find_class("com.geargames.aow.entities","BuildingType"),handle);if(handle)roots.take(handle);void* type=nullptr;
    for(size_t i=0;a&&i<array_length(a);++i){void* t=((void**)((char*)a+32))[i];if(*(int8_t*)((char*)t+0x30)==category){type=t;break;}}
    if(!type)return nullptr;void* levels=get(type,0x18);int count=levels?*(int*)((char*)levels+0x18):0;
    if(count<1)return nullptr;if(level>=count)level=count-1;
    int hx=*(int16_t*)((char*)hq+0x40),hy=*(int16_t*)((char*)hq+0x42);
    auto valid=reinterpret_cast<bool(*)(void*,void*,int,int,int16_t,void*)>(image_base+0x4922a7c);
    int8_t upg=level,state=construction_state,base=0;int16_t angle=0;bool schedule=false;int request=0,extra=0;
    for(int r=6;r<=28;r+=2)for(int dy=-r;dy<=r;dy+=2)for(int dx=-r;dx<=r;dx+=2){int x=hx+dx,y=hy+dy;
        if(x<2||y<2||!valid(battle,type,x,y,0,nullptr))continue;void* ex=nullptr;void* args[]={battle,side,type,&x,&y,&upg,&angle,&state,&base,&schedule,&request,&extra};
        runtime_invoke(class_method(find_class("com.geargames.aow.entities.battle","BuildingAct"),"$Vb",12),nullptr,args,&ex);
        if(ex){fail("original_building_factory",ex);return nullptr;}return building(side,category);}
    return nullptr;
}
static void skill_step(void* battle,void* side,void* hq,int now){
    // Seeded HQ visibility/territory is updated by the engine after tick 1.
    // Reuse the original command sample's tick-30 start, rather than asking
    // normal placement to accept an uninitialised construction area.
    if(now<30)return;
    void* barracks=building(side,5);
    if(skill_build_tick<0){
        if(!seed(battle,side,hq,2,0)||!seed(battle,side,hq,4,0)){fail("skill_initial_buildings");return;}
        Roots roots;void* c=ai_lab::command("AICommBuSet",now,roots,1),*type=local_dictionary_item(get(side,0xf8),1105);if(!c||!type){fail("skill_build_command");return;}
        *(int*)((char*)c+0x30)=1105;*(int*)((char*)c+0x3c)=1;
        void* context=static_object("com.geargames.aow.scenes.battle","ClientBattleContext","m_logicCommandContext"),*accumulation=get(context,0x40);
        void* ex=nullptr;void* args[]={c};if(accumulation)invoke_named(accumulation,"ApplyBuildingAccumulationSum",1,args,&ex);if(ex){fail("skill_build_accumulation",ex);return;}
        auto valid=reinterpret_cast<bool(*)(void*,void*,int,int,int16_t,void*)>(image_base+0x4922a7c);
        int hx=*(int16_t*)((char*)hq+0x40),hy=*(int16_t*)((char*)hq+0x42),tested=0,last_code=-999;
        for(int radius=4;radius<=24;radius+=2)for(int dy=-radius;dy<=radius;dy+=2)for(int dx=-radius;dx<=radius;dx+=2){
            if(abs(dx)!=radius&&abs(dy)!=radius)continue;int x=hx+dx,y=hy+dy;if(x<0||y<0||!valid(battle,type,x,y,0,nullptr))continue;
            *(int16_t*)((char*)c+0x40)=x;*(int16_t*)((char*)c+0x42)=y;*(int16_t*)((char*)c+0x44)=0;
            last_code=ai_lab::perform(battle,c);
            if(!last_code){barracks=building(side,5);if(!barracks){fail("skill_native_build_registration");return;}skill_build_tick=now;
                log("{\"phase\":\"original_construction\",\"tick\":%d,\"state\":%d,\"native_tick_constr\":%d,\"expected_duration\":%d,\"native_build_command\":true}",now,*(int8_t*)((char*)barracks+0x47),*(int*)((char*)barracks+0x90),skill_construction);return;}
            if(++tested>=32){log("{\"native_build_last_code\":%d}",last_code);fail("skill_native_build_refused");return;}}
        fail("skill_native_build_footprint");return;
    }
    if(!barracks){fail("skill_barracks_missing");return;}
    if(!bought){if(*(int8_t*)((char*)barracks+0x47)!=1){if(now-skill_build_tick>skill_construction+100)fail("skill_construction_timeout");return;}
        Roots roots;void* c=ai_lab::command("AICommBuyUnit",now,roots,1);if(!c){fail("skill_buy_command");return;}
        *(int*)((char*)c+0x34)=*(int*)((char*)barracks+0x10);*(int*)((char*)c+0x30)=1100;
        int gold=*(int*)((char*)side+0x16c),code=ai_lab::perform(battle,c);
        log("{\"phase\":\"original_infantry_buy\",\"tick\":%d,\"construction_elapsed\":%d,\"result\":%d,\"gold_before\":%d,\"gold_after\":%d}",now,now-skill_build_tick,code,gold,*(int*)((char*)side+0x16c));
        if(code){fail("skill_original_buy");return;}bought=true;bought_tick=now;
    }
    void* units=get(side,0x140);
    for(size_t i=0;units&&i<array_length(units);++i){void* u=((void**)((char*)units+32))[i],*t=get(u,0x40);if(!t||*(int*)((char*)t+0x10)!=1100)continue;
        void* weapons=get(u,0x28),*w=weapons&&*(int*)((char*)weapons+0x18)?list_item(weapons,0):nullptr,*type=get(w,0x10);
        int damage=type?*(int*)((char*)type+0x28):-1;
        bool pass=damage==skill_damage&&type==list_item(get(list_item(get(t,0x18),0),0x18),0)&&!memcmp(&before,&progress,sizeof(progress));
        log("{\"phase\":\"original_infantry_born\",\"tick\":%d,\"unit_id\":%d,\"actual_damage\":%d,\"expected_damage\":%d,\"pass\":%s,\"player_state_unchanged\":true}",now,*(int*)((char*)u+0x10),damage,skill_damage,pass?"true":"false");
        skill_protection_audit::run(battle,u,hq);skill_chest_audit::run(battle,u);skill_flag_audit::run(battle,u);stopped=true;return;}
    if(now-bought_tick>1500)fail("skill_birth_timeout");
}
static void step(void* battle){
    skill_chest_audit::step(battle);
    skill_flag_audit::step(battle);
    if(!active||stopped)return;int now=number(battle,"$Qo");if(now-last<10)return;last=now;
    if(memcmp(&before,&progress,sizeof(progress))){fail("player_state_changed");return;}
    if(!frozen){if(!local_duel::seed(battle,now)){fail("two_seat_seed");return;}skill_economy_audit::run(battle);frozen=true;log("{\"phase\":\"two_seat_seed\",\"tick\":%d}",now);}
    if(skill_phase3_audit::run(battle)){stopped=true;return;}
    void* side=local_dictionary_item(get(battle,0x178),1);void* hq=building(side,1);
    if(!side||!hq){fail("headquarters");return;}
    if(skill_mode){skill_step(battle,side,hq,now);return;}
    if(!bought){
        void* power=seed(battle,side,hq,2,2);void* base=power?seed(battle,side,hq,18,2):nullptr;
        if(!base){fail("diagnostic_buildings");return;}
        int side_id=1,type=1172,slot=0,id=*(int*)((char*)base+0x10);void* ex=nullptr;void* args[]={battle,&side_id,&type,&slot,&id};
        int gold_before=*(int*)((char*)side+0x16c);
        void* result=runtime_invoke(class_method(find_class("com.geargames.aow.entities.battle","HeroAct"),"$Ee",5),nullptr,args,&ex);
        int code=result&&!ex?*(int16_t*)((char*)result+16):-999;
        log("{\"phase\":\"original_buy\",\"tick\":%d,\"result\":%d,\"gold_before\":%d,\"gold_after\":%d,\"energy\":%d,\"queue\":%d,\"exception\":\"%s\"}",now,code,gold_before,*(int*)((char*)side+0x16c),*(int*)((char*)side+0x40),number(get(base,0x98),"get_Count"),ex?class_name(object_class(ex)):"none");
        if(ex||code){fail("original_buy",ex);return;}bought=true;bought_tick=now;
    }
    void* a=get(side,0x140);void* hero=nullptr;
    for(size_t i=0;a&&i<array_length(a);++i){void* u=((void**)((char*)a+32))[i];void* t=get(u,0x40);if(t&&*(int*)((char*)t+0x10)==1172&&get(u,0x2a8)){hero=u;break;}}
    if(hero){void* model=get(hero,0x2a8);void* type=get(hero,0x40);
        log("{\"phase\":\"original_hero_born\",\"tick\":%d,\"unit_id\":%d,\"hero_class\":%d,\"hp\":%d,\"energy\":%d,\"abilities\":%d,\"active\":%s,\"passive\":%s,\"player_state_unchanged\":true}",now,*(int*)((char*)hero+0x10),*(int*)((char*)type+0xa0),*(int*)((char*)hero+0x184),*(int*)((char*)side+0x40),number(get(model,0x10),"get_Count"),get(model,0x18)?"true":"false",get(model,0x20)?"true":"false");
        skill_hero_protection_audit::run(battle,hero);
        skill_hero_cooldown_audit::run(battle,hero);
        FILE* f=fopen(DATA_ROOT "hero-live-born.txt","w");if(f){visited_count=0;dump_object(f,hero,0,2);fclose(f);chmod(DATA_ROOT "hero-live-born.txt",0644);}stopped=true;
    }else if(now-bought_tick>4000)fail("birth_timeout");
}
}
