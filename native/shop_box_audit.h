// Detached original content/getter/message audit, never claims for the player.
#pragma once
namespace shop_box_audit {
static void run(){
    if(!progress_ready||access(DATA_ROOT "shop-box-audit.request",F_OK))return;unlink(DATA_ROOT "shop-box-audit.request");auto before=progress;
    FILE* f=fopen(DATA_ROOT "shop-box-audit.jsonl","w");if(!f)return;chmod(DATA_ROOT "shop-box-audit.jsonl",0644);
    bool ok=local_shop_boxes::initialize()&&local_shop_boxes::installed;int cases=0,messages=0;
    for(int faction:{1,2})for(int stars:{0,75,360}){
        auto s=local_progress::seed(1000,30,0,stars);
        // These receipts are detached, earned-model fixtures for original gates.
        void* stones=local_career::list(local_career::key(faction),"careerStones");
        for(int i=0;stones&&i<*(int*)((char*)stones+0x18);++i){void* stone=list_item(stones,i);if(stone&&*(int*)((char*)stone+0x20)<=stars&&s.career_count<local_progress::kCareerCapacity)s.career_ids[s.career_count++]=*(int*)((char*)stone+0x10);}
        bool pass=local_shop_boxes::supported(s,faction);int ticket=0;
        for(const auto& item:shop_box_content::items)if(item.prob>0){local_supply_rewards::Candidates c;auto e=local_supply_rewards::candidates(item,s,faction,c);pass&=e!=supply_policy::Unsupported;
            if(e==supply_policy::Unsupported)fprintf(f,"{\"unsupported_item\":%d,\"faction\":%d,\"stars\":%d}\n",item.id,faction,stars);if(item.id==12019){pass&=e==supply_policy::Eligible&&c.type==15&&c.count==1&&c.ids[0]==1;++ticket;}}
        supply_policy::Draw d;supply_types::Grant g[128];int count=0;
        pass=pass&&local_shop_boxes::prepare(s,faction,d,g,count)&&shop_box_store::claim(s,d,g,count);
        local_faction::Roots roots;void* message=pass?local_supply::reward_message(s,roots,true):nullptr,*items=local_supply::get(message,0x28);
        pass=pass&&ticket==1&&items&&*(int*)((char*)items+0x18)==count&&*(bool*)((char*)message+0x45)&&!s.shop.games&&s.shop.pending;
        for(int i=0;items&&i<count;++i){void* row=list_item(items,i);pass&=row&&original_stat_equals(row,"get_Type",1,g[i].type)&&original_stat_equals(row,"get_TypeId",4,g[i].id)&&original_stat_equals(row,"get_Amount",4,g[i].amount);}
        ok&=pass;++cases;messages+=message!=nullptr;
        fprintf(f,"{\"phase\":\"original_free_box\",\"faction\":%d,\"stars\":%d,\"pass\":%s,\"grants\":%d,\"original_message\":%s}\n",faction,stars,pass?"true":"false",count,message?"true":"false");
    }
    auto s=local_progress::seed(0,0,0,0);supply_policy::Draw ticket;ticket.count=2;ticket.rows[0]={12000,200};ticket.rows[1]={12019,1};supply_types::Grant g[]={{0,0,200},{15,1,1}};
    bool pass=shop_box_store::claim(s,ticket,g,2)&&!local_progress::stock_amount(s,15,1);local_faction::Roots roots;
    void* message=pass?local_supply::reward_message(s,roots,true):nullptr,*items=local_supply::get(message,0x28),*row=items?list_item(items,1):nullptr;
    pass=pass&&row&&original_stat_equals(row,"get_Type",1,15)&&original_stat_equals(row,"get_Amount",4,1)&&local_items::text_equal(local_supply::get(row,0x28),"单机无需赛事入场券");
    ok&=pass;fprintf(f,"{\"phase\":\"obsolete_ticket_receipt\",\"pass\":%s,\"ticket_stock\":0,\"original_type\":15}\n",pass?"true":"false");
    bool unchanged=!memcmp(&before,&progress,sizeof(progress));
    fprintf(f,"{\"summary\":true,\"pass\":%s,\"cases\":%d,\"messages\":%d,\"player_state_unchanged\":%s,\"player_save_writes\":0,\"battle_runs\":0}\n",ok&&unchanged?"true":"false",cases,messages,unchanged?"true":"false");fclose(f);
}
}
