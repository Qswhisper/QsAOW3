#pragma once
namespace contract_box_audit {
using Roots=local_faction::Roots;
static void run(){
    if(!progress_ready||access(DATA_ROOT "contract-box-audit.request",F_OK))return;unlink(DATA_ROOT "contract-box-audit.request");
    local_progress::StateBuffer initial;if(!initial.data)return;*initial.data=progress;
    FILE* f=fopen(DATA_ROOT "contract-box-audit.jsonl","w");if(!f)return;chmod(DATA_ROOT "contract-box-audit.jsonl",0644);
    bool ok=local_contract_boxes::installed&&local_contract_boxes::initialize();int cases=0;void* rank=local_career::lookup("SHARED","profileRanks",4);
    for(int faction:{1,2}){
        local_progress::StateBuffer storage;if(!storage.data){ok=false;break;}auto& s=*storage.data;s=local_progress::seed(1000,10,0,rank?*(int*)((char*)rank+0x14):0);
        s.contract_points=777;bool pass=local_skill::rank(s)==4&&contract_box_policy::initialize(s.contract_boxes,s.contract_points);s.contract_points+=1350;
        Roots roots;void* user=saved("UserConfiguration"),*cfg=local_faction::clone(local_supply::get(user,0x128),roots);
        pass=pass&&cfg&&local_faction::empty_field(cfg,"boxes",roots)&&local_faction::empty_field(cfg,"boxesWeek",roots)&&local_contract_boxes::configuration(cfg,s,roots);
        fprintf(f,"{\"phase\":\"setup\",\"faction\":%d,\"rank\":%d,\"pass\":%s}\n",faction,local_skill::rank(s),pass?"true":"false");
        pass=pass&&*(int*)((char*)local_supply::get(cfg,0x88)+0x18)==5&&*(int*)((char*)local_supply::get(cfg,0x90)+0x18)==2&&*(int*)((char*)cfg+0x80)==1350&&*(int*)((char*)cfg+0x84)==1350;
        for(int track=0;pass&&track<2;++track){int n=track?2:5;for(int i=0;pass&&i<n;++i){const auto* b=contract_box_policy::next(s.contract_boxes,track,4);
            supply_policy::Draw d;supply_types::Grant g[128];int count=0;
            pass=b&&local_supply_rewards::supported_pool(*b,contract_box_content::items,s,faction)&&local_supply_rewards::prepare_pool(*b,contract_box_content::items,s.contract_boxes.claims,s,faction,d,g,128,count)&&contract_box_store::claim(s,track,4,b->id,d,g,count);
            Roots one;void* message=pass?local_supply::reward_message(s,one,false,true):nullptr,*items=local_supply::get(message,0x28),*box=local_supply::get(message,0x20);
            pass=pass&&items&&box&&*(int*)((char*)items+0x18)==count&&*(bool*)((char*)message+0x45)&&original_stat_equals(box,"get_Category",4,track?11:10)&&original_stat_equals(box,"get_Points",4,b?b->points:0);
            for(int j=0;pass&&j<count;++j){void* row=list_item(items,j);pass=original_stat_equals(row,"get_Type",1,g[j].type)&&original_stat_equals(row,"get_TypeId",4,g[j].id)&&original_stat_equals(row,"get_Amount",4,g[j].amount);}
            ++cases;fprintf(f,"{\"phase\":\"original_contract_box_message\",\"faction\":%d,\"box\":%d,\"pass\":%s,\"grants\":%d}\n",faction,b?b->id:0,pass?"true":"false",count);s.contract_boxes.pending=0;
        }}
        pass=pass&&s.contract_boxes.tracks[0].cycles==1&&s.contract_boxes.tracks[1].cycles==1&&contract_box_policy::balance(s.contract_boxes,0,s.contract_points)==1150&&contract_box_policy::balance(s.contract_boxes,1,s.contract_points)==50;
        char directory[640];snprintf(directory,sizeof(directory),DATA_ROOT "contract-box-private-XXXXXX");local_progress::StateBuffer restored;
        pass=pass&&restored.data&&mkdtemp(directory)&&local_progress::write(directory,s)&&local_progress::load(directory,*restored.data)==1&&!memcmp(&s,restored.data,sizeof(s));
        fprintf(f,"{\"phase\":\"two_rows_and_private_cold_store\",\"faction\":%d,\"pass\":%s,\"root\":\"%s\"}\n",faction,pass?"true":"false",directory);ok&=pass;
    }
    bool unchanged=!memcmp(initial.data,&progress,sizeof(progress));
    fprintf(f,"{\"summary\":true,\"pass\":%s,\"cases\":%d,\"player_state_unchanged\":%s,\"player_save_writes\":0,\"battle_runs\":0}\n",ok&&unchanged?"true":"false",cases,unchanged?"true":"false");fclose(f);
}
}
