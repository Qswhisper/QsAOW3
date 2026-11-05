#pragma once
namespace effective_box_audit {
using Roots=local_faction::Roots;
static bool preview(const supply_content::Box& b,const supply_content::Item* pool,int& shown,int& removed){
    void* raw=local_supply::dictionary_list(local_supply::get(saved("SHARED"),0xa8),b.id);
    void* projected=local_supply::dictionary_list(effective_boxes::context_items(),b.id);
    int count=raw?*(int*)((char*)raw+0x18):0,output=projected?*(int*)((char*)projected+0x18):0;
    if(count!=b.count)return false;int cursor=0;
    for(int i=0;i<count;++i){const auto& d=pool[b.first+i];void* row=local_supply::original_item(b.id,d.id);
        if(!row||*(int*)((char*)row+0x18)!=d.prob||*(int*)((char*)row+0x1c)!=d.prob_txt||*(int*)((char*)row+0x20)!=d.type||*(int*)((char*)row+0x24)!=d.type_id||*(int*)((char*)row+0x40)!=d.minimum||*(int*)((char*)row+0x44)!=d.maximum)return false;
        if(effective_box_policy::removed(d.type,d.type_id)){++removed;continue;}
        if(cursor>=output||list_item(projected,cursor++)!=row)return false;++shown;
    }return cursor==output;
}
static bool clean_draw(const supply_content::Box& b,const supply_content::Item* pool,const supply_policy::Draw& d,const supply_types::Grant* grants,int n){
    if(!supply_policy::valid_draw_pool(b,pool,d,true))return false;
    for(int i=0;i<n;++i)if(!effective_box_policy::future_grant(grants[i].type))return false;return n>0;
}
static bool message(const local_progress::State& s,const supply_types::Grant* g,int n,bool shop,bool contract){
    Roots roots;void* msg=local_supply::reward_message(s,roots,shop,contract),*rows=local_supply::get(msg,0x28);
    if(!msg||!rows||*(int*)((char*)rows+0x18)!=n||!*(bool*)((char*)msg+0x45))return false;
    for(int i=0;i<n;++i){void* v=list_item(rows,i);
        if(!original_stat_equals(v,"get_Type",1,g[i].type)||!original_stat_equals(v,"get_TypeId",4,g[i].id)||!original_stat_equals(v,"get_Amount",4,g[i].amount))return false;}
    return true;
}
static void run(){
    if(!progress_ready||access(DATA_ROOT "effective-box-audit.request",F_OK))return;unlink(DATA_ROOT "effective-box-audit.request");
    local_progress::StateBuffer initial;if(!initial.data)return;*initial.data=progress;
    FILE* f=fopen(DATA_ROOT "effective-box-audit.jsonl","w");if(!f)return;chmod(DATA_ROOT "effective-box-audit.jsonl",0644);
    bool ok=effective_boxes::installed&&local_supply::initialize()&&local_shop_boxes::initialize()&&local_contract_boxes::initialize()&&effective_boxes::publish();
    int boxes=0,shown=0,removed=0;
    if(ok){for(const auto& b:supply_content::boxes){ok=preview(b,supply_content::items,shown,removed)&&ok;++boxes;}
        ok=preview(shop_box_content::box,shop_box_content::items,shown,removed)&&ok;++boxes;
        for(const auto& b:contract_box_content::boxes){ok=preview(b,contract_box_content::items,shown,removed)&&ok;++boxes;}}
    fprintf(f,"{\"phase\":\"original_preview_projection\",\"pass\":%s,\"retained_definition_slots\":%d,\"preview_items\":%d,\"removed_preview_rows\":%d,\"canonical_source_unchanged\":%s}\n",ok?"true":"false",boxes,shown,removed,ok?"true":"false");
    int cases=0;void* rank=local_career::lookup("SHARED","profileRanks",4);
    for(int faction:{1,2}){
        local_progress::StateBuffer storage;if(!storage.data){ok=false;break;}auto& s=*storage.data;
        s=local_progress::seed(1000,10,0,rank?*(int*)((char*)rank+0x14):0);s.quest_warpoints=200;
        bool pass=local_progress::grant_stock(s,11,0,5)&&local_progress::grant_stock(s,7,4,6)&&contract_box_policy::initialize(s.contract_boxes,0);s.contract_points=1350;
        for(int service=0;pass&&service<3;++service)for(int i=0;i<(service==2?7:1);++i){
            const auto* b=service==0?supply_policy::box(401):service==1?&shop_box_content::box:&contract_box_content::boxes[i];
            const auto* pool=service==0?supply_content::items:service==1?shop_box_content::items:contract_box_content::items;
            uint64_t claims=service==0?s.supply.claims:service==1?s.shop.claims:s.contract_boxes.claims;
            supply_policy::Draw draw;supply_types::Grant g[128];int n=0;
            pass=local_supply_rewards::supported_pool(*b,pool,s,faction)&&local_supply_rewards::prepare_pool(*b,pool,claims,s,faction,draw,g,128,n)&&clean_draw(*b,pool,draw,g,n);
            if(pass)pass=service==0?supply_store::claim(s,4,b->id,draw,g,n):service==1?shop_box_store::claim(s,draw,g,n):contract_box_store::claim(s,i<5?0:1,4,b->id,draw,g,n);
            pass=pass&&message(s,g,n,service==1,service==2)&&local_progress::stock_amount(s,11,0)==5&&local_progress::stock_amount(s,7,4)==6;
            fprintf(f,"{\"phase\":\"future_original_message\",\"faction\":%d,\"box\":%d,\"pass\":%s,\"items\":%d,\"grants\":%d}\n",faction,b->id,pass?"true":"false",draw.count,n);++cases;
            s.supply.pending=s.shop.pending=s.contract_boxes.pending=0;ok&=pass;
        }
    }
    int history=0;
    const int old_ids[]={12003,12010,12019};const supply_types::Grant old_gifts[]={{7,4,1},{11,0,1},{15,1,1}};
    for(int i=0;i<3;++i){local_progress::StateBuffer storage,restored;if(!storage.data||!restored.data){ok=false;break;}auto& s=*storage.data;
        s=local_progress::seed(300,0,0,9);bool pass=local_progress::grant_stock(s,11,0,5)&&local_progress::grant_stock(s,7,4,6);
        auto& l=s.shop;l.claims=1;l.pending=1;l.draw_count=l.grant_count=2;l.draws[0]={12000,300};l.draws[1]={old_ids[i],1};l.grants[0]={0,0,300};l.grants[1]=old_gifts[i];
        char path[640];snprintf(path,sizeof(path),DATA_ROOT "effective-box-private-XXXXXX");
        pass=pass&&message(s,l.grants,2,true,false)&&mkdtemp(path)&&local_progress::write(path,s)&&local_progress::load(path,*restored.data)==1&&!memcmp(&s,restored.data,sizeof(s))&&message(*restored.data,restored.data->shop.grants,2,true,false);
        fprintf(f,"{\"phase\":\"historical_pending_cold_restore\",\"type\":%d,\"pass\":%s,\"private_root\":\"%s\"}\n",old_gifts[i].type,pass?"true":"false",path);ok&=pass;++history;
    }
    bool unchanged=!memcmp(initial.data,&progress,sizeof(progress));
    fprintf(f,"{\"summary\":true,\"pass\":%s,\"future_messages\":%d,\"legacy_pending_messages\":%d,\"player_unchanged\":%s,\"player_save_writes\":0,\"battle_runs\":0}\n",ok&&unchanged?"true":"false",cases,history,unchanged?"true":"false");fclose(f);
}
}
