// Included inside local_duel, after the original recipe and visibility helpers.
#pragma once
static int economy_style(int id){const auto* a=offline_world_runtime::actor(id);return a?a->style:offline_world::Balanced;}
static int unit_count(void* s,void* type){
    int n=0;void* units=*(void**)((char*)s+0x140);
    for(size_t i=0;units&&i<array_length(units);++i){void* u=((void**)((char*)units+32))[i];if(u&&*(int*)((char*)u+0x184)>0&&*(void**)((char*)u+0x40)==type)++n;}return n;
}
static int army_cp(void* s){
    int n=0;void* units=*(void**)((char*)s+0x140);
    for(size_t i=0;units&&i<array_length(units);++i){void* u=((void**)((char*)units+32))[i],*t=u?*(void**)((char*)u+0x40):nullptr;if(t&&*(int*)((char*)u+0x184)>0)n+=std::max(0,int(*(int8_t*)((char*)t+0x44)));}return n;
}
struct PowerObservation {ai_economy::Power ready,planned;};
static PowerObservation observed_power(void* s){
    PowerObservation p;void* bs=*(void**)((char*)s+0x148);
    for(size_t i=0;bs&&i<array_length(bs);++i){void* b=((void**)((char*)bs+32))[i];if(!b||*(bool*)((char*)b+0x139))continue;
        void* t=*(void**)((char*)b+0x38),*ls=t?*(void**)((char*)t+0x18):nullptr;int l=*(int8_t*)((char*)b+0x60);void* row=ls?list_item(ls,l):nullptr;
        int e=row?*(int16_t*)((char*)row+0x60):0;p.planned.add(e);if(*(uint8_t*)((char*)b+0x47)==1)p.ready.add(e);
    }return p;
}
static void economy_sample(void* s,int now,bool force=false){
    int id=*(int*)((char*)s+0x10);auto& a=seats[id-1];a.money.observe(ai_lab::gold(s),now);
    if(!force&&now-a.last_economy_log<(offline_world_runtime::batch_fixture?100:500))return;a.last_economy_log=now;
    void* hq=ai_lab::building(s,base_id(s));auto power=observed_power(s);
    FILE* f=fopen(DATA_ROOT "ai-economy.jsonl","a");if(!f)return;
    fprintf(f,"{\"tick\":%d,\"side\":%d,\"controller\":%d,\"style\":%d,\"cash\":%d,\"observed_income\":%lld,\"other_outflow\":%lld,\"order_spend\":%lld,\"refund\":%lld,\"reconciled\":%s,\"reserved\":%d,\"spendable\":%d,\"goal\":%d,\"action\":%d,\"kind\":%d,\"budget\":%d,\"until\":%d,\"plan_started\":%u,\"plan_paid\":%u,\"plan_canceled\":%u,\"orders\":%d,\"rejected\":%d,\"troops\":%d,\"hq\":%d,\"cp_used\":%d,\"cp_capacity\":%d,\"idle_max\":%d,\"blocked_cash\":%d,\"blocked_cp\":%d,\"blocked_native\":%d,\"ready_samples\":%d,\"busy_samples\":%d,\"power_supply\":%d,\"power_load\":%d,\"planned_power_supply\":%d,\"planned_power_load\":%d}\n",
        now,id,offline_world_runtime::planner(id)?1:0,economy_style(id),ai_lab::gold(s),(long long)a.money.income,(long long)a.money.outflow,(long long)a.money.spent,(long long)a.money.refund,a.money.reconciled()?"true":"false",a.plan.reserved(ai_lab::gold(s)),a.plan.spendable(ai_lab::gold(s)),a.plan.goal,a.plan.action,a.plan.kind,a.plan.price,a.plan.until,a.plan.started,a.plan.completed,a.plan.canceled,a.orders,a.rejected,alive(s,0x140),hq?int(*(int8_t*)((char*)hq+0x60)):-1,army_cp(s),*(int*)((char*)s+0x34c),a.max_idle,a.block_cash,a.block_cp,a.block_native,a.ready_samples,a.busy_samples,power.ready.supplied,power.ready.used,power.planned.supplied,power.planned.used);
    fclose(f);
}
static void economy_refresh(void* battle,void* s,void* enemy,int now){
    int id=*(int*)((char*)s+0x10);auto& a=seats[id-1];a.money.observe(ai_lab::gold(s),now);
    if(!offline_world_runtime::planner(id))return;
    int observer=-1,visible=0;auto threat=guard_threat(battle,s,enemy,observer,visible);
    a.economy_threat=threat.id!=0;
    if(threat.id&&a.plan.active()&&(!a.reserve_rebuild&&a.developed||a.plan.kind!=ai_economy::Build)){
        log("PLAN_RELEASE tick=%d side=%d goal=%d reason=visible-threat",now,id,a.plan.goal);a.plan.release(ai_economy::Threat,now);
    }
    if(a.plan.active()&&now>=a.plan.until){log("PLAN_RELEASE tick=%d side=%d goal=%d reason=deadline",now,id,a.plan.goal);a.plan.release(ai_economy::Expired,now);}
    // Keep the original guard state transition in army(); the economic
    // decision uses this separate current observation.
}
static bool economy_infrastructure(void* battle,void* s,int now){
    int id=*(int*)((char*)s+0x10);auto& a=seats[id-1];if(!offline_world_runtime::planner(id)||!a.developed||a.reserve_rebuild||a.guard.defending||a.economy_threat)return false;
    void* hq=ai_lab::building(s,base_id(s));if(!hq||*(uint8_t*)((char*)hq+0x47)!=1)return false;
    int level=*(int8_t*)((char*)hq+0x60),style=economy_style(id);
    // A real population block asks the same original paid HQ recipe to expand.
    int cp=army_cp(s),capacity=*(int*)((char*)s+0x34c);
    if(capacity>0&&cp+2>=capacity){void* t=*(void**)((char*)hq+0x38),*ls=t?*(void**)((char*)t+0x18):nullptr;
        if(ls&&level+1<*(int*)((char*)ls+0x18)){satisfy_tech_requirements(battle,s,list_item(ls,level+1),now,true);return true;}}
    if(level<1||alive(s,0x140)<3)return false;
    int goal_kind=0,quantity=0;ai_tech_dependencies::Observation observed;
    if(!observe_tech(s,observed))return false;
    int supply=observed.completed[15]+observed.pending[15];
    int desired=style==offline_world::Technology?std::min(4,1+now/1800):style==offline_world::Support?std::min(4,1+now/2200):level>=2?std::min(4,1+now/3500):1;
    if(supply<desired){goal_kind=2;quantity=supply+1;}
    // Observe the native energy recipe values, including facilities being
    // constructed. Do not disable or override native brownout behavior.
    auto power=observed_power(s).planned;
    if(power.needs()&&!observed.pending[10]){goal_kind=1;quantity=observed.completed[10]+1;}
    if(!goal_kind)return false;
    void* type=local_dictionary_item(*(void**)((char*)s+0xf8),base_id(s)+goal_kind),*ls=type?*(void**)((char*)type+0x18):nullptr,*goal=ls?list_item(ls,0):nullptr;
    if(!goal)return false;satisfy_tech_requirements(battle,s,goal,now,true,quantity);return a.reserve_tech;
}
static bool visible_air(void* battle,void* s){
    int observer=alliance_of(battle,s);if(observer<0)return false;
    for(int id=1;id<=seat_count();++id){void* other=side(battle,id);if(!other||alliance_of(battle,other)==observer)continue;void* us=*(void**)((char*)other+0x140);
        for(size_t j=0;us&&j<array_length(us);++j){void* u=((void**)((char*)us+32))[j],*t=u?*(void**)((char*)u+0x40):nullptr,*v=u?*(void**)((char*)u+0x198):nullptr;
            if(t&&v&&*(int*)((char*)u+0x184)>0&&*(int8_t*)((char*)t+0x30)==3&&observer<int(array_length(v))&&*((uint8_t*)v+32+observer))return true;}}
    return false;
}
static bool economy_recruit(void* battle,void* s,int now){
    int id=*(int*)((char*)s+0x10),style=economy_style(id);auto& a=seats[id-1];int free_cp=*(int*)((char*)s+0x34c)-army_cp(s);
    bool air=visible_air(battle,s),affordable=false,spent=false;int idle=0;void* buildings=*(void**)((char*)s+0x148);
    for(size_t i=0;buildings&&i<array_length(buildings);++i){
        void* b=((void**)((char*)buildings+32))[i];if(!b||*(bool*)((char*)b+0x139)||*(uint8_t*)((char*)b+0x47)!=1)continue;
        void* bt=*(void**)((char*)b+0x38),*ls=bt?*(void**)((char*)bt+0x18):nullptr;int tier=*(int8_t*)((char*)b+0x60);void* row=ls?list_item(ls,tier):nullptr,*units=row?*(void**)((char*)row+0x20):nullptr;
        int n=units?*(int*)((char*)units+0x18):0;if(n<=0||n>32)continue;++a.ready_samples;
        // Keep at most one waiting order behind an active native train. The
        // engine removes that train from Queue while CurrTrain stays nonnull.
        void* queue=*(void**)((char*)b+0x98);if(!queue||*(int*)((char*)queue+0x18)>0){++a.busy_samples;continue;}++idle;
        void* best=nullptr;int best_score=-1,best_price=0;
        for(int k=0;k<n;++k){void* t=list_item(units,k);if(!t)continue;int price=*(int*)((char*)t+0x48),cp=*(int8_t*)((char*)t+0x44),uid=*(int*)((char*)t+0x10),category=*(int8_t*)((char*)t+0x30),own=unit_count(s,t);
            if(price<=0||cp<=0||category<1||category>6)continue;
            if(cp>free_cp){++a.block_cp;continue;}
            // Original positive hire weights determine useful native choices;
            // local style changes their relative priority, never their stats.
            void* probs=*(void**)((char*)t+0x88);int weight=0;for(size_t z=0;probs&&z<array_length(probs);++z)weight=std::max(weight,((int*)((char*)probs+32))[z]);
            if(weight<=0)continue;int role=category==1?50:category==2?100:category==3?95:category==4?90:110;
            if(style==offline_world::Pressure){if(category==1)role=110;else if(category==3)role=60;}
            if(style==offline_world::Technology){if(category==1)role=25;else if(category==3||category==6)role=150;}
            if(style==offline_world::Support&&air&&uid==base_id(s)+12)role=260;
            if(own>=3&&(*(int*)((char*)t+0x58)&1))continue;
            int score=role*100/(2+own)+std::min(100,weight);
            uint32_t seed=offline_world_runtime::batch_fixture?offline_world_runtime::batch_job.policy_seed:offline_world::mix(local_match::run);
            score+=offline_world::mix(uint64_t(seed)+id*101+uid*17+now/200)%17;
            if(a.plan.kind==ai_economy::Unit&&a.plan.action==uid)score+=10000;
            if(score>best_score){best=t;best_score=score;best_price=price;}
        }
        if(!best)continue;int uid=*(int*)((char*)best+0x10),cash=ai_lab::gold(s),available=a.plan.spendable(cash);
        if(a.plan.kind==ai_economy::Unit&&a.plan.action==uid)available=cash;
        if((a.guard.defending||a.economy_threat)&&a.plan.active()&&!a.reserve_rebuild){a.plan.release(ai_economy::Threat,now);available=cash;}
        if(best_price>available){++a.block_cash;
            if(!a.plan.active()&&!a.guard.defending&&alive(s,0x140)>=3)budget(s,uid,uid,ai_economy::Unit,best_price,now);
            continue;
        }
        affordable=true;Roots roots;void* command=ai_lab::command("AICommBuyUnit",now,roots,id);if(!command)return fail("economy-buy-command");
        *(int*)((char*)command+0x30)=uid;*(int*)((char*)command+0x34)=*(int*)((char*)b+0x10);
        int before=cash,code=ai_lab::perform(battle,command);if(ai_lab::stopped)return fail("economy-buy-native-exception");
        if(!code){++a.purchases;spent|=ai_lab::gold(s)<before;free_cp-=*(int8_t*)((char*)best+0x44);log("HIRE_PLANNED tick=%d side=%d type=%d producer=%d style=%d gold=%d->%d",now,id,uid,*(int*)((char*)b+0x10),style,before,ai_lab::gold(s));}
        else {++a.block_native;log("PRODUCTION_BLOCK tick=%d side=%d type=%d producer=%d native_code=%d",now,id,uid,*(int*)((char*)b+0x10),code);}
    }
    if(idle&&affordable&&!spent){if(a.idle_since<0)a.idle_since=now;a.max_idle=std::max(a.max_idle,now-a.idle_since);}else a.idle_since=-1;
    economy_sample(s,now);return true;
}
static void batch_report(void* battle,const char* status,int tick,int winner,int reason){
    if(!offline_world_runtime::batch_fixture)return;const auto& j=offline_world_runtime::batch_job;const auto* route=local_match_maps::find(j.scenario);
    FILE* f=fopen(DATA_ROOT "ai-batch-result.tmp","w");if(!f){fail("batch-result-open");return;}
    fprintf(f,"{\"schema\":1,\"job_id\":\"%s\",\"strategy_version\":\"economy-v2-final\",\"status\":\"%s\",\"tick\":%d,\"native_reason\":%d,\"winner_alliance\":%d,\"scenario\":%d,\"map\":%d,\"mode\":%d,\"growth\":%d,\"policy_seed\":%u,\"engine_seed_controlled\":false,\"career_writes\":false,\"seats\":[",j.id,status,tick,reason,winner,j.scenario,route?route->map:0,j.mode,j.growth,j.policy_seed);
    for(int i=1;i<=seat_count();++i){void* s=side(battle,i);auto& a=seats[i-1];if(s)economy_sample(s,tick,true);
        fprintf(f,"%s{\"side\":%d,\"controller\":%d,\"tier\":%d,\"style\":%d,\"faction\":%d,\"buildings\":%d,\"units\":%d,\"spend\":%lld,\"income\":%lld,\"orders\":%d,\"rejected\":%d,\"plan_paid\":%u,\"plan_canceled\":%u,\"max_affordable_idle\":%d,\"cash_reconciled\":%s}",i>1?",":"",i,j.seats[i-1].controller,j.seats[i-1].tier,j.seats[i-1].style,(base_id(s)-1000)/100+1,s?alive(s,0x148):0,s?alive(s,0x140):0,(long long)a.money.spent,(long long)a.money.income,a.orders,a.rejected,a.plan.completed,a.plan.canceled,a.max_idle,a.money.reconciled()?"true":"false");}
    fprintf(f,"]}\n");bool ok=fflush(f)==0&&fsync(fileno(f))==0;if(fclose(f))ok=false;
    if(ok)ok=rename(DATA_ROOT "ai-batch-result.tmp",DATA_ROOT "ai-batch-result.json")==0;
    if(ok)chmod(DATA_ROOT "ai-batch-result.json",0644);else fail("batch-result-commit");
}
