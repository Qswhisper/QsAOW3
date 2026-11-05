// Original category4 box receipts -> original Daily count/reward, private only.
#pragma once
namespace free_box_contract_audit {
using local_contracts::Roots;
static bool open(local_progress::State& s,int faction){
    if(s.shop.claims){s.shop.pending=0;for(int i=0;i<3;++i){uint64_t run=s.shop.allowed[0]+1;bool changed=false;
        if(!shop_box_policy::begin(s.shop,shop_box_policy::Campaign,run)||!shop_box_policy::finish(s.shop,shop_box_policy::Campaign,run,true,changed)||!changed)return false;}}
    supply_policy::Draw draw;supply_types::Grant grants[128];int count=0;
    return local_shop_boxes::prepare(s,faction,draw,grants,count)&&shop_box_store::claim(s,draw,grants,count);
}
static void run(){
    if(!progress_ready||access(DATA_ROOT "free-box-contract-audit.request",F_OK))return;unlink(DATA_ROOT "free-box-contract-audit.request");
    local_progress::StateBuffer before,fixture,active,loaded;if(!before.data||!fixture.data||!active.data||!loaded.data)return;*before.data=progress;
    void* user=saved("UserConfiguration"),*original=local_contracts::get(user,0x128);Roots roots;
    int index=local_contracts::index(20020);const auto* d=index>=0?contract_content::definitions+index:nullptr;
    void* box=local_supply::original_box(12000),*rank=local_career::lookup("SHARED","profileRanks",4);
    bool ok=d&&d->kind==4&&d->type==14&&d->subtype==0&&d->param==4&&d->value==3&&d->points==10&&d->xp==0&&
        box&&original_stat_equals(box,"get_Category",4,4)&&rank;
    *fixture.data=local_progress::seed(0,0,0,rank?*(int*)((char*)rank+0x14):0);auto& s=*active.data;
    bool old_open=ok&&open(*fixture.data,local_faction::active);
    bool setup=old_open&&local_contracts::prepare_state(*fixture.data,s)&&contract_policy::receipt(s,20020)&&local_contracts::eligible(*d,s);
    void* first=setup?local_contracts::daily(s,index,roots):nullptr;
    bool old_ignored=first&&*(int*)((char*)first+0x1c)==0&&*(int8_t*)((char*)first+0x18)==0;
    bool counts=old_ignored;int opens=0;
    for(int n=1;n<=3&&counts;++n){if(!open(s,local_faction::active)){counts=false;break;}++opens;
        void* model=local_contracts::daily(s,index,roots);
        counts=model&&*(int*)((char*)model+0x1c)==n&&*(int8_t*)((char*)model+0x18)==(n==3?1:0);
        uint64_t same=s.shop.claims;void* repeat=local_contracts::daily(s,index,roots);
        counts=counts&&repeat&&s.shop.claims==same&&*(int*)((char*)repeat+0x1c)==n;
    }
    s.shop.pending=0;int credit=0,gold=0;int wallet=s.credits,points=s.contract_points;
    bool paid=counts&&local_contracts::reward(s,*d,credit,gold)&&credit==600&&!gold&&
        contract_policy::claim(s,20020,local_contracts::counter(s,*d),3,credit,gold,0,10)&&s.credits==wallet+600&&s.contract_points==points+10;
    *loaded.data=s;bool duplicate=paid&&!contract_policy::claim(s,20020,local_contracts::counter(s,*d),3,credit,gold,0,10)&&!memcmp(&s,loaded.data,sizeof(s));
    void* reset=paid?local_contracts::daily(s,index,roots):nullptr;
    bool zero=reset&&*(int*)((char*)reset+0x1c)==0&&*(int8_t*)((char*)reset+0x18)==0;
    char directory[640];snprintf(directory,sizeof(directory),DATA_ROOT "free-box-contract-private-XXXXXX");
    bool cold=paid&&mkdtemp(directory)&&local_progress::write(directory,s)&&local_progress::load(directory,*loaded.data)==1&&
        !memcmp(&s,loaded.data,sizeof(s))&&contract_policy::count(*loaded.data,20020,local_contracts::counter(*loaded.data,*d),3)==0;
    bool fresh=cold&&open(*loaded.data,local_faction::active)&&contract_policy::count(*loaded.data,20020,local_contracts::counter(*loaded.data,*d),3)==1;
    bool isolated=!memcmp(before.data,&progress,sizeof(progress))&&local_contracts::get(user,0x128)==original;
    ok=ok&&setup&&old_ignored&&counts&&opens==3&&paid&&duplicate&&zero&&cold&&fresh&&isolated;
    FILE* f=fopen(DATA_ROOT "free-box-contract-audit.jsonl","w");if(!f)return;chmod(DATA_ROOT "free-box-contract-audit.jsonl",0644);
    fprintf(f,"{\"summary\":true,\"pass\":%s,\"contract\":20020,\"original_box\":12000,\"category\":4,\"old_open_ignored\":%s,\"new_private_opens\":%d,\"original_daily_counts\":%s,\"original_credits\":%d,\"original_points\":10,\"duplicate_refused\":%s,\"next_original_daily_zero\":%s,\"cold_store\":%s,\"next_new_open_count_one\":%s,\"player_state_unchanged\":%s,\"player_save_writes\":0,\"player_box_opens\":0,\"battle_runs\":0}\n",ok?"true":"false",old_ignored?"true":"false",opens,counts?"true":"false",credit,duplicate?"true":"false",zero?"true":"false",cold?"true":"false",fresh?"true":"false",isolated?"true":"false");fclose(f);
}
}
