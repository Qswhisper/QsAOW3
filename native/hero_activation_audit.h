// Original-engine activation contract on independently decoded objects.
// Never published as a player battle; no career changes or save writes.
#pragma once
namespace hero_activation_audit {
using local_research_runtime::Roots;
static void* get(void* o,int offset){return o?*(void**)((char*)o+offset):nullptr;}
static int count(void* dictionary){void* ex=nullptr;void* box=dictionary?invoke_named(dictionary,"get_Count",0,nullptr,&ex):nullptr;return !ex&&box?*(int*)((char*)box+16):-1;}
static void row(FILE* f,const char* phase,void* side,void* ex){
    void* cooldown=get(side,0x78);
    fprintf(f,"{\"phase\":\"%s\",\"exception\":\"%s\",\"unit_types\":%d,\"bindings\":%d,\"active_heroes\":%d,\"cooldown_slots\":%d,\"field44\":%d,\"field48\":%d,\"field58\":%d,\"field5c\":%d}\n",phase,ex?class_name(object_class(ex)):"none",count(get(side,0xf0)),count(get(side,0x68)),count(get(side,0x70)),cooldown?(int)array_length(cooldown):-1,side?*(int*)((char*)side+0x44):-1,side?*(int*)((char*)side+0x48):-1,side?*(int*)((char*)side+0x58):-1,side?*(int*)((char*)side+0x5c):-1);
}
static void run(){
    if(!progress_ready||access(DATA_ROOT "hero-activation-audit.request",F_OK))return;
    struct Scope {Scope(){local_hero_battle::auditing=true;}~Scope(){local_hero_battle::auditing=false;}} scope;
    unlink(DATA_ROOT "hero-activation-audit.request");auto before=progress;Roots roots;preplaced_research_audit::Fixture fixture;
    FILE* f=fopen(DATA_ROOT "hero-activation-audit.jsonl","w");if(!f)return;chmod(DATA_ROOT "hero-activation-audit.jsonl",0644);
    // Match the already accepted normal training input, including its full
    // native prices and CP configuration; an embedded first mission has
    // tutorial-only production restrictions and zero-price variants.
    const auto* scenario=local_scenario(202);
    void* message=scenario&&!scenario->embedded?read_scenario_message(scenario->file,5040,"@hero-native-fixture"):nullptr;
    fixture.battle=get(message,0x20);fixture.side=local_dictionary_item(get(fixture.battle,0x178),1);
    fixture.barrier=(decltype(fixture.barrier))dlsym(il2cpp,"il2cpp_gc_wbarrier_set_field");
    bool fixture_ready=fixture.side&&fixture.barrier&&fixture.samples;void* battle=fixture.battle;
    void* own=fixture.side;void* ex=nullptr;
    if(own)for(const char* field:{"$cg","$Cg","$dg"})field_set(own,class_field_from_name(object_class(own),field),nullptr);
    row(f,"decoded",own,nullptr);
    bool reset=false;void* args[]={&reset};if(own)invoke_named(own,"$hU",1,args,&ex);
    row(f,"original_side_init",own,ex);
    void* info=object_new(find_class("com.geargames.aow.scenes.battle","InitializationBattleInfo"));
    bool ready=fixture_ready&&roots.keep(info)&&!ex;
    if(ready){invoke_named(info,".ctor",0,nullptr,&ex);ready=!ex;}
    if(ready){field_set(info,class_field_from_name(object_class(info),"Battle"),battle);*(int*)((char*)info+0x20)=1;
        ready=local_hero_battle::prepare(info,battle,1);}
    row(f,"prepared_native_input",own,ex);
    int tick=0;void* tick_args[]={&tick};if(ready)invoke_named(battle,"$ro",1,tick_args,&ex);
    void* source=local_career::lookup("BATTLE","unitTypes",172),*hero=nullptr;
    uintptr_t handle=0;void* types=ready?local_dictionary_values(get(own,0xf0),find_class("com.geargames.aow.entities","UnitType"),handle):nullptr;
    if(handle)roots.take(handle);
    for(size_t i=0;types&&i<array_length(types);++i){void* type=((void**)((char*)types+32))[i];if(generic_unit_research::identity(type,source))hero=type;}
    int side_id=1,hero_id=hero?*(int*)((char*)hero+0x10):-1,slot=0;
    void* activation_args[]={battle,&side_id,&hero_id,&slot};void* klass=find_class("com.geargames.aow.entities.battle","HeroAct");
    void* result=ready&&hero?runtime_invoke(class_method(klass,"$be",4),nullptr,activation_args,&ex):nullptr;
    int locked=result&&!ex?*(int16_t*)((char*)result+16):-999;
    fprintf(f,"{\"local_unowned_result\":%d,\"local_unowned_refused_without_binding\":%s}\n",locked,locked<0&&!ex&&count(get(own,0x68))==0?"true":"false");
    generic_unit_research::Effect effect;generic_unit_research::Plan plan;int matched=0;
    bool projected=ready&&!ex&&generic_unit_research::resolve(local_hero::first_level(172,2),effect)&&plan.add(effect)&&generic_unit_research::apply(own,get(own,0xf0),plan,matched)&&matched==1;
    // The native slot limit derives from living hero producers. A decoded
    // first-battle input has none until its scenario runs; model that explicit
    // precondition rather than disabling the native combination/building gate.
    fixture.unit_array=get(own,0x140);fixture.building_array=get(own,0x148);
    uintptr_t buildings_handle=0;void* buildings=projected?local_dictionary_values(get(own,0xf8),find_class("com.geargames.aow.entities","BuildingType"),buildings_handle):nullptr;
    if(buildings_handle)roots.take(buildings_handle);void* producer=nullptr;
    for(size_t i=0;buildings&&i<array_length(buildings);++i){void* b=((void**)((char*)buildings+32))[i],*levels=get(b,0x18);
        for(int j=0;levels&&j<*(int*)((char*)levels+0x18);++j){void* units=get(list_item(levels,j),0x20);
            for(int k=0;units&&k<*(int*)((char*)units+0x18);++k)if(generic_unit_research::identity(list_item(units,k),source))producer=b;}}
    bool producer_ready=producer&&fixture.spawn(producer,0,0,true,0);
    bool hero_base_ready=false;
    for(size_t i=0;buildings&&i<array_length(buildings);++i){void* b=((void**)((char*)buildings+32))[i];
        if(*(int8_t*)((char*)b+0x30)!=18)continue;void* levels=get(b,0x18);int n=levels?*(int*)((char*)levels+0x18):0;
        hero_base_ready=n>0&&fixture.spawn(b,n-1,0,true,1);break;}
    // Fixture.spawn preserves a zero-filled construction state for stat
    // migration tests. Activation needs actually completed native buildings.
    for(int i=0;i<fixture.count;++i)if(fixture.samples[i].building)*(int8_t*)((char*)fixture.samples[i].entity+0x47)=1;
    // Production checks resolve the building through Battle's original global
    // dynamics index, in addition to the owning side's building array.
    void* dynamics=fixture.field_list(object_class(battle),"$rb",roots);
    if(dynamics){for(int i=0;i<fixture.count;++i){void* entity=fixture.samples[i].entity;int id=*(int*)((char*)entity+0x10);
        void* add_args[]={&id,entity};invoke_named(dynamics,"Add",2,add_args,&ex);if(ex)break;}
        if(!ex)field_set(battle,class_field_from_name(object_class(battle),"$rb"),dynamics);}
    void* projected_hero=local_dictionary_item(get(own,0xf0),hero_id);
    void* direct_old=nullptr;void* direct_args[]={battle,own,projected_hero,&slot,direct_old};void* direct_ex=nullptr;
    void* direct=projected_hero?runtime_invoke(class_method(klass,"$Be",5),nullptr,direct_args,&direct_ex):nullptr;
    int direct_code=direct&&!direct_ex?*(int16_t*)((char*)direct+16):-999;
    void* compatibility_args[]={own,projected_hero};void* compatible=projected_hero?runtime_invoke(class_method(klass,"$De",2),nullptr,compatibility_args,&direct_ex):nullptr;
    void* side_args[]={own};void* limit=runtime_invoke(class_method(find_class("com.geargames.aow.entities.battle","SideAct"),"$tf",1),nullptr,side_args,&direct_ex);
    auto resolver_slot=(void**)(image_base+160992016);reinterpret_cast<void(*)(void*)>(image_base+0x3cfb0d4)(resolver_slot);
    void* lookup_args[]={battle,get(own,0xf0),&hero_id,own};void* resolved=runtime_invoke(*resolver_slot,nullptr,lookup_args,&direct_ex);
    fprintf(f,"{\"direct_guard\":%d,\"combination_allowed\":%s,\"hero_slot_limit\":%d,\"lookup_matches_private\":%s,\"resolved_id\":%d,\"resolved_class\":%d,\"private_class\":%d,\"diagnostic_exception\":\"%s\"}\n",direct_code,compatible&&*(bool*)((char*)compatible+16)?"true":"false",limit?*(int*)((char*)limit+16):-1,resolved==projected_hero?"true":"false",resolved?*(int*)((char*)resolved+0x10):-1,resolved?*(int*)((char*)resolved+0xa0):-1,projected_hero?*(int*)((char*)projected_hero+0xa0):-1,direct_ex?class_name(object_class(direct_ex)):"none");
    void* locked_ex=nullptr;
    if(producer_ready&&projected_hero){*(int*)((char*)projected_hero+0xa0)=0;
        result=runtime_invoke(class_method(klass,"$Be",5),nullptr,direct_args,&locked_ex);
        locked=result&&!locked_ex?*(int16_t*)((char*)result+16):-999;*(int*)((char*)projected_hero+0xa0)=1;}
    void* commands=object_new(find_class("com.geargames.aow.scenes.battle.logicservice","ClientCommandAdd"));
    if(projected&&roots.keep(commands)){invoke_named(commands,".ctor",0,nullptr,&ex);if(!ex)field_set(battle,class_field_from_name(object_class(battle),"$EB"),commands);}
    bool capture_ready=local_hero_battle::add_original!=nullptr;
    if(capture_ready)local_hero_battle::capture_begin();
    result=projected&&producer_ready&&hero_base_ready&&!ex&&capture_ready?runtime_invoke(class_method(klass,"$be",4),nullptr,activation_args,&ex):nullptr;
    int acquired=result&&!ex?*(int16_t*)((char*)result+16):-999;
    fprintf(f,"{\"canonical_hero_hp\":%d,\"canonical_hero_life\":%d,\"canonical_hero_price\":%d,\"canonical_hero_cp\":%d,\"canonical_train_ticks\":%d}\n",source?*(int*)((char*)source+0x34):-1,source?*(int16_t*)((char*)source+0x32):-1,source?*(int*)((char*)source+0x48):-1,source?*(int8_t*)((char*)source+0x44):-1,source?*(int*)((char*)source+0x38):-1);
    row(f,"native_activation",own,ex);
    fprintf(f,"{\"energy_after_activation\":%d,\"activation_price\":%d,\"actual_tick\":%d}\n",own?*(int*)((char*)own+0x40):-1,hero?*(int*)((char*)hero+0xa8):-1,tick);
    fprintf(f,"{\"private_producer_ready\":%s,\"producer_id\":%d,\"producer_state\":%d}\n",producer_ready?"true":"false",producer?*(int*)((char*)producer+0x10):-1,producer_ready?*(int8_t*)((char*)fixture.samples[0].entity+0x20):-1);
    fprintf(f,"{\"private_hero_base_ready\":%s,\"native_activation_pass\":%s}\n",hero_base_ready?"true":"false",!ex&&acquired==0&&count(get(own,0x68))==1?"true":"false");
    fprintf(f,"{\"original_energy_events\":%d,\"original_energy_output\":%d,\"private_commands\":%d,\"locked_exception\":\"%s\"}\n",local_hero_battle::captured_energy_count,local_hero_battle::captured_energy,local_hero_battle::captured_count,locked_ex?class_name(object_class(locked_ex)):"none");
    if(acquired==0&&!ex&&fixture.count>=2){
        void* base=fixture.samples[1].entity;
        void* queue=fixture.field_list(object_class(base),"queue",roots);
        if(queue)field_set(base,class_field_from_name(object_class(base),"queue"),queue);
        // HeroAct.de/ee use UnitType.Price directly. SideAct.Rf is the hero's
        // bound-slot lookup, not a discounted price function.
        void* price=invoke_named(projected_hero,"get_Price",0,nullptr,&ex);
        int cost=price&&!ex?*(int*)((char*)price+16):-1;
        if(cost>=0)*(int*)((char*)own+0x16c)=cost;
        int building_id=*(int*)((char*)base+0x10);
        void* guard_args[]={battle,own,projected_hero,&slot,&building_id};
        void* guard=!ex?runtime_invoke(class_method(klass,"$de",5),nullptr,guard_args,&ex):nullptr;
        int guard_code=guard&&!ex?*(int16_t*)((char*)guard+16):-999;
        void* buy_args[]={battle,&side_id,&hero_id,&slot,&building_id};
        void* bought=guard_code==0&&!ex?runtime_invoke(class_method(klass,"$Ee",5),nullptr,buy_args,&ex):nullptr;
        int bought_code=bought&&!ex?*(int16_t*)((char*)bought+16):-999;
        fprintf(f,"{\"production_guard\":%d,\"production_result\":%d,\"original_price\":%d,\"gold_after\":%d,\"energy_after_buy\":%d,\"queue_size\":%d,\"energy_events_after_buy\":%d,\"energy_output_after_buy\":%d,\"production_exception\":\"%s\"}\n",guard_code,bought_code,cost,*(int*)((char*)own+0x16c),*(int*)((char*)own+0x40),queue?*(int*)((char*)queue+0x18):-1,local_hero_battle::captured_energy_count,local_hero_battle::captured_energy,ex?class_name(object_class(ex)):"none");
        if(ex){FILE* detail=fopen(DATA_ROOT "hero-production-private-error.txt","w");if(detail){
            void* error=nullptr;void* stack=invoke_named(ex,"get_StackTrace",0,nullptr,&error);char text[8192]={};
            if(stack&&!error&&local_research_runtime::ascii(stack,text,sizeof(text)))fprintf(detail,"%s\n",text);
            visited_count=0;dump_object(detail,ex,0,1);fclose(detail);chmod(DATA_ROOT "hero-production-private-error.txt",0644);}}
    }
    // Boundary contract: a private original event carries an absolute balance.
    // This is a supplied event value, not evidence of a real hero's cost debit.
    void* event_ex=nullptr;int supplied_balance=11;void* event_args[]={own,&supplied_balance};
    if(capture_ready&&commands)invoke_named(commands,"addCommandSideUraniumChange",2,event_args,&event_ex);
    bool event_applied=!event_ex&&*(int*)((char*)own+0x40)==11&&local_hero_battle::captured_energy==11;
    bool refilled=local_hero_battle::prepare(info,battle,1)&&*(int*)((char*)own+0x40)==15;
    fprintf(f,"{\"supplied_event_balance_sync\":%s,\"refill_after_balance_change\":%s,\"actual_native_debit_proven\":false}\n",event_applied?"true":"false",refilled?"true":"false");
    if(capture_ready)local_hero_battle::capture_end();
    fprintf(f,"{\"original_locked_result\":%d,\"original_acquired_result\":%d,\"projected\":%s,\"hero_id\":%d,\"info_energy\":%d,\"info_max\":%d,\"exception\":\"%s\"}\n",locked,acquired,projected?"true":"false",hero_id,info?*(int*)((char*)info+0x34):-1,info?*(int*)((char*)info+0x38):-1,ex?class_name(object_class(ex)):"none");
    fprintf(f,"{\"player_state_unchanged\":%s,\"save_written\":false,\"battle_started\":false}\n",!memcmp(&before,&progress,sizeof(progress))?"true":"false");
    fclose(f);LOG("Hero original activation input audit completed exception=%s",ex?class_name(object_class(ex)):"none");
}
}
