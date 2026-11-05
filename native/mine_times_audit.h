#pragma once
namespace mine_times_audit {
using namespace mine_research;
struct Restore {local_progress::StateBuffer old;int faction=local_faction::active;
    Restore(){if(old.data)*old.data=progress;}
    ~Restore(){if(old.data)progress=*old.data;local_faction::active=faction;research_view::invalidate();}};
static uint64_t canonical_hash(){uint64_t h=1469598103934665603ull;for(int i=0;i<3;++i){void* p=original(i);if(!p)return 0;
    for(int k=0x10;k<0x68;++k)h=(h^((uint8_t*)p)[k])*1099511628211ull;}return h;}
static bool values(void* side,const MineTimes& d,int& checked){
    for(int i=0;i<3;++i){void* old=original(i),*p=local_dictionary_item(get(side,0x100),i*2);int expected=0;
        if(!p||p==old||!same(p,old)||!research_effects::mine_time_value(*(int*)((char*)old+0x34),d.reduction[i],expected)||
           !original_stat_equals(p,"get_SetTime",4,expected))return false;
        uint8_t bytes[0x58];memcpy(bytes,(char*)p+0x10,sizeof(bytes));memcpy(bytes+0x24,(char*)old+0x34,4);
        if(memcmp(bytes,(char*)old+0x10,sizeof(bytes)))return false;++checked;
    }return true;
}
static bool migration(void* source,const MineTimes& sum,int& links){
    preplaced_research_audit::Fixture fixture;if(!fixture.init("fmc1m"))return false;Roots roots;
    void* side=fixture.side,*foreign=local_research_runtime::clone(side,roots);if(!foreign)return false;
    field_set(side,class_field_from_name(object_class(side),"$hg"),get(source,0x100));
    field_set(side,class_field_from_name(object_class(side),"$Eg"),get(source,0xd8));
    void* prepared=local_research_runtime::clone(side,roots);int matched=0;if(!prepared||!prepare(prepared,sum,matched)||matched!=3)return false;
    void* field=class_field_from_name(object_class(fixture.battle),"$hB"),*grid_class=field?class_from_type(field_type(field)):nullptr;
    void* row_class=grid_class?class_element(grid_class):nullptr,*mine_class=find_class("com.geargames.aow.entities","Mine");
    void* grid=row_class?array_new(row_class,1):nullptr,*row=mine_class?array_new(mine_class,6):nullptr;
    if(!roots.keep(grid)||!roots.keep(row)||!fixture.array_put(grid,0,row))return false;
    field_set(fixture.battle,field,grid);void* objects[6]={};
    for(int i=0;i<6;++i){void* mine=object_new(mine_class);if(!roots.keep(mine))return false;objects[i]=mine;
        field_set(mine,class_field_from_name(mine_class,"type"),original(i/2));
        field_set(mine,class_field_from_name(mine_class,"side"),i%2?foreign:side);
        *(int16_t*)((char*)mine+0x2a)=13;if(!fixture.array_put(row,i,mine))return false;}
    preplaced_research::Migration plan;if(!plan.mines(fixture.battle,side,prepared)||plan.mine_links!=3)return false;
    for(int i=0;i<6;++i)if(get(objects[i],0x18)!=original(i/2))return false;plan.commit();
    for(int i=0;i<6;++i){void* expected=i%2?original(i/2):local_dictionary_item(get(prepared,0x100),(i/2)*2);
        if(get(objects[i],0x18)!=expected||*(int16_t*)((char*)objects[i]+0x2a)!=13)return false;}links=plan.mine_links;return true;
}
static __attribute__((noinline)) void run(){
    if(!progress_ready||access(DATA_ROOT "mine-times-audit.request",F_OK))return;unlink(DATA_ROOT "mine-times-audit.request");
    Restore restore;if(!restore.old.data)return;FILE* f=fopen(DATA_ROOT "mine-times-audit.jsonl","w");if(!f)return;chmod(DATA_ROOT "mine-times-audit.jsonl",0644);
    bool ok=local_faction::active==2&&research_view::mine_original&&research_view::ready();Roots roots;
    void* source=ok?research_view::cache.side:nullptr;ok=ok&&roots.keep(source);uint64_t hash=canonical_hash();
    MineTimes sum;int rows_count=0,checked=0,predebit=0,prefixes=0,links=0,ui=0;int ids[9]={};
    local_progress::StateBuffer receipt,loaded;if(!receipt.data||!loaded.data){fclose(f);return;}
    *receipt.data=local_progress::seed(100000000,0,0,10000);int total_cost=0;
    void* rows=generic_unit_research::levels("RESISTANCE");
    for(int i=0;rows&&i<*(int*)((char*)rows+0x18);++i){void* row=list_item(rows,i);if(!candidate(row))continue;
        MineTimes delta;bool pass=resolve(row,delta)&&shape(delta)&&purchase_supported(row)&&local_research_supported(row);if(pass)++predebit;
        int id=*(int*)((char*)row+0x10);if(rows_count>=9){ok=false;break;}ids[rows_count++]=id;
        pass=pass&&research_effects::mine_times_add(sum,delta,sum)&&shape(sum);
        Roots one;void* a=source?local_research_runtime::clone(source,one):nullptr;int matched=0;
        pass=pass&&a&&prepare(a,sum,matched)&&matched==3&&values(a,sum,checked);
        void* b=pass?local_research_runtime::clone(a,one):nullptr;int again=0;
        pass=pass&&b&&prepare(b,sum,again)&&again==3&&values(b,sum,checked);if(pass)++prefixes;
        auto bad=sum;bad.reduction[2]=original(2)?*(int*)((char*)original(2)+0x34):INT_MAX;
        void* previous=get(a,0x100);int ignored=0;pass=pass&&!prepare(a,bad,ignored)&&get(a,0x100)==previous;
        int cost=*(int*)((char*)row+0x20),xp=*(int*)((char*)row+0x28);int wallet=receipt.data->credits;
        pass=pass&&local_progress::buy_upgrade(*receipt.data,id,cost,xp)&&receipt.data->credits==wallet-cost&&
            !local_progress::buy_upgrade(*receipt.data,id,cost,xp)&&receipt.data->credits==wallet-cost;total_cost+=cost;
        fprintf(f,"{\"phase\":\"original_row\",\"faction\":2,\"id\":%d,\"owner\":111,\"kind\":\"mine\",\"cost\":%d,\"cumulative_ticks\":[%d,%d,%d],\"pass\":%s}\n",id,cost,sum.reduction[0],sum.reduction[1],sum.reduction[2],pass?"true":"false");ok&=pass;
    }
    // Original research-view mapping observes the same private receipts, with
    // canonical and synthetic/foreign references kept out of this projection.
    progress=*receipt.data;research_view::invalidate();void* owner=local_career::lookup("BATTLE","unitTypes",111);
    for(int i=0;i<3;++i){void* old=original(i),*copy=research_view::mapped_mine(old,owner);int expected=0;
        bool pass=research_effects::mine_time_value(*(int*)((char*)old+0x34),sum.reduction[i],expected)&&copy!=old&&
            original_stat_equals(copy,"get_SetTime",4,expected);
        Roots one;void* synthetic=local_research_runtime::clone(old,one);pass=pass&&synthetic&&research_view::mapped_mine(synthetic,owner)==synthetic;
        ok&=pass;ui+=pass;fprintf(f,"{\"phase\":\"original_mine\",\"id\":%d,\"name\":%d,\"base_ticks\":%d,\"reduction\":%d,\"final_ticks\":%d,\"ui_mapping\":%s}\n",i*2,*(int16_t*)((char*)old+0x14),*(int*)((char*)old+0x34),sum.reduction[i],expected,pass?"true":"false");
    }
    auto* plan=new generic_unit_research::Plan;MineTimes saved;bool saved_plan=generic_unit_research::collect(*plan)&&collect(saved)&&!memcmp(&saved,&sum,sizeof(sum));delete plan;ok&=saved_plan;
    // Malformed live rows must never reach the lower-level debit operation.
    void* original_row=rows_count?local_upgrade_level(ids[0]):nullptr,*bad_row=local_research_runtime::clone(original_row,roots);
    void* desc=string_new("{#20088<0,1>} {#20089<0,1>} {#20090<0,2>} {#20013<1>}");
    bool atomic=bad_row&&roots.keep(desc);if(atomic){field_set(bad_row,class_field_from_name(object_class(bad_row),"desc"),desc);atomic=!local_research_supported(bad_row);}ok&=atomic;
    bool migrated=ok&&migration(source,sum,links);ok&=migrated;
    // Full fresh-input preparation includes mines before existing modifications.
    const LocalScenario* route=nullptr;for(const auto& r:local_scenarios)if(r.faction==2&&r.rebase_growth){route=&r;break;}
    void* input=route?read_scenario_message(route->file,5040,"@mine-times-saved-integration"):nullptr;
    void* battle=get(input,0x20),*side=local_dictionary_item(get(battle,0x178),1);bool outer=side&&apply_local_battle_research(battle,1,true);
    ok&=outer;
    char path[512];snprintf(path,sizeof(path),DATA_ROOT "mine-times-private-%d",getpid());
    bool cold=mkdir(path,0700)==0&&local_progress::write(path,*receipt.data)&&local_progress::load(path,*loaded.data)==1&&
        !memcmp(receipt.data,loaded.data,sizeof(progress));int wallet=loaded.data->credits;
    cold=cold&&rows_count==9&&!local_progress::buy_upgrade(*loaded.data,ids[0],1300,0)&&loaded.data->credits==wallet&&wallet==100000000-total_cost;ok&=cold;
    bool mod=false;void* combined=source?local_research_runtime::clone(source,roots):nullptr;int matched=0;
    if(combined&&prepare(combined,sum,matched)&&matched==3&&local_progress::grant_stock(*loaded.data,17,1112,7)&&local_progress::equip_modification(*loaded.data,111,1112)&&local_mod_effects::prepare_side(combined,*loaded.data)){
        mod=true;for(int i=0;i<3;++i){void* mine=local_dictionary_item(get(combined,0x100),i*2);int before=*(int*)((char*)original(i)+0x34)-sum.reduction[i];mod=mod&&mine&&*(int*)((char*)mine+0x34)>0&&*(int*)((char*)mine+0x34)<before;}}
    ok&=mod&&hash&&canonical_hash()==hash;
    progress=*restore.old.data;local_faction::active=restore.faction;research_view::invalidate();bool unchanged=!memcmp(restore.old.data,&progress,sizeof(progress));
    fprintf(f,"{\"summary\":true,\"pass\":%s,\"candidates\":%d,\"accepted\":%d,\"prefixes\":%d,\"native_mine_getters\":%d,\"active_faction_predebit\":%d,\"own_mine_links\":%d,\"original_ui_mines\":%d,\"saved_plan\":%s,\"unknown_mixed_row_refused\":%s,\"fresh_outer_preparation\":%s,\"private_cold_receipt\":%s,\"existing_mine_modification_combined\":%s,\"canonical_and_player_state_unchanged\":%s,\"player_save_writes\":0,\"battle_runs\":0}\n",ok&&rows_count==9&&prefixes==9&&predebit==9&&ui==3&&links==3&&unchanged?"true":"false",rows_count,rows_count,prefixes,checked,predebit,links,ui,saved_plan?"true":"false",atomic?"true":"false",outer?"true":"false",cold?"true":"false",mod?"true":"false",unchanged&&canonical_hash()==hash?"true":"false");fclose(f);
    if(cold){char file[640];for(int i=0;i<2;++i){snprintf(file,sizeof(file),"%s/local-progress.%d",path,i);unlink(file);}rmdir(path);}
}
}
