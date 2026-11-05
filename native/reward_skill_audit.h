// Original integer oracle + legal skill receipts + real reward-service paths.
// Every state, ledger and save below is private; no player battle/result/grant.
#pragma once
namespace reward_skill_audit {
static void run(){
    if(access(DATA_ROOT "reward-skill-audit.request",F_OK))return;unlink(DATA_ROOT "reward-skill-audit.request");
    FILE* f=fopen(DATA_ROOT "reward-skill-audit.jsonl","w");if(!f)return;chmod(DATA_ROOT "reward-skill-audit.jsonl",0644);
    local_progress::StateBuffer before,a,b,c;if(!before.data||!a.data||!b.data||!c.data){fclose(f);return;}*before.data=progress;
    bool ok=local_reward_skills::installed&&local_skill::initialize(),saved=false,atomic=false;int oracle=0,results=0,boxes=0,career=0;
    void* controller=find_class("com.geargames.aow.scenes.main.gui.skills","SkillsController");
    const void* method=controller?class_method(controller,"GetBoostedValueInt",3):nullptr;ok=ok&&method;
    for(int card_id:{24,36})for(int level=0;ok&&level<=11;++level){local_research_runtime::Roots roots;void* ex=nullptr;
        void* card=local_career::skill_card(card_id),*skill=object_new(find_class("com.geargames.aow.entities","Skill"));int max=11,stock=0;
        ok=card&&roots.keep(skill);void* ctor[]={card,&level,&max,&stock};if(ok)invoke_named(skill,".ctor",4,ctor,&ex);
        int effect=card?*(int*)((char*)card+0x38):0;
        for(int base:{0,1,33,100,200,10049,99999}){int expected=0;bool visual=true;void* args[]={&base,skill,&visual};
            void* result=ok&&!ex?runtime_invoke(method,nullptr,args,&ex):nullptr;
            bool pass=result&&!ex&&contract_policy::credits_bonus(base,effect,level,expected)&&*(int*)((char*)result+16)==expected;
            ok=ok&&pass;++oracle;
        }
    }
    fprintf(f,"{\"phase\":\"original_integer_oracle\",\"pass\":%s,\"cases\":%d,\"cards\":[24,36],\"levels\":12}\n",ok?"true":"false",oracle);fflush(f);
    auto& earned=*a.data;earned=local_progress::seed(100000000,0,0,50);void* cards[128];int n=local_career::skill_cards(cards,128);ok=ok&&n>0;
    for(int i=0;ok&&i<n;++i)ok=local_progress::grant_stock(earned,8,*(int*)((char*)cards[i]+0x10),10000);
    for(int round=0;ok&&!local_progress::has_skill_node(earned,282)&&round<32;++round){bool changed=false;
        for(int i=0;i<local_skill::graph.count;++i){int node=local_skill::graph.nodes[i].id;if(local_progress::has_skill_node(earned,node))continue;
            if(local_skill::plan(earned,node,*b.data)){earned=*b.data;changed=true;}if(local_progress::has_skill_node(earned,282))break;}
        if(!changed)break;
    }
    skill_tree_policy::Summary summary;ok=ok&&local_skill::graph.summarize(earned,summary)&&summary.level[24]>0&&summary.level[36]>0;
    *b.data=earned;b.data->stars=9;bool guards=!local_skill::plan(*b.data,254,*c.data)&&!local_skill::plan(earned,282,*c.data);
    fprintf(f,"{\"phase\":\"legal_original_nodes\",\"pass\":%s,\"nodes\":%u,\"card24_level\":%d,\"card36_level\":%d,\"rank_and_duplicate_refused\":%s}\n",ok&&guards?"true":"false",earned.skill_count,summary.level[24],summary.level[36],guards?"true":"false");ok=ok&&guards;fflush(f);
    int overflow=123;ok=ok&&!contract_policy::credits_bonus(INT_MAX,50,11,overflow)&&overflow==123;
    for(int mode:{1,2,4})for(bool formal:{false,true})for(int outcome=1;ok&&outcome<=6;++outcome)for(int reason:{1,2}){
        local_match_store::State match;match.mode=mode;bool pass=local_match_store::begin_pvp(match,formal)&&local_match_store::freeze_career_stars(match,formal,formal?5:0,formal?9:0)&&local_match_store::settle(match,1,(local_match_store::Outcome)outcome,18010,reason,outcome==1?1:0,nullptr,nullptr)&&local_warpoints::stamp_for(match,earned);
        int base=0,expected=0;pass=pass&&local_warpoints::initialize()&&warpoint_rewards::award(mode==1?local_warpoints::solo:local_warpoints::team,formal?warpoint_rewards::FormalPvp:warpoint_rewards::Training,(warpoint_rewards::Outcome)(outcome-1),reason,18010,base)&&local_reward_skills::apply(earned,24,base,expected)&&match.reserved==expected;
        match.checksum=local_match_store::hash(match);*b.data=earned;bool changed=false;
        pass=pass&&warpoint_store::reconcile(*b.data,match,changed)&&b.data->quest_warpoints==expected&&b.data->stars==earned.stars+(formal&&reason==1&&outcome==1?9:0);
        if(formal)pass=pass&&changed&&warpoint_store::reconcile(*b.data,match,changed)&&!changed;
        ok=ok&&pass;++results;
    }
    fprintf(f,"{\"phase\":\"frozen_result_and_replay\",\"pass\":%s,\"cases\":%d,\"old_true_terminal_pipeline_reused\":true}\n",ok?"true":"false",results);fflush(f);
    for(int seed=0;ok&&seed<8;++seed){*b.data=earned;b.data->skill_count=0;earned.supply.claims=b.data->supply.claims=seed;
        supply_policy::Draw raw,boosted;supply_types::Grant original[128],grants[128];int original_count=0,count=0;const auto* box=supply_policy::box(401);
        bool pass=box&&local_supply_rewards::prepare(*box,*b.data,2,raw,original,128,original_count)&&local_supply_rewards::prepare(*box,earned,2,boosted,grants,128,count)&&count==original_count&&raw.count==boosted.count;
        for(int i=0;pass&&i<raw.count;++i)pass=raw.rows[i].item==boosted.rows[i].item&&raw.rows[i].amount==boosted.rows[i].amount;
        for(int i=0;pass&&i<count;++i){int expected=original[i].amount;pass=original[i].type==grants[i].type&&original[i].id==grants[i].id;if(grants[i].type==0)pass=pass&&local_reward_skills::box_credits(earned,expected,expected);pass=pass&&grants[i].amount==expected;}
        ok=ok&&pass;++boxes;
        if(seed==0&&ok){earned.quest_warpoints=1000;*b.data=earned;ok=supply_store::claim(*b.data,1,401,boosted,grants,count);
            *c.data=earned;c.data->credits=INT_MAX;auto before_hash=local_progress::checksum(*c.data);atomic=!supply_store::claim(*c.data,1,401,boosted,grants,count)&&local_progress::checksum(*c.data)==before_hash;ok=ok&&atomic;
            char root[256];snprintf(root,sizeof(root),DATA_ROOT "reward-skill-private");mkdir(root,0700);ok=ok&&local_progress::write(root,*b.data)&&local_progress::load(root,*c.data)==1&&c.data->supply.pending&&c.data->supply.grant_count==b.data->supply.grant_count;
            for(unsigned i=0;ok&&i<c.data->supply.grant_count;++i)ok=!memcmp(&c.data->supply.grants[i],&b.data->supply.grants[i],sizeof(grants[0]));
            saved=ok&&!supply_store::claim(*c.data,1,401,boosted,grants,count);ok=ok&&saved;
        }
    }
    earned.supply=supply_types::Ledger{};earned.stars=360;
    for(int faction:{1,2}){void* stones=local_career::list(local_career::key(faction),"careerStones");int n=stones?*(int*)((char*)stones+0x18):0;
        for(int i=0;ok&&i<n;++i){void* stone=list_item(stones,i),*rewards=*(void**)((char*)stone+0x18);int m=rewards?*(int*)((char*)rewards+0x18):0;
            for(int j=0;ok&&j<m;++j){void* gift=list_item(rewards,j);if(*(int*)((char*)gift+0x14)!=14)continue;
                int id=*(int*)((char*)gift+0x18);if(id<11030||id>11036)continue;
                int gate=*(int*)((char*)stone+0x20);earned.stars=gate>360?gate:360;
                local_progress::CareerReward raw[4],resolved[128];int raw_count=0,count=0;bool pass=career_boxes::resolve(id,1,raw,4,raw_count)&&local_career::resolve_stone(::saved("UserConfiguration"),earned,faction,stone,resolved,128,count);
                for(int k=0;pass&&k<raw_count;++k){int expected=raw[k].amount;if(raw[k].type==0)pass=local_reward_skills::box_credits(earned,expected,expected);bool found=false;for(int l=0;l<count;++l)if(resolved[l].type==raw[k].type&&resolved[l].id==raw[k].id&&resolved[l].amount==expected)found=true;pass=pass&&found;}
                fprintf(f,"{\"phase\":\"career_box\",\"faction\":%d,\"box\":%d,\"original_stars\":%d,\"fixture_stars\":%d,\"pass\":%s}\n",faction,id,gate,earned.stars,pass?"true":"false");ok=ok&&pass;++career;
            }
        }
    }
    bool unchanged=!memcmp(before.data,&progress,sizeof(progress));ok=ok&&boxes==8&&career==14&&unchanged;
    fprintf(f,"{\"summary\":true,\"pass\":%s,\"oracle_cases\":%d,\"result_cases\":%d,\"stable_draw_cases\":%d,\"career_box_cases\":%d,\"private_save_replay\":%s,\"overflow_atomic\":%s,\"player_state_unchanged\":%s,\"player_save_writes\":0,\"battle_runs\":0}\n",ok?"true":"false",oracle,results,boxes,career,saved?"true":"false",atomic?"true":"false",unchanged?"true":"false");fclose(f);
}
}
