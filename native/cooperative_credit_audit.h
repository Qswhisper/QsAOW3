// Original card/formula and legal card25 purchases on independent model states.
#pragma once
namespace cooperative_credit_audit {
static void run(){
    if(!progress_ready||access(DATA_ROOT "cooperative-credit-audit.request",F_OK))return;
    unlink(DATA_ROOT "cooperative-credit-audit.request");
    FILE* f=fopen(DATA_ROOT "cooperative-credit-audit.jsonl","w");if(!f)return;chmod(DATA_ROOT "cooperative-credit-audit.jsonl",0644);
    local_progress::StateBuffer before,fixture,next;bool ok=before.data&&fixture.data&&next.data;
    if(!ok){fclose(f);return;}*before.data=progress;*fixture.data=local_progress::seed(INT_MAX,0,0,1000000);
    ok=local_skill::initialize()&&local_reward_skills::cooperative_credit_installed&&local_faction::active==2;
    void* card=local_career::skill_card(25);ok=ok&&card&&*(int*)((char*)card+0x38)==100&&local_skill::graph.max_level[25]==11;
    const void* oracle=class_method(find_class("com.geargames.aow.scenes.main.gui.skills","SkillsController"),"GetBoostedValueInt",3);
    for(int id=1;ok&&id<100;++id)if(local_skill::graph.max_level[id])ok=local_progress::grant_stock(*fixture.data,8,id,1000000);
    int checked=0,paid=0,oracle_cases=0;
    auto inspect=[&](){int level=-1,amount=0;bool pass=local_reward_skills::cooperative_level(*fixture.data,level)&&level==paid&&cooperative_credit_policy::budget(level,amount);
        local_research_runtime::Roots roots;void* skill=object_new(find_class("com.geargames.aow.entities","Skill")),*ex=nullptr;int maximum=11,stock=0;void* args[]={card,&level,&maximum,&stock};
        pass=pass&&oracle&&roots.keep(skill);if(pass)invoke_named(skill,".ctor",4,args,&ex);
        for(int base:{1,33,2770,10049}){bool visual=true;void* a[]={&base,skill,&visual};void* result=pass&&!ex?runtime_invoke(oracle,nullptr,a,&ex):nullptr;int expected=0;
            pass=pass&&result&&!ex&&contract_policy::credits_bonus(base,100,level,expected)&&*(int*)((char*)result+16)==expected;++oracle_cases;}
        cooperative_store::State game;game.match.scenario=973;game.match.map=558;game.match.mode=3;game.match.opponent=1;
        pass=pass&&cooperative_store::begin(game)&&cooperative_store::freeze_credits(game,level)&&game.credit_level==(uint32_t)level&&game.credit_run==game.match.next;
        fprintf(f,"{\"level\":%d,\"budget\":%d,\"pass\":%s,\"private_model\":true}\n",level,amount,pass?"true":"false");++checked;ok&=pass;
    };
    if(ok)inspect();
    // Other receipts only establish hypothetical graph prerequisites here;
    // they do not assert those other effects or cards are implemented.
    for(int round=0;ok&&paid<11&&round<512;++round){bool changed=false;
        for(int i=0;ok&&i<local_skill::graph.count&&paid<11;++i){const auto& n=local_skill::graph.nodes[i];if(local_progress::has_skill_node(*fixture.data,n.id))continue;
            skill_tree_policy::Summary summary;if(!local_skill::graph.summarize(*fixture.data,summary,n.id))continue;
            if(n.card==25){int credits=fixture.data->credits,cards=local_progress::stock_amount(*fixture.data,8,25);
                ok=local_skill::plan(*fixture.data,n.id,*next.data)&&next.data->credits==credits-n.credits&&local_progress::stock_amount(*next.data,8,25)==cards-n.cards;
                if(ok){*fixture.data=*next.data;++paid;inspect();}}
            else ok=local_skill::graph.purchase(*fixture.data,n.id);
            changed=true;
        }if(!changed)break;
    }
    bool unchanged=!memcmp(before.data,&progress,sizeof(progress));
    fprintf(f,"{\"summary\":true,\"complete\":true,\"pass\":%s,\"card\":25,\"effect\":100,\"levels_checked\":%d,\"legal_private_purchases\":%d,\"original_formula_cases\":%d,\"player_unchanged\":%s,\"player_save_writes\":0,\"player_credit_grants\":0,\"scene_runs\":0,\"growth_version\":%u,\"growth_size\":%zu,\"cooperative_size\":%zu}\n",ok&&paid==11&&checked==12&&unchanged?"true":"false",checked,paid,oracle_cases,unchanged?"true":"false",local_progress::kVersion,sizeof(progress),sizeof(cooperative_store::WireState));fclose(f);
}
}
