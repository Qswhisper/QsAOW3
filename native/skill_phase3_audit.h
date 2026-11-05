// Legal phase2 bridge plus original deployment duration; no player writes.
#pragma once
namespace skill_phase3_audit {
using Roots=local_research_runtime::Roots;
static bool pending,passed;static local_progress::State before;
static const char* path=DATA_ROOT "skill-phase3-audit.jsonl";
static void* get(void* p,int o){return p?*(void**)((char*)p+o):nullptr;}
static void log(const char* format,...){FILE* f=fopen(path,"a");if(!f)return;va_list a;va_start(a,format);vfprintf(f,format,a);va_end(a);fputc('\n',f);fclose(f);chmod(path,0644);}
static bool fixture(local_progress::State& state){
    if(access(DATA_ROOT "skill-phase3-audit.request",F_OK))return true;
    unlink(DATA_ROOT "skill-phase3-audit.request");before=progress;pending=true;passed=true;
    FILE* f=fopen(path,"w");if(f)fclose(f);
    state=local_progress::seed(1000000,0,0,10000);local_progress::State next;
    for(int card:{1,2,3,4,5,6,7,8,9,10,19,20,21,23,27,28,40})passed=passed&&local_progress::grant_stock(state,8,card,1000);
    for(int node:{111,112,113,114,121,123,132,134,142,143,144,211,213,214,221,233,234,241,244,251,253,261}){
        auto low=state;low.stars=9;bool guard=node!=241||!local_skill::plan(low,node,next);
        int credits=state.credits;bool bought=passed&&guard&&local_skill::plan(state,node,next);
        if(node==241)bought=bought&&next.credits==credits&&local_progress::stock_amount(next,8,11)==0;
        passed=passed&&bought;if(bought)state=next;
    }
    skill_tree_policy::Summary summary;passed=passed&&local_skill::graph.summarize(state,summary)&&summary.stage==3&&state.skill_count==22&&state.credits==914700;
    passed=passed&&!local_skill::plan(state,241,next);
    local_skill::Roots roots;void* info=passed?local_skill::info(saved("UserConfiguration"),state,roots):nullptr;
    void* skill=skill_waiver_audit::find_skill(info,11),*raw=local_career::skill_card(11),*shown=skill?get(skill,0x10):nullptr;
    void* free_node=local_skill::presented_node(local_skill::originals[local_skill::graph.index(241)]);
    bool view=skill&&shown&&shown!=raw&&shown==get(free_node,0x20)&&*(int*)((char*)shown+0x38)==0&&
        *(int*)((char*)free_node+0x28)==0&&*(int*)((char*)free_node+0x2c)==0&&*(int*)((char*)raw+0x38)==30;
    if(view){int value=3000;void* ex=nullptr;void* a[]={&value};void* output=invoke_named(skill,"applyDown",1,a,&ex);view=output&&!ex&&*(int*)((char*)output+16)==value;}
    char directory[]=DATA_ROOT "skill-phase3-fixture-XXXXXX";local_progress::State restored;
    bool saved=passed&&mkdtemp(directory)&&local_progress::write(directory,state)&&local_progress::load(directory,restored)==1&&!memcmp(&state,&restored,sizeof(state));
    passed=passed&&view&&saved;
    log("{\"phase\":\"legal_phase3_bridge\",\"pass\":%s,\"nodes\":%u,\"credits\":%d,\"stage\":%d,\"free_card11_view\":%s,\"private_save_reload\":%s,\"rank_and_duplicate_guards\":true,\"player_save_writes\":0}",passed?"true":"false",state.skill_count,state.credits,summary.stage,view?"true":"false",saved?"true":"false");return passed;
}
static bool prepared(void* side){
    if(!pending)return true;
    bool ok=skill_flag_audit::coefficient(side,6,5)&&skill_flag_audit::coefficient(side,7,1)&&skill_flag_audit::coefficient(side,10,2)&&skill_flag_audit::coefficient(side,20,2);
    passed=passed&&ok;log("{\"phase\":\"legal_phase3_projection\",\"pass\":%s,\"capture6\":5,\"flag7\":1,\"deployment10\":2,\"cp20\":2}",ok?"true":"false");return ok;
}
static bool native_duration(void* battle,int level,int& actual,int& expected,bool& negative_guard){
    Roots roots;void* source=local_dictionary_item(get(battle,0x178),1);
    void* b=local_research_runtime::clone(battle,roots),*side=local_research_runtime::clone(source,roots),*gai=local_research_runtime::clone(get(battle,0x140),roots);
    void* commands=gai?local_research_runtime::new_list(get(gai,0x30),roots):nullptr;
    void* sides=object_new(object_class(get(battle,0x178)));void* ex=nullptr;
    if(!b||!side||!gai||!commands||!roots.keep(sides))return false;
    invoke_named(sides,".ctor",0,nullptr,&ex);int id=1;void* a[]={&id,side};if(!ex)invoke_named(sides,"Add",2,a,&ex);
    if(ex||!skill_flag_audit::put(b,0x178,sides)||!skill_flag_audit::put(b,0x208,nullptr)||!skill_flag_audit::put(b,0x140,gai)||!skill_flag_audit::put(gai,0x30,commands))return false;
    int raw[100]={};raw[10]=20*level;if(!skill_chest::prepare(side,0,0,raw))return false;
    int now=skill_flag_audit::tick(b),base=10049;if(now<0)return false;
    *(int*)((char*)side+0x170)=123456;int negative=-1;void* invalid[]={b,&id,&negative};
    runtime_invoke(class_method(find_class("com.geargames.aow.entities.battle","SideAct"),"$Ig",3),nullptr,invalid,&ex);
    negative_guard=!ex&&*(int*)((char*)side+0x170)==123456;
    void* input[]={b,&id,&base};if(!ex)runtime_invoke(class_method(find_class("com.geargames.aow.entities.battle","SideAct"),"$Ig",3),nullptr,input,&ex);
    // Original Ig includes this tick BEFORE percentage scaling. Base10049
    // distinguishes that order from an incorrect +1 after integer rounding.
    actual=*(int*)((char*)side+0x170)-now;expected=(base+1)*(100+2*level)/100;
    return !ex&&actual==expected&&negative_guard;
}
static bool run(void* battle){
    if(!pending)return false;pending=false;bool native=true;
    for(int level=0;level<=11;++level){int actual=-1,expected=-1;bool guard=false;
        bool ok=native_duration(battle,level,actual,expected,guard);native=native&&ok;
        log("{\"phase\":\"original_deployment_duration\",\"level\":%d,\"actual\":%d,\"expected\":%d,\"negative_guard\":%s,\"pass\":%s}",level,actual,expected,guard?"true":"false",ok?"true":"false");}
    bool unchanged=!memcmp(&before,&progress,sizeof(before));
    log("{\"summary\":true,\"pass\":%s,\"native_deployment_levels\":12,\"player_state_unchanged\":%s,\"player_save_writes\":0}",passed&&native&&unchanged?"true":"false",unchanged?"true":"false");return true;
}
}
