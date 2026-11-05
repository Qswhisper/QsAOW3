// Detached original inventory sources/messages; never claim or save player state.
#pragma once
namespace item_inventory_audit {
static bool amount(void* dictionary,void* key,int expected){
    void* ex=nullptr;int value=-1;void* args[]={key,&value};void* found=dictionary?invoke_named(dictionary,"TryGetValue",2,args,&ex):nullptr;
    return found&&!ex&&*(bool*)((char*)found+16)&&value==expected;
}
static void run(){
    if(!progress_ready||access(DATA_ROOT "item-inventory-audit.request",F_OK))return;unlink(DATA_ROOT "item-inventory-audit.request");auto before=progress;
    FILE* f=fopen(DATA_ROOT "item-inventory-audit.jsonl","w");if(!f)return;chmod(DATA_ROOT "item-inventory-audit.jsonl",0644);
    bool ok=local_items::use_guards_installed&&local_items::initialize()&&local_items::rebind_speedups();int cases=0,messages=0,blocked=0,new_types=0;
    void* user=saved("UserConfiguration");void* original_hero=local_items::get(user,0x110),*original_upgrade=local_items::get(user,0x100);
    void* original_batteries=local_items::get(original_hero,0x18),*original_speed=local_items::get(original_upgrade,0x30);
    local_faction::Roots roots;void* copy=local_faction::clone(user,roots);auto inventory=local_progress::seed(0,0,0,9);
    for(int id=1;id<=4;++id)ok=local_progress::grant_stock(inventory,7,id,id+2)&&ok;
    for(int id=1;id<=3;++id)ok=local_progress::grant_stock(inventory,10,id,id+4)&&ok;
    ok=local_progress::grant_stock(inventory,11,0,7)&&copy&&local_items::prepare(copy,inventory)&&ok;
    for(int id=1;id<=4;++id)ok=amount(local_items::get(local_items::get(copy,0x100),0x30),local_items::speedup(id),id+2)&&ok;
    for(int id=1;id<=3;++id)ok=amount(local_items::get(local_items::get(copy,0x110),0x18),local_items::battery(id),id+4)&&ok;
    bool independent=local_items::get(user,0x110)==original_hero&&local_items::get(user,0x100)==original_upgrade&&local_items::get(original_hero,0x18)==original_batteries&&local_items::get(original_upgrade,0x30)==original_speed;
    ok=ok&&independent;
    fprintf(f,"{\"phase\":\"original_typed_inventory\",\"pass\":%s,\"speedup_definitions\":4,\"battery_definitions\":3,\"source_isolated\":%s,\"consumption_enabled\":false}\n",ok?"true":"false",independent?"true":"false");
    for(int faction:{1,2})for(int rank:{1,6}){void* definition=local_career::lookup("SHARED","profileRanks",rank);
        auto state=local_progress::seed(0,0,0,definition?*(int*)((char*)definition+0x14):0);state.quest_warpoints=10000;
        for(int id:{401,402,403,404,405,406}){const auto* box=supply_policy::box(id);supply_policy::Draw draw,repeated;supply_types::Grant grants[128],again[128];int count=0,n=0;
            bool pass=box&&local_supply_rewards::supported(*box,state,faction)&&local_supply_rewards::prepare(*box,state,faction,draw,grants,128,count)&&local_supply_rewards::prepare(*box,state,faction,repeated,again,128,n)&&count==n&&!memcmp(grants,again,n*sizeof(grants[0]))&&draw.count==repeated.count&&!memcmp(draw.rows,repeated.rows,draw.count*sizeof(draw.rows[0]));
            for(int i=0;pass&&i<count;++i){const auto& g=grants[i];pass=g.type==2||local_career::supported(user,{g.type,g.id,g.amount},state.stars,faction);if(g.type==7||g.type==10||g.type==11)++new_types;}
            pass=pass&&supply_store::claim(state,rank,id,draw,grants,count);local_faction::Roots message_roots;void* message=pass?local_supply::reward_message(state,message_roots):nullptr,*list=local_supply::get(message,0x28);
            pass=pass&&message&&list&&*(int*)((char*)list+0x18)==count&&*(bool*)((char*)message+0x45)&&!*(int*)((char*)message+0x5c);
            for(int i=0;pass&&i<count;++i){void* item=list_item(list,i);pass=item&&original_stat_equals(item,"get_Amount",4,grants[i].amount)&&original_stat_equals(item,"get_TypeId",4,grants[i].id);}
            if(message)++messages;ok=ok&&pass;++cases;state.supply.pending=0;
            fprintf(f,"{\"phase\":\"private_original_reward\",\"faction\":%d,\"rank\":%d,\"box\":%d,\"grants\":%d,\"pass\":%s}\n",faction,rank,id,count,pass?"true":"false");
        }
        ok=ok&&state.quest_warpoints==6000&&state.supply.cycle==1&&state.supply.claims==6&&!state.supply.group&&!state.supply.step;
        const supply_content::Box* next=nullptr;int cost=0;ok=ok&&supply_policy::next(state.supply,rank,next,cost)&&next->id==401&&cost==200;
    }
    for(const auto& group:supply_policy::groups)if(group.rank>=7){auto s=local_progress::seed(0,0,0,0);void* r=local_career::lookup("SHARED","profileRanks",group.rank);s.stars=r?*(int*)((char*)r+0x14):0;
        for(int i=0;i<group.count;++i){const auto* b=supply_policy::box(group.boxes[i]);bool modification=false;
            for(int j=0;b&&j<b->count;++j){const auto& item=supply_content::items[b->first+j];modification=modification||(item.type==26&&item.prob>0);}
            bool enabled=b&&local_supply_rewards::supported(*b,s,1);ok=ok&&(enabled!=modification);if(!enabled)++blocked;
            fprintf(f,"{\"phase\":\"higher_rank_service\",\"rank\":%d,\"box\":%d,\"enabled\":%s,\"contains_modification\":%s,\"pass\":%s}\n",group.rank,group.boxes[i],enabled?"true":"false",modification?"true":"false",enabled!=modification?"true":"false");}}
    local_item_guards::consume(nullptr,nullptr,nullptr); // Offline rejection cannot reach the native server command.
    bool isolated=!memcmp(&before,&progress,sizeof(progress));local_supply::list_for(progress,local_faction::active);
    fprintf(f,"{\"summary\":true,\"pass\":%s,\"private_reward_cases\":%d,\"native_messages\":%d,\"new_grant_rows\":%d,\"unsupported_mod_boxes\":%d,\"use_guards\":%s,\"runtime_version\":%u,\"runtime_size\":%zu,\"player_state_unchanged\":%s,\"player_save_writes\":0,\"player_claims\":0,\"battle_runs\":0}\n",ok&&isolated&&new_types>0?"true":"false",cases,messages,new_types,blocked,local_items::use_guards_installed?"true":"false",progress.version,sizeof(progress),isolated?"true":"false");fclose(f);
}
}
