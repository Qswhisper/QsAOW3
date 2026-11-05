// Filter immutable reward routes BEFORE the local contract projection clears
// original calendar box lists. Never export dates, progress or owned rewards.
#pragma once
namespace daily_box_content_audit {
static void capture(void*,void* cfg){
    if(access(DATA_ROOT "daily-box-content-audit.request",F_OK))return;
    unlink(DATA_ROOT "daily-box-content-audit.request");
    FILE* f=fopen(DATA_ROOT "daily-box-content-audit.jsonl","w");if(!f)return;chmod(DATA_ROOT "daily-box-content-audit.jsonl",0644);
    bool ok=cfg&&!strcmp(class_name(object_class(cfg)),"DailyConfiguration");int seen[64]={},count=0,rewards=0;
    fprintf(f,"{\"record\":\"scope\",\"before_local_projection\":true,\"player_fields_exported\":false,\"memory_writes\":0,\"level_min\":%d,\"legacy_rank_min\":%d,\"legacy_dailies_for_chest\":%d}\n",ok?*(int*)((char*)cfg+0x28):-1,ok?*(int*)((char*)cfg+0x2c):-1,ok?*(uint8_t*)((char*)cfg+0x61):-1);
    auto box=[&](void* b,const char* route){
        if(!b){fprintf(f,"{\"record\":\"route\",\"route\":\"%s\",\"box\":null}\n",route);return;}
        if(strcmp(class_name(object_class(b)),"SupplyBox")){ok=false;return;}
        int id=*(int*)((char*)b+0x10);fprintf(f,"{\"record\":\"route\",\"route\":\"%s\",\"box\":%d}\n",route,id);
        for(int i=0;i<count;++i)if(seen[i]==id)return;if(count==64){ok=false;return;}seen[count++]=id;
        fprintf(f,"{\"record\":\"box\",\"id\":%d,\"category\":%d,\"rank\":%d,\"points\":%d,\"active\":%d,\"prize_min\":%d,\"prize_max\":%d}\n",id,*(int*)((char*)b+0x14),*(int*)((char*)b+0x18),*(int*)((char*)b+0x1c),*(int8_t*)((char*)b+0x20),*(int8_t*)((char*)b+0x40),*(int8_t*)((char*)b+0x41));
        void* rows=local_supply::dictionary_list(local_supply::get(saved("SHARED"),0xa8),id);int n=rows?*(int*)((char*)rows+0x18):0;
        if(n<1||n>128){ok=false;return;}
        for(int i=0;i<n;++i){void* row=list_item(rows,i);if(!row||*(int*)((char*)row+0x14)!=id){ok=false;continue;}
            fprintf(f,"{\"record\":\"item\",\"id\":%d,\"box_id\":%d,\"prob\":%d,\"prob_txt\":%d,\"type\":%d,\"type_id\":%d,\"minimum\":%d,\"maximum\":%d}\n",*(int*)((char*)row+0x10),id,*(int*)((char*)row+0x18),*(int*)((char*)row+0x1c),*(int*)((char*)row+0x20),*(int*)((char*)row+0x24),*(int*)((char*)row+0x40),*(int*)((char*)row+0x44));++rewards;
        }
    };
    if(ok){box(local_supply::get(cfg,0x68),"legacy_chest");
        for(int offset:{0x88,0x90}){void* rows=local_supply::get(cfg,offset);int n=rows?*(int*)((char*)rows+0x18):0;
            fprintf(f,"{\"record\":\"route_count\",\"route\":\"%s\",\"count\":%d}\n",offset==0x88?"day":"week",n);
            if(n<0||n>32){ok=false;continue;}for(int i=0;i<n;++i)box(list_item(rows,i),offset==0x88?"day":"week");}
    }
    fprintf(f,"{\"summary\":true,\"pass\":%s,\"boxes\":%d,\"items\":%d,\"player_fields_exported\":false,\"player_save_writes\":0}\n",ok?"true":"false",count,rewards);fclose(f);
}
}
