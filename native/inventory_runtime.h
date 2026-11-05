// Local inventory adapter for the original AICommBoost -> SideAct.yf path.
// The original command selects paid vs scenario-granted stock and debits it
// BEFORE calling yf. Persist that observed debit before the native effects.
#pragma once
#include "boost_use_policy.h"
namespace local_inventory {
static bool installed;
static void* prepared_side,*prepared_battle;
static int prepared_faction;
static bool session_active(){return offline_network&&(local_run_active||local_match::playing||local_coop::playing);}
static uint64_t session_run(){return local_match::playing?local_match::run:(local_coop::playing?local_coop::state.next:local_run);}
struct Command {
    void* side; void* paid; void* granted;
    int id, before, grant_before; bool committed=false, refused=false;
};
static thread_local Command* current;
static void (*applied_observer)(void*,void*,int);
static int amount(void* boost) { return boost?*(int*)((char*)boost+0x18):0; }
static bool notify_applied(void* battle,const Command& c,int result){
    if(!boost_use_policy::consumed(result,c.committed,c.refused,c.before,amount(c.paid),c.grant_before,amount(c.granted)))return false;
    if(applied_observer)applied_observer(battle,c.side,c.id);return true;
}
static void* item(void* side,int offset,int id) {
    return side?local_dictionary_item(*(void**)((char*)side+offset),id):nullptr;
}
static bool prepare(void* battle,int own_side) {
    if(!installed || !progress_ready || !battle) return false;
    void* side=local_dictionary_item(*(void**)((char*)battle+0x178),own_side);
    void* old=side?*(void**)((char*)side+0x108):nullptr;
    if(!old) return false;
    local_research_runtime::Roots roots;
    void* stock=object_new(object_class(old));
    if(!roots.keep(stock))return false;
    void* ex=nullptr;invoke_named(stock,".ctor",0,nullptr,&ex);if(ex)return false;
    for(uint32_t i=0;i<progress.stock_count;++i) {
        const auto& saved_stock=progress.stocks[i];
        // Hero blueprints share the persistent ledger, never the battle boost
        // dictionary. Keeping a card must not prevent a later battle starting.
        if(saved_stock.type!=4)continue; // Every other kind has its own original typed source.
        void* prototype=local_career::lookup("BATTLE","boosts",saved_stock.id);
        void* boost=object_new(find_class("com.geargames.aow.entities.battle","Boost"));
        // Each item is attached to the rooted dictionary before the next
        // allocation; only the current item needs an additional temporary root.
        local_career::Roots one;
        if(saved_stock.type!=4 || !prototype || !one.keep(boost))return false;
        void* args[]={prototype,(void*)&saved_stock.amount};
        invoke_named(boost,".ctor",2,args,&ex);if(ex)return false;
        void* add_args[]={(void*)&saved_stock.id,boost};
        invoke_named(stock,"Add",2,add_args,&ex);if(ex)return false;
    }
    // Script grants at +0x110 are deliberately retained, never imported into
    // the career. Replacing +0x108 removes the capture guest's paid inventory.
    field_set(side,class_field_from_name(object_class(side),"$Hg"),stock);
    prepared_side=side;prepared_battle=battle;prepared_faction=local_faction::active;
    LOG("Local inventory prepared faction=%d side=%d stockTypes=%u",prepared_faction,own_side,progress.stock_count);
    return true;
}
static int16_t (*apply_original)(void*,void*,void*,void*);
static int16_t apply(void* battle,void* side,void* type,void* method) {
    Command* c=current;
    if(!c || c->side!=side || !type || *(int*)((char*)type+0x10)!=c->id)
        return apply_original(battle,side,type,method);
    int debit=c->before-amount(c->paid);
    if(debit) {
        auto next=progress;
        bool ok=debit==1 && !c->committed && prepared_faction==local_faction::active &&
            local_progress::consume_stock(next,4,c->id,debit) && local_progress::write(local_faction::root(),next);
        if(!ok) {
            // This boundary precedes all effects and result UI commands.
            // Restore the original input's debit and return a real error.
            if(c->paid)*(int*)((char*)c->paid+0x18)=c->before;
            if(c->granted)*(int*)((char*)c->granted+0x18)=c->grant_before;
            c->refused=true;
            LOG("Local inventory refused before effects id=%d debit=%d",c->id,debit);
            return -1;
        }
        progress=next;c->committed=true;
        LOG("Local inventory consumed faction=%d run=%llu id=%d remaining=%d generation=%llu",
            prepared_faction,(unsigned long long)session_run(),c->id,local_progress::stock_amount(progress,4,c->id),(unsigned long long)progress.generation);
    }
    int16_t result=apply_original(battle,side,type,method);
    void* active=*(void**)((char*)side+0x3d0);
    void* active_type=active?*(void**)((char*)active+0x10):nullptr;
    LOG("Local inventory effect id=%d result=%d active=%d start=%d finish=%d",c->id,result,
        active_type?*(int*)((char*)active_type+0x10):-1,active?*(int*)((char*)active+0x18):-1,active?*(int*)((char*)active+0x1c):-1);
    return result;
}
static int16_t (*purchase_original)(void*,void*,void*);
static int16_t purchase(void* self,void* battle,void* method) {
    if(session_active()) {LOG("Local inventory battle purchase disabled; use owned stock");return -1;}
    return purchase_original(self,battle,method);
}
static bool (*ui_original)(void*,float,void*);
static bool ui(void* self,float delta,void* method) {
    void* source=self?*(void**)((char*)self+0x20):nullptr;
    if(!session_active() || !source || !local_progress::stock_types(progress,4) ||
        !*(int16_t*)((char*)source+0x2c) || *((uint8_t*)source+0x12))
        return ui_original(self,delta,method);
    // Copy the original switcher. Only expose the original owned-boost button
    // during an interactive phase; keep every other tutorial/event flag.
    local_career::Roots roots;void* adapted=object_new(object_class(source));
    if(!roots.keep(source)||!roots.keep(adapted))return false;
    void* ex=nullptr;void* args[]={source};invoke_named(adapted,".ctor",1,args,&ex);
    if(ex){LOG("Local inventory UI copy failed");return false;}
    *((uint8_t*)adapted+0x12)=true;
    field_set(self,class_field_from_name(object_class(self),"m_uiSwitcher"),adapted);
    bool result=ui_original(self,delta,method);
    field_set(self,class_field_from_name(object_class(self),"m_uiSwitcher"),source);
    LOG("Local inventory original boost button exposed events=%d",*(int16_t*)((char*)source+0x2c));
    return result;
}
static int16_t (*command_original)(void*,void*,void*);
static void replay_input() {
    if(!local_run_active || !progress_ready)return;
    if(!access(DATA_ROOT "inventory-hit.request",F_OK)) {
        unlink(DATA_ROOT "inventory-hit.request");
        void* hit=static_object("","UICamera","mRayHitObject");void* ex=nullptr;
        void* name=hit?invoke_named(hit,"get_name",0,nullptr,&ex):nullptr;
        char label[121]={};
        if(name&&!ex){int n=*(int*)((char*)name+16);auto chars=(uint16_t*)((char*)name+20);
            for(int i=0;i<n&&i<120;++i)label[i]=chars[i]<128?(char)chars[i]:'?';}
        LOG("Local inventory hit object=%p name=%s",hit,label);
    }
    bool replay=false;
    for(uint32_t i=0;i<progress.count;++i)
        if(progress.missions[i].id==local_mission_id && progress.missions[i].wins>0)replay=true;
    if(!replay)return;
    void* screen=static_object("com.geargames.aow.scenes.battle","ClientBattleContext","m_screen");
    void* dispatcher=screen?*(void**)((char*)screen+0x1d0):nullptr;
    void* gui=dispatcher?*(void**)((char*)dispatcher+0x60):nullptr;
    void* blocker=gui?*(void**)((char*)gui+0x398):nullptr;
    void* blocker_ex=nullptr;
    void* blocker_active=blocker?invoke_named(blocker,"get_IsActive",0,nullptr,&blocker_ex):nullptr;
    if(blocker_active && !blocker_ex && *((uint8_t*)blocker_active+16)) {
        invoke_named(blocker,"Deactivate",0,nullptr,&blocker_ex);
        LOG("Local replay tutorial click blocker released=%d run=%llu",!blocker_ex,(unsigned long long)local_run);
    }
    void* controller=static_object("com.geargames.aow.scenes.battle","ClientBattleContext","m_tutorialController");
    void* darken=controller?*(void**)((char*)controller+0x48):nullptr;
    void* colliders=darken?*(void**)((char*)darken+0x38):nullptr;
    if(!colliders)return;
    void* ex=nullptr;bool active=false;void* args[]={&active};
    invoke_named(colliders,"SetActive",1,args,&ex);
    static uint64_t reported;
    if(reported!=local_run || ex){LOG("Local replay tutorial input mask released=%d run=%llu; quest conditions unchanged",!ex,(unsigned long long)local_run);reported=local_run;}
}
static int16_t command(void* self,void* battle,void* method) {
    if(!session_active()||!self||*(int*)((char*)self+0x28)!=1)
        return command_original(self,battle,method);
    if(!progress_ready||!prepared_side||battle!=prepared_battle||prepared_faction!=local_faction::active||
       *(int*)((char*)prepared_side+0x10)!=1||local_dictionary_item(*(void**)((char*)battle+0x178),1)!=prepared_side){
        LOG("Local inventory player command refused: current battle/owned stock not prepared");return -1;}
    int id=*(int*)((char*)self+0x30);
    Command c{prepared_side,item(prepared_side,0x108,id),item(prepared_side,0x110,id),id,0,0};
    c.before=amount(c.paid);c.grant_before=amount(c.granted);
    Command* previous=current;current=&c;
    int16_t result=command_original(self,battle,method);
    current=previous;
    bool consumed=notify_applied(battle,c,result);
    LOG("Local inventory command id=%d result=%d paid=%d->%d grant=%d->%d committed=%d refused=%d consumed_event=%d",
        id,result,c.before,amount(c.paid),c.grant_before,amount(c.granted),c.committed,c.refused,consumed);
    return result;
}
static void install() {
    const uint32_t signature[]={0xa9ba7bfd,0xa9016ffc,0xa90267fa,0xa9035ff8};
    apply_original=(decltype(apply_original))install_hook(0x47fba1c,signature,(void*)apply);
    command_original=(decltype(command_original))install_hook(0x491e4c8,signature,(void*)command);
    const uint32_t purchase_signature[]={0xa9bb6ffe,0xa90167fa,0xa9025ff8,0xa90357f6};
    purchase_original=(decltype(purchase_original))install_hook(0x491ef9c,purchase_signature,(void*)purchase);
    const uint32_t ui_signature[]={0xf81c0ffe,0xa9015ff8,0xa90257f6,0xa9034ff4};
    ui_original=(decltype(ui_original))install_hook(0x81bb83c,ui_signature,(void*)ui);
    installed=apply_original&&command_original&&purchase_original&&ui_original;
    LOG("Local inventory hooks installed=%d",installed);
}
}
