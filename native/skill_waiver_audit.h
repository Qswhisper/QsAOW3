// obsolete interval/uranium nodes and a legal first-stage bridge.
#pragma once
namespace skill_waiver_audit {
using local_skill::get;
static void* find_skill(void* info,int card){void* list=get(info,0x20);
    for(int i=0;list&&i<*(int*)((char*)list+0x18);++i){void* s=list_item(list,i),*c=get(s,0x10);if(c&&*(int*)((char*)c+0x10)==card)return s;}return nullptr;}
static void run(){
    if(!progress_ready||access(DATA_ROOT "skill-waiver-audit.request",F_OK))return;
    unlink(DATA_ROOT "skill-waiver-audit.request");auto before=progress;
    FILE* f=fopen(DATA_ROOT "skill-waiver-audit.json","w");if(!f)return;chmod(DATA_ROOT "skill-waiver-audit.json",0644);
    bool ok=local_skill::initialize();void* shared=ok?local_skill::runtime_shared():nullptr,*original=saved("SHARED");
    bool topology=shared&&shared!=original&&!memcmp((char*)shared+0x10,(char*)original+0x10,0xa8)&&
        !memcmp((char*)shared+0xc0,(char*)original+0xc0,0x80);int waived=0,price_sum=0;
    for(int i=0;topology&&i<local_skill::graph.count;++i){const auto& n=local_skill::graph.nodes[i];
        void* raw=local_skill::originals[i],*view=local_skill::presented_node(raw),*c=get(view,0x20),*edges=get(view,0x30);price_sum+=n.credits;
        topology=view&&view!=raw&&get(view,0x18)==get(raw,0x18)&&*(int*)((char*)view+0x10)==n.id&&
            *(int*)((char*)view+0x28)==skill_tree_policy::credits(n)&&*(int*)((char*)view+0x2c)==skill_tree_policy::cards(n)&&
            *(int*)((char*)raw+0x28)==n.credits&&*(int*)((char*)raw+0x2c)==n.cards&&edges&&array_length(edges)==(size_t)n.edge_count;
        for(int j=0;topology&&j<n.edge_count;++j)topology=((void**)((char*)edges+32))[j]==local_skill::presented_node(local_skill::originals[local_skill::graph.index(n.edges[j])]);
        if(local_progress::waived_skill_card(n.card)){++waived;topology=topology&&c!=get(raw,0x20)&&*(int*)((char*)c+0x10)==n.card&&*(int*)((char*)c+0x38)==0&&*(int*)((char*)c+0x3c)==0&&*(int*)((char*)get(raw,0x20)+0x38)==(n.card==13?40:30);}
        else topology=topology&&c==get(raw,0x20);
    }
    topology=topology&&waived==33&&price_sum==78184700;
    void* published=static_object("com.geargames.aow.scenes.main","ClientMainContext","<SharedBaseDictionary>k__BackingField");
    void* tree=static_object("com.geargames.aow.scenes.main","ClientMainContext","<MilestonesDataSource>k__BackingField");
    void* tree_ex=nullptr;void* stages=tree?invoke_named(tree,"get_Milestones",0,nullptr,&tree_ex):nullptr;
    void* first_stage=stages&&!tree_ex?list_item(stages,0):nullptr;void* first_nodes=get(first_stage,0x20);bool published_tree=published==shared&&first_nodes;
    for(size_t j=0;published_tree&&j<array_length(first_nodes);++j){void* n=((void**)((char*)first_nodes+32))[j];published_tree=n&&n==local_skill::presented_node(n);}
    topology=topology&&published_tree;
    auto fixture=local_progress::seed(100000,0,0,10000);local_progress::grant_stock(fixture,8,5,1000);local_progress::grant_stock(fixture,8,27,1000);
    // Unsupported ancestor effects are NOT enabled. This private policy-only
    // fixture satisfies their canonical receipts to isolate the node.
    bool ancestors=ok&&local_skill::graph.purchase(fixture,114)&&local_skill::graph.purchase(fixture,134);fixture.credits=0;
    local_progress::State next;bool purchase=ancestors&&local_skill::plan(fixture,144,next)&&next.credits==0&&next.skill_count==3&&
        local_progress::stock_amount(next,8,13)==0;
    auto low=fixture;low.stars=9;auto missing=local_progress::seed(0,0,0,10000);local_progress::State denied;
    bool guards=!local_skill::plan(low,144,denied)&&!local_skill::plan(missing,144,denied)&&purchase&&!local_skill::plan(next,144,denied);
    int later=0;for(int i=0;i<local_skill::graph.count;++i)if(local_skill::graph.nodes[i].card==13&&local_skill::graph.nodes[i].stage>1){later=local_skill::graph.nodes[i].id;break;}
    guards=guards&&later&&!local_skill::plan(next,later,denied)&&!local_skill::plan(fixture,111,denied);
    local_skill::Roots roots;void* initial_info=ancestors?local_skill::info(saved("UserConfiguration"),fixture,roots):nullptr;
    void* info=purchase?local_skill::info(saved("UserConfiguration"),next,roots):nullptr;
    void* skill=find_skill(info,13),*source=object_new(find_class("com.geargames.aow.assets.scenes.main.network","SkillInfoDataSource"));
    void* logger=object_new(find_class("com.geargames.aow.logger.wrapper","LoggerWrapper"));void* ex=nullptr;
    bool model=initial_info&&info&&skill&&roots.keep(source)&&roots.keep(logger);
    if(model){invoke_named(logger,".ctor",0,nullptr,&ex);void* args[]={logger};if(!ex)invoke_named(source,".ctor",1,args,&ex);
        void* initial[]={initial_info};if(!ex)invoke_named(source,"SetSkillInfo",1,initial,&ex);
        int stage=1;void* update[]={skill,local_skill::presented_node(local_skill::originals[local_skill::graph.index(144)]),&stage};
        if(!ex)invoke_named(source,"SetSkill",3,update,&ex);
        void* actual=!ex?invoke_named(source,"get_Configuration",0,nullptr,&ex):nullptr;
        model=!ex&&actual&&get(actual,0x10)&&*(int*)((char*)get(actual,0x10)+0x18)==3&&*(int*)((char*)skill+0x18)==1;
    }
    bool neutral=model;
    for(int level:{1,11}){if(!model)break;local_skill::Roots one;void* s=object_new(find_class("com.geargames.aow.entities","Skill"));int max=11,amount=0;
        void* card=get(skill,0x10);void* args[]={card,&level,&max,&amount};
        if(!one.keep(s)){neutral=false;break;}ex=nullptr;invoke_named(s,".ctor",4,args,&ex);
        int value=3000;void* input[]={&value};void* output=!ex?invoke_named(s,"applyDown",1,input,&ex):nullptr;
        neutral=neutral&&output&&!ex&&*(int*)((char*)output+16)==3000;
    }
    char path[]=DATA_ROOT "skill-waiver-fixture-XXXXXX";local_progress::State restored;
    bool persisted=purchase&&mkdtemp(path)&&local_progress::write(path,next)&&local_progress::load(path,restored)==1&&!memcmp(&next,&restored,sizeof(next));
    auto bridge=local_progress::seed(1000000,0,0,10000);bool bridge_ok=true;
    for(int card:{1,2,3,4,5,8,9,21,23,27})bridge_ok=bridge_ok&&local_progress::grant_stock(bridge,8,card,1000);
    local_progress::State candidate;bool early_stage_refused=!local_skill::plan(bridge,214,candidate);
    for(int node:{111,112,113,114,121,123,132,134,142,143,144}){
        bool bought=bridge_ok&&local_skill::plan(bridge,node,candidate);bridge_ok=bridge_ok&&bought;if(bought)bridge=candidate;
    }
    skill_tree_policy::Summary bridge_summary;
    bridge_ok=bridge_ok&&local_skill::graph.summarize(bridge,bridge_summary)&&bridge_summary.stage==2&&bridge.skill_count==11&&bridge.credits==979600;
    bool deploy_purchase=bridge_ok&&local_skill::plan(bridge,214,candidate);if(deploy_purchase)bridge=candidate;
    bool cp_ancestor_refused=!local_skill::plan(bridge,221,candidate);
    auto bridge_low=local_progress::seed(0,0,0,9);bool free_rank_refused=!local_skill::plan(bridge_low,113,candidate);
    auto bridge_free=local_progress::seed(0,0,0,10000);bool free_root=local_skill::plan(bridge_free,113,candidate)&&candidate.credits==0&&local_progress::stock_amount(candidate,8,12)==0;
    void* bridge_info=bridge_ok?local_skill::info(saved("UserConfiguration"),bridge,roots):nullptr;
    void* interval=find_skill(bridge_info,12),*uranium=find_skill(bridge_info,13);
    bool distinct_cards=interval&&uranium&&get(interval,0x10)!=get(uranium,0x10)&&
        *(int*)((char*)get(interval,0x10)+0x10)==12&&*(int*)((char*)get(uranium,0x10)+0x10)==13&&
        get(interval,0x10)==get(local_skill::presented_node(local_skill::originals[local_skill::graph.index(113)]),0x20)&&
        get(uranium,0x10)==get(local_skill::presented_node(local_skill::originals[local_skill::graph.index(144)]),0x20);
    char seed_path[512];snprintf(seed_path,sizeof(seed_path),DATA_ROOT "offline-seeds/fmc%dm.ggs",local_faction::active);
    local_research_runtime::Roots battle_roots;
    void* config=campaign_seeds::decode(seed_path,battle_roots),*battle=get(config,0x10),*side=local_dictionary_item(get(battle,0x178),1);
    void* units=get(side,0xf0),*buildings=get(side,0xf8);
    uint64_t old_units=hero_battle_projection_audit::type_hash(units),old_buildings=skill_protection_audit::hash(buildings,true);
    bool bridge_projection=deploy_purchase&&side&&old_units&&old_buildings&&skill_combat::project(side,bridge,units,buildings)&&
        old_units==hero_battle_projection_audit::type_hash(units)&&old_buildings==skill_protection_audit::hash(buildings,true);
    auto coefficient=[&](int key,int expected){void* map=get(side,0x3e8),*error=nullptr;int value=-1;void* args[]={&key,&value};void* found=invoke_named(map,"TryGetValue",2,args,&error);return found&&!error&&*(bool*)((char*)found+16)&&value==expected;};
    bridge_projection=bridge_projection&&coefficient(4,10)&&coefficient(5,5)&&coefficient(9,4)&&coefficient(20,0)&&coefficient(21,2);
    char bridge_path[]=DATA_ROOT "skill-stage-bridge-XXXXXX";local_progress::State bridge_disk;
    bool bridge_saved=bridge_projection&&mkdtemp(bridge_path)&&local_progress::write(bridge_path,bridge)&&local_progress::load(bridge_path,bridge_disk)==1&&!memcmp(&bridge,&bridge_disk,sizeof(bridge));
    bridge_ok=bridge_ok&&early_stage_refused&&deploy_purchase&&cp_ancestor_refused&&free_rank_refused&&free_root&&distinct_cards&&bridge_projection&&bridge_saved;
    bool isolated=!memcmp(&before,&progress,sizeof(before));
    fprintf(f,"{\"pass\":%s,\"waived_nodes\":%d,\"view_graph_and_shared_references\":%s,\"canonical_prices_unchanged\":%s,\"zero_credit_zero_card_purchase\":%s,\"rank_dependency_stage_duplicate_guards\":%s,\"original_source_update\":%s,\"original_applydown_neutral_levels1_11\":%s,\"private_save_round_trip\":%s,\"private_save_root\":\"%s\",\"first_stage_bridge\":%s,\"stage2_deploy_purchase\":%s,\"cp_missing_ancestor_refused\":%s,\"distinct_waived_card_models\":%s,\"legal_saved_combat_projection\":%s,\"bridge_nodes\":%u,\"bridge_credits\":%d,\"bridge_private_save_root\":\"%s\",\"player_state_unchanged\":%s,\"player_save_writes\":0,\"battle_runs\":0}\n",
        topology&&purchase&&guards&&model&&neutral&&persisted&&bridge_ok&&isolated?"true":"false",waived,topology?"true":"false",topology?"true":"false",purchase?"true":"false",guards?"true":"false",model?"true":"false",neutral?"true":"false",persisted?"true":"false",persisted?path:"",bridge_ok?"true":"false",deploy_purchase?"true":"false",cp_ancestor_refused?"true":"false",distinct_cards?"true":"false",bridge_projection?"true":"false",bridge.skill_count,bridge.credits,bridge_saved?bridge_path:"",isolated?"true":"false");fclose(f);
}
}
