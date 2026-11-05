// Original getter/message contracts on detached models, never player claims.
#pragma once
namespace competition_track_audit {
static void run(){
    if(!progress_ready||access(DATA_ROOT "competition-track-audit.request",F_OK))return;unlink(DATA_ROOT "competition-track-audit.request");
    FILE* f=fopen(DATA_ROOT "competition-track-audit.jsonl","w");if(!f)return;chmod(DATA_ROOT "competition-track-audit.jsonl",0644);
    local_progress::StateBuffer before,fixture;bool ok=before.data&&fixture.data;if(!ok){fclose(f);return;}*before.data=progress;
    ok=local_competition_tracks::installed&&local_competition_tracks::content()&&local_faction::active==2;
    *fixture.data=local_progress::seed(1000,30,0,110);bool changed=false;
    ok=ok&&local_skill::rank(*fixture.data)==7;
    for(uint64_t i=1;ok&&i<=20;++i)ok=competition_track_policy::begin(fixture.data->competition_tracks,10140,101406,i)&&
        competition_track_policy::settle(fixture.data->competition_tracks,10140,i,true,20,changed)&&changed;
    int cells=0,boxes=0,messages=0,obsolete=0;
    for(int premium=0;ok&&premium<2;++premium)for(int row=0;ok&&row<20;++row){
        const auto& d=competition_track_content::rewards[premium][row];supply_types::Grant grants[128];int count=0;
        bool pass=local_competition_tracks::prepare(*fixture.data,row,premium,grants,count)&&competition_track_store::claim(*fixture.data,row,premium,grants,count);
        local_faction::Roots roots;void* p=pass?local_competition_tracks::model(*fixture.data,roots):nullptr;
        void* msg=pass?local_competition_tracks::message(*fixture.data,roots):nullptr;
        pass=pass&&p&&msg&&original_stat_equals(p,"get_Progress",4,20)&&original_stat_equals(p,"get_PremiumUnlocked",1,1);
        bool send_error=false;void* check_args[]={p,&send_error};void* ex=nullptr;
        void* checked=p?runtime_invoke(class_method(find_class("com.geargames.extensions","ConvertedCodeExtensions"),"TryCheckBattlePassInfo",2),nullptr,check_args,&ex):nullptr;
        pass=pass&&checked&&!ex&&*(bool*)((char*)checked+16);
        void* info=local_supply::get(p,0x10),*list=local_supply::get(info,premium?0x28:0x20);
        pass=pass&&list&&*(int*)((char*)list+0x18)==20&&local_supply::get(info,0x18);
        void* taken=local_supply::get(p,premium?0x20:0x18);
        pass=pass&&taken&&array_length(taken)==3&&(*((uint8_t*)taken+32+(row>>3))&(1u<<(row&7)));
        void* items=local_supply::get(msg,d.type==14?0x28:0x20);int n=items?*(int*)((char*)items+0x18):0;
        pass=pass&&items&&n==(count?count:1);for(int i=0;i<count;++i){void* r=list_item(items,i);pass=pass&&r&&original_stat_equals(r,"get_Type",1,grants[i].type)&&
            original_stat_equals(r,"get_TypeId",4,grants[i].id)&&original_stat_equals(r,"get_Amount",4,grants[i].amount);}
        if(d.type==14){++boxes;pass=pass&&*(bool*)((char*)msg+0x45)&&!*(int*)((char*)msg+0x5c);}
        if(local_competition_tracks::obsolete(d)){++obsolete;pass=pass&&!count;}
        if(msg)++messages;++cells;ok=pass;
        fprintf(f,"{\"phase\":\"original_track_model_and_message\",\"row\":%d,\"premium\":%d,\"type\":%d,\"id\":%d,\"grants\":%d,\"pass\":%s}\n",row,premium,d.type,d.id,count,pass?"true":"false");
        if(pass)ok=competition_track_policy::dismiss(fixture.data->competition_tracks);
    }
    bool unchanged=!memcmp(before.data,&progress,sizeof(progress));
    fprintf(f,"{\"summary\":true,\"complete\":true,\"pass\":%s,\"cells\":%d,\"boxes\":%d,\"messages\":%d,\"obsolete_cells\":%d,\"player_unchanged\":%s,\"player_save_writes\":0,\"battle_runs\":0,\"growth_version\":%u,\"growth_size\":%zu}\n",ok&&unchanged&&cells==40?"true":"false",cells,boxes,messages,obsolete,unchanged?"true":"false",local_progress::kVersion,sizeof(progress));fclose(f);
}
}
