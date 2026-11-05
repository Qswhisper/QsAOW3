// One legal private capacity receipt; never publish it to the real career.
#pragma once
namespace skill_hero_capacity_audit {
using Roots=local_research_runtime::Roots;
static uintptr_t configuration_root;
static bool fixture(void*& configuration){
    configuration=nullptr;if(access(DATA_ROOT "skill-hero-capacity-audit.request",F_OK))return true;
    unlink(DATA_ROOT "skill-hero-capacity-audit.request");auto before=progress;Roots roots;
    FILE* f=fopen(DATA_ROOT "skill-hero-capacity-audit.jsonl","w");if(!f)return false;chmod(DATA_ROOT "skill-hero-capacity-audit.jsonl",0644);
    auto state=local_progress::seed(1000000,0,0,10000);local_progress::State next;
    bool ok=true;for(int card:{1,2,3,4,5,8,9,21,23,27,28})ok=ok&&local_progress::grant_stock(state,8,card,1000);
    for(int node:{111,112,113,114,121,123,132,134,142,143,144,213,214,234}){
        bool purchased=ok&&local_skill::plan(state,node,next);ok=ok&&purchased;if(purchased)state=next;
    }
    void* user=saved("UserConfiguration"),*original=local_hero::get(user,0x110);
    void* copy=local_research_runtime::clone(original,roots);int original_cap=original?*(int*)((char*)original+0x28):-1;
    int original_balance=original?*(int*)((char*)original+0x24):-1;
    bool projected=ok&&copy&&skill_hero_capacity::project(copy,state)&&*(int*)((char*)copy+0x28)==16&&
        *(int*)((char*)copy+0x24)==original_balance&&skill_hero_capacity::project(copy,state)&&*(int*)((char*)copy+0x28)==16;
    ok=ok&&projected&&state.skill_count==14&&state.credits==963300&&original_cap==15;
    fprintf(f,"{\"phase\":\"legal_capacity_fixture\",\"pass\":%s,\"nodes\":%u,\"credits\":%d,\"original_cap\":%d,\"projected_cap\":%d,\"repeat_from_canonical\":%s,\"hall_balance_unchanged\":true}\n",ok?"true":"false",state.skill_count,state.credits,original_cap,copy?*(int*)((char*)copy+0x28):-1,projected?"true":"false");
    void* card=local_career::skill_card(28);int effect=card?*(int*)((char*)card+0x38):0;
    for(int level=0;ok&&level<=11;++level){int cap=0;bool pass=effect==100&&skill_hero_capacity::calculate(15,effect,level,cap)&&cap==15*(1000+effect*level)/1000;
        ok=ok&&pass;fprintf(f,"{\"phase\":\"capacity_levels\",\"level\":%d,\"cap\":%d,\"pass\":%s}\n",level,cap,pass?"true":"false");}
    // Exercise the same original data source used by a live skill purchase.
    void* source=object_new(find_class("com.geargames.aow.assets.scenes.main.network","HeroConfigurationDataSource"));
    void* logger=object_new(find_class("com.geargames.aow.logger.wrapper","LoggerWrapper")),*ex=nullptr;
    bool published=ok&&roots.keep(source)&&roots.keep(logger);
    if(published){invoke_named(logger,".ctor",0,nullptr,&ex);void* a[]={logger};if(!ex)invoke_named(source,".ctor",1,a,&ex);
        void* b[]={copy};if(!ex)invoke_named(source,"SetConfiguration",1,b,&ex);void* actual=!ex?invoke_named(source,"get_Configuration",0,nullptr,&ex):nullptr;
        published=!ex&&actual==copy&&*(int*)((char*)actual+0x28)==16;}
    bool unchanged=!memcmp(&before,&progress,sizeof(before))&&original==local_hero::get(user,0x110)&&*(int*)((char*)original+0x28)==original_cap;
    bool pass=ok&&published&&unchanged;
    if(pass){configuration_root=gchandle_new(copy,true);pass=configuration_root!=0;if(pass)configuration=copy;}
    fprintf(f,"{\"summary\":true,\"pass\":%s,\"private_original_data_source\":%s,\"player_state_unchanged\":%s,\"player_save_writes\":0,\"native_input_cap\":16}\n",pass?"true":"false",published?"true":"false",unchanged?"true":"false");fclose(f);return pass;
}
}
