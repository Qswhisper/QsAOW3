// Shared native seat setup, AI controller and team elimination adapter.
// Player seat 1 stays manual; other seats use independent native workspaces.
// No synthetic damage, free trained units, career rewards or full-bot claim.
#pragma once
#include "ai_tech_policy.h"
#include "ai_tech_dependencies.h"
#include "ai_economy_policy.h"
#include "ai_water_plan.h"
#include "control_point_rules.h"
#include "control_point_plan.h"
#include <atomic>
namespace local_duel {
using local_research_runtime::Roots;
using local_bot_runtime::put;
static bool stopped, seeded, damaged, combat_sample, stop_recorded;
static int last_tick, sequence;
static int terminal_candidate=-1,terminal_since=-1;
static bool finish_requested;
static std::atomic<bool> requested_surrender{false};
static bool base_test; // Opt-in two-AI recovery fixture; never controls the player seat.
static bool tech_test; // Opt-in mixed-faction production fixture; no player match.
static bool tank_test; // Bounded NPC-only extension of the private production fixture.
static bool air_test; // Native aircraft production after the observed tank tier.
static bool advanced_test;
static bool advanced_air_test;
static bool naval_test;
static int naval_fixture_chapter=202;
static void reset_naval();
static void reset_advanced();
static void reset_advanced_air();
static void need_producer_area(void*,int);
static bool advanced_enabled();
namespace control_points {static void reset(int);static int private_mode;}
static local_progress::State tech_before;
struct Seat { int purchases=0, births=0, ids[128]={}, last_move=-1000, last_decision=-1000, assemble_since=-1,regroup_until=0,next_regroup=0,next_tech=0, next_tech_notice=0, ground_tier_born=0, tank_id=0, tank_x=0, tank_y=0, air_id=0, air_x=0, air_y=0, producer_site_cursor=0; bool developed=false, reserve_rebuild=false, reserve_tech=false, vehicle_factory_seen=false, expanded_army=false, tech_graph_ready=false, tech_graph_refused=false, tank_ordered=false, tank_moved=false, air_factory_seen=false, air_ordered=false, air_moved=false; uint8_t completed=0; ai_guard_policy::State guard; ai_tech_dependencies::Graph tech_graph;
 ai_economy::Plan plan;ai_economy::Money money;bool economy_threat=false;int orders=0,rejected=0,idle_since=-1,max_idle=0,legal_idle_ticks=0,legal_samples=0,ready_samples=0,busy_samples=0,block_cp=0,block_cash=0,block_native=0,last_economy_log=-1000; };
static Seat seats[8];
static void batch_report(void*,const char*,int,int=-1,int=0);
static int configured_seats=2;
static int seat_count(){return configured_seats;}
static bool allow_script() {return seeded&&!stopped;}
static void log(const char* format, ...) {
    FILE* f=fopen(DATA_ROOT "art3offline-duel.txt","a");if(!f)return;
    va_list args;va_start(args,format);vfprintf(f,format,args);va_end(args);
    fputc('\n',f);fclose(f);chmod(DATA_ROOT "art3offline-duel.txt",0644);
}
static bool fail(const char* phase) {log("FAIL phase=%s",phase);stopped=true;return false;}
static void record_order(void* battle,void* command,int before,int after,int code){
    int id=*(int*)((char*)command+0x28);if(id<1||id>seat_count())return;
    const char* name=class_name(object_class(command));int kind=!strcmp(name,"AICommBuSet")?ai_economy::Build:!strcmp(name,"AICommBuildingUpgrade")?ai_economy::Upgrade:!strcmp(name,"AICommBuyUnit")?ai_economy::Unit:0;
    if(!kind)return;auto& a=seats[id-1];int now=ai_lab::tick(battle);a.money.order(before,after,now);++a.orders;if(code)++a.rejected;
    log("ECON_ORDER tick=%d side=%d kind=%d target=%d code=%d gold=%d->%d native_price=1",now,id,kind,*(int*)((char*)command+0x30),code,before,after);
    if(a.plan.active()&&a.plan.kind==kind&&(kind!=ai_economy::Unit||a.plan.action==*(int*)((char*)command+0x30))){
        log("PLAN_RELEASE tick=%d side=%d goal=%d action=%d reason=%s",now,id,a.plan.goal,a.plan.action,code?"native-reject":"paid-order");
        a.plan.release(code?ai_economy::Rejected:ai_economy::Completed,now);
    }
}
static bool budget(void* s,int goal,int action,int kind,int price,int now,bool emergency=false){
    int id=*(int*)((char*)s+0x10);auto& a=seats[id-1];
    if(!offline_world_runtime::planner(id))return ai_lab::gold(s)>=price;
    int old=a.plan.goal,old_action=a.plan.action;int income=std::max(10,int(a.money.income*100/std::max(100,now-30)));
    if(!a.plan.propose(goal,action,kind,price,now,income,emergency))return false;
    if(old!=a.plan.goal||old_action!=a.plan.action)log("PLAN_START tick=%d side=%d goal=%d action=%d kind=%d budget=%d until=%d income_per_100=%d",now,id,goal,action,kind,price,a.plan.until,income);
    return ai_lab::gold(s)>=price;
}
static bool clear(void* collection) {
    if(!collection)return true;
    void* result=nullptr;return local_bot_runtime::call(collection,"Clear",0,nullptr,result);
}
static void* side(void* battle,int id) {return battle?local_dictionary_item(*(void**)((char*)battle+0x178),id):nullptr;}
static int base_id(void* s) {
    void* fraction=s?*(void**)((char*)s+0xd8):nullptr;
    int id=fraction?*(int*)((char*)fraction+0x10):0;
    if(local_coop::playing&&s&&(id==1||id==2)){
        // Original cooperative inputs allocate prototype families per seat.
        int candidate=*(int*)((char*)s+0x10)*1000+(id-1)*100;
        void* type=local_dictionary_item(*(void**)((char*)s+0xf8),candidate);
        return type&&*(int8_t*)((char*)type+0x30)==1?candidate:-1;
    }
    return id==1?1000:(id==2?1100:-1);
}
static bool empty_side(void* s) {
    // Preserve the native pre-initialization contract: the engine allocates
    // runtime arrays after research has prepared the empty side's definitions.
    if(!put(s,0x140,nullptr)||!put(s,0x148,nullptr))return false;
    // Remove tutorial-only restrictions, preserving normal native validators.
    for(int offset:{0xb0,0xb8,0xc0})if(!put(s,offset,nullptr))return false;
    // UnbuildZone belongs to the mission/side, not the retained map's terrain.
    // In particular the donor side came from a different tutorial map. Carrying
    // those rectangles into a custom duel leaves invisible forbidden areas.
    void* zones=*(void**)((char*)s+0x280);int zone_count=zones?*(int*)((char*)zones+0x18):0;
    if(zone_count<0||zone_count>128)return false;
    if(zones&&(!clear(zones)||*(int*)((char*)zones+0x18)))return false;
    if(naval_test)log("LOCAL_FREE_BUILD source_side=%d removed_mission_zones=%d original_map_terrain_preserved=1",*(int*)((char*)s+0x10),zone_count);
    *(int*)((char*)s+0x16c)=1200;
    return true;
}
static bool prepare(void* battle,void* donor) {
    configured_seats=2;
    base_test=!local_match::playing&&!access(DATA_ROOT "ai-base-test.enabled",F_OK);
    tech_test=!local_match::playing&&!access(DATA_ROOT "ai-tech-lab.enabled",F_OK);
    naval_test=tech_test&&!access(DATA_ROOT "ai-naval-lab.enabled",F_OK);
    advanced_air_test=tech_test&&!access(DATA_ROOT "ai-advanced-air-lab.enabled",F_OK);
    advanced_test=tech_test&&(!access(DATA_ROOT "ai-advanced-lab.enabled",F_OK)||advanced_air_test);
    air_test=tech_test&&(!access(DATA_ROOT "ai-air-lab.enabled",F_OK)||advanced_test);
    tank_test=tech_test&&(!access(DATA_ROOT "ai-tank-lab.enabled",F_OK)||air_test||naval_test);
    if(tech_test)tech_before=progress;
    FILE* f=fopen(DATA_ROOT "art3offline-duel.txt","w");
    if(f){const auto* input=local_match_maps::find(naval_fixture_chapter);fprintf(f,"schema=%s map=%d local_start_budget=1200 career_writes=0 full_bot=0 player_side=%d\n",local_match::playing?"local-player-1v1-v2":"local-duel-v1",local_match::playing?local_match::state.map:(naval_test&&input?input->map:4),local_match::playing?1:0);fclose(f);}
    stopped=seeded=damaged=combat_sample=stop_recorded=false;last_tick=-100;sequence=0;
    terminal_candidate=terminal_since=-1;finish_requested=false;requested_surrender.store(false);
    control_points::reset(offline_world_runtime::batch_fixture?offline_world_runtime::batch_job.mode:local_match::playing?local_match::state.mode:control_points::private_mode?control_points::private_mode:1);
    for(auto& s:seats)s=Seat{};
    if(FILE* ef=fopen(DATA_ROOT "ai-economy.jsonl","w")){fclose(ef);chmod(DATA_ROOT "ai-economy.jsonl",0644);}
    reset_advanced();
    reset_advanced_air();
    reset_naval();
    if(base_test)log("RECOVERY_FIXTURE natural_damage_only=1 production_targets=1 career_writes=0");
    if(tech_test)log("TECHNOLOGY_FIXTURE production_only=1 attack_orders=0 career_writes=0");
    if(tank_test)log("TANK_FIXTURE npc_side=2 original_prices=1 original_weights=1 career_writes=0");
    if(air_test)log("AIR_FIXTURE npc_side=2 original_producer_kind=9 original_prices=1 career_writes=0");
    if(!battle||!donor||battle==donor)return fail("fresh-decode");
    void* first=side(battle,1);void* second=side(donor,1);void* old=side(battle,2);
    if(!first||!second||!old||first==second)return fail("independent-seat");
    int own_fraction=(base_id(first)-1000)/100+1,other_fraction=(base_id(second)-1000)/100+1;
    if(local_match::playing&&(own_fraction!=local_faction::active||other_fraction!=local_match::state.opponent))return fail("opposing-faction-contract");
    log("FACTIONS player=%d ai=%d independent_templates=1",own_fraction,other_fraction);
    // A second native deserialization supplies independent mutable collections.
    // Never shallow-clone the live player and accidentally share its queues.
    for(int offset:{0xf0,0xf8,0x140,0x148}) {
        void* a=*(void**)((char*)first+offset);void* b=*(void**)((char*)second+offset);
        if(a&&a==b)return fail("shared-seat-collection");
    }
    Roots roots;
    void* sides=*(void**)((char*)battle+0x180);
    void* dictionary=*(void**)((char*)battle+0x178);
    void* alliances=*(void**)((char*)battle+0x170);
    // Archived starts may defer the convenience SideList until engine init.
    if(!sides) {
        sides=local_campaign::field_list(object_class(battle),"$Ob",roots);
        if(!sides||!put(battle,0x180,sides))return fail("create-seat-list");
    }
    log("CONTAINERS dictionary=%d list=%d alliances=%zu",dictionary!=nullptr,sides!=nullptr,alliances?array_length(alliances):0);
    if(!dictionary||!alliances||array_length(alliances)!=2)return fail("seat-container");
    void* owner[2]={};
    for(int i=0;i<2;++i) {
        void* a=((void**)((char*)alliances+32))[i];
        void* members=a?*(void**)((char*)a+0x18):nullptr;
        int count=members?*(int*)((char*)members+0x18):0;
        for(int j=0;j<count;++j) {
            void* member=list_item(members,j);
            if(member==first)owner[0]=a;
            if(member==old)owner[1]=a;
        }
    }
    if(!owner[0]||!owner[1]||owner[0]==owner[1])return fail("opposing-alliances");
    if(!empty_side(first)||!empty_side(second))return fail("empty-armies");
    if(offline_world_runtime::gameplay_scope())for(void* s:{first,second})for(int offset:{0x108,0x110})
        if(!clear(*(void**)((char*)s+offset)))return fail("clear-captured-inventory");
    *(int*)((char*)second+0x10)=2;
    if(!clear(sides)||!clear(dictionary))return fail("clear-seat-index");
    for(int id=1;id<=2;++id) {
        void* value=id==1?first:second;void* ignored=nullptr;void* args[]={&id,value};
        if(!local_bot_runtime::call(dictionary,"Add",2,args,ignored)||!local_campaign::append(sides,value))return fail("publish-seat");
    }
    for(int i=0;i<2;++i) {
        void* a=owner[i];void* members=*(void**)((char*)a+0x18);
        if(!clear(members)||!local_campaign::append(members,i?second:first))return fail("publish-alliance");
        for(int offset:{0x78,0x80,0x88,0xa0,0xa8,0xb0,0xb8,0xc0})
            if(!clear(*(void**)((char*)a+offset)))return fail("clear-preplaced");
    }
    if(!clear(*(void**)((char*)battle+0x1a8)))return fail("clear-entity-index");
    void* gai=*(void**)((char*)battle+0x140);
    if(!gai||!clear(*(void**)((char*)gai+0x28))||!clear(*(void**)((char*)gai+0x30)))return fail("remove-tutorial-input");
    // Any serialized side configuration must agree with the replacement seat.
    void* config=*(void**)((char*)gai+0x20);
    void* configs=config?*(void**)((char*)config+0x10):nullptr;int faction=-1;
    int count=configs?*(int*)((char*)configs+0x18):0;
    void* donor_gai=*(void**)((char*)donor+0x140);
    void* donor_config=donor_gai?*(void**)((char*)donor_gai+0x20):nullptr;
    void* donor_configs=donor_config?*(void**)((char*)donor_config+0x10):nullptr;
    int donor_count=donor_configs?*(int*)((char*)donor_configs+0x18):0;
    for(int j=0;j<donor_count;++j){void* c=list_item(donor_configs,j);if(c&&*(int*)((char*)c+0x10)==1)faction=*((int8_t*)c+0x18);}
    if(faction<0)return fail("opposing-gai-faction");
    for(int j=0;j<count;++j){void* c=list_item(configs,j);if(c&&*(int*)((char*)c+0x10)==2&&faction>=0){*((int8_t*)c+0x18)=faction;*((bool*)c+0x19)=false;}}
    for(int j=count-1;j>=0;--j) {
        void* c=list_item(configs,j);int id=c?*(int*)((char*)c+0x10):0;
        if(id==1||id==2)continue;
        void* ignored=nullptr;void* args[]={&j};
        if(!local_bot_runtime::call(configs,"RemoveAt",1,args,ignored))return fail("remove-script-seat-config");
    }
    log("PREPARED seats=2 independent_decode=1 independent_unit_dictionaries=1 independent_building_dictionaries=1 tutorial_commands=0 tutorial_bases=0 preplaced_units=0 preplaced_buildings=0");
    return true;
}
static void* side_config(void* battle,int id) {
    void* gai=battle?*(void**)((char*)battle+0x140):nullptr;
    void* config=gai?*(void**)((char*)gai+0x20):nullptr;
    void* list=config?*(void**)((char*)config+0x10):nullptr;
    int n=list?*(int*)((char*)list+0x18):0;
    for(int i=0;i<n;++i){void* c=list_item(list,i);if(c&&*(int*)((char*)c+0x10)==id)return c;}
    return nullptr;
}
static int alliance_of(void* battle,void* s) {
    void* alliances=*(void**)((char*)battle+0x170);
    for(size_t i=0;alliances&&i<array_length(alliances);++i){
        void* a=((void**)((char*)alliances+32))[i];void* members=a?*(void**)((char*)a+0x18):nullptr;
        int n=members?*(int*)((char*)members+0x18):0;
        for(int j=0;j<n;++j)if(list_item(members,j)==s)return (int)i;
    }
    return -1;
}
static bool extend_teams(void* battle,void* ally_battle,void* enemy_battle) {
    if(configured_seats!=2&&configured_seats!=4)return fail("team-capacity");
    int first_id=configured_seats+1;
    void* sources[]={ally_battle,enemy_battle};void* values[]={side(ally_battle,1),side(enemy_battle,1)};
    if(!values[0]||!values[1]||ally_battle==enemy_battle)return fail("team-independent-sources");
    void* dictionary=*(void**)((char*)battle+0x178);void* sides=*(void**)((char*)battle+0x180);
    void* alliances=*(void**)((char*)battle+0x170);
    void* gai=*(void**)((char*)battle+0x140);void* config=*(void**)((char*)gai+0x20);
    void* configs=*(void**)((char*)config+0x10);
    for(int i=0;i<2;++i){
        void* value=values[i];int id=i+first_id;void* owner_side=side(battle,i+1);
        if(base_id(value)!=base_id(owner_side))return fail("team-faction");
        for(int old=1;old<id;++old)for(int offset:{0xf0,0xf8,0x140,0x148}){
            void* a=*(void**)((char*)value+offset);void* b=*(void**)((char*)side(battle,old)+offset);
            if(value==side(battle,old)||(a&&a==b))return fail("team-shared-collection");
        }
        if(!empty_side(value))return fail("team-empty-army");
        for(int offset:{0x108,0x110})if(!clear(*(void**)((char*)value+offset)))return fail("team-inventory");
        int index=alliance_of(battle,owner_side);if(index<0)return fail("team-alliance");
        void* owner=((void**)((char*)alliances+32))[index];void* members=*(void**)((char*)owner+0x18);
        void* c=side_config(sources[i],1);if(!c||c==side_config(battle,1)||c==side_config(battle,2))return fail("team-config");
        *(int*)((char*)value+0x10)=id;*(int*)((char*)c+0x10)=id;
        *(int*)((char*)c+0x14)=*(int*)((char*)owner+0x10);*((bool*)c+0x19)=false;
        void* ignored=nullptr;void* args[]={&id,value};
        if(!local_bot_runtime::call(dictionary,"Add",2,args,ignored)||!local_campaign::append(sides,value)||
           !local_campaign::append(members,value)||!local_campaign::append(configs,c))return fail("team-publish");
        log("TEAM_SEAT side=%d alliance_index=%d faction=%d independent_decode=1",id,index,(base_id(value)-1000)/100+1);
    }
    configured_seats+=2;
    for(int id=1;id<=configured_seats;++id)if(alliance_of(battle,side(battle,id))!=alliance_of(battle,side(battle,id%2?1:2)))return fail("team-membership");
    if(configured_seats==4)log("TEAMS seats=4 player=1 ally=3 enemies=2,4 human_control_untouched=1");
    else log("TEAMS seats=6 player=1 allies=3,5 enemies=2,4,6 human_control_untouched=1");
    return true;
}
static bool array_ref(void* array,size_t index,void* value){
    auto barrier=(void(*)(void*,void**,void*))dlsym(il2cpp,"il2cpp_gc_wbarrier_set_field");
    if(!array||index>=array_length(array)||!barrier)return false;
    void** slot=(void**)((char*)array+32+index*8);barrier(array,slot,value);return *slot==value;
}
static bool extend_free_for_all(void* battle,void* source){
    if(configured_seats<2||configured_seats>=6||!source||source==battle)return fail("ffa-capacity-or-source");
    int id=configured_seats+1;void* value=side(source,1);if(!value||base_id(value)!=base_id(side(battle,2)))return fail("ffa-faction");
    for(int old=1;old<id;++old)for(int offset:{0xf0,0xf8,0x140,0x148}){
        void* a=*(void**)((char*)value+offset),*b=*(void**)((char*)side(battle,old)+offset);
        if(value==side(battle,old)||(a&&a==b))return fail("ffa-shared-collection");}
    int owner_index=alliance_of(source,value);void* old_alliances=*(void**)((char*)source+0x170);
    void* owner=owner_index>=0?((void**)((char*)old_alliances+32))[owner_index]:nullptr;
    Roots roots;void* alliances=owner?array_new(object_class(owner),id):nullptr;
    if(!roots.keep(alliances)||!empty_side(value))return fail("ffa-owner-or-empty-army");
    for(int offset:{0x108,0x110})if(!clear(*(void**)((char*)value+offset)))return fail("ffa-inventory");
    for(int offset:{0x78,0x80,0x88,0xa0,0xa8,0xb0,0xb8,0xc0})if(!clear(*(void**)((char*)owner+offset)))return fail("ffa-preplaced");
    void* members=*(void**)((char*)owner+0x18);
    if(!clear(members)||!local_campaign::append(members,value))return fail("ffa-single-member");
    void* previous=*(void**)((char*)battle+0x170);
    for(int i=1;i<id;++i){int index=alliance_of(battle,side(battle,i));
        void* a=index>=0?((void**)((char*)previous+32))[index]:nullptr;
        void* c=side_config(battle,i);if(!a||!c)return fail("ffa-existing-alliance-or-config");
        if(!array_ref(alliances,i-1,a))return fail("ffa-array-reference");
        *(int*)((char*)a+0x10)=i;*(int*)((char*)c+0x14)=i;
    }
    void* c=side_config(source,1);if(!c||!array_ref(alliances,id-1,owner))return fail("ffa-side-config");
    *(int*)((char*)owner+0x10)=id;*(int*)((char*)value+0x10)=id;*(int*)((char*)c+0x10)=id;*(int*)((char*)c+0x14)=id;*(bool*)((char*)c+0x19)=false;
    void* dictionary=*(void**)((char*)battle+0x178),*sides=*(void**)((char*)battle+0x180);
    void* gai=*(void**)((char*)battle+0x140),*config=*(void**)((char*)gai+0x20),*configs=*(void**)((char*)config+0x10);
    void* ignored=nullptr;void* args[]={&id,value};
    if(!local_bot_runtime::call(dictionary,"Add",2,args,ignored)||!local_campaign::append(sides,value)||!local_campaign::append(configs,c)||!put(battle,0x170,alliances))return fail("ffa-publish");
    configured_seats=id;
    for(int i=1;i<=id;++i)if(alliance_of(battle,side(battle,i))!=i-1)return fail("ffa-independent-membership");
    log("FFA_SEAT side=%d alliance_index=%d faction=%d independent_decode=1 independent_armies=1",id,id-1,(base_id(value)-1000)/100+1);
    return true;
}
#include "control_point_runtime.h"
static bool seed(void* battle,int now) {
    if(local_match::playing&&*((bool*)battle+0xdc)&&!skill_chest::initial_resources(side(battle,1)))return fail("local-initial-resource-skill");
    auto valid=reinterpret_cast<bool(*)(void*,void*,int,int,int16_t,void*)>(image_base+0x4922a7c);
    const auto* route=local_match_maps::find(offline_world_runtime::batch_fixture?offline_world_runtime::batch_job.scenario:local_match::playing?local_match::state.scenario:(naval_test?naval_fixture_chapter:hero_live_lab::active?202:201));
    if(!route)return fail("unsupported-map-route");
    bool automatic=local_match_maps::automatic(route);ai_spawn_plan::Point positions[6];ai_spawn_runtime::Layout map;
    if(automatic){
        void* s=side(battle,1);void* type=s?local_dictionary_item(*(void**)((char*)s+0xf8),base_id(s)):nullptr;
        if(seat_count()>route->max_seats||!ai_spawn_runtime::sample(battle,type,map)||
           !ai_spawn_plan::plan(map.mask,map.width,map.height,seat_count(),positions,map.transit,local_battle_format::free_for_all(control_points::session.mode)))return fail("original-terrain-spawn-plan");
        log("SPAWN_PLAN map=%d seats=%d width=%d height=%d ground_centers=%d original_grid=1 native_occupancy=1",route->map,seat_count(),map.width,map.height,map.valid);
    }
    for(int id=1;id<=seat_count();++id) {
        void* s=side(battle,id);int headquarters=base_id(s);
        void* type=s&&headquarters>=0?local_dictionary_item(*(void**)((char*)s+0xf8),headquarters):nullptr;
        if(!type||ai_lab::entities(s,0x140)||ai_lab::entities(s,0x148))return fail("nonempty-start");
        int cx=id%2?route->x1:route->x2,cy=(id%2?route->y1:route->y2)+((id-1)/2)*(id%2?16:-16),x=0,y=0;bool found=false;
        if(automatic){x=positions[id-1].x;y=positions[id-1].y;found=valid(battle,type,x,y,0,nullptr);}
        for(int r=0;!automatic&&r<=12&&!found;r+=2)for(int dy=-r;dy<=r&&!found;dy+=2)for(int dx=-r;dx<=r;dx+=2) {
            int px=cx+dx,py=cy+dy;
            if(px<2||py<2||!valid(battle,type,px,py,0,nullptr))continue;
            bool separated=true;
            for(int prior=1;prior<id;++prior){
                void* other=ai_lab::building(side(battle,prior),base_id(side(battle,prior)));
                if(other){int dx=px-*(int16_t*)((char*)other+0x40),dy=py-*(int16_t*)((char*)other+0x42);
                    int minimum=ai_team_policy::allied(id,prior)?100:256;if(dx*dx+dy*dy<minimum)separated=false;}
            }
            if(!separated)continue;
            x=px;y=py;found=true;break;
        }
        if(!found)return fail("hq-footprint");
        // Exact argument mapping from GAICommandBuilding.execute at 0x4604510.
        int8_t level=0,state=1,base=0;int16_t angle=0;bool schedule=false;int request=0,extra=0;
        void* args[]={battle,s,type,&x,&y,&level,&angle,&state,&base,&schedule,&request,&extra};void* exception=nullptr;
        runtime_invoke(class_method(find_class("com.geargames.aow.entities.battle","BuildingAct"),"$Vb",12),nullptr,args,&exception);
        void* hq=ai_lab::building(s,headquarters);
        if(exception||!hq)return fail("native-hq-registration");
        log("SEED tick=%d side=%d building=%d cell=%d,%d state=%d gold=%d units=%d",now,id,*(int*)((char*)hq+0x10),x,y,*((uint8_t*)hq+0x47),ai_lab::gold(s),ai_lab::entities(s,0x140));
        seats[id-1].money.observe(ai_lab::gold(s),now);
    }
    if(control_points::enabled()&&(!automatic||!control_points::setup(battle,map,positions,now)))return fail("control-point-layout-or-native-factory");
    // GAICommandAliveAllianceWin.execute (0x4600510) forwards switchOff
    // to this original Battle setter. Enable only after both bases exist.
    bool switch_off=false;void* args[]={&switch_off};void* ignored=nullptr;
    if(!local_bot_runtime::call(battle,"$bn",1,args,ignored)||*((bool*)battle+0xe0))return fail("native-alive-alliance-rule");
    log("RULE native_alive_alliance_win=1 winner_assigned_by_adapter=0");
    if(!local_match::playing&&!access(DATA_ROOT "ai-duel-fast.enabled",F_OK)) {
        void* executor=static_object("com.geargames.aow.scenes.battle","ClientBattleContext","m_logicExecutor");
        int target=offline_world_runtime::batch_fixture?offline_world_runtime::batch_job.max_tick+20:offline_world_runtime::strategy_fixture||local_battle_format::free_for_all(control_points::session.mode)?30000:naval_test&&!access(DATA_ROOT "ai-naval-site-audit.enabled",F_OK)?(!access(DATA_ROOT "ai-naval-recon-audit.enabled",F_OK)?4600:1200):(naval_test||advanced_air_test)?45000:advanced_test?30000:tank_test?18000:(base_test?9000:12000);void* fast_args[]={&target};void* result=nullptr;
        if(!executor||!local_bot_runtime::call(executor,"FastExecuteLogicToTick",1,fast_args,result))return fail("native-fast-forward-request");
        log("FAST_FORWARD original_logic_thread_api=1 from=%d target=%d skip_ticks=0",now,target);
    }
    return true;
}
static int alive(void* s,int offset) {
    void* array=s?*(void**)((char*)s+offset):nullptr;int count=0;
    for(size_t i=0;array&&i<array_length(array);++i) {
        void* value=((void**)((char*)array+32))[i];if(!value)continue;
        if(offset==0x148) {if(!*((bool*)value+0x139))++count;}
        else if(*(int*)((char*)value+0x184)>0)++count;
    }
    return count;
}
static int team_alive(void* battle,int team_side,int offset) {
    int total=0;int owner=alliance_of(battle,side(battle,team_side));
    if(owner<0)return 0;
    for(int id=1;id<=seat_count();++id)if(alliance_of(battle,side(battle,id))==owner)total+=alive(side(battle,id),offset);
    return total;
}
static void* opponent(void* battle,void* s) {
    int owner=alliance_of(battle,s),best=INT_MAX;void* chosen=nullptr;
    void* hq=ai_lab::building(s,base_id(s));int x=hq?*(int16_t*)((char*)hq+0x40):0,y=hq?*(int16_t*)((char*)hq+0x42):0;
    for(int id=1;id<=seat_count();++id){
        void* enemy=side(battle,id);if(!enemy||alliance_of(battle,enemy)==owner||(!alive(enemy,0x148)&&!alive(enemy,0x140)))continue;
        void* target=ai_lab::building(enemy,base_id(enemy));int distance=INT_MAX-1;
        if(target){int dx=*(int16_t*)((char*)target+0x40)-x,dy=*(int16_t*)((char*)target+0x42)-y;distance=dx*dx+dy*dy;}
        if(distance<best){best=distance;chosen=enemy;}
    }
    return chosen;
}
static bool terminal(void* battle,int now) {
    if(finish_requested)return true;
    if(!seeded)return false;
    if(local_match::playing&&requested_surrender.exchange(false)){
        // The original UI has confirmed a human exit and sent its command.
        // A match can have several surviving AI armies. Use the original
        // explicit-exit builder; never choose an arbitrary surviving opponent.
        int reason=6;void* args[]={battle,&reason};void* exception=nullptr;
        const void* method=class_method(find_class("com.geargames.aow.entities.battle","BattleAct"),"$s",2);
        if(!method){fail("local-match-original-exit-method");return true;}finish_requested=true;
        runtime_invoke(method,nullptr,args,&exception);
        if(exception){finish_requested=false;fail("local-match-original-exit-dispatch");return true;}
        log("SURRENDER_DISPATCH tick=%d original_exit_builder=1 reason=6 winner_assigned_by_adapter=0",now);return true;
    }
    if(!offline_world_runtime::gameplay_scope()&&!control_points::enabled()&&!local_battle_format::free_for_all(control_points::session.mode)&&(!damaged||!seats[0].developed||!seats[1].developed))return false;
    int b1=team_alive(battle,1,0x148),b2=team_alive(battle,2,0x148);
    int u1=team_alive(battle,1,0x140),u2=team_alive(battle,2,0x140);
    int buildings[6]={},units[6]={};
    for(int id=1;id<=seat_count();++id){buildings[id-1]=alive(side(battle,id),0x148);units[id-1]=alive(side(battle,id),0x140);}
    int winner=ai_team_policy::winner(buildings,units,seat_count());
    if(local_battle_format::free_for_all(control_points::session.mode)){
        winner=local_battle_format::ffa_winner(buildings,units,seat_count());
        if(winner== -2){fail("ffa-terminal-observation");return true;}
    }
    if(control_points::enabled()){
        auto result=control_point_policy::terminal(control_point_policy::rule(control_points::session.mode),b1+u1,b2+u2,
            control_points::session.scores[0],control_points::session.scores[1],control_point_rules::score_target);
        if(result==control_point_policy::Result::Invalid){fail("control-point-terminal-observation");return true;}
        winner=result==control_point_policy::Result::First?1:result==control_point_policy::Result::Second?2:result==control_point_policy::Result::Draw?0:-1;
    }
    if(winner<0){terminal_candidate=terminal_since=-1;return false;}
    if(winner!=terminal_candidate){terminal_candidate=winner;terminal_since=now;return false;}
    if(now-terminal_since<20)return false;
    void* alliances=*(void**)((char*)battle+0x170);int8_t index=-1;
    for(size_t i=0;alliances&&i<array_length(alliances);++i) {
        void* a=((void**)((char*)alliances+32))[i];void* members=a?*(void**)((char*)a+0x18):nullptr;
        int count=members?*(int*)((char*)members+0x18):0;
        for(int j=0;j<count;++j)if(list_item(members,j)==side(battle,winner))index=(int8_t)i;
    }
    void* points=*(void**)((char*)battle+0xd0);
    if((index<0&&winner!=0)||!points||strcmp(class_name(object_class(points)),"PvEBattleResults")){fail("terminal-contract");return true;}
    control_points::session.draw=winner==0;
    log("TERMINAL tick=%d stable_since=%d winner_side=%d winner_index=%d buildings=%d,%d units=%d,%d mode=%d scores=%d,%d",now,terminal_since,winner,index,b1,b2,u1,u2,control_points::session.mode,control_points::session.scores[0],control_points::session.scores[1]);
    // The archived PvE template has no PvP terminal evaluator. Derive the
    // result from actual elimination, then use the original winner/reason
    // setters and result/command dispatcher ($S), not a win-button shortcut.
    int reason=1;void* ignored=nullptr;void* win_args[]={&index};void* reason_args[]={&reason};
    if(!local_bot_runtime::call(battle,"$to",1,win_args,ignored)||
       !local_bot_runtime::call(battle,"$tM",1,reason_args,ignored)){fail("native-result-setters");return true;}
    void* args[]={battle};void* exception=nullptr;
    const void* method=class_method(find_class("com.geargames.aow.entities.battle","BattleAct"),"$S",1);
    if(!method){fail("native-result-dispatch-method");return true;}
    finish_requested=true;
    runtime_invoke(method,nullptr,args,&exception);
    if(exception){fail("native-result-dispatch");return true;}
    log("DISPATCH original_result_builder=1 original_finish_commands=1 rule_mode=%d observed_native_state=1",control_points::session.mode);
    return true;
}
static void finished(bool win,int finish_tick,int last_tick) {
    if(stop_recorded)return;
    void* battle=static_object("com.geargames.aow.scenes.battle","ClientBattleContext","m_battle");
    if(!battle||!seeded){fail("finish-without-local-battle");return;}
    void* points=*(void**)((char*)battle+0xd0);
    void* winner=points&&!strcmp(class_name(object_class(points)),"PvEBattleResults")?*(void**)((char*)points+0x70):nullptr;
    int reason=*(int*)((char*)battle+0xd8),winner_id=winner?*(int*)((char*)winner+0x10):-1;
    int b1=alive(side(battle,1),0x148),b2=alive(side(battle,2),0x148);
    int u1=alive(side(battle,1),0x140),u2=alive(side(battle,2),0x140);
    log("NATIVE_STOP win=%d finish_tick=%d last_tick=%d reason=%d winner_alliance=%d buildings=%d,%d units=%d,%d",win,finish_tick,last_tick,reason,winner_id,b1,b2,u1,u2);
    // A local last-result receipt is separate from campaign currency/stars.
    // It records an observed native callback, never manufactures a winner.
    const char* temporary=DATA_ROOT "local-duel-result.tmp";
    FILE* f=fopen(temporary,"w");if(!f){fail("result-open");return;}
    int wrote=fprintf(f,"{\"schema\":1,\"mode\":\"native-duel-experiment\",\"win\":%s,\"finish_tick\":%d,\"last_tick\":%d,\"native_reason\":%d,\"winner_alliance\":%d,\"alive_buildings\":[%d,%d],\"alive_units\":[%d,%d],\"career_writes\":false}\n",win?"true":"false",finish_tick,last_tick,reason,winner_id,b1,b2,u1,u2);
    bool ok=wrote>0&&fflush(f)==0&&fsync(fileno(f))==0;
    if(fclose(f))ok=false;
    if(ok)ok=rename(temporary,DATA_ROOT "local-duel-result.json")==0;
    if(!ok){fail("result-commit");return;}
    stop_recorded=true;stopped=true;log("RESULT saved=1 campaign_reward=0 native_callback=1");
    batch_report(battle,reason==1?"normal":"native_abnormal",finish_tick,winner_id,reason);
}
#include "ai_naval_map_runtime.h"
#include "ai_tech_dependency_runtime.h"
static bool develop(void* battle,void* s,int now) {
    int id=*(int*)((char*)s+0x10);Seat& seat=seats[id-1];
    int headquarters=base_id(s);if(headquarters<0)return fail("unsupported-faction-prototypes");
    void* hq=ai_lab::building(s,headquarters);
    ai_base_policy::Phase phases[4];
    for(int i=0;i<4;++i) {
        void* b=ai_lab::building(s,headquarters+ai_base_policy::types[i]-1100);
        phases[i]=!b?ai_base_policy::Missing:(*((uint8_t*)b+0x47)==1?ai_base_policy::Ready:ai_base_policy::Constructing);
    }
    auto plan=ai_base_policy::inspect(hq!=nullptr,phases,seat.completed);
    seat.completed=plan.completed;seat.reserve_rebuild=plan.reserve;
    seat.developed=plan.complete||!hq;
    if(!hq||!plan.count)return true;
    for(int candidate=0;candidate<plan.count;++candidate) {
    int index=plan.candidates[candidate],type_id=headquarters+ai_base_policy::types[index]-1100;
    bool rebuild=(seat.completed&(1u<<index))!=0;
    Roots roots;void* c=ai_lab::command("AICommBuSet",now,roots,id);if(!c)return fail("build-command");
    *(int*)((char*)c+0x30)=type_id;*(int*)((char*)c+0x3c)=++sequence;
    void* type=local_dictionary_item(*(void**)((char*)s+0xf8),type_id);
    if(!type)return fail("build-type");
    if(offline_world_runtime::planner(id)){
        void* levels=*(void**)((char*)type+0x18),*first=levels?list_item(levels,0):nullptr;
        if(first)satisfy_tech_requirements(battle,s,first,now,true);
        return !stopped;
    }
    if(local_coop::playing){
        void* levels=*(void**)((char*)type+0x18);void* first=levels?list_item(levels,0):nullptr;
        // Wait for the original base price before trying placement. This does
        // not grant resources or bypass the final native command validators.
        if(first&&*(int*)((char*)first+0x6c)>ai_lab::gold(s))continue;
    }
    int hx=*(int16_t*)((char*)hq+0x40),hy=*(int16_t*)((char*)hq+0x42),tested=0;
    auto valid=reinterpret_cast<bool(*)(void*,void*,int,int,int16_t,void*)>(image_base+0x4922a7c);
    for(int r=4;r<=24&&tested<32;r+=2)for(int dy=-r;dy<=r&&tested<32;dy+=2)for(int dx=-r;dx<=r&&tested<32;dx+=2) {
        if(abs(dx)!=r&&abs(dy)!=r)continue;int x=hx+dx,y=hy+dy;
        if(x<0||y<0||!valid(battle,type,x,y,0,nullptr))continue;
        *(int16_t*)((char*)c+0x40)=x;*(int16_t*)((char*)c+0x42)=y;
        int before=ai_lab::gold(s),code=ai_lab::perform(battle,c);
        if(ai_lab::stopped)return fail("build-exception");
        if(!code){phases[index]=ai_base_policy::Constructing;seat.reserve_rebuild=ai_base_policy::inspect(true,phases,seat.completed).reserve;log("BUILD tick=%d side=%d type=%d cell=%d,%d gold=%d->%d rebuild=%d",now,id,type_id,x,y,before,ai_lab::gold(s),rebuild);return true;}
        if(++tested==32){if(now%100==0)log("WAIT tick=%d side=%d type=%d code=%d gold=%d rebuild=%d",now,id,type_id,code,before,rebuild);break;}
    }
    if(!tested&&now%100==0)log("WAIT tick=%d side=%d type=%d no-footprint=1 rebuild=%d",now,id,type_id,rebuild);
    // A rejected native prerequisite/cost/placement is not a controller failure.
    // Continue with another missing requirement, then retry on future income.
    }
    return true;
}
static bool advance(void* battle,void* s,int now) {
    int id=*(int*)((char*)s+0x10);Seat& seat=seats[id-1];seat.reserve_tech=false;
    if(offline_world_runtime::planner(id)&&seat.economy_threat)return true;
    if((!offline_world_runtime::gameplay_scope()&&!local_coop::playing&&!tech_test)||!seat.developed||seat.reserve_rebuild)return true;
    int headquarters=base_id(s);void* hq=ai_lab::building(s,headquarters);
    if(!hq||*((uint8_t*)hq+0x47)!=1)return true;
    int level=*((int8_t*)hq+0x60);
    const auto* commander=offline_world_runtime::gameplay_scope()?offline_world_runtime::actor(id):nullptr;
    int tech_army=commander&&commander->style==offline_world::Pressure?5:commander&&commander->style==offline_world::Technology?2:3;
    if(level<1){
        void* type=*(void**)((char*)hq+0x38);void* levels=type?*(void**)((char*)type+0x18):nullptr;
        void* next=levels&&*(int*)((char*)levels+0x18)>level+1?list_item(levels,level+1):nullptr;
        // Available is a workshop/research state, not the native battle
        // upgrade permission (even the existing HQ tier may carry zero).
        // Keep that state intact and let the original command check its actual
        // contribution/tech/cost prerequisites. Never write the HQ level.
        if(!next||alive(s,0x140)<tech_army)return true;
        if(offline_world_runtime::planner(id)){
            if(!seat.guard.defending)satisfy_tech_requirements(battle,s,next,now,true);
            return !stopped;
        }
        int price=*(int*)((char*)next+0x6c);
        seat.reserve_tech=price>0;
        if(ai_lab::gold(s)<price||now<seat.next_tech)return true;
        seat.next_tech=now+100;
        Roots roots;void* command=ai_lab::command("AICommBuildingUpgrade",now,roots,id);
        if(!command)return fail("hq-upgrade-command");
        *(int*)((char*)command+0x30)=*(int*)((char*)hq+0x10);
        int before=ai_lab::gold(s),code=ai_lab::perform(battle,command);
        if(ai_lab::stopped)return fail("hq-upgrade-exception");
        if(!code){seat.reserve_tech=false;log("HQ_UPGRADE tick=%d side=%d from=%d gold=%d->%d state=%d native_duration=1",now,id,level,before,ai_lab::gold(s),*((uint8_t*)hq+0x47));}
        else {seat.reserve_tech=false;log("TECH_WAIT tick=%d side=%d upgrade_code=%d gold=%d",now,id,code,before);}
        return true;
    }
    int type_id=headquarters+7;void* factory=ai_lab::building(s,type_id);
    if(factory){
        if(!seat.vehicle_factory_seen&&*((uint8_t*)factory+0x47)==1){
            seat.vehicle_factory_seen=true;log("VEHICLE_FACTORY_READY tick=%d side=%d type=%d native_construction=1",now,id,type_id);
        }
        if(!ai_tech_policy::extended(offline_world_runtime::gameplay_scope(),tech_test))return true;
        auto upgrade=ai_tech_policy::next(level,*((uint8_t*)factory+0x47)==1,*((int8_t*)factory+0x60),alive(s,0x140)*3/tech_army,seat.guard.defending,seat.expanded_army);
        void* target=upgrade==ai_tech_policy::Headquarters?hq:upgrade==ai_tech_policy::Factory?factory:nullptr;
        int current=target?*((int8_t*)target+0x60):-1;
        void* prototype=target?*(void**)((char*)target+0x38):nullptr,*tiers=prototype?*(void**)((char*)prototype+0x18):nullptr;
        void* tier=tiers&&current>=0&&current+1<*(int*)((char*)tiers+0x18)?list_item(tiers,current+1):nullptr;
        if(!tier)return true;
        if(offline_world_runtime::planner(id)){satisfy_tech_requirements(battle,s,tier,now,true);return !stopped;}
        if(!satisfy_tech_requirements(battle,s,tier,now))return true;
        int price=*(int*)((char*)tier+0x6c);if(price<0)return fail("ground-tech-negative-price");
        seat.reserve_tech=price>0;
        if(ai_lab::gold(s)<price||now<seat.next_tech)return true;
        seat.next_tech=now+100;
        Roots roots;void* command=ai_lab::command("AICommBuildingUpgrade",now,roots,id);if(!command)return fail("ground-tech-command");
        *(int*)((char*)command+0x30)=*(int*)((char*)target+0x10);
        int before=ai_lab::gold(s),code=ai_lab::perform(battle,command);seat.reserve_tech=false;
        if(ai_lab::stopped)return fail("ground-tech-exception");
        if(!code)log("GROUND_TECH_UPGRADE tick=%d side=%d type=%d from=%d gold=%d->%d state=%d native_duration=1",now,id,*(int*)((char*)prototype+0x10),current,before,ai_lab::gold(s),*((uint8_t*)target+0x47));
        else log("TECH_WAIT tick=%d side=%d type=%d from=%d code=%d gold=%d",now,id,*(int*)((char*)prototype+0x10),current,code,before);
        return true;
    }
    void* type=local_dictionary_item(*(void**)((char*)s+0xf8),type_id);
    void* levels=type?*(void**)((char*)type+0x18):nullptr;void* first=levels?list_item(levels,0):nullptr;
    if(!first)return true; // Original build validator owns battle availability.
    // Initial ground production shares the same contribution/quota, full
    // placement preflight and persistent search cursor as later producers.
    satisfy_tech_requirements(battle,s,first,now,true);
    return !stopped;
}
static bool advance_air(void* battle,void* s,int now){
    int id=*(int*)((char*)s+0x10);Seat& seat=seats[id-1];
    if(seat.reserve_rebuild||seat.reserve_tech)return true;
    void* hq=ai_lab::building(s,base_id(s)),*factory=ai_lab::building(s,base_id(s)+7);
    if(!hq||*((uint8_t*)hq+0x47)!=1||!ai_tech_policy::air(offline_world_runtime::gameplay_scope()||air_test,*((int8_t*)hq+0x60),
        factory&&*((uint8_t*)factory+0x47)==1,factory?*((int8_t*)factory+0x60):0,seat.ground_tier_born,alive(s,0x140),seat.guard.defending))return true;
    // Original kind9 is the air factory; kind8 is the advanced ground factory.
    void* type=local_dictionary_item(*(void**)((char*)s+0xf8),base_id(s)+9);
    if(!type||*((int8_t*)type+0x30)!=9)return true;
    void* existing=ai_lab::building(s,base_id(s)+9);
    if(existing){if(*((uint8_t*)existing+0x47)==1&&!seat.air_factory_seen){seat.air_factory_seen=true;
            log("AIR_FACTORY_READY tick=%d side=%d type=%d native_construction=1",now,id,*(int*)((char*)type+0x10));}return true;}
    void* levels=*(void**)((char*)type+0x18),*goal=levels?list_item(levels,0):nullptr;
    // The same original-recipe planner also builds this goal through the
    // original command. It still waits for in-progress inputs and real money.
    if(goal)satisfy_tech_requirements(battle,s,goal,now,true);
    return !stopped;
}
#include "ai_advanced_runtime.h"
#include "ai_advanced_air_runtime.h"
#include "ai_naval_runtime.h"
#include "ai_naval_site_audit.h"
static ai_guard_policy::Threat guard_threat(void* battle,void* s,void* enemy,int& observer,int& visible_threats) {
    ai_guard_policy::Threat best{};observer=alliance_of(battle,s);visible_threats=0;
    if(observer<0||observer>=8)return best;
    for(int enemy_id=1;enemy_id<=seat_count();++enemy_id){
    void* attacker_side=side(battle,enemy_id);
    if(!attacker_side||alliance_of(battle,attacker_side)==observer)continue;
    void* attackers=*(void**)((char*)attacker_side+0x140);
    for(size_t i=0;attackers&&i<array_length(attackers);++i) {
        void* u=((void**)((char*)attackers+32))[i];if(!u||*(int*)((char*)u+0x184)<=0)continue;
        // Unit.Visible is bool[] at +198; the original fog code owns it.
        void* visible=*(void**)((char*)u+0x198);
        if(!visible||array_length(visible)>8||(size_t)observer>=array_length(visible)||!*((uint8_t*)visible+32+observer))continue;
        bool intruding=false;for(int friendly=1;friendly<=seat_count();++friendly){
        void* base_side=side(battle,friendly);if(!base_side||alliance_of(battle,base_side)!=observer)continue;
        void* bases=*(void**)((char*)base_side+0x148);
        for(size_t j=0;bases&&j<array_length(bases);++j) {
            void* b=((void**)((char*)bases+32))[j];
            if(!b||*((bool*)b+0x139)||*(int*)((char*)b+0x5c)<=0||*((int8_t*)b+0x20)!=observer)continue;
            ai_guard_policy::consider(best,*(int*)((char*)u+0x10),*(int16_t*)((char*)u+0x48),*(int16_t*)((char*)u+0x4a),true,true,
                *(int16_t*)((char*)b+0x40),*(int16_t*)((char*)b+0x42));
            ai_guard_policy::Threat nearby;ai_guard_policy::consider(nearby,*(int*)((char*)u+0x10),*(int16_t*)((char*)u+0x48),*(int16_t*)((char*)u+0x4a),true,true,
                *(int16_t*)((char*)b+0x40),*(int16_t*)((char*)b+0x42));intruding|=nearby.id!=0;
        }
        }
        if(intruding)++visible_threats;
    }
    }
    return best;
}
struct Goal { int x,y,id,kind; };
static int ground_production_tier(void* s,void* type){
    void* factory=local_dictionary_item(*(void**)((char*)s+0xf8),base_id(s)+7);
    void* levels=factory?*(void**)((char*)factory+0x18):nullptr;
    int count=levels?*(int*)((char*)levels+0x18):0;if(count<1||count>5)return -1;
    for(int k=0;k<count;++k){void* tier=list_item(levels,k),*units=tier?*(void**)((char*)tier+0x20):nullptr;
        int n=units?*(int*)((char*)units+0x18):0;if(n<0||n>32)return -1;
        for(int j=0;j<n;++j)if(list_item(units,j)==type)return k;}
    return -1;
}
static int ground_recruit_budget(void* s,void* factory){
    if(!factory||*((uint8_t*)factory+0x47)!=1||*((int8_t*)factory+0x60)<1)return 0;
    void* prototype=*(void**)((char*)factory+0x38),*levels=prototype?*(void**)((char*)prototype+0x18):nullptr;
    int level=*((int8_t*)factory+0x60);
    void* tier=levels&&level<*(int*)((char*)levels+0x18)?list_item(levels,level):nullptr,*units=tier?*(void**)((char*)tier+0x20):nullptr;
    int n=units?*(int*)((char*)units+0x18):0,price=INT_MAX;if(n<0||n>32)return 0;
    for(int j=0;j<n;++j){void* type=list_item(units,j);int cost=type?*(int*)((char*)type+0x48):0;
        if(cost>0&&cost<price&&ground_production_tier(s,type)==level)price=cost;}
    return price==INT_MAX?0:price;
}
static bool basic_air_type(void* s,void* type){
    if(!type||*((int8_t*)type+0x30)!=3)return false;
    void* factory=local_dictionary_item(*(void**)((char*)s+0xf8),base_id(s)+9),*levels=factory?*(void**)((char*)factory+0x18):nullptr;
    void* first=levels?list_item(levels,0):nullptr,*units=first?*(void**)((char*)first+0x20):nullptr;
    int n=units?*(int*)((char*)units+0x18):0;if(n<0||n>32)return false;
    for(int i=0;i<n;++i)if(list_item(units,i)==type)return true;return false;
}
static int air_recruit_budget(void* s,void* factory){
    if(!factory||*((uint8_t*)factory+0x47)!=1)return 0;
    void* prototype=*(void**)((char*)factory+0x38),*levels=prototype?*(void**)((char*)prototype+0x18):nullptr;
    void* first=levels?list_item(levels,0):nullptr,*units=first?*(void**)((char*)first+0x20):nullptr;
    int n=units?*(int*)((char*)units+0x18):0,price=INT_MAX;if(n<0||n>32)return 0;
    for(int i=0;i<n;++i){void* type=list_item(units,i);int cost=type?*(int*)((char*)type+0x48):0;
        if(cost>0&&cost<price&&basic_air_type(s,type))price=cost;}return price==INT_MAX?0:price;
}
#include "ai_economy_runtime.h"
static bool army(void* battle,void* s,void* enemy,int now,const Goal* goal=nullptr,int excluded_unit=0,const int* excluded_units=nullptr,int excluded_count=0) {
    int id=*(int*)((char*)s+0x10);Seat& seat=seats[id-1];
    bool extended_scope=ai_tech_policy::extended(offline_world_runtime::gameplay_scope(),tech_test);
    void* units=*(void**)((char*)s+0x140);int ids[256],unit_x[256],unit_y[256],count=0,sum_x=0,sum_y=0,scouts=0;
    int move_limit=(offline_world_runtime::gameplay_scope()||tank_test)?256:32;
    for(size_t i=0;units&&i<array_length(units);++i) {
        void* u=((void**)((char*)units+32))[i];if(!u)continue;
        int uid=*(int*)((char*)u+0x10);void* type=*(void**)((char*)u+0x40);
        if(*(void**)((char*)u+0x18)!=s||!type||local_dictionary_item(*(void**)((char*)s+0xf0),*(int*)((char*)type+0x10))!=type)return fail("unit-ownership-or-prototype");
        bool seen=false;for(int j=0;j<seat.births;++j)if(seat.ids[j]==uid)seen=true;
        if(!seen&&seat.births<128){seat.ids[seat.births++]=uid;log("BIRTH tick=%d side=%d id=%d type=%d life=%d max=%d",now,id,uid,*(int*)((char*)type+0x10),*(int*)((char*)u+0x184),*(int*)((char*)type+0x34));}
        int hp=*(int*)((char*)u+0x184),max=*(int*)((char*)type+0x34);
        if(!observe_advanced(s,u,type,hp,now))return false;
        if(!observe_advanced_air(s,u,type,hp,now))return false;
        int production_tier=hp>0?ground_production_tier(s,type):-1;
        if(hp>0&&!seat.expanded_army&&production_tier==1){
            seat.expanded_army=true;log("EXPANDED_ARMY tick=%d side=%d id=%d type=%d native_birth=1",now,id,uid,*(int*)((char*)type+0x10));}
        if(hp>0&&production_tier>seat.ground_tier_born){seat.ground_tier_born=production_tier;
            log("GROUND_TIER_BORN tick=%d side=%d tier=%d id=%d type=%d native_birth=1",now,id,production_tier,uid,*(int*)((char*)type+0x10));}
        if(hp>0&&production_tier==2&&!seat.tank_id){seat.tank_id=uid;seat.tank_x=*(int16_t*)((char*)u+0x48);seat.tank_y=*(int16_t*)((char*)u+0x4a);}
        if(hp>0&&uid==seat.tank_id&&!seat.tank_moved&&(*(int16_t*)((char*)u+0x48)!=seat.tank_x||*(int16_t*)((char*)u+0x4a)!=seat.tank_y)){
            seat.tank_moved=true;log("TANK_MOVED tick=%d side=%d id=%d from=%d,%d to=%d,%d native_pathing=1",now,id,uid,seat.tank_x,seat.tank_y,*(int16_t*)((char*)u+0x48),*(int16_t*)((char*)u+0x4a));}
        if((offline_world_runtime::gameplay_scope()||air_test)&&hp>0&&!seat.air_id&&basic_air_type(s,type)){
            seat.air_id=uid;seat.air_x=*(int16_t*)((char*)u+0x48);seat.air_y=*(int16_t*)((char*)u+0x4a);
            log("AIR_BORN tick=%d side=%d id=%d type=%d native_birth=1",now,id,uid,*(int*)((char*)type+0x10));}
        if(hp>0&&uid==seat.air_id&&!seat.air_moved&&(*(int16_t*)((char*)u+0x48)!=seat.air_x||*(int16_t*)((char*)u+0x4a)!=seat.air_y)){
            seat.air_moved=true;log("AIR_MOVED tick=%d side=%d id=%d from=%d,%d to=%d,%d native_pathing=1",now,id,uid,seat.air_x,seat.air_y,*(int16_t*)((char*)u+0x48),*(int16_t*)((char*)u+0x4a));}
        if(hp>0&&hp<max){if(!damaged)log("DAMAGE tick=%d side=%d unit=%d life=%d max=%d",now,id,uid,hp,max);damaged=true;}
        bool excluded=uid==excluded_unit;
        for(int j=0;excluded_units&&j<excluded_count&&j<8;++j)if(uid==excluded_units[j])excluded=true;
        if(hp>0&&!excluded){bool reserved=false;if(!control_points::order(battle,s,u,type,now,scouts,reserved))return fail("control-point-ai-order");excluded|=reserved;}
        if(naval_enabled()&&hp>0){bool scout_reserved=false;
            if(!naval_recon_unit(battle,s,u,type,now,scout_reserved))return fail("naval-recon-command");excluded|=scout_reserved;
            if(*(int8_t*)((char*)type+0x30)==4){if(!naval_unit(battle,s,enemy,u,type,now))return false;excluded=true;}}
        if(hp>0&&count<move_limit&&!excluded){ids[count]=uid;unit_x[count]=*(int16_t*)((char*)u+0x48);unit_y[count]=*(int16_t*)((char*)u+0x4a);sum_x+=unit_x[count];sum_y+=unit_y[count];++count;}
        if(now%200==0)log("UNIT tick=%d side=%d id=%d cell=%d,%d life=%d attack=%d",now,id,uid,*(int16_t*)((char*)u+0x48),*(int16_t*)((char*)u+0x4a),hp,*(void**)((char*)u+0x248)!=nullptr);
    }
    void* barracks=ai_lab::building(s,base_id(s)+5);
    void* factory=(offline_world_runtime::gameplay_scope()||local_coop::playing||tech_test)?ai_lab::building(s,base_id(s)+7):nullptr;
    bool barracks_ready=barracks&&*((uint8_t*)barracks+0x47)==1;
    bool factory_ready=factory&&*((uint8_t*)factory+0x47)==1;
    void* airfield=(offline_world_runtime::gameplay_scope()||air_test)?ai_lab::building(s,base_id(s)+9):nullptr;
    bool air_ready=airfield&&*((uint8_t*)airfield+0x47)==1;
    bool may_recruit=(offline_world_runtime::gameplay_scope()||local_coop::playing||base_test||tech_test)?((barracks_ready||factory_ready||air_ready)&&!seat.reserve_rebuild&&!seat.reserve_tech):seat.developed;
    int factory_level=factory_ready?*((int8_t*)factory+0x60):0;
    int vehicle_budget=extended_scope?ground_recruit_budget(s,factory):0;
    bool first_air=air_ready&&!seat.air_id;int aircraft_budget=first_air?air_recruit_budget(s,airfield):0;
    int budget=first_air?aircraft_budget:vehicle_budget;
    int advanced_budget=advanced_recruit_budget(s);if(advanced_budget>budget)budget=advanced_budget;
    int higher_air_budget=advanced_air_budget(s);if(higher_air_budget>budget)budget=higher_air_budget;
    int sea_budget=naval_budget(s);if(sea_budget>budget)budget=sea_budget;
    if(!ai_tech_policy::recruit((first_air||advanced_budget>0||higher_air_budget>0||sea_budget>0)?false:seat.ground_tier_born>=factory_level,seat.guard.defending,count,budget,ai_lab::gold(s))){
        may_recruit=false;if(now%500==0)log("%s_BUDGET tick=%d side=%d target_price=%d gold=%d native_prices=1",first_air?"AIR":"GROUND",now,id,budget,ai_lab::gold(s));}
    if(offline_world_runtime::planner(id)){
        if(!economy_recruit(battle,s,now))return false;
    }else if(may_recruit&&(offline_world_runtime::gameplay_scope()||local_coop::playing||seat.purchases<(advanced_test?128:tank_test?64:32))) {
        void* b=barracks_ready?barracks:factory_ready?factory:airfield;void* queue=b?*(void**)((char*)b+0x98):nullptr;
        if(factory_ready&&queue&&*(int*)((char*)queue+0x18)>0){b=factory;queue=*(void**)((char*)b+0x98);}
        if(air_ready&&queue&&*(int*)((char*)queue+0x18)>0){b=airfield;queue=*(void**)((char*)b+0x98);}
        void* advanced_factory=advanced_enabled()?ai_lab::building(s,base_id(s)+8):nullptr;
        if(advanced_factory&&*((uint8_t*)advanced_factory+0x47)==1&&queue&&*(int*)((char*)queue+0x18)>0){b=advanced_factory;queue=*(void**)((char*)b+0x98);}
        if(queue&&*(int*)((char*)queue+0x18)==0) {
            int before=ai_lab::gold(s);
            void* hq=ai_lab::building(s,base_id(s));
            int profile=ai_tech_policy::profile(extended_scope,hq?*((int8_t*)hq+0x60):0,factory_level);
            if(!local_bot_runtime::recruit(battle,s,now,profile))return fail("native-recruit");
            seat.money.order(before,ai_lab::gold(s),now);
            if(before>ai_lab::gold(s)){++seat.purchases;log("HIRE tick=%d side=%d purchases=%d gold=%d->%d",now,id,seat.purchases,before,ai_lab::gold(s));}
        }
    }
    int observer=-1,visible_threats=0;auto threat=guard_threat(battle,s,enemy,observer,visible_threats);
    auto posture=ai_guard_policy::update(seat.guard,threat.id!=0,now);
    if(posture.changed)log("POSTURE tick=%d side=%d mode=%s threat=%d observer=%d visible_only=1",now,id,posture.defending?"defend":"attack",threat.id,observer);
    auto tuning=offline_world_runtime::tuning(id);
    int interval=posture.defending?tuning.defend:tuning.move;
    if(!threat.id&&offline_world_runtime::gameplay_scope()&&count>0&&count<tuning.assemble){
        if(seat.assemble_since<0)seat.assemble_since=now;
        if(now-seat.assemble_since<tuning.wait)return true;
    }else seat.assemble_since=-1;
    if(!posture.hold&&count>0&&(offline_world_runtime::gameplay_scope()||local_coop::playing||combat_sample||count>=3)&&(posture.changed||now-seat.last_move>=interval)) {
        void* target=base_test?ai_lab::building(enemy,base_id(enemy)+3):nullptr;
        if(!target)target=ai_lab::building(enemy,base_id(enemy));
        int tx=0,ty=0,target_id=0,target_kind=0;
        if(target) {tx=*(int16_t*)((char*)target+0x40);ty=*(int16_t*)((char*)target+0x42);target_id=*(int*)((char*)target+0x10);target_kind=1;}
        else {
            int best=INT_MAX,cx=sum_x/count,cy=sum_y/count;
            // Continue after the HQ: nearest live structure, then surviving army.
            // Native movement/auto-acquisition still owns pathing and damage.
            for(int offset:{0x148,0x140}) {
                void* array=*(void**)((char*)enemy+offset);
                for(size_t i=0;array&&i<array_length(array);++i) {
                    void* value=((void**)((char*)array+32))[i];if(!value)continue;
                    if(offset==0x148?(*((bool*)value+0x139)||*(int*)((char*)value+0x5c)<=0):(*(int*)((char*)value+0x184)<=0))continue;
                    int p=offset==0x148?0x40:0x48,x=*(int16_t*)((char*)value+p),y=*(int16_t*)((char*)value+p+2);
                    int distance=(x-cx)*(x-cx)+(y-cy)*(y-cy);
                    if(distance<best){best=distance;tx=x;ty=y;target_id=*(int*)((char*)value+0x10);target_kind=offset==0x148?1:2;}
                }
                if(target_kind)break;
            }
        }
        if(tuning.tactical&&!threat.id&&observer>=0){
            int cx=sum_x/count,cy=sum_y/count,nearest=INT_MAX,visible=0;void* seen=nullptr;
            for(int enemy_id=1;enemy_id<=seat_count();++enemy_id){void* other=side(battle,enemy_id);if(!other||alliance_of(battle,other)==observer)continue;
                void* army=*(void**)((char*)other+0x140);for(size_t k=0;army&&k<array_length(army);++k){void* unit=((void**)((char*)army+32))[k];
                    if(!unit||*(int*)((char*)unit+0x184)<=0)continue;void* vision=*(void**)((char*)unit+0x198),*type=*(void**)((char*)unit+0x40);
                    if(!vision||observer>=int(array_length(vision))||!*((uint8_t*)vision+32+observer)||!type||*(int8_t*)((char*)type+0x30)>=3)continue;
                    int dx=*(int16_t*)((char*)unit+0x48)-cx,dy=*(int16_t*)((char*)unit+0x4a)-cy,d=dx*dx+dy*dy;
                    if(d>144)continue;++visible;if(d<nearest){nearest=d;seen=unit;}
                }}
            if(visible>count+count/2&&now>=seat.next_regroup){seat.regroup_until=now+100;seat.next_regroup=now+600;
                log("TACTIC_REGROUP tick=%d side=%d own=%d visible_ground=%d bounded_until=%d",now,id,count,visible,seat.regroup_until);}
            if(now<seat.regroup_until){void* home=ai_lab::building(s,base_id(s));if(home){tx=*(int16_t*)((char*)home+0x40);ty=*(int16_t*)((char*)home+0x42);target_id=*(int*)((char*)home+0x10);target_kind=1;}}
            else if(seen){tx=*(int16_t*)((char*)seen+0x48);ty=*(int16_t*)((char*)seen+0x4a);target_id=*(int*)((char*)seen+0x10);target_kind=2;
                log("TACTIC_INTERCEPT tick=%d side=%d target=%d visible_ground=%d fog_unchanged=1",now,id,target_id,visible);}
        }
        if(goal){tx=goal->x;ty=goal->y;target_id=goal->id;target_kind=goal->kind;}
        if(!target_kind)return true;
        int defenders=threat.id?count:0;
        if(threat.id&&tuning.tactical&&count>=8){const auto* commander=offline_world_runtime::actor(id);
            defenders=commander&&commander->style==offline_world::Support?count*2/3:count/2;
            defenders=std::max(defenders,std::min(count,visible_threats*2));
            // Nearby defenders respond first; a serious visible incursion can
            // recall the whole army instead of blindly preserving a 50/50 split.
            std::pair<int,int> ordered[256];for(int i=0;i<count;++i){int dx=unit_x[i]-threat.x,dy=unit_y[i]-threat.y;ordered[i]={dx*dx+dy*dy,ids[i]};}
            std::sort(ordered,ordered+count);for(int i=0;i<count;++i)ids[i]=ordered[i].second;
            log("TACTIC_SPLIT tick=%d side=%d defend=%d attack=%d visible_threats=%d nearest_first=1",now,id,defenders,count-defenders,visible_threats);}
        for(int start=0;start<count;){
        bool defend=start<defenders;int limit=defend?defenders:count;
        int batch=std::min(32,limit-start),batch_x=defend?threat.x:tx,batch_y=defend?threat.y:ty;
        Roots roots;void* c=ai_lab::command("AICommUnitsMove",now,roots,id);
        void* a=array_new(find_class("System","Int32"),batch);if(!c||!roots.keep(a))return fail("move-command");
        bool tank_in_batch=false,air_in_batch=false;
        for(int i=0;i<batch;++i){((int*)((char*)a+32))[i]=ids[start+i];if(ids[start+i]==seat.tank_id)tank_in_batch=true;if(ids[start+i]==seat.air_id)air_in_batch=true;}
        *(int16_t*)((char*)c+0x2e)=batch_x;
        *(int16_t*)((char*)c+0x30)=batch_y;
        if(!put(c,0x40,a))return fail("move-ids");
        int code=ai_lab::perform(battle,c);if(code)return fail("move-rejected");
        advanced_order(s,ids+start,batch,now);
        advanced_air_order(s,ids+start,batch,now);
        if(tank_in_batch){seat.tank_ordered=true;log("TANK_ORDER tick=%d side=%d id=%d batch=%d native_command=1",now,id,seat.tank_id,start/32);}
        if(air_in_batch){seat.air_ordered=true;log("AIR_ORDER tick=%d side=%d id=%d batch=%d native_command=1",now,id,seat.air_id,start/32);}
        start+=batch;
        }
        seat.last_move=now;log("MOVE tick=%d side=%d count=%d target=%d,%d code=0 target_kind=%d target_id=%d posture=%s enemy_side=%d",now,id,count,tx,ty,target_kind,target_id,posture.defending?"defend":"attack",*(int*)((char*)enemy+0x10));
    }
    if(damaged&&seats[0].births>=3&&seats[1].births>=3&&count<seat.births) {
        if(!combat_sample)log("PASS two-seat-development-and-combat tick=%d side=%d births=%d remaining=%d damage=1 tutorial_commands=0 career_writes=0",now,id,seat.births,count);
        combat_sample=true;
    }
    return true;
}
static bool technology_fixture_result(void* battle,int now){
    if(!tech_test)return false;bool complete=true;
    if(naval_test)return naval_fixture_result(battle,now);
    if(advanced_air_test)return advanced_air_fixture_result(battle,now);
    if(advanced_test)return advanced_fixture_result(battle,now);
    if(air_test){
        void* s=side(battle,2),*field=s?ai_lab::building(s,base_id(s)+9):nullptr;Seat& seat=seats[1];
        bool ready=field&&*((uint8_t*)field+0x47)==1,unchanged=!memcmp(&tech_before,&progress,sizeof(progress));
        bool pass=ready&&seat.ground_tier_born>=2&&seat.air_id&&seat.air_ordered&&seat.air_moved&&unchanged;
        if(!pass&&now<17500)return false;
        FILE* f=fopen(DATA_ROOT "ai-air-production-fixture.jsonl","w");if(!f){fail("air-fixture-report");return true;}
        fprintf(f,"{\"summary\":true,\"pass\":%s,\"tick\":%d,\"side\":2,\"base\":%d,\"air_factory_ready\":%s,\"tank_id\":%d,\"air_id\":%d,\"native_air_order\":%s,\"native_air_moved\":%s,\"battle_gold\":%d,\"private_production_fixture\":true,\"native_fast_execute\":true,\"player_state_unchanged\":%s,\"player_match\":false,\"career_writes\":false}\n",pass?"true":"false",now,s?base_id(s):-1,ready?"true":"false",seat.tank_id,seat.air_id,seat.air_ordered?"true":"false",seat.air_moved?"true":"false",s?ai_lab::gold(s):0,unchanged?"true":"false");
        fclose(f);chmod(DATA_ROOT "ai-air-production-fixture.jsonl",0644);log("AIR_FIXTURE_RESULT tick=%d pass=%d career_writes=0",now,pass);stopped=true;return true;
    }
    if(tank_test){
        void* s=side(battle,2),*hq=s?ai_lab::building(s,base_id(s)):nullptr,*factory=s?ai_lab::building(s,base_id(s)+7):nullptr;
        Seat& seat=seats[1];int hq_tier=hq?*((int8_t*)hq+0x60):-1,factory_tier=factory?*((int8_t*)factory+0x60):-1;
        bool ready=factory&&*((uint8_t*)factory+0x47)==1;
        bool unchanged=!memcmp(&tech_before,&progress,sizeof(progress));
        bool pass=hq_tier>=2&&factory_tier==2&&ready&&seat.ground_tier_born>=2&&seat.tank_ordered&&seat.tank_moved&&unchanged;
        if(!pass&&now<17500)return false;
        FILE* f=fopen(DATA_ROOT "ai-tank-production-fixture.jsonl","w");if(!f){fail("tank-fixture-report");return true;}
        fprintf(f,"{\"summary\":true,\"pass\":%s,\"tick\":%d,\"side\":2,\"base\":%d,\"hq_tier\":%d,\"factory_tier\":%d,\"factory_ready\":%s,\"ground_tier_born\":%d,\"tank_id\":%d,\"native_tank_order\":%s,\"native_tank_moved\":%s,\"battle_gold\":%d,\"private_production_fixture\":true,\"native_fast_execute\":true,\"player_state_unchanged\":%s,\"player_match\":false,\"career_writes\":false}\n",pass?"true":"false",now,s?base_id(s):-1,hq_tier,factory_tier,ready?"true":"false",seat.ground_tier_born,seat.tank_id,seat.tank_ordered?"true":"false",seat.tank_moved?"true":"false",s?ai_lab::gold(s):0,unchanged?"true":"false");
        fclose(f);chmod(DATA_ROOT "ai-tank-production-fixture.jsonl",0644);log("TANK_FIXTURE_RESULT tick=%d pass=%d career_writes=0",now,pass);stopped=true;return true;
    }
    for(int i=1;i<=2;++i){void* s=side(battle,i),*hq=s?ai_lab::building(s,base_id(s)):nullptr;
        complete=complete&&hq&&*((int8_t*)hq+0x60)>=2&&*((uint8_t*)hq+0x47)==1&&seats[i-1].expanded_army;}
    if(!complete&&now<11000)return false;
    FILE* f=fopen(DATA_ROOT "ai-tech-production-fixture.jsonl","w");if(!f){fail("tech-fixture-report");return true;}
    chmod(DATA_ROOT "ai-tech-production-fixture.jsonl",0644);bool pass=complete;
    void* act=find_class("com.geargames.aow.entities.battle","SideAct");const void* predicate=act?class_method(act,"$Mf",4):nullptr;
    for(int i=1;i<=2;++i){void* s=side(battle,i),*hq=s?ai_lab::building(s,base_id(s)):nullptr,*type=hq?*(void**)((char*)hq+0x38):nullptr;
        int tier=hq?*((int8_t*)hq+0x60):-1,state=hq?*((uint8_t*)hq+0x47):-1;ai_tech_dependencies::Observation observed;
        bool counted=s&&observe_tech(s,observed);void* levels=type?*(void**)((char*)type+0x18):nullptr,*goal=levels&&*(int*)((char*)levels+0x18)>2?list_item(levels,2):nullptr;
        void* ex=nullptr,*boxed=nullptr;if(predicate&&goal){void* args[]={battle,s,goal,type};boxed=runtime_invoke(predicate,nullptr,args,&ex);}
        bool original=boxed&&!ex&&*(bool*)((char*)boxed+16),ok=tier>=2&&state==1&&counted&&observed.completed[10]>=2&&observed.completed[15]>=2&&original&&seats[i-1].expanded_army;
        pass=pass&&ok;fprintf(f,"{\"side\":%d,\"base\":%d,\"pass\":%s,\"hq_tier\":%d,\"hq_state\":%d,\"power_count\":%d,\"supply_count\":%d,\"original_requirements_pass\":%s,\"expanded_unit_born\":%s,\"battle_gold\":%d,\"exception\":\"%s\"}\n",i,s?base_id(s):-1,ok?"true":"false",tier,state,observed.completed[10],observed.completed[15],original?"true":"false",seats[i-1].expanded_army?"true":"false",s?ai_lab::gold(s):0,ex?class_name(object_class(ex)):"none");
    }
    bool unchanged=!memcmp(&tech_before,&progress,sizeof(progress));pass=pass&&unchanged;
    fprintf(f,"{\"summary\":true,\"pass\":%s,\"tick\":%d,\"private_production_fixture\":true,\"native_fast_execute\":true,\"player_state_unchanged\":%s,\"player_match\":false,\"career_writes\":false}\n",pass?"true":"false",now,unchanged?"true":"false");fclose(f);
    log("TECHNOLOGY_FIXTURE_RESULT tick=%d pass=%d career_writes=0",now,pass);stopped=true;return true;
}
static void step(void* battle) {
    if(!active||stopped)return;
    if(ai_lab::stopped){fail("native-command-exception");return;}
    int now=ai_lab::tick(battle);
    void* info=*(void**)((char*)battle+0x148);
    if(info&&!seeded)*((bool*)info+0x24)=true;
    if(now<30||now-last_tick<10)return;last_tick=now;
    if(!local_match::playing&&now>(offline_world_runtime::batch_fixture?offline_world_runtime::batch_job.max_tick:offline_world_runtime::strategy_fixture||local_battle_format::free_for_all(control_points::session.mode)?30000:(naval_test||advanced_air_test)?45000:advanced_test?30000:tank_test?18000:12000)){log("END bounded-limit tick=%d damage=%d",now,damaged);batch_report(battle,"tick_timeout",now);stopped=true;return;}
    if(!info){fail("gai-info");return;}
    if(!seeded){if(!seed(battle,now))return;seeded=true;}
    for(int id=1;id<=seat_count();++id){void* s=side(battle,id);if(s)economy_sample(s,now);}
    *((bool*)info+0x24)=false; // Empty script; retain native alliance/finish processing.
    if(naval_site_audit(battle,now))return;
    if(!control_points::observe(battle,now)){fail("control-point-observation-or-progress-ui");return;}
    if(now%200==0)log("SURVIVORS tick=%d buildings=%d,%d units=%d,%d is_pve=%d winner_index=%d reason=%d",now,
        team_alive(battle,1,0x148),team_alive(battle,2,0x148),team_alive(battle,1,0x140),team_alive(battle,2,0x140),
        *((bool*)battle+0xdc),*((int8_t*)battle+0x2a8),*(int*)((char*)battle+0xd8));
    if(seat_count()>2&&now%200==0)for(int id=1;id<=seat_count();++id)
        log("TEAM_ALIVE tick=%d side=%d alliance=%d buildings=%d units=%d",now,id,alliance_of(battle,side(battle,id)),alive(side(battle,id),0x148),alive(side(battle,id),0x140));
    if(terminal(battle,now))return;
    for(int id=(local_match::playing||tank_test)?2:1;id<=seat_count();++id) {
        void* s=side(battle,id);if(!s){fail("missing-seat");return;}
        auto tuning=offline_world_runtime::tuning(id);if(now-seats[id-1].last_decision<tuning.think)continue;
        seats[id-1].last_decision=now;
        if(!local_bot_runtime::ready(s)&&!local_bot_runtime::prepare(battle,s,now)){fail("workspace");return;}
        void* enemy=opponent(battle,s);if(!enemy)continue;
        economy_refresh(battle,s,enemy,now);
        Goal staging{};const Goal* goal=nullptr;
        if(tech_test){void* hq=ai_lab::building(s,base_id(s));if(hq){staging={*(int16_t*)((char*)hq+0x40)+(id==1?12:-12),*(int16_t*)((char*)hq+0x42),0,1};goal=&staging;}}
        if((offline_world_runtime::gameplay_scope()||base_test||!seats[id-1].developed)&&!develop(battle,s,now))return;
        if(!economy_infrastructure(battle,s,now)&&!advance(battle,s,now))return;
        // After basic ground production, an explored native coastal site gets
        // its first ship before the later air/advanced producer goals. Once a
        // ship is actually born, the regular tech order resumes.
        bool first_naval=naval_first_pending(s);
        if((first_naval&&!advance_naval(battle,s,now))||!advance_air(battle,s,now)||!advance_advanced(battle,s,now)||!advance_air_tiers(battle,s,now)||(!first_naval&&!advance_naval(battle,s,now))||!army(battle,s,enemy,now,goal)||stopped)return;
    }
    technology_fixture_result(battle,now);
}
}
