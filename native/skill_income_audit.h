// Legal original node chain and original integer/native income consumers.
#pragma once
namespace skill_income_audit {
using Roots=local_research_runtime::Roots;
static void* get(void* p,int o){return p?*(void**)((char*)p+o):nullptr;}
static void run(){
    if(access(DATA_ROOT "skill-income-audit.request",F_OK))return;
    unlink(DATA_ROOT "skill-income-audit.request");auto before=progress;
    FILE* f=fopen(DATA_ROOT "skill-income-audit.jsonl","w");if(!f)return;chmod(DATA_ROOT "skill-income-audit.jsonl",0644);
    auto state=local_progress::seed(1000000,0,0,10000);local_progress::State next;bool ok=true;
    for(int card:{1,2,3,4,5,8,9,21,23,27,34})ok=ok&&local_progress::grant_stock(state,8,card,1000);
    for(int node:{111,112,113,114,121,123,132,134,142,143,144,213,214,242}){
        if(node==242){auto low=state;low.stars=9;auto poor=state;poor.credits=15999;auto empty=state;
            ok=ok&&!local_skill::plan(low,node,next)&&!local_skill::plan(poor,node,next)&&
                local_progress::consume_stock(empty,8,34,993)&&!local_skill::plan(empty,node,next);}
        bool bought=ok&&local_skill::plan(state,node,next);ok=ok&&bought;if(bought)state=next;
    }
    auto missing=local_progress::seed(1000000,0,0,10000);
    ok=ok&&state.skill_count==14&&state.credits==955400&&local_progress::stock_amount(state,8,34)==992&&
        !local_skill::plan(state,242,next)&&local_progress::grant_stock(missing,8,34,1000)&&!local_skill::plan(missing,242,next);
    fprintf(f,"{\"phase\":\"legal_income_receipt\",\"pass\":%s,\"nodes\":%u,\"credits\":%d,\"card34_stock\":%d,\"rank_price_card_ancestor_duplicate_guards\":true}\n",ok?"true":"false",state.skill_count,state.credits,local_progress::stock_amount(state,8,34));
    fflush(f);
    void* card=local_career::skill_card(34);int effect=card?*(int*)((char*)card+0x38):0,cases=0;
    void* controller=find_class("com.geargames.aow.scenes.main.gui.skills","SkillsController");
    for(int level=0;ok&&level<=11;++level){Roots roots;void* ex=nullptr;
        void* skill=object_new(find_class("com.geargames.aow.entities","Skill"));int max=11,cards=0,base_percent=100;bool visual=false;
        bool pass=effect==15&&roots.keep(skill);void* a[]={card,&level,&max,&cards};if(pass)invoke_named(skill,".ctor",4,a,&ex);
        void* b[]={&base_percent,skill,&visual};
        void* boxed=pass&&!ex?runtime_invoke(class_method(controller,"GetBoostedValueInt",3),nullptr,b,&ex):nullptr;
        int percent=boxed&&!ex?*(int*)((char*)boxed+16)-100:-1;
        void* message=pass?read_scenario_message(local_scenario(202)->file,5040,"@skill-income-audit"):nullptr,*battle=get(message,0x20);
        void* side=local_dictionary_item(get(battle,0x178),1);pass=pass&&side&&roots.keep(message)&&!ex;
        bool reset=false;void* init[]={&reset};if(pass)invoke_named(side,"$hU",1,init,&ex);
        void* building_class=find_class("com.geargames.aow.entities","Building");
        // Original gf scans the fixed 100 building slots, including null slots.
        // A zero-length array violates that runtime contract before income math.
        void* empty=pass&&building_class?array_new(building_class,100):nullptr;
        pass=pass&&!ex&&roots.keep(empty)&&skill_flag_audit::put(side,0x148,empty);
        int raw[100]={};raw[34]=effect*level;
        pass=pass&&skill_chest::prepare(side,0,0,raw)&&skill_flag_audit::coefficient(side,34,percent)&&percent==15*level/10;
        int base=10049;if(pass){*(int*)((char*)side+0x174)=base;*(int8_t*)((char*)side+0x178)=100;}
        void* c[]={battle,side};boxed=pass?runtime_invoke(class_method(find_class("com.geargames.aow.entities.battle","SideAct"),"$gf",2),nullptr,c,&ex):nullptr;
        int actual=boxed&&!ex?*(int*)((char*)boxed+16):-1,expected=base*(100+percent)/100;
        pass=pass&&!ex&&actual==expected;ok=ok&&pass;++cases;
        fprintf(f,"{\"phase\":\"original_integer_income\",\"level\":%d,\"raw\":%d,\"percent\":%d,\"base\":%d,\"income\":%d,\"expected\":%d,\"pass\":%s,\"exception\":\"%s\"}\n",level,raw[34],percent,base,actual,expected,pass?"true":"false",ex?class_name(object_class(ex)):"none");
        fflush(f);
    }
    bool unchanged=!memcmp(&before,&progress,sizeof(before));
    fprintf(f,"{\"summary\":true,\"pass\":%s,\"levels\":%d,\"player_state_unchanged\":%s,\"player_save_writes\":0,\"battle_runs\":0}\n",ok&&cases==12&&unchanged?"true":"false",cases,unchanged?"true":"false");fclose(f);
}
}
