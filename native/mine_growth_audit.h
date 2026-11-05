#pragma once
namespace mine_growth_audit {
using namespace mine_growth;
static uint64_t hash(){uint64_t h=1469598103934665603ull;for(int i=0;i<4;++i){void* p=original(i);if(!p)return 0;
    for(int k=0x10;k<0x68;++k)h=(h^((uint8_t*)p)[k])*1099511628211ull;}return h;}
static bool fixture_failure(int line){LOG("Mine growth detached fixture refused line=%d",line);return false;}
static bool values(void* side,const Plan& p,const research_effects::MineTimes& time,int& checked){
    for(int i=0;i<4;++i){void* old=original(i),*mine=local_dictionary_item(get(side,0x100),i*2);MineGrowth v;
        if(!mine||!research_effects::mine_growth_values(read(old),p.mines[i],v))return false;if(i<3)v.time-=time.reduction[i];
        const char* names[]={"get_DamageLight","get_DamageMedium","get_DamageHeavy"};
        for(int k=0;k<3;++k)if(!original_stat_equals(mine,names[k],4,v.damage[k]))return false;
        int available=*(int8_t*)((char*)old+0x18);if((i==1||i==2)&&p.mines[i].life&&available==0)available=1;
        if(!original_stat_equals(mine,"get_Life",2,v.life)||!original_stat_equals(mine,"get_ExplosionRadius",4,v.radius)||
           !original_stat_equals(mine,"get_SetTime",4,v.time)||!original_stat_equals(mine,"get_Available",1,available))return false;checked+=7;
        uint8_t bytes[0x58];memcpy(bytes,(char*)mine+0x10,sizeof(bytes));
        for(int o:{0x1c,0x20,0x24,0x28,0x34})memcpy(bytes+o-0x10,(char*)old+o,4);
        memcpy(bytes+0x38-0x10,(char*)old+0x38,2);bytes[0x18-0x10]=((uint8_t*)old)[0x18];
        if(memcmp(bytes,(char*)old+0x10,sizeof(bytes)))return false;
    }return true;
}
static bool references(void* source,const Plan& p,const research_effects::MineTimes& time,int& mine_count,int& queue_count,bool& alias_refused){
    preplaced_research_audit::Fixture x;if(!x.init("fmc2m"))return fixture_failure(__LINE__);Roots roots;
    // A land seed need not contain Cayman125. Supply its actual retained
    // prototype to this detached mixed-category fixture; no public route changes.
    void* old_units=get(x.side,0xf0);uintptr_t unit_handle=0;
    void* unit_values=local_dictionary_values(old_units,find_class("com.geargames.aow.entities","UnitType"),unit_handle);
    if(!unit_values||!roots.take(unit_handle))return fixture_failure(__LINE__);
    void* units=local_mod_effects::clone_dictionary(old_units,unit_values,roots);if(!units)return fixture_failure(__LINE__);
    for(size_t j=0;j<array_length(unit_values);++j){void* type=((void**)((char*)unit_values+32))[j];if(!type)return fixture_failure(__LINE__);
        int id=*(int*)((char*)type+0x10);if(id==111||id==125)continue;
        if(!local_mod_effects::add(units,id,type))return fixture_failure(__LINE__);}
    for(int id:{111,125}){void* level=local_upgrade_level(id==111?10900:12490),*type=get(get(level,0x48),0x48);
        void* states=get(type,0x18);int n=states?*(int*)((char*)states+0x18):0;
        LOG("Mine growth detached unit id=%d present_in_first_mission=%d original_states=%d life=%d/%d",id,!!local_dictionary_item(old_units,id),n,type?*(int16_t*)((char*)type+0x32):0,type?*(int*)((char*)type+0x34):0);
        if(!type||*(int*)((char*)type+0x10)!=id||n<1||!local_mod_effects::add(units,id,type))return fixture_failure(__LINE__);}
    field_set(x.side,class_field_from_name(object_class(x.side),"$gg"),units);
    void* fixture_fraction=get(x.side,0xd8);
    LOG("Mine growth detached fixture source faction=%d canonical mine faction=2",fixture_fraction?*(int*)((char*)fixture_fraction+0x10):-1);
    if(!fixture_fraction||*(int*)((char*)fixture_fraction+0x10)!=2)return fixture_failure(__LINE__);
    field_set(x.side,class_field_from_name(object_class(x.side),"$hg"),get(source,0x100));
    void* foreign=local_research_runtime::clone(x.side,roots),*prepared=local_research_runtime::clone(x.side,roots);int matched=0;
    if(!foreign||!prepared||!prepare(prepared,p,matched)||matched!=4)return fixture_failure(__LINE__);
    void* own_units[2]={};for(int i=0;i<2;++i){void* type=local_dictionary_item(get(x.side,0xf0),i?125:111);
        if(!type||!x.spawn(type,0,0,false,i))return fixture_failure(__LINE__);own_units[i]=x.samples[i].entity;}
    void* grid_field=class_field_from_name(object_class(x.battle),"$hB"),*grid_class=grid_field?class_from_type(field_type(grid_field)):nullptr;
    void* row_class=grid_class?class_element(grid_class):nullptr,*mine_class=find_class("com.geargames.aow.entities","Mine");
    void* grid=row_class?array_new(row_class,1):nullptr,*row=mine_class?array_new(mine_class,24):nullptr;
    if(!roots.keep(grid)||!roots.keep(row)||!x.array_put(grid,0,row))return fixture_failure(__LINE__);field_set(x.battle,grid_field,grid);
    void* objects[24]={},*before_types[24]={};int before_life[24]={};uint8_t before_bytes[24][0x58];
    for(int i=0;i<24;++i){Roots one;int category=i/6,state=(i%6)/2;void* old=local_dictionary_item(get(source,0x100),category*2);
        void* mine=object_new(mine_class);if(!one.keep(mine))return fixture_failure(__LINE__);objects[i]=mine;before_types[i]=old;
        field_set(mine,class_field_from_name(mine_class,"type"),old);field_set(mine,class_field_from_name(mine_class,"side"),i%2?foreign:x.side);
        int maximum=*(int16_t*)((char*)old+0x38);before_life[i]=state==2?0:maximum-(state==1?7:0);*(int16_t*)((char*)mine+0x2a)=before_life[i];
        *(int*)((char*)mine+0x20)=100+i;*(int*)((char*)mine+0x24)=200+i;
        if(!x.array_put(row,i,mine))return fixture_failure(__LINE__);memcpy(before_bytes[i],mine,0x58);}
    void* commands[4]={};for(int i=0;i<4;++i){void* klass=find_class("com.geargames.aow.entities.battle","MineLaying"),*c=object_new(klass);
        if(!roots.keep(c))return fixture_failure(__LINE__);commands[i]=c;field_set(c,class_field_from_name(klass,"$jJ"),local_dictionary_item(get(source,0x100),i*2));
        field_set(c,class_field_from_name(klass,"$kJ"),own_units[i==3?1:0]);*(int*)((char*)c+0x10)=123+i;*(int*)((char*)c+0x14)=456+i;
        if(i==0)field_set(c,class_field_from_name(klass,"$JJ"),objects[2]);}
    field_set(own_units[0],class_field_from_name(object_class(own_units[0]),"mine_laying_curr"),commands[0]);
    field_set(own_units[1],class_field_from_name(object_class(own_units[1]),"mine_laying_curr"),commands[3]);
    void* sets[2]={};for(int i=0;i<2;++i){void* klass=find_class("com.geargames.aow.entities.battle","MineLayingSet"),*set=object_new(klass);
        if(!roots.keep(set))return fixture_failure(__LINE__);sets[i]=set;void* list=x.field_list(klass,"$KJ",roots);if(!list||!x.add(list,commands[i+1]))return fixture_failure(__LINE__);
        field_set(set,class_field_from_name(klass,"$KJ"),list);}
    void* unit_class=object_class(own_units[0]);field_set(own_units[0],class_field_from_name(unit_class,"mine_laying_set_curr"),sets[0]);
    void* set_list=x.field_list(unit_class,"mine_laying_sets",roots);if(!set_list||!x.add(set_list,sets[1]))return fixture_failure(__LINE__);
    field_set(own_units[0],class_field_from_name(unit_class,"mine_laying_sets"),set_list);
    for(int i=0;i<x.count;++i)memcpy(x.samples[i].bytes,x.samples[i].entity,0x2e8);
    preplaced_research::Migration staged;if(!staged.prepare(x.battle,x.side,prepared)||!x.unchanged())return fixture_failure(__LINE__);
    for(int i=0;i<24;++i)if(memcmp(objects[i],before_bytes[i],0x58))return fixture_failure(__LINE__);
    for(int i=0;i<4;++i)if(get(commands[i],0x18)!=local_dictionary_item(get(source,0x100),i*2))return fixture_failure(__LINE__);
    void* fake_foreign=object_new(unit_class);if(!roots.keep(fake_foreign))return fixture_failure(__LINE__);
    field_set(fake_foreign,class_field_from_name(find_class("com.geargames.aow.entities","Dynamic"),"side"),foreign);
    field_set(fake_foreign,class_field_from_name(unit_class,"mine_laying_curr"),commands[0]);
    alias_refused=!staged.protected_entity(fake_foreign,x.side);if(!alias_refused)return fixture_failure(__LINE__);
    field_set(x.side,class_field_from_name(object_class(x.side),"$hg"),get(prepared,0x100));staged.commit();
    for(int i=0;i<24;++i){void* expected=i%2?before_types[i]:local_dictionary_item(get(prepared,0x100),(i/6)*2);
        int hp=before_life[i];if(!(i%2)&&hp>0)hp+=*(int16_t*)((char*)expected+0x38)-*(int16_t*)((char*)before_types[i]+0x38);
        if(get(objects[i],0x18)!=expected||!original_stat_equals(objects[i],"get_Life",2,hp))return fixture_failure(__LINE__);
        uint8_t bytes[0x58];memcpy(bytes,objects[i],0x58);memcpy(bytes+0x18,before_bytes[i]+0x18,8);memcpy(bytes+0x2a,before_bytes[i]+0x2a,2);
        if(memcmp(bytes,before_bytes[i],0x58))return fixture_failure(__LINE__);mine_count+=!(i%2);}
    for(int i=0;i<4;++i){if(get(commands[i],0x18)!=local_dictionary_item(get(prepared,0x100),i*2)||
        *(int*)((char*)commands[i]+0x10)!=123+i||*(int*)((char*)commands[i]+0x14)!=456+i)return fixture_failure(__LINE__);++queue_count;}
    // Re-enter the actual outer preparation: dictionaries, queues and live HP
    // must retain the same values instead of accumulating the research twice.
    int hp[24];for(int i=0;i<24;++i)hp[i]=*(int16_t*)((char*)objects[i]+0x2a);
    if(!apply_local_battle_research(x.battle,1))return fixture_failure(__LINE__);int checked=0;if(!values(x.side,p,time,checked))return fixture_failure(__LINE__);
    for(int i=0;i<24;++i)if(*(int16_t*)((char*)objects[i]+0x2a)!=hp[i])return fixture_failure(__LINE__);
    for(int i=0;i<4;++i)if(get(commands[i],0x18)!=local_dictionary_item(get(x.side,0x100),i*2)||
        *(int*)((char*)commands[i]+0x10)!=123+i||*(int*)((char*)commands[i]+0x14)!=456+i)return fixture_failure(__LINE__);
    return true;
}
static __attribute__((noinline)) void run(){
    if(!progress_ready||access(DATA_ROOT "mine-growth-audit.request",F_OK))return;unlink(DATA_ROOT "mine-growth-audit.request");
    building_weapon_audit::Restore restore;if(!restore.saved.data)return;FILE* out=fopen(DATA_ROOT "mine-growth-audit.jsonl","w");if(!out)return;chmod(DATA_ROOT "mine-growth-audit.jsonl",0644);
    local_faction::active=2;progress=local_progress::seed(100000000,0,0,10000);void* rows=generic_unit_research::levels("RESISTANCE");bool ok=true;
    for(int i=0;rows&&i<*(int*)((char*)rows+0x18);++i){void* row=list_item(rows,i);if(!mine_research::candidate(row))continue;
        ok=ok&&local_progress::buy_upgrade(progress,*(int*)((char*)row+0x10),*(int*)((char*)row+0x20),*(int*)((char*)row+0x28));}
    research_effects::MineTimes time;ok=ok&&mine_research::collect(time);research_view::invalidate();ok=ok&&research_view::ready();Roots roots;
    void* source=ok?research_view::cache.side:nullptr;ok=ok&&roots.keep(source);uint64_t before=hash();Plan p;
    int candidates=0,accepted=0,predebit=0,checked=0,prefixes=0,own=0,queues=0,ui=0;bool alias=false;
    // Land timing alone does not grant the two undeveloped mine categories.
    bool no_free=source&&*(int8_t*)((char*)local_dictionary_item(get(source,0x100),2)+0x18)==0&&
        *(int8_t*)((char*)local_dictionary_item(get(source,0x100),4)+0x18)==0;ok&=no_free;
    for(int i=0;rows&&i<*(int*)((char*)rows+0x18);++i){void* row=list_item(rows,i);if(!candidate(row))continue;++candidates;Effect e;
        if(!resolve(row,e)){ok=false;fprintf(out,"{\"phase\":\"unexpected_original_refusal\",\"id\":%d,\"pass\":false}\n",*(int*)((char*)row+0x10));continue;}
        bool pass=purchase_supported(row)&&local_research_supported(row);if(pass)++predebit;
        pass=pass&&research_effects::mine_growth_add(p.mines[e.mine],e.delta,p.mines[e.mine])&&shape(p);
        Roots one;void* a=source?local_research_runtime::clone(source,one):nullptr;int matched=0;
        pass=pass&&a&&prepare(a,p,matched)&&values(a,p,time,checked);void* b=pass?local_research_runtime::clone(a,one):nullptr;
        pass=pass&&b&&prepare(b,p,matched)&&values(b,p,time,checked);if(pass)++prefixes;
        auto bad=p;bad.mines[e.mine].life=INT16_MAX;void* previous=get(a,0x100);int ignored=0;
        pass=pass&&!prepare(a,bad,ignored)&&get(a,0x100)==previous;
        int id=*(int*)((char*)row+0x10);pass=pass&&local_progress::buy_upgrade(progress,id,*(int*)((char*)row+0x20),*(int*)((char*)row+0x28));ok&=pass;++accepted;
        fprintf(out,"{\"phase\":\"original_row\",\"faction\":2,\"id\":%d,\"owner\":%d,\"kind\":\"mine\",\"mine\":%d,\"pass\":%s}\n",id,*(int*)((char*)get(get(row,0x48),0x48)+0x10),e.mine*2,pass?"true":"false");}
    Plan saved;auto* units=new generic_unit_research::Plan;bool saved_plan=collect(saved)&&!memcmp(&saved,&p,sizeof(p))&&generic_unit_research::collect(*units);delete units;ok&=saved_plan;
    research_view::invalidate();for(int i=0;i<4;++i){void* old=original(i),*owner=local_career::lookup("BATTLE","unitTypes",i<3?111:125),*copy=research_view::mapped_mine(old,owner);MineGrowth v;
        bool pass=copy&&copy!=old&&research_effects::mine_growth_values(read(old),p.mines[i],v)&&original_stat_equals(copy,"get_Life",2,v.life)&&original_stat_equals(copy,"get_Available",1,1);
        ok&=pass;ui+=pass;fprintf(out,"{\"phase\":\"original_mine\",\"id\":%d,\"life_before\":%d,\"life_after\":%d,\"time_reduction\":%d,\"available_with_receipts\":%s}\n",i*2,read(old).life,v.life,p.mines[i].time,pass?"true":"false");}
    bool migrated=ok&&references(source,p,time,own,queues,alias);ok&=migrated&&before&&hash()==before;
    progress=*restore.saved.data;local_faction::active=restore.faction;research_view::invalidate();bool unchanged=!memcmp(restore.saved.data,&progress,sizeof(progress));
    fprintf(out,"{\"summary\":true,\"pass\":%s,\"candidates\":%d,\"accepted\":%d,\"prefixes\":%d,\"active_faction_predebit\":%d,\"native_mine_getters\":%d,\"original_ui_mines\":%d,\"own_mine_links\":%d,\"queued_mine_references\":%d,\"foreign_queue_alias_refused\":%s,\"timing_alone_did_not_unlock_categories\":%s,\"saved_plan\":%s,\"whole_outer_repeat_and_hp_preserved\":%s,\"canonical_and_player_state_unchanged\":%s,\"player_save_writes\":0,\"battle_runs\":0}\n",ok&&accepted==79&&prefixes==79&&predebit==79&&ui==4&&own==12&&queues==4&&unchanged?"true":"false",candidates,accepted,prefixes,predebit,checked,ui,own,queues,alias?"true":"false",no_free?"true":"false",saved_plan?"true":"false",migrated?"true":"false",unchanged&&hash()==before?"true":"false");fclose(out);
}
}
