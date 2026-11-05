// Original card/model/price presentation and frozen local credit contract.
// Uses private models and receipts only, never grants a real player result.
#pragma once
namespace formal_credit_audit {
using Roots=local_research_runtime::Roots;
static void* get(void* p,int o){return p?*(void**)((char*)p+o):nullptr;}
static void run(){
    if(!progress_ready||access(DATA_ROOT "formal-credit-audit.request",F_OK)||!local_match::prices_copy)return;
    unlink(DATA_ROOT "formal-credit-audit.request");FILE* f=fopen(DATA_ROOT "formal-credit-audit.jsonl","w");if(!f)return;chmod(DATA_ROOT "formal-credit-audit.jsonl",0644);
    local_progress::StateBuffer before,earned,next;if(!before.data||!earned.data||!next.data){fclose(f);return;}*before.data=progress;
    bool ready=local_skill::initialize()&&skill_combat::supported(37)&&local_progress::waived_skill_card(22),ok=ready;int oracle=0,waived=0,ranks=0,max_budget=0;
    void* card=local_career::skill_card(37),*controller=find_class("com.geargames.aow.scenes.main.gui.skills","SkillsController");ok=ok&&card&&*(int*)((char*)card+0x38)==50;
    for(int level=0;ok&&level<=11;++level){Roots roots;void* skill=object_new(find_class("com.geargames.aow.entities","Skill")),*ex=nullptr;int max=11,cards=0,base=2770,expected=0;bool visual=true;
        bool pass=roots.keep(skill);void* a[]={card,&level,&max,&cards};if(pass)invoke_named(skill,".ctor",4,a,&ex);void* b[]={&base,skill,&visual};void* value=pass&&!ex?runtime_invoke(class_method(controller,"GetBoostedValueInt",3),nullptr,b,&ex):nullptr;
        pass=value&&!ex&&formal_credit_rewards::budget(level,expected)&&*(int*)((char*)value+16)==expected;ok=ok&&pass;++oracle;
    }
    fprintf(f,"{\"phase\":\"original_card37_integer\",\"pass\":%s,\"cases\":%d,\"base\":2770,\"level11\":4293}\n",ok?"true":"false",oracle);fflush(f);
    bool topology=ok;for(int i=0;topology&&i<local_skill::graph.count;++i){const auto& node=local_skill::graph.nodes[i];if(node.card!=22)continue;
        void* raw=local_skill::originals[i],*view=local_skill::presented_node(raw),*shown=get(view,0x20),*canonical=get(raw,0x20);
        void* edges=get(view,0x30);
        topology=view&&view!=raw&&get(view,0x18)==get(raw,0x18)&&edges&&array_length(edges)==(size_t)node.edge_count&&*(int*)((char*)view+0x28)==0&&*(int*)((char*)view+0x2c)==0&&shown&&shown!=canonical&&*(int*)((char*)shown+0x38)==0&&*(int*)((char*)canonical+0x38)==200;
        for(int j=0;topology&&j<node.edge_count;++j)topology=((void**)((char*)edges+32))[j]==local_skill::presented_node(local_skill::originals[local_skill::graph.index(node.edges[j])]);
        bool structure=topology;int neutral=-1,direct=-1,skill_level=-1;bool card_matches=false;const char* error="none";
        if(topology){Roots one;void* skill=object_new(find_class("com.geargames.aow.entities","Skill")),*ex=nullptr;int level=11,max=11,stock=0,base=2770;bool visual=true;topology=one.keep(skill);void* a[]={shown,&level,&max,&stock};if(topology)invoke_named(skill,".ctor",4,a,&ex);void* b[]={&base,skill,&visual};void* v=topology&&!ex?runtime_invoke(class_method(controller,"GetBoostedValueInt",3),nullptr,b,&ex):nullptr;
            if(skill){skill_level=*(int*)((char*)skill+0x18);card_matches=get(skill,0x10)==shown;}if(v&&!ex)neutral=*(int*)((char*)v+16);if(ex)error=class_name(object_class(ex));
            if(topology&&!ex)direct=reinterpret_cast<int(*)(int,void*,bool,void*)>(image_base+0x7f37f88)(base,skill,true,nullptr);topology=v&&!ex&&neutral==2770&&direct==2770;
        }
        fprintf(f,"{\"phase\":\"card22_node_detail\",\"pass\":%s,\"node\":%d,\"structure\":%s,\"view_distinct\":%s,\"place_same\":%s,\"edge_count\":%llu,\"expected_edges\":%d,\"cost\":%d,\"cards\":%d,\"raw_effect\":%d,\"view_effect\":%d,\"neutral\":%d,\"direct\":%d,\"level\":%d,\"card_matches\":%s,\"exception\":\"%s\"}\n",topology?"true":"false",node.id,structure?"true":"false",view&&view!=raw?"true":"false",get(view,0x18)==get(raw,0x18)?"true":"false",(unsigned long long)(edges?array_length(edges):0),node.edge_count,view?*(int*)((char*)view+0x28):-1,view?*(int*)((char*)view+0x2c):-1,canonical?*(int*)((char*)canonical+0x38):-1,shown?*(int*)((char*)shown+0x38):-1,neutral,direct,skill_level,card_matches?"true":"false",error);fflush(f);
        ++waived;
    }
    ok=ok&&topology&&waived==11;fprintf(f,"{\"phase\":\"original_card22_views\",\"pass\":%s,\"nodes\":%d,\"neutral_effect\":%s}\n",topology&&waived==11?"true":"false",waived,topology?"true":"false");fflush(f);
    *earned.data=local_progress::seed(100000000,0,0,50);void* cards[128];int n=local_career::skill_cards(cards,128);bool setup=ready&&n>0;
    for(int i=0;setup&&i<n;++i){int id=*(int*)((char*)cards[i]+0x10);if(!local_progress::waived_skill_card(id))setup=local_progress::grant_stock(*earned.data,8,id,10000);}
    for(int round=0;setup&&!local_progress::has_skill_node(*earned.data,162)&&round<32;++round){bool changed=false;for(int i=0;i<local_skill::graph.count;++i){int id=local_skill::graph.nodes[i].id;if(local_progress::has_skill_node(*earned.data,id))continue;if(local_skill::plan(*earned.data,id,*next.data)){*earned.data=*next.data;changed=true;}if(local_progress::has_skill_node(*earned.data,162))break;}if(!changed)break;}
    int level=0,budget=0;bool legal=local_progress::has_skill_node(*earned.data,162)&&local_reward_skills::pvp_level(*earned.data,level)&&level==1&&local_reward_skills::apply(*earned.data,37,2770,budget)&&budget==2908;
    earned.data->credits=0;unsigned previous=earned.data->skill_count;legal=legal&&local_progress::stock_amount(*earned.data,8,22)==0&&local_skill::plan(*earned.data,182,*next.data)&&next.data->credits==0&&next.data->skill_count==previous+1&&local_progress::stock_amount(*next.data,8,22)==0;
    if(legal)*earned.data=*next.data;
    *next.data=*earned.data;next.data->stars=9;if(legal)--next.data->skill_count;local_progress::StateBuffer denied;if(!denied.data){fclose(f);return;}
    bool gates=legal&&!local_skill::plan(*next.data,182,*denied.data)&&!local_skill::plan(*earned.data,182,*denied.data);
    *next.data=local_progress::seed(0,0,0,50);gates=gates&&!local_skill::plan(*next.data,182,*denied.data);
    int raw_index=local_skill::graph.index(162);gates=gates&&raw_index>=0&&local_skill::graph.nodes[raw_index].credits==6800&&local_skill::graph.nodes[raw_index].cards==5;
    ok=ok&&legal&&gates;fprintf(f,"{\"phase\":\"legal37_and_free22\",\"pass\":%s,\"card37_level\":%d,\"card37_credits\":%d,\"zero_card22_stock\":true,\"parent_rank_duplicate_guards\":%s}\n",legal&&gates?"true":"false",level,budget,gates?"true":"false");fflush(f);
    void* rank_rows=local_career::list("SHARED","profileRanks");for(int i=0;rank_rows&&i<*(int*)((char*)rank_rows+0x18);++i){int amount=0,floor=0,next_floor=0;bool pass=local_career_stars::from_rank(list_item(rank_rows,i),amount,floor,next_floor)&&amount<=0x3fff;ok=ok&&pass;if(amount>max_budget)max_budget=amount;++ranks;}
    local_match_store::State actual;bool frozen=local_match_store::begin_pvp(actual,true)&&local_career_stars::freeze(actual,true)&&actual.version==7;actual.checksum=local_match_store::hash(actual);frozen=frozen&&local_match_store::valid(actual);ok=ok&&frozen&&ranks==30;
    bool prices=true;int reward_rows=0,cost_rows=0;void* copy=local_match::prices_copy,*source=local_match::prices_source;
    for(int offset:{0x30,0x38}){void* dictionary=get(copy,offset),*entries=get(dictionary,0x18);int count=dictionary?*(int*)((char*)dictionary+0x20):0;if(!entries||count<1||count>64){prices=false;break;}
        for(int i=0;prices&&i<count;++i){char* e=(char*)entries+32+i*24;if(*(int*)e<0)continue;int key=*(int*)(e+8);void* set=*(void**)(e+16),*list=get(set,0x10),*raw=local_dictionary_item(get(source,offset),key);int n=list?*(int*)((char*)list+0x18):0;if(set==raw||n<1||n>32){prices=false;break;}
            for(int j=0;prices&&j<n;++j){void* item=list_item(list,j);bool credit=offset==0x30&&*(int8_t*)((char*)item+0x10)==0&&*(int*)((char*)item+0x14)==0;prices=*(int*)((char*)item+0x40)==(credit?2770:0)&&*(int*)((char*)item+0x48)==0;}
            if(offset==0x30)++reward_rows;else ++cost_rows;
        }
    }
    void* ex=nullptr,*first=invoke_named(copy,"get_FirstWinBonus",0,nullptr,&ex),*possible=ex?nullptr:invoke_named(copy,"get_FirstWinBonusPossible",0,nullptr,&ex);prices=prices&&!ex&&first&&possible&&!*(int*)((char*)first+16)&&!*(int*)((char*)possible+16);
    ok=ok&&prices;fprintf(f,"{\"phase\":\"original_hall_prices\",\"pass\":%s,\"reward_rows\":%d,\"zero_cost_rows\":%d,\"firstwin_zero\":%s}\n",prices?"true":"false",reward_rows,cost_rows,prices?"true":"false");
    bool unchanged=!memcmp(before.data,&progress,sizeof(progress));ok=ok&&unchanged;
    fprintf(f,"{\"summary\":true,\"pass\":%s,\"original_card37_cases\":%d,\"waived22_nodes\":%d,\"original_ranks\":%d,\"max_frozen_stars\":%d,\"actual_policy_v7\":%s,\"actual_credit_level\":%d,\"legal37_and22\":%s,\"hall_prices\":%s,\"player_state_unchanged\":%s,\"player_save_writes\":0,\"battle_runs\":0}\n",ok?"true":"false",oracle,waived,ranks,max_budget,frozen?"true":"false",local_match_store::credit_level(actual),legal&&gates?"true":"false",prices?"true":"false",unchanged?"true":"false");fclose(f);
}
}
