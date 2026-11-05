// Opt-in legal skill receipt and one original hero-death cooldown sample.
// Runs only in the existing no-save hero battle fixture.
#pragma once
namespace skill_hero_cooldown_audit {
using Roots=local_research_runtime::Roots;
static bool pending,passed;static local_progress::State before;
static const char* path=DATA_ROOT "skill-hero-cooldown-audit.jsonl";
static void* get(void* p,int o){return p?*(void**)((char*)p+o):nullptr;}
static void log(const char* fmt,...){FILE* f=fopen(path,"a");if(!f)return;va_list a;va_start(a,fmt);vfprintf(f,fmt,a);va_end(a);fputc('\n',f);fclose(f);chmod(path,0644);}
static bool fixture(void* side){
    if(access(DATA_ROOT "skill-hero-cooldown-audit.request",F_OK))return true;
    unlink(DATA_ROOT "skill-hero-cooldown-audit.request");before=progress;pending=true;passed=true;
    FILE* f=fopen(path,"w");if(f)fclose(f);
    auto state=local_progress::seed(1000000,0,0,10000);local_progress::State next;
    for(int card:{1,2,3,4,5,8,9,21,23,27,29})passed=passed&&local_progress::grant_stock(state,8,card,1000);
    for(int node:{111,112,113,114,121,123,132,134,142,143,144,164}){
        if(node==164){auto low=state;low.stars=9;auto poor=state;poor.credits=3399;auto empty=state;
            passed=passed&&!local_skill::plan(low,node,next)&&!local_skill::plan(poor,node,next)&&
                local_progress::consume_stock(empty,8,29,991)&&!local_skill::plan(empty,node,next);}
        bool bought=passed&&local_skill::plan(state,node,next);passed=passed&&bought;if(bought)state=next;
    }
    skill_tree_policy::Summary summary;
    bool legal=passed&&local_skill::graph.summarize(state,summary)&&state.skill_count==12&&summary.level[29]==1;
    bool duplicate=!local_skill::plan(state,164,next);
    // Independent missing-ancestor check; a duplicate must not mask this guard.
    auto missing=local_progress::seed(1000000,0,0,10000);
    bool ancestor=local_progress::grant_stock(missing,8,29,1000)&&!local_skill::plan(missing,164,next);
    bool projected=legal&&skill_combat::prepare(side,state)&&skill_flag_audit::coefficient(side,29,3);
    passed=legal&&duplicate&&ancestor&&projected&&local_progress::stock_amount(state,8,29)==990;
    log("{\"phase\":\"legal_cooldown_receipt\",\"pass\":%s,\"nodes\":%u,\"credits\":%d,\"card29_stock\":%d,\"key29\":3,\"player_save_writes\":0}",passed?"true":"false",state.skill_count,state.credits,local_progress::stock_amount(state,8,29));
    void* card=local_career::skill_card(29);int effect=card?*(int*)((char*)card+0x38):0;
    for(int level=0;passed&&level<=11;++level){Roots roots;void* skill=object_new(find_class("com.geargames.aow.entities","Skill"));void* ex=nullptr;
        int maximum=11,cards=0,base=300,raw[100]={};raw[29]=effect*level;
        void* clone=local_research_runtime::clone(side,roots);bool ok=effect==30&&roots.keep(skill)&&clone;
        void* a[]={card,&level,&maximum,&cards};if(ok)invoke_named(skill,".ctor",4,a,&ex);
        void* b[]={&base};void* value=ok&&!ex?invoke_named(skill,"applyDown",1,b,&ex):nullptr;
        int result=value&&!ex?*(int*)((char*)value+16):-1,expected=base*(100-3*level)/100;
        ok=ok&&!ex&&result==expected&&skill_chest::prepare(clone,0,0,raw)&&skill_flag_audit::coefficient(clone,29,3*level);
        passed=passed&&ok;log("{\"phase\":\"original_applydown_and_projection\",\"level\":%d,\"duration\":%d,\"expected\":%d,\"pass\":%s}",level,result,expected,ok?"true":"false");
    }
    return passed;
}
static void run(void* battle,void* hero){
    if(!pending)return;pending=false;void* side=get(hero,0x18),*parameter=get(side,0x238),*slots=get(side,0x78);
    int slot=0,base=parameter?*(int*)((char*)parameter+0xa4):-1,now=skill_flag_audit::tick(battle);
    void* ex=nullptr;void* lookup[]={side,get(hero,0x40)};
    void* found=runtime_invoke(class_method(find_class("com.geargames.aow.entities.battle","SideAct"),"$Rf",2),nullptr,lookup,&ex);
    slot=found&&!ex?*(int*)((char*)found+16):-1;
    bool ok=passed&&base>0&&slots&&slot>=0&&(size_t)slot<array_length(slots)&&now>=0&&skill_flag_audit::coefficient(side,29,3);
    int old=ok?((int*)((char*)slots+32))[slot]:-1;
    int16_t cause=0;void* bullet=nullptr,*shooter=nullptr;void* args[]={battle,hero,&cause,bullet,shooter};
    if(ok)runtime_invoke(class_method(find_class("com.geargames.aow.entities.battle","UnitAct"),"$NH",5),nullptr,args,&ex);
    int end=ok?((int*)((char*)slots+32))[slot]:-1,expected=now+base*97/100;
    ok=ok&&!ex&&end==expected&&end!=old;
    log("{\"phase\":\"original_hero_death\",\"pass\":%s,\"base_ticks\":%d,\"tick\":%d,\"slot\":%d,\"old_deadline\":%d,\"deadline\":%d,\"expected\":%d,\"exception\":\"%s\"}",ok?"true":"false",base,now,slot,old,end,expected,ex?class_name(object_class(ex)):"none");
    bool unchanged=!memcmp(&before,&progress,sizeof(before));
    log("{\"summary\":true,\"pass\":%s,\"original_hero_death\":%s,\"player_state_unchanged\":%s,\"player_save_writes\":0,\"real_player_result\":false}",ok&&unchanged?"true":"false",ok?"true":"false",unchanged?"true":"false");
}
}
