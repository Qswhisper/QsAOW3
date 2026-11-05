// Original skill/controller/research-price consumers on private model objects.
#pragma once
namespace skill_research_discount_audit {
using Roots=local_skill::Roots;
static void* get(void* p,int o){return p?*(void**)((char*)p+o):nullptr;}
static void run(){
    if(access(DATA_ROOT "skill-research-discount-audit.request",F_OK))return;
    unlink(DATA_ROOT "skill-research-discount-audit.request");auto before=progress;Roots roots;
    FILE* f=fopen(DATA_ROOT "skill-research-discount-audit.jsonl","w");if(!f)return;chmod(DATA_ROOT "skill-research-discount-audit.jsonl",0644);
    auto state=local_progress::seed(1000000,0,0,10000);local_progress::State next;
    bool ok=true;for(int card:{1,2,3,4,5,8,21,23,27,29,38})ok=ok&&local_progress::grant_stock(state,8,card,1000);
    for(int node:{111,112,113,114,121,123,132,134,142,143,144,164,174}){
        if(node==174){auto low=state;low.stars=9;auto poor=state;poor.credits=6799;auto empty=state;
            ok=ok&&!local_skill::plan(low,node,next)&&!local_skill::plan(poor,node,next)&&
                local_progress::consume_stock(empty,8,38,996)&&!local_skill::plan(empty,node,next);}
        bool bought=ok&&local_skill::plan(state,node,next);ok=ok&&bought;if(bought)state=next;
    }
    ok=ok&&state.skill_count==13&&state.credits==969400&&local_progress::stock_amount(state,8,38)==995&&!local_skill::plan(state,174,next);
    auto missing=local_progress::seed(1000000,0,0,10000);
    ok=ok&&local_progress::grant_stock(missing,8,38,1000)&&!local_skill::plan(missing,174,next);
    fprintf(f,"{\"phase\":\"legal_research_discount_receipt\",\"pass\":%s,\"nodes\":%u,\"credits\":%d,\"card38_stock\":%d,\"original_rank_price_card_ancestor_duplicate_guards\":true}\n",ok?"true":"false",state.skill_count,state.credits,local_progress::stock_amount(state,8,38));
    void* info=ok?local_skill::info(saved("UserConfiguration"),state,roots):nullptr;
    void* klass=find_class("com.geargames.aow.scenes.main.gui.skills","SkillsController"),*controller=object_new(klass);
    void* field=klass?class_field_from_name(klass,"m_skillByCard"):nullptr;
    void* dictionary=field?object_new(class_from_type(field_type(field))):nullptr;void* ex=nullptr;
    ok=ok&&info&&roots.keep(controller)&&roots.keep(dictionary);
    if(ok)invoke_named(dictionary,".ctor",0,nullptr,&ex);
    void* rows=get(info,0x20);int count=rows?*(int*)((char*)rows+0x18):0;
    for(int i=0;ok&&!ex&&i<count;++i){void* skill=list_item(rows,i),*card=get(skill,0x10);int id=card?*(int*)((char*)card+0x10):0;
        void* a[]={&id,skill};invoke_named(dictionary,"Add",2,a,&ex);}
    if(ok&&!ex){local_skill::ref(controller,"m_skillInfo",info);local_skill::ref(controller,"m_skillByCard",dictionary);}
    int id=38;void* a[]={&id};void* selected=ok&&!ex?invoke_named(controller,"GetSkillByCardId",1,a,&ex):nullptr;
    ok=ok&&!ex&&selected&&get(selected,0x10)==local_career::skill_card(38)&&*(int*)((char*)selected+0x18)==1;
    void* calc=find_class("com.geargames.aow.ugui.army.models","UpgradeCostCalculator");int cases=0;
    for(int level=0;ok&&level<=11;++level){*(int*)((char*)selected+0x18)=level;
        for(int price:{0,1,49,1003,65535,INT_MAX}){void* err=nullptr;void* args[]={&price,selected};
            void* result=runtime_invoke(class_method(calc,"TryApplyDiscountBySkills",2),nullptr,args,&err);
            int actual=result&&!err?*(int*)((char*)result+16):-1;
            int expected=(int)(int64_t(price)*(1000-20*level)/1000);
            bool pass=!err&&actual==expected;ok=ok&&pass;++cases;
            fprintf(f,"{\"phase\":\"original_research_price\",\"level\":%d,\"base\":%d,\"price\":%d,\"expected\":%d,\"pass\":%s}\n",level,price,actual,expected,pass?"true":"false");}
    }
    int price=1003;void* none=nullptr;void* fallback[]={&price,none};ex=nullptr;
    void* value=runtime_invoke(class_method(calc,"TryApplyDiscountBySkills",2),nullptr,fallback,&ex);
    bool no_skill=value&&!ex&&*(int*)((char*)value+16)==price;
    bool unchanged=!memcmp(&before,&progress,sizeof(before));
    fprintf(f,"{\"summary\":true,\"pass\":%s,\"price_cases\":%d,\"original_controller_selection\":%s,\"no_skill_preserved\":%s,\"player_state_unchanged\":%s,\"player_save_writes\":0,\"battle_runs\":0}\n",ok&&no_skill&&unchanged?"true":"false",cases,selected?"true":"false",no_skill?"true":"false",unchanged?"true":"false");fclose(f);
}
}
