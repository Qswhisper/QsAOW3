// Opt-in local strategy experiment. Uses the original command validators,
// economy, construction and training on the original logic thread.
// This is a technical sample on chapter 02's existing map, never a new chapter.
#pragma once
#include <stdarg.h>
namespace ai_lab {
static bool active, frozen, stopped;
static unsigned sequence, attempts, purchases;
static int last_tick = -100, initial_units = -1;
static int initial_ids[64], initial_id_count, goal_x, goal_y, script_pos;
static bool moving;
static bool naval_sample(int chapter){
    const auto* route=local_match_maps::find(chapter);
    return route&&route->faction==2&&!access(DATA_ROOT "ai-naval-lab.enabled",F_OK)&&!access(DATA_ROOT "ai-tech-lab.enabled",F_OK)&&!access(DATA_ROOT "ai-lab.enabled",F_OK);
}
static int gold(void* side) { return *(int*)((char*)side + 0x16c); }
static void log(const char* format, ...) {
    FILE* f = fopen(DATA_ROOT "art3offline-ai-lab.txt", "a");
    if (!f) return;
    va_list args; va_start(args, format); vfprintf(f, format, args); va_end(args);
    fputc('\n', f); fclose(f); chmod(DATA_ROOT "art3offline-ai-lab.txt", 0644);
}
static bool begin(int chapter,bool player_match=false) {
    active = player_match || offline_world_runtime::strategy_fixture || ((chapter == 201 || naval_sample(chapter)) && !access(DATA_ROOT "ai-lab.enabled", F_OK));
    local_bot_runtime::reset(active,player_match);
    local_duel::active=active&&local_bot_runtime::enabled&&(player_match||!access(DATA_ROOT "ai-duel-lab.enabled",F_OK));
    frozen = stopped = false; sequence = attempts = purchases = 0; last_tick = -100; initial_units = -1;
    initial_id_count = 0; moving = false;
    if (active) {
        FILE* f = fopen(DATA_ROOT "art3offline-ai-lab.txt", "w");
        if (f) { fprintf(f,"schema=ai-lab-v2 chapter=%d player_match=%d progression_writes=0\n",chapter,player_match); fclose(f); }
    }
    return active;
}
static void* own(void* battle) {
    return local_dictionary_item(*(void**)((char*)battle + 0x178), 1);
}
static int tick(void* battle) {
    auto klass = object_class(battle);
    auto get = *(int(**)(void*,void*))((char*)klass + 0x1018);
    return get(battle, *(void**)((char*)klass + 0x1020));
}
static int entities(void* side, int offset) {
    void* a = *(void**)((char*)side + offset); int count = 0;
    for (size_t i = 0; a && i < array_length(a); ++i)
        if (((void**)((char*)a + 32))[i]) ++count;
    return count;
}
static void* building(void* side, int type_id) {
    void* a = *(void**)((char*)side + 0x148);
    for (size_t i = 0; a && i < array_length(a); ++i) {
        void* b = ((void**)((char*)a + 32))[i];
        void* type = b ? *(void**)((char*)b + 0x38) : nullptr;
        if (type && *(int*)((char*)type + 0x10) == type_id && !*((uint8_t*)b + 0x139)) return b;
    }
    return nullptr;
}
static void (*script_original)(void*,void*);
static void script(void* battle, void* method) {
    if(hero_live_lab::active){hero_live_lab::script(battle,method,script_original);return;}
    if (!active) { script_original(battle, method); return; }
    if (local_duel::active) {
        // The duel has no tutorial commands/bases. Preserve the original
        // post-script/army checks once both native starting bases exist.
        if(local_duel::allow_script())script_original(battle,method);
        return;
    }
    if (frozen) return;
    // Reuse only the real scenario's opening HQ, budget and starting army.
    // Once that seed exists, no tutorial commands or automatic actions run.
    script_original(battle, method);
    void* side = own(battle);
    if (side && building(side, 1100)) {
        void* info=*(void**)((char*)battle+0x148); bool pause=true; void* exception=nullptr; void* args[]={&pause};
        invoke_named(info,"set_Frozen",1,args,&exception);
        if(!info||exception||!*((uint8_t*)info+0x24)) {log("FAIL phase=script-freeze");stopped=true;return;}
        script_pos=*(int*)((char*)info+0x18);
        frozen = true;
        log("SEED tick=%d gold=%d buildings=%d units=%d script_frozen=1 script_pos=%d workspace=%d geometry=%d", tick(battle),
            gold(side), entities(side, 0x148), entities(side,0x140),script_pos,
            *(void**)((char*)battle+0x138)!=nullptr,*(void**)((char*)side+0xe8)!=nullptr);
    }
}
static void* command(const char* name, int now, local_research_runtime::Roots& roots, int id=1) {
    void* c = object_new(find_class("com.geargames.aow.entities.battle.aicomm", name));
    if (!roots.keep(c)) return nullptr;
    void* exception = nullptr; invoke_named(c, ".ctor", 0, nullptr, &exception);
    void* args[] = {&now, &id};
    auto base = find_class("com.geargames.aow.entities.battle.aicomm", "AIComm");
    runtime_invoke(class_method(base, ".ctor", 2), c, args, &exception);
    if (exception) { log("FAIL phase=command-constructor"); stopped = true; return nullptr; }
    return c;
}
static int perform(void* battle, void* c) {
    if (!c) return -1;
    int seat=*(int*)((char*)c+0x28);void* payer=local_dictionary_item(*(void**)((char*)battle+0x178),seat);int before=payer?gold(payer):0;
    void* exception = nullptr; void* args[] = {battle, c};
    void* result = runtime_invoke(class_method(find_class("com.geargames.aow.entities.battle", "BattleAct"), "$f", 2),
        nullptr, args, &exception);
    if (exception || !result) { log("FAIL phase=command-execution exception=%s", exception ? class_name(object_class(exception)) : "null"); stopped = true; return -1; }
    int code=*(int16_t*)((char*)result + 16);
    if(local_duel::active&&payer)local_duel::record_order(battle,c,before,gold(payer),code);
    return code;
}
static void step(void* battle) {
    if (!active || !frozen || stopped) return;
    int now = tick(battle);
    if (now < 30 || now - last_tick < 10) return;
    last_tick = now;
    void* side = own(battle); if (!side) { stopped = true; return; }
    void* info=*(void**)((char*)battle+0x148);
    if(!info||!*((uint8_t*)info+0x24)||*(int*)((char*)info+0x18)!=script_pos) {log("FAIL phase=script-advanced tick=%d",now);stopped=true;return;}
    if (initial_units < 0) {
        initial_units = entities(side,0x140); log("BASELINE tick=%d units=%d gold=%d",now,initial_units,gold(side));
        void* a=*(void**)((char*)side+0x140);
        for(size_t i=0;a&&i<array_length(a);++i) {
            void* u=((void**)((char*)a+32))[i];
            if(u) { if(initial_id_count>=64) {log("FAIL initial-army-too-large");stopped=true;return;} initial_ids[initial_id_count++]=*(int*)((char*)u+0x10); }
        }
    }
    if (now > 6000 || attempts >= 200) { log("END reason=bounded-limit tick=%d purchases=%u", now, purchases); stopped = true; return; }
    if(local_bot_runtime::enabled&&!local_bot_runtime::ready(side)&&!local_bot_runtime::prepare(battle,side,now)) {
        log("FAIL phase=native-workspace-initialization");stopped=true;return;
    }
    void* hq = building(side, 1100); if (!hq) { log("END reason=HQ-lost tick=%d", now); stopped = true; return; }
    const int plan[] = {1103, 1101, 1102, 1105};
    int type_id = 0;
    for (int id : plan) {
        void* b = building(side, id);
        if (!b) { type_id = id; break; }
        if (*((uint8_t*)b + 0x47) != 1) return; // Observe completion before advancing.
    }
    local_research_runtime::Roots roots;
    if (type_id) {
        void* type = local_dictionary_item(*(void**)((char*)side + 0xf8), type_id);
        if (!type) { log("FAIL missing-building-type=%d", type_id); stopped = true; return; }
        int hx = *(int16_t*)((char*)hq + 0x40), hy = *(int16_t*)((char*)hq + 0x42);
        auto valid = reinterpret_cast<bool(*)(void*,void*,int,int,int16_t,void*)>(image_base + 0x4922a7c);
        void* c = command("AICommBuSet", now, roots); if (!c) return;
        *(int*)((char*)c + 0x30) = type_id;
        *(int*)((char*)c + 0x3c) = ++sequence;
        void* ctx = static_object("com.geargames.aow.scenes.battle", "ClientBattleContext", "m_logicCommandContext");
        void* accumulation = ctx ? *(void**)((char*)ctx + 0x40) : nullptr;
        if (accumulation) {
            void* exception = nullptr; void* args[] = {c};
            invoke_named(accumulation,"ApplyBuildingAccumulationSum",1,args,&exception);
            if (exception) { log("FAIL phase=accumulation"); stopped=true; return; }
        }
        unsigned tested = 0;
        // Try bounded nearby free footprints; the original command also checks
        // tech, cost, energy, connection and all other placement constraints.
        for (int radius = 4; radius <= 24; radius += 2) for (int dy = -radius; dy <= radius; dy += 2)
            for (int dx = -radius; dx <= radius; dx += 2) {
                if (abs(dx) != radius && abs(dy) != radius) continue;
                int x = hx + dx, y = hy + dy;
                if (x < 0 || y < 0 || !valid(battle, type, x, y, 0, nullptr)) continue;
                // Verified setter bodies: SKA=x(+40), TKA=y(+42), MKA=angle(+44).
                *(int16_t*)((char*)c + 0x40) = x; *(int16_t*)((char*)c + 0x42) = y;
                *(int16_t*)((char*)c + 0x44) = 0;
                int before = gold(side); int code = perform(battle, c);
                if (!code) {
                    ++attempts; log("BUILD tick=%d type=%d cell=%d,%d code=%d gold=%d->%d buildings=%d", now,
                        type_id, x, y, code, before, gold(side), entities(side, 0x148)); return;
                }
                if (stopped) return;
                if (attempts == 0 && tested == 0) {
                    log("REJECT tick=%d code=%d hq=%d,%d candidate=%d,%d accumulation=%d,%d",now,code,hx,hy,x,y,*(int*)((char*)c+0x34),*(int*)((char*)c+0x38));
                    FILE* f=fopen(DATA_ROOT "art3offline-ai-lab-reject.txt","w");
                    if(f) {visited_count=0;dump_object(f,side,0,1);visited_count=0;dump_object(f,c,0,1);fclose(f);chmod(DATA_ROOT "art3offline-ai-lab-reject.txt",0644);}
                }
                if (++tested >= 32) { ++attempts; if (attempts < 5 || attempts % 10 == 0) log("WAIT tick=%d type=%d code=%d gold=%d", now,type_id,code,before); return; }
            }
        ++attempts; if (attempts < 5 || attempts % 10 == 0) log("WAIT tick=%d type=%d reason=no-free-footprint hq=%d,%d", now,type_id,hx,hy);
        return;
    }
    void* barracks = building(side, 1105);
    if (purchases < 3) {
        void* queue = *(void**)((char*)barracks + 0x98);
        if (queue && *(int*)((char*)queue + 0x18)) return;
        if(local_bot_runtime::enabled) {
            int before=gold(side); ++attempts;
            if(!local_bot_runtime::recruit(battle,side,now)) {stopped=true;return;}
            int spent=before-gold(side);
            if(spent>0) ++purchases;
            if(spent>0||attempts%10==0) log("NATIVE_HIRE tick=%d purchased=%u spent=%d gold=%d units=%d",now,purchases,spent,gold(side),entities(side,0x140));
            return;
        }
        void* c = command("AICommBuyUnit", now, roots); if (!c) return;
        // Original SendBuildingBuyUnit: ElA=building(+34), FlA=type(+30).
        *(int*)((char*)c + 0x34) = *(int*)((char*)barracks + 0x10);
        *(int*)((char*)c + 0x30) = 1100;
        int before = gold(side); int code = perform(battle, c); ++attempts;
        if (!code) ++purchases;
        log("HIRE tick=%d code=%d purchased=%u gold=%d->%d units=%d",now,code,purchases,before,gold(side),entities(side,0x140));
    } else {
        void* units=*(void**)((char*)side+0x140); void* trained[3]={}; int count=0;
        for(size_t i=0;units&&i<array_length(units)&&count<3;++i) {
            void* u=((void**)((char*)units+32))[i]; if(!u) continue;
            int id=*(int*)((char*)u+0x10); bool old=false;
            for(int j=0;j<initial_id_count;++j) if(initial_ids[j]==id) old=true;
            if(!old) trained[count++]=u;
        }
        if(count!=3) return;
        if(!moving) {
            int hx=*(int16_t*)((char*)hq+0x40),hy=*(int16_t*)((char*)hq+0x42);
            int ux=*(int16_t*)((char*)trained[0]+0x48),uy=*(int16_t*)((char*)trained[0]+0x4a);
            auto valid=reinterpret_cast<bool(*)(void*,void*,int,int,int16_t,void*)>(image_base+0x4922a7c);
            void* type=local_dictionary_item(*(void**)((char*)side+0xf8),1105); bool found=false;
            for(int r=8;r<=24&&!found;r+=2) for(int dy=-r;dy<=r&&!found;dy+=2) for(int dx=-r;dx<=r;dx+=2) {
                int x=hx+dx,y=hy+dy;
                if(x>=0&&y>=0&&(abs(x-ux)>6||abs(y-uy)>6)&&valid(battle,type,x,y,0,nullptr)) {goal_x=x;goal_y=y;found=true;break;}
            }
            if(!found) {log("FAIL phase=goal-placement");stopped=true;return;}
            void* c=command("AICommUnitsMove",now,roots); if(!c) return;
            void* ids=array_new(find_class("System","Int32"),3); if(!roots.keep(ids)) {stopped=true;return;}
            for(int i=0;i<3;++i) {
                ((int*)((char*)ids+32))[i]=*(int*)((char*)trained[i]+0x10);
                log("UNIT tick=%d id=%d from=%d,%d goal=%d,%d",now,((int*)((char*)ids+32))[i],
                    *(int16_t*)((char*)trained[i]+0x48),*(int16_t*)((char*)trained[i]+0x4a),goal_x,goal_y);
            }
            *(int*)((char*)c+0x34)=0;
            *(int16_t*)((char*)c+0x2e)=goal_x; *(int16_t*)((char*)c+0x30)=goal_y;
            field_set(c,class_field_from_name(object_class(c),"$ir"),ids);
            int code=perform(battle,c);log("MOVE tick=%d code=%d units=3 goal=%d,%d",now,code,goal_x,goal_y);
            if(code) {stopped=true;return;} moving=true;
        } else {
            int arrived=0;
            for(int i=0;i<3;++i) {
                int x=*(int16_t*)((char*)trained[i]+0x48),y=*(int16_t*)((char*)trained[i]+0x4a);
                if(abs(x-goal_x)<=2&&abs(y-goal_y)<=2) ++arrived;
                if(now%100==0) log("POSITION tick=%d id=%d cell=%d,%d goal=%d,%d",now,*(int*)((char*)trained[i]+0x10),x,y,goal_x,goal_y);
            }
            if(arrived==3) {
                log("PASS development-and-goal tick=%d buildings=%d units=%d initial_units=%d purchases=%u arrived=%d script_frozen=1",now,
                    entities(side,0x148),entities(side,0x140),initial_units,purchases,arrived);stopped=true;
            }
        }
    }
}
static void (*tick_original)(void*,void*);
static void update(void* battle, void* method) {
    if(hero_live_lab::active){}
    else if(local_coop::playing)local_coop::step(battle);
    else if(local_duel::active) local_duel::step(battle); else step(battle);
    tick_original(battle, method);
    local_hero_battle::observe(battle);
    hero_live_lab::step(battle);
}
static void (*automatic_original)(void*,void*,void*);
static void automatic(void* self,void* battle,void* method) {
    if(active||hero_live_lab::active) {return;}
    automatic_original(self,battle,method);
}
static void install() {
    // Hooks remain inert outside an explicitly selected local mode.
    // The player entry must work without diagnostic files.
    if (interaction_trace::enabled) {LOG("AI lab requires interaction trace disabled");return;}
    const uint32_t script_signature[] = {0xd101c3ff,0xa9045ffe,0xa90557f6,0xa9064ff4};
    const uint32_t tick_signature[] = {0xd103c3ff,0xa9097bfd,0xa90a6ffc,0xa90b67fa};
    script_original = (decltype(script_original))install_hook(0x4756ab0, script_signature, (void*)script);
    tick_original = (decltype(tick_original))install_hook(0x4688328, tick_signature, (void*)update);
    const uint32_t automatic_signature[]={0xd101c3ff,0xa90367fe,0xa9045ff8,0xa90557f6};
    automatic_original=(decltype(automatic_original))install_hook(0x460a52c,automatic_signature,(void*)automatic);
    LOG("AI lab hooks script=%d tick=%d",script_original!=nullptr,tick_original!=nullptr);
}
}
