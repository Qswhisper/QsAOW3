// One batch: private original skill source + legal transaction + effect routes.
#pragma once
namespace skill_runtime_audit {
using local_research_runtime::Roots;
static void* get(void* p,int o){return p?*(void**)((char*)p+o):nullptr;}
static void* skill(void* info,int id){void* list=get(info,0x20);for(int i=0;list&&i<*(int*)((char*)list+0x18);++i){void* s=list_item(list,i);if(*(int*)((char*)get(s,0x10)+0x10)==id)return s;}return nullptr;}
static bool weapon_values(void* old,void* next,void* s){
    if(!old&&!next)return true;if(!old||!next||*(int*)((char*)old+0x18)!=*(int*)((char*)next+0x18))return false;
    for(int i=0;i<*(int*)((char*)old+0x18);++i){void* a=list_item(old,i),*b=list_item(next,i);if(!a||!b||a==b)return false;
        for(int j=0;j<3;++j){int base=*(int*)((char*)a+0x28+j*4);
            int expected=reinterpret_cast<int(*)(int,void*,bool,void*)>(image_base+0x7f37f88)(base,s,true,nullptr);
            if(*(int*)((char*)b+0x28+j*4)!=expected||*(int*)((char*)b+0x34+j*4)!=expected)return false;}
        for(int j=0;j<i;++j)if(list_item(old,j)==a&&list_item(next,j)!=b)return false;
    }return true;
}
static void run(){
    if(!progress_ready||access(DATA_ROOT "skill-runtime-audit.request",F_OK))return;
    unlink(DATA_ROOT "skill-runtime-audit.request");auto before=progress;bool ok=local_skill::initialize();
    FILE* f=fopen(DATA_ROOT "skill-runtime-audit.jsonl","w");if(!f)return;chmod(DATA_ROOT "skill-runtime-audit.jsonl",0644);
    local_skill::Roots models;auto fixture=local_progress::seed(10000,0,0,10000);
    ok=ok&&local_progress::grant_stock(fixture,8,1,20)&&local_progress::grant_stock(fixture,8,3,20)&&local_progress::grant_stock(fixture,8,8,20);
    void* start=ok?local_skill::info(saved("UserConfiguration"),fixture,models):nullptr;
    auto low=fixture;low.stars=9;local_progress::State next;
    bool rank_refused=!local_skill::plan(low,111,next),unknown_refused=!local_skill::plan(fixture,113,next);
    ok=ok&&rank_refused&&unknown_refused&&local_skill::plan(fixture,111,next);if(ok)fixture=next;
    void* first=ok?local_skill::info(saved("UserConfiguration"),fixture,models):nullptr;
    ok=ok&&local_skill::plan(fixture,121,next);if(ok)fixture=next;
    void* learned=ok?local_skill::info(saved("UserConfiguration"),fixture,models):nullptr;
    void* source=object_new(find_class("com.geargames.aow.assets.scenes.main.network","SkillInfoDataSource"));
    void* logger=object_new(find_class("com.geargames.aow.logger.wrapper","LoggerWrapper"));void* ex=nullptr;
    ok=ok&&models.keep(source)&&models.keep(logger)&&start&&first&&learned;
    if(ok){invoke_named(logger,".ctor",0,nullptr,&ex);void* args[]={logger};if(!ex)invoke_named(source,".ctor",1,args,&ex);
        void* info_args[]={start};if(!ex)invoke_named(source,"SetSkillInfo",1,info_args,&ex);
        int milestone=1;int first_index=local_skill::graph.index(111),second_index=local_skill::graph.index(121);
        void* a[]={skill(first,1),local_skill::originals[first_index],&milestone};if(!ex)invoke_named(source,"SetSkill",3,a,&ex);
        void* b[]={skill(learned,3),local_skill::originals[second_index],&milestone};if(!ex)invoke_named(source,"SetSkill",3,b,&ex);
        void* actual=!ex?invoke_named(source,"get_Configuration",0,nullptr,&ex):nullptr;
        ok=!ex&&actual&&get(actual,0x10)&&*(int*)((char*)get(actual,0x10)+0x18)==2&&
            skill(actual,1)&&*(int*)((char*)skill(actual,1)+0x18)==1&&*(int*)((char*)skill(actual,1)+0x20)==0&&
            skill(actual,3)&&*(int*)((char*)skill(actual,3)+0x18)==1&&fixture.credits==6600&&fixture.skill_count==2;
    }
    fprintf(f,"{\"phase\":\"original_source_transaction\",\"pass\":%s,\"rank_gate_refused\":%s,\"unsupported_effect_refused\":%s,\"credits\":%d,\"skill_nodes\":%u,\"exception\":\"%s\"}\n",ok?"true":"false",rank_refused?"true":"false",unknown_refused?"true":"false",fixture.credits,fixture.skill_count,ex?class_name(object_class(ex)):"none");
    int unit_cases[100]={},building_cases=0;bool effects=ok;
    void* cards[128];int count=local_career::skill_cards(cards,128);
    for(int i=0;effects&&i<count;++i){int id=*(int*)((char*)cards[i]+0x10);if(!skill_combat::supported(id)||id==8||id==23)continue;
        local_skill::Roots one;int level=1,max=11,amount=0;void* s=object_new(find_class("com.geargames.aow.entities","Skill"));
        if(!one.keep(s)){effects=false;break;}void* args[]={cards[i],&level,&max,&amount};ex=nullptr;invoke_named(s,".ctor",4,args,&ex);if(ex){effects=false;break;}
        int bonus=*(int*)((char*)cards[i]+0x38);void* values=local_career::list("BATTLE",id==2||id==3?"buildingTypes":"unitTypes");
        for(int j=0;effects&&values&&j<*(int*)((char*)values+0x18);++j){void* type=list_item(values,j);
            if(id!=2&&id!=3&&skill_combat::unit_card(type)!=id)continue;
            Roots roots;skill_combat::Weapons weapons;void* copy=id==2||id==3?skill_combat::building(type,id==2?bonus:0,id==3?bonus:0,weapons,roots):skill_combat::unit(type,bonus,weapons,roots);
            bool pass=copy&&copy!=type;void* old_levels=get(type,0x18),*new_levels=get(copy,0x18);
            for(int k=0;pass&&old_levels&&k<*(int*)((char*)old_levels+0x18);++k){void* old=list_item(old_levels,k),*now=list_item(new_levels,k);
                if(id==3){int base=*(int*)((char*)old+0x64),expected=base;void* arg[]={&base};void* value=base>0?invoke_named(s,"applyDown",1,arg,&ex):nullptr;
                    if(base>0)expected=value&&!ex?*(int*)((char*)value+16):-1;pass=expected==*(int*)((char*)now+0x64);}
                else pass=weapon_values(get(old,0x18),get(now,0x18),s);
            }
            if(!pass)fprintf(f,"{\"effect_failure\":true,\"card\":%d,\"prototype\":%d}\n",id,*(int*)((char*)type+0x10));
            effects=effects&&pass;if(id==2||id==3)++building_cases;else ++unit_cases[id];
        }
    }
    fprintf(f,"{\"phase\":\"original_effect_getters\",\"pass\":%s,\"infantry\":%d,\"vehicles\":%d,\"aircraft\":%d,\"navy\":%d,\"building_cases\":%d}\n",effects?"true":"false",unit_cases[1],unit_cases[32],unit_cases[33],unit_cases[19],building_cases);
    bool projection=ok;
    for(int faction:{1,2}){Roots roots;char path[512];snprintf(path,sizeof(path),DATA_ROOT "offline-seeds/fmc%dm.ggs",faction);
        void* config=campaign_seeds::decode(path,roots),*battle=get(config,0x10),*side=local_dictionary_item(get(battle,0x178),1),*enemy=local_dictionary_item(get(battle,0x178),2);
        void* units=get(side,0xf0),*buildings=get(side,0xf8),*other=get(enemy,0xf0);
        uint64_t original=hero_battle_projection_audit::type_hash(units),other_hash=hero_battle_projection_audit::type_hash(other);
        bool pass=original&&other_hash&&skill_combat::project(side,fixture,units,buildings)&&skill_combat::project(side,fixture,units,buildings)&&
            original==hero_battle_projection_audit::type_hash(units)&&other_hash==hero_battle_projection_audit::type_hash(other)&&get(enemy,0xf0)==other;
        projection=projection&&pass;fprintf(f,"{\"phase\":\"immutable_battle_projection\",\"faction\":%d,\"pass\":%s}\n",faction,pass?"true":"false");
    }
    char root[]=DATA_ROOT "skill-save-fixture-XXXXXX";bool persisted=ok&&mkdtemp(root)&&local_progress::write(root,fixture);local_progress::State restored;
    persisted=persisted&&local_progress::load(root,restored)==1&&!memcmp(&fixture,&restored,sizeof(fixture));
    bool isolated=!memcmp(&before,&progress,sizeof(progress));
    fprintf(f,"{\"summary\":true,\"pass\":%s,\"graph_nodes\":%d,\"supported_cards\":6,\"private_save_round_trip\":%s,\"private_save_root\":\"%s\",\"player_state_unchanged\":%s,\"player_saves_written\":false,\"battle_started\":false}\n",ok&&effects&&projection&&persisted&&isolated?"true":"false",local_skill::graph.count,persisted?"true":"false",persisted?root:"",isolated?"true":"false");
    fclose(f);LOG("Local skill runtime batch audit pass=%d",ok&&effects&&projection&&persisted&&isolated);
}
}
