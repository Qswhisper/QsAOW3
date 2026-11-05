// One batch of isolated original-model, reward and side publication checks.
#pragma once
namespace modification_service_audit {
using namespace local_modifications;
static void unlock_units(local_progress::State& s,int faction){
    void* stones=local_career::list(local_career::key(faction),"careerStones");
    for(int i=0;stones&&i<*(int*)((char*)stones+0x18);++i){void* stone=list_item(stones,i);if(*(int*)((char*)stone+0x20)>s.stars)continue;
        if(s.career_count<local_progress::kCareerCapacity)s.career_ids[s.career_count++]=*(int*)((char*)stone+0x10);}
}
static bool side_case(const modification_content::Definition& d,int& links){
    preplaced_research_audit::Fixture fixture;if(!fixture.init("fmc1m"))return false;
    local_research_runtime::Roots roots;auto c=local_mod_effects::compiled(d.id);void* type=get(c?c->definition:nullptr,0x38),*side=fixture.side;if(!type)return false;
    void* units=local_mod_effects::clone_dictionary(get(side,0xf0),get(side,0xf0),roots),*mines=local_mod_effects::clone_dictionary(get(side,0x100),get(side,0x100),roots),*buildings=local_mod_effects::clone_dictionary(get(side,0xf8),get(side,0xf8),roots);
    if(!units||!mines||!buildings)return false;
    for(const char* name:{"unitTypes","mineTypes"}){bool unit=!strcmp(name,"unitTypes");void* list=local_career::list("BATTLE",name);
        for(int i=0;list&&i<*(int*)((char*)list+0x18);++i){void* v=list_item(list,i);if(unit&&get(v,0x20)!=get(type,0x20))continue;if(!local_mod_effects::add(unit?units:mines,*(int*)((char*)v+0x10),v))return false;}}
    ref(side,"$gg",units);ref(side,"$Gg",buildings);ref(side,"$hg",mines);ref(side,"$Eg",get(type,0x20));
    auto* state=new(std::nothrow) local_progress::State(local_progress::seed(0,0,0,360));if(!state)return false;
    bool ok=local_progress::grant_stock(*state,17,d.id,7)&&local_progress::equip_modification(*state,d.unit,d.id);
    void* prepared=local_research_runtime::clone(side,roots);uint64_t before=weapon_parameters_audit::hash(type);
    ok=ok&&prepared&&local_mod_effects::prepare_side(prepared,*state);delete state;
    void* projected=local_dictionary_item(get(prepared,0xf0),d.unit);ok=ok&&projected&&projected!=type&&before==weapon_parameters_audit::hash(type);
    // Three loaded entities preserve damage/death across stat penalties.
    for(int damage:{0,10,-1})if(ok)ok=fixture.spawn(type,0,damage,false,fixture.count);
    preplaced_research::Migration migration;migration.allow_mod_penalties=true;
    // The actual progress is consulted only for the per-unit penalty guard;
    // restore it before any publish/save path can execute.
    local_progress::StateBuffer backup;if(!backup.data)return false;*backup.data=progress;
    ok=ok&&local_progress::grant_stock(progress,17,d.id,7)&&local_progress::equip_modification(progress,d.unit,d.id);
    for(int i=0;ok&&i<fixture.count;++i)ok=migration.entity(fixture.samples[i].entity,side,prepared,false);
    progress=*backup.data;
    if(ok){migration.commit();for(int i=0;i<fixture.count;++i){auto& s=fixture.samples[i];int old_max=*(int*)((char*)type+0x34),new_max=*(int*)((char*)projected+0x34);int wanted=s.life>0?s.life+new_max-old_max:s.life;if(s.life>0&&wanted<1)wanted=1;
        ok=ok&&get(s.entity,0x40)==projected&&*(int*)((char*)s.entity+0x184)==wanted&&*(int16_t*)((char*)s.entity+0x182)==wanted;}}
    links+=migration.weapon_links;
    for(int i=0;ok&&i<c->count;++i){const auto& g=c->groups[i];if(g.key.kind<3)continue;void* old=g.key.kind==3?get(side,0xd8):local_dictionary_item(get(side,0x100),g.key.entity),*next=g.key.kind==3?get(prepared,0xd8):local_dictionary_item(get(prepared,0x100),g.key.entity);int base=0,value=0,expected=0;
        ok=old&&next&&old!=next&&local_mod_effects::read(old,g.key.kind,g.key.property,base)&&local_mod_effects::calculate(g,base,expected)&&local_mod_effects::read(next,g.key.kind,g.key.property,value)&&value==expected;}
    return ok;
}
static void run(){
    if(!progress_ready||access(DATA_ROOT "modification-service-audit.request",F_OK))return;unlink(DATA_ROOT "modification-service-audit.request");
    FILE* f=fopen(DATA_ROOT "modification-service-audit.jsonl","w");if(!f)return;chmod(DATA_ROOT "modification-service-audit.jsonl",0644);
    local_progress::StateBuffer backup;if(!backup.data){fclose(f);return;}*backup.data=progress;bool ok=installed&&ready;int configs=0,sides=0,links=0,rewards=0,boxes=0;
    fprintf(f,"{\"phase\":\"service_ready\",\"hooks\":%s,\"ready\":%s,\"pass\":%s}\n",installed?"true":"false",ready?"true":"false",ok?"true":"false");
    for(int faction:{1,2}){local_progress::StateBuffer storage;auto& state=*storage.data;state=local_progress::seed(0,0,0,360);unlock_units(state,faction);
        for(const auto& d:modification_content::definitions)if(d.faction==faction)ok=local_progress::grant_stock(state,17,d.id,7)&&ok;
        for(const auto& d:modification_content::definitions)if(d.faction==faction){Roots roots;auto private_state=state;bool pass=local_progress::equip_modification(private_state,d.unit,d.id);void* cfg=configuration(private_state,faction,roots),*set=local_supply::dictionary_list(get(cfg,0x10),d.unit),*put=get(set,0x10);
            pass=pass&&cfg&&set&&put&&get(put,0x10)==local_mod_effects::original(d.id)&&*(uint64_t*)((char*)put+0x18)==3155378975999999999ULL&&*(int*)((char*)put+0x20)==7&&*(int*)((char*)cfg+0x20)==5&&*(int*)((char*)cfg+0x24)==10;
            void* message=construct(find_class("com.geargames.aow.messages.modification","UnitModificationConfigurationUpdate"),roots);void* ex=nullptr;void* args[]={cfg};if(message)invoke_named(message,".ctor",1,args,&ex);pass=pass&&message&&!ex&&get(message,0x20)==cfg;
            ok=ok&&pass;++configs;fprintf(f,"{\"phase\":\"original_configuration\",\"id\":%d,\"pass\":%s}\n",d.id,pass?"true":"false");}
        for(int rarity:{1,2}){supply_content::Item item={1,1000,100,26,4999+rarity,4,7};local_supply_rewards::Candidates pool;bool pass=local_modifications::candidates(item,state,faction,pool)==supply_policy::Eligible&&pool.count==(rarity==1?5:2)&&pool.type==17;ok=ok&&pass;
            fprintf(f,"{\"phase\":\"eligible_pool\",\"faction\":%d,\"rarity\":%d,\"count\":%d,\"pass\":%s}\n",faction,rarity,pool.count,pass?"true":"false");}
        for(const auto& group:supply_policy::groups){void* rank=local_career::lookup("SHARED","profileRanks",group.rank);auto eligible=local_progress::seed(0,0,0,rank?*(int*)((char*)rank+0x14):0);unlock_units(eligible,faction);eligible.quest_warpoints=10000000;
            for(int i=0;i<group.count;++i){const auto* b=supply_policy::box(group.boxes[i]);bool supported=b&&local_supply_rewards::supported(*b,eligible,faction);if(!supported)continue;supply_policy::Draw draw;supply_types::Grant grants[128];int count=0;bool pass=local_supply_rewards::prepare(*b,eligible,faction,draw,grants,128,count);
                // A private pending journal exercises the exact original reward-message path.
                eligible.supply.claims=1;eligible.supply.last_box=b->id;eligible.supply.last_cost=1;eligible.supply.draw_count=draw.count;eligible.supply.grant_count=count;
                for(int j=0;j<draw.count;++j)eligible.supply.draws[j]=draw.rows[j];for(int j=0;j<count;++j)eligible.supply.grants[j]=grants[j];
                Roots roots;void* message=pass?local_supply::reward_message(eligible,roots):nullptr;pass=pass&&message;ok=ok&&pass;++rewards;
                fprintf(f,"{\"phase\":\"original_box_message\",\"faction\":%d,\"box\":%d,\"pass\":%s}\n",faction,b->id,pass?"true":"false");eligible.supply={};}
        }
    }
    for(const auto& d:modification_content::definitions){bool pass=side_case(d,links);ok=ok&&pass;++sides;fprintf(f,"{\"phase\":\"private_side\",\"id\":%d,\"pass\":%s}\n",d.id,pass?"true":"false");}
    for(int id=11030;id<=11036;++id){local_progress::CareerReward values[4];int count=0;bool pass=career_boxes::resolve(id,1,values,4,count);auto state=local_progress::seed(0,0,0,360);pass=pass&&local_progress::claim_career(state,id,0,values,count)&&!local_progress::claim_career(state,id,0,values,count);ok=ok&&pass;++boxes;fprintf(f,"{\"phase\":\"career_box\",\"id\":%d,\"tokens\":%d,\"pass\":%s}\n",id,state.supply.tokens,pass?"true":"false");}
    bool isolated=!memcmp(backup.data,&progress,sizeof(progress));local_supply::list_for(progress,local_faction::active);
    fprintf(f,"{\"summary\":true,\"pass\":%s,\"configs\":%d,\"private_sides\":%d,\"weapon_links\":%d,\"box_messages\":%d,\"career_boxes\":%d,\"player_state_unchanged\":%s,\"player_save_writes\":0,\"player_equips\":0,\"battle_runs\":0}\n",ok&&isolated?"true":"false",configs,sides,links,rewards,boxes,isolated?"true":"false");fclose(f);
}
}
