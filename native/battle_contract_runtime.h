// Observe the original shared native achievement event call. The original
// engine owns kill attribution, building erasure, chest pickup and event IDs.
#pragma once
#include <pthread.h>
namespace battle_contract_runtime {
static void (*original)(void*,void*,int,int,void*);
static void (*observer)(void*,void*,int,int); // Explicit private audit only.
static pthread_mutex_t mutex=PTHREAD_MUTEX_INITIALIZER;
struct Capture {uint64_t run=0;uint32_t mask=0;int faction=0;void* battle=nullptr;bool failed=false;battle_contract_events::Counts counts;timed_contract_events::Counts timed;uint32_t boost_used=0;};
static Capture capture;
static bool trace;static int trace_count;
static bool available(){return original!=nullptr;}
static void boost_applied(void*,void*,int);
static bool boost_available(){return local_inventory::installed&&local_inventory::applied_observer==boost_applied;}
static int (*logic_tick)(void*,void*);
static bool timed_available(){return available()&&logic_tick;}
static bool mask_for(const local_progress::State& state,uint32_t& mask){
    mask=0;
    for(int i=0;i<battle_contract_events::count;++i){
        const auto& rule=contract_content::battle_rules[i];int index=local_contracts::index(rule.id);
        if(index<0)return false;const auto& definition=contract_content::definitions[index];
        if(local_contracts::eligible(definition,state)){
            int n=battle_contract_policy::count(state,rule.id);if(n<0)return false;
            if(n<rule.value)mask|=1u<<i;
        }
    }
    for(int i=0;i<battle_contract_events::result_count;++i){
        const auto& rule=contract_content::result_rules[i];int index=local_contracts::index(rule.id);
        if(index<0)return false;const auto& definition=contract_content::definitions[index];
        if(local_contracts::eligible(definition,state)){
            int n=battle_contract_policy::count(state,rule.id);if(n<0)return false;
            if(n<rule.value)mask|=1u<<(battle_contract_events::count+i);
        }
    }
    const auto& timed=contract_content::timed_rules[0];int timed_index=local_contracts::index(timed.id);
    if(timed_index<0)return false;
    if(local_contracts::eligible(contract_content::definitions[timed_index],state)){
        int n=battle_contract_policy::count(state,timed.id);if(n<0)return false;
        if(n<timed.value)mask|=battle_contract_events::timed_mask;
    }
    for(int i=0;i<boost_contract_events::count;++i){
        const auto& rule=contract_content::boost_rules[i];int index=local_contracts::index(rule.id);if(index<0)return false;
        if(local_contracts::eligible(contract_content::definitions[index],state)){
            int n=battle_contract_policy::count(state,rule.id);if(n<0)return false;
            if(n<rule.value)mask|=boost_contract_events::eligible(i);
        }
    }
    const auto& activity=contract_content::activity_rules[0];int activity_index=local_contracts::index(activity.id);
    if(activity_index<0)return false;
    if(local_contracts::eligible(contract_content::definitions[activity_index],state)){
        int n=battle_contract_policy::count(state,activity.id);if(n<0)return false;
        if(n<activity.value)mask|=activity_contract_events::eligible;
    }
    return true;
}
static bool begin(local_match_store::State& state){
    uint32_t mask=0;
    if(local_match_store::formal(state)&&!mask_for(progress,mask))return false;
    if(!local_match_store::freeze_contracts(state,mask))return false;
    pthread_mutex_lock(&mutex);capture={};capture.run=state.next;capture.mask=mask;capture.faction=local_faction::active;pthread_mutex_unlock(&mutex);
    LOG("Local battle contract start frozen run=%llu mask=%u formal=%d",(unsigned long long)state.next,mask,local_match_store::formal(state));return true;
}
static void event(void* battle,void* side,int key,int amount,void* method){
    original(battle,side,key,amount,method);
    if(observer)observer(battle,side,key,amount);
    if(trace&&trace_count<32&&(key==30||key==16||key==27||key==50||key==64||key==68)){
        void* context=static_object("com.geargames.aow.scenes.battle","ClientBattleContext","m_battle");
        bool same=battle&&battle==context;
        bool own=same&&side==local_dictionary_item(*(void**)((char*)battle+0x178),1);
        FILE* f=fopen(DATA_ROOT "battle-contract-events-trace.jsonl","a");
        if(f){fprintf(f,"{\"key\":%d,\"amount\":%d,\"current_context\":%s,\"player_side\":%s,\"seeded\":%s,\"formal\":%s}\n",key,amount,same?"true":"false",own?"true":"false",local_duel::seeded?"true":"false",local_match::playing&&local_match_store::formal(local_match::state)?"true":"false");fclose(f);++trace_count;}
    }
    if(!offline_network||!local_match::playing||local_match::pending_result||!local_duel::seeded||
       !local_match_store::formal(local_match::state)||local_match::state.next<=local_match::state.settled)return;
    bool known=key==30&&timed_available();for(const auto& rule:contract_content::battle_rules)if(rule.subtype==key)known=true;
    if(!known)return;
    void* active=static_object("com.geargames.aow.scenes.battle","ClientBattleContext","m_battle");
    if(!battle||battle!=active||side!=local_dictionary_item(*(void**)((char*)battle+0x178),1))return;
    pthread_mutex_lock(&mutex);
    if(capture.run==local_match::run&&capture.faction==local_faction::active&&capture.mask){
        if(capture.battle&&capture.battle!=battle)capture.failed=true;
        capture.battle=battle;
        if(!battle_contract_events::add(capture.mask,capture.counts,key,amount))capture.failed=true;
        if(key==30&&(capture.mask&battle_contract_events::timed_mask)){
            int tick=logic_tick?logic_tick(battle,nullptr):-1;
            if(!timed_contract_events::add(capture.mask,capture.timed,key,amount,tick))capture.failed=true;
        }
    }
    pthread_mutex_unlock(&mutex);
}
static bool stamp(local_match_store::State& state,void* battle){
    if(state.version<8||!state.contract_mask||!local_match_store::normal(state))return local_match_store::stamp_contracts(state,{});
    pthread_mutex_lock(&mutex);auto current=capture;pthread_mutex_unlock(&mutex);
    if(current.run!=state.settled||current.mask!=(state.contract_mask&battle_contract_events::start_mask)||current.faction!=local_faction::active||
       current.failed||(current.battle&&current.battle!=battle))return false;
    int activity=-1,percent=-1;
    if(state.version>=12&&(state.contract_mask&activity_contract_events::eligible)&&state.outcome==local_match_store::Victory){
        void* active=static_object("com.geargames.aow.scenes.battle","ClientBattleContext","m_battle");
        void* side=battle?local_dictionary_item(*(void**)((char*)battle+0x178),1):nullptr;
        if(battle!=active||!activity_contract_runtime::read(battle,side,activity,percent)){
            LOG("Local activity contract final source failed run=%llu; result not stamped",(unsigned long long)state.settled);return false;}
        LOG("Local activity contract final source run=%llu raw=%d percent=%d threshold=70 strict=1 player_side=1",(unsigned long long)state.settled,activity,percent);
    }
    return local_match_store::stamp_contracts(state,current.counts,current.timed,current.boost_used,activity);
}
static void boost_applied(void* battle,void* side,int id){
    if(!offline_network||!local_match::playing||local_match::pending_result||!local_duel::seeded||
       !local_match_store::formal(local_match::state)||local_match::state.next<=local_match::state.settled)return;
    bool known=false;for(const auto& rule:contract_content::boost_rules)if(rule.boost==id)known=true;if(!known)return;
    void* active=static_object("com.geargames.aow.scenes.battle","ClientBattleContext","m_battle");
    if(!battle||battle!=active||side!=local_dictionary_item(*(void**)((char*)battle+0x178),1))return;
    pthread_mutex_lock(&mutex);
    if(capture.run==local_match::run&&capture.faction==local_faction::active&&(capture.mask&battle_contract_events::boost_mask)){
        if(capture.battle&&capture.battle!=battle)capture.failed=true;
        capture.battle=battle;if(!boost_contract_events::add(capture.mask,capture.boost_used,id))capture.failed=true;
    }
    pthread_mutex_unlock(&mutex);
}
static void install(){
    const uint32_t signature[]={0xd101c3ff,0xa9017bfd,0xa9026ffc,0xa90367fa};
    original=(decltype(original))install_hook(0x48005ec,signature,(void*)event);
    local_contracts::battle_available=available;
    const uint32_t tick_signature[]={0xb940b000,0xd65f03c0};
    const uint32_t set_tick_signature[]={0xb900b001,0xd65f03c0};
    // This bootstrap thread only binds exact native code. IL2CPP metadata
    // lookup belongs on the attached engine thread, where the model audit
    // independently checks the original getter/setter on decoded objects.
    if(!memcmp((void*)(image_base+0x4674f94),tick_signature,sizeof(tick_signature))&&
       !memcmp((void*)(image_base+0x4674f9c),set_tick_signature,sizeof(set_tick_signature)))
        logic_tick=(decltype(logic_tick))(image_base+0x4674f94);
    local_contracts::timed_available=timed_available;
    local_inventory::applied_observer=boost_applied;local_contracts::boost_available=boost_available;
    activity_contract_runtime::bind();local_contracts::activity_available=activity_contract_runtime::available;
    trace=!access(DATA_ROOT "battle-contract-events-trace.request",F_OK);trace_count=0;
    if(trace){FILE* f=fopen(DATA_ROOT "battle-contract-events-trace.jsonl","w");if(f){fclose(f);chmod(DATA_ROOT "battle-contract-events-trace.jsonl",0644);}}
    LOG("Local original battle achievement event hook=%d frozen_event_rules=5 timed_result_rules=1 timed_building_clock=%d private_events_ignored=1",original!=nullptr,timed_available());
}
}
