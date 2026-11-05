// Original pool eligibility/labels and detached mixed reward messages.
#pragma once
namespace obsolete_speedup_audit {
static void run(){
    if(!progress_ready||access(DATA_ROOT "obsolete-speedup-audit.request",F_OK))return;
    unlink(DATA_ROOT "obsolete-speedup-audit.request");auto before=progress;
    FILE* f=fopen(DATA_ROOT "obsolete-speedup-audit.jsonl","w");if(!f)return;chmod(DATA_ROOT "obsolete-speedup-audit.jsonl",0644);
    bool ok=local_career::obsolete_speedup_gifts&&local_items::initialize();int rows=0,mask=0,messages=0;
    for(const auto& box:supply_content::boxes)for(int j=0;j<box.count;++j){const auto& item=supply_content::items[box.first+j];int id=local_items::speedup_id(item.type);if(!id||item.prob<=0)continue;
        auto state=local_progress::seed(0,0,0,9);local_supply_rewards::Candidates pool;
        bool pass=local_supply_rewards::candidates(item,state,1,pool)==supply_policy::Eligible&&pool.type==7&&pool.count==1&&pool.ids[0]==id;
        void* source=local_supply::original_item(box.id,item.id);local_faction::Roots roots;
        void* label=source?local_modifications::box_amount(source,nullptr):nullptr;char expected[160];
        if(item.minimum==item.maximum)snprintf(expected,sizeof(expected),"×%d 单机研究已即时完成",item.minimum);
        else snprintf(expected,sizeof(expected),"×%d～%d 单机研究已即时完成",item.minimum,item.maximum);
        pass=pass&&roots.keep(label)&&local_items::text_equal(label,expected);ok&=pass;++rows;mask|=1<<(id-1);
        fprintf(f,"{\"pool_row\":true,\"box\":%d,\"item\":%d,\"battery_unrelated\":true,\"speedup\":%d,\"weight\":%d,\"minimum\":%d,\"maximum\":%d,\"pass\":%s}\n",box.id,item.id,id,item.prob,item.minimum,item.maximum,pass?"true":"false");
    }
    for(int id=1;id<=4;++id){local_faction::Roots roots;char expected[160];snprintf(expected,sizeof(expected),"×%d 单机研究已即时完成",id+2);
        void* label=local_modifications::amount(7,id+2,nullptr,nullptr);bool pass=roots.keep(label)&&local_items::text_equal(label,expected);
        auto state=local_progress::seed(0,0,0,9);local_progress::grant_stock(state,7,id,5);local_progress::CareerReward r{7,id,id+2};
        pass=pass&&local_career::supported(saved("UserConfiguration"),r,9,2)&&local_progress::claim_career(state,90000+id,0,&r,1)&&local_progress::stock_amount(state,7,id)==5;
        auto again=state;pass=pass&&!local_progress::claim_career(state,90000+id,0,&r,1)&&!memcmp(&state,&again,sizeof(state));ok&=pass;
        fprintf(f,"{\"career\":true,\"id\":%d,\"pass\":%s}\n",id,pass?"true":"false");
    }
    for(int faction:{1,2}){
        auto state=local_progress::seed(0,0,0,18);state.quest_warpoints=10000;
        for(int id=1;id<=4;++id)local_progress::grant_stock(state,7,id,id);
        for(int step=0;step<6;++step){local_faction::Roots roots;const supply_content::Box* box=nullptr;int cost=0,count=0;
            supply_policy::Draw draw;supply_types::Grant grants[128];
            bool pass=supply_policy::next(state.supply,3,box,cost)&&local_supply_rewards::prepare(*box,state,faction,draw,grants,128,count)&&supply_store::claim(state,3,box->id,draw,grants,count);
            for(int id=1;id<=4;++id)pass=pass&&local_progress::stock_amount(state,7,id)==id;
            void* message=pass?local_supply::reward_message(state,roots):nullptr,*items=local_supply::get(message,0x28);
            pass=pass&&items&&*(int*)((char*)items+0x18)==count&&*(bool*)((char*)message+0x45);
            if(items)for(int i=0;i<count;++i){void* item=list_item(items,i);pass=pass&&item&&original_stat_equals(item,"get_Amount",4,grants[i].amount)&&original_stat_equals(item,"get_TypeId",4,grants[i].id);}
            ok&=pass;++messages;state.supply.pending=0;
            fprintf(f,"{\"message\":true,\"faction\":%d,\"step\":%d,\"pass\":%s}\n",faction,step,pass?"true":"false");
        }
    }
    bool unchanged=!memcmp(&before,&progress,sizeof(progress));
    fprintf(f,"{\"summary\":true,\"pass\":%s,\"complete\":true,\"original_pool_rows\":%d,\"speedup_mask\":%d,\"native_messages\":%d,\"player_unchanged\":%s,\"player_save_writes\":0,\"scene_runs\":0}\n",ok&&mask==15&&unchanged?"true":"false",rows,mask,messages,unchanged?"true":"false");fclose(f);
}
}
