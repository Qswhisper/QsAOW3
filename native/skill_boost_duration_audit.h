// Fresh original battle objects only; no inventory debit, player battle or save.
#pragma once
namespace skill_boost_duration_audit {
using Roots=local_research_runtime::Roots;
static void* get(void* p,int o){return p?*(void**)((char*)p+o):nullptr;}
static void run(){
    if(access(DATA_ROOT "skill-boost-duration-audit.request",F_OK))return;
    unlink(DATA_ROOT "skill-boost-duration-audit.request");auto before=progress;
    FILE* f=fopen(DATA_ROOT "skill-boost-duration-audit.jsonl","w");if(!f)return;chmod(DATA_ROOT "skill-boost-duration-audit.jsonl",0644);
    void* original=local_career::lookup("BATTLE","boosts",1),*card=local_career::skill_card(35);
    int effect=card?*(int*)((char*)card+0x38):0;bool ok=original&&effect==20;int cases=0;
    const auto* route=local_scenario(202);
    for(int level=0;ok&&level<=11;++level){Roots roots;
        void* message=read_scenario_message(route->file,5040,"@skill-boost-duration-audit"),*battle=get(message,0x20);
        void* side=local_dictionary_item(get(battle,0x178),1),*type=local_research_runtime::clone(original,roots);
        void* commands=object_new(find_class("com.geargames.aow.entities.battle","CommandAdd"));void* ex=nullptr;
        bool pass=side&&type&&roots.keep(message)&&roots.keep(commands);
        bool reset=false;void* init[]={&reset};if(pass)invoke_named(side,"$hU",1,init,&ex);
        if(pass&&!ex)invoke_named(commands,".ctor",0,nullptr,&ex);
        pass=pass&&!ex&&skill_flag_audit::put(battle,0x208,nullptr);
        if(pass)local_skill::ref(battle,"$EB",commands);
        int tick=1234,base=10049,raw[100]={};raw[35]=effect*level;
        void* set_tick[]={&tick};if(pass)invoke_named(battle,"$ro",1,set_tick,&ex);
        if(pass){*(int16_t*)((char*)type+0x40)=base;*(int*)((char*)side+0x2f4)=0;}
        pass=pass&&!ex&&skill_chest::prepare(side,0,0,raw)&&skill_flag_audit::coefficient(side,35,2*level);
        void* args[]={battle,side,type};if(pass)runtime_invoke(class_method(find_class("com.geargames.aow.entities.battle","SideAct"),"$zf",3),nullptr,args,&ex);
        void* active=get(side,0x3d0);int expected=base*(100+2*level)/100;
        // Direct original zf result at tick1234 establishes field18=start, field1c=end.
        int start=active?*(int*)((char*)active+0x18):-1,end=active?*(int*)((char*)active+0x1c):-1;
        pass=pass&&!ex&&active&&get(active,0x10)==type&&start==tick&&end==tick+expected;
        // Original BoostActive fields are obfuscated; report both actual values
        // so a failed interval assumption cannot be hidden by a tolerance.
        ok=ok&&pass;++cases;
        fprintf(f,"{\"phase\":\"original_boost_duration\",\"level\":%d,\"base\":%d,\"start\":%d,\"end\":%d,\"duration\":%d,\"expected\":%d,\"pass\":%s,\"exception\":\"%s\"}\n",level,base,start,end,end-start,expected,pass?"true":"false",ex?class_name(object_class(ex)):"none");
    }
    auto missing=local_progress::seed(1000000,0,0,10000);local_progress::State next;
    bool guard=local_progress::grant_stock(missing,8,35,1000)&&!local_skill::plan(missing,274,next);
    bool unchanged=!memcmp(&before,&progress,sizeof(before));
    fprintf(f,"{\"summary\":true,\"pass\":%s,\"levels\":%d,\"missing_ancestor_refused\":%s,\"player_state_unchanged\":%s,\"inventory_debits\":0,\"player_save_writes\":0,\"battle_runs\":0}\n",ok&&cases==12&&guard&&unchanged?"true":"false",cases,guard?"true":"false",unchanged?"true":"false");fclose(f);
}
}
