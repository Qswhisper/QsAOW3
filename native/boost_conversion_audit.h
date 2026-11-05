// Retained conversion recipes, original UI cost fields and publication shapes.
// No player click, inventory mutation, external message or save.
#pragma once
namespace boost_conversion_audit {
using Roots=local_faction::Roots;
static void* get(void* p,int o){return p?*(void**)((char*)p+o):nullptr;}
static void run(){
    if(access(DATA_ROOT "boost-conversion-audit.request",F_OK))return;
    unlink(DATA_ROOT "boost-conversion-audit.request");auto before=progress;
    FILE* f=fopen(DATA_ROOT "boost-conversion-audit.jsonl","w");if(!f)return;chmod(DATA_ROOT "boost-conversion-audit.jsonl",0644);
    void* rows=local_career::list("BATTLE","boosts"),*misc=get(saved("UserConfiguration"),0x178);
    bool ok=local_boost_conversion::installed&&rows&&misc&&!strcmp(class_name(object_class(misc)),"MiscConfiguration");
    int blue=misc?*(int*)((char*)misc+0x10):-1,red=misc?*(int*)((char*)misc+0x14):-1;
    fprintf(f,"{\"phase\":\"retained_conversion_prices\",\"hooks\":%s,\"blue\":%d,\"red\":%d}\n",local_boost_conversion::installed?"true":"false",blue,red);
    void* active=static_object("com.geargames.aow.config","ClientContext","m_activeProfile"),*error=nullptr;
    void* live=active?invoke_named(active,"get_Boosts",0,nullptr,&error):nullptr;
    void* user_profile=get(saved("UserConfiguration"),0x28),*user_stock=get(user_profile,0x98);
    auto count=[&](void* v){void* e=nullptr;void* b=v?invoke_named(v,"get_Count",0,nullptr,&e):nullptr;return b&&!e?*(int*)((char*)b+16):-1;};
    fprintf(f,"{\"phase\":\"runtime_inventory_source\",\"active_profile_boosts\":%d,\"user_boosts\":%d,\"same_dictionary\":%s,\"same_profile\":%s,\"exception\":\"%s\"}\n",count(live),count(user_stock),live==user_stock?"true":"false",get(active,0x10)==user_profile?"true":"false",error?class_name(object_class(error)):"none");
    ok=ok&&blue==20&&red==20;int mapped=0,unmapped=0,views=0;
    for(int i=0;ok&&i<*(int*)((char*)rows+0x18);++i){Roots roots;void* source=list_item(rows,i),*out=local_boost_conversion::target(source);
        int category=source?*(int8_t*)((char*)source+0x2a):-1,id=source?*(int*)((char*)source+0x10):-1;
        if(category==2){ok=ok&&!out;continue;}
        if(!out){++unmapped;continue;}++mapped;
        bool match=*(int8_t*)((char*)out+0x2a)==category+1&&
            local_boost_conversion::same_text(get(source,0x18),get(out,0x18))&&local_boost_conversion::same_actions(get(source,0x30),get(out,0x30));
        if(id==0||id==1)match=match&&*(int*)((char*)out+0x10)==id+1;
        void* button=object_new(find_class("com.geargames.aow.scenes.main.gui","GUIMainStoreButtonSwapBoostConvert"));
        bool view=match&&roots.keep(button);if(view)local_skill::ref(button,"m_goods",source);
        for(int multiple:{1,3}){if(!view)break;*(int*)((char*)button+0x80)=multiple;
            local_boost_conversion::modifier(button,nullptr);int price=*(int*)((char*)button+0x30);
            view=price==20*multiple;++views;}
        local_boost_conversion::Publication publication;
        bool messages=view&&local_boost_conversion::messages(source,out,20,1,roots,publication);
        void* ex=nullptr;void* removed=messages?invoke_named(publication.remove,"get_Values",0,nullptr,&ex):nullptr;
        void* added=messages&&!ex?invoke_named(publication.add,"get_Values",0,nullptr,&ex):nullptr;
        void* entry=removed?list_item(removed,0):nullptr;int amount=-1;void* a[]={out,&amount};
        void* found=added&&!ex?invoke_named(added,"TryGetValue",2,a,&ex):nullptr;
        messages=messages&&!ex&&entry&&get(entry,0x10)==source&&*(int*)((char*)entry+0x18)==20&&
            found&&*(bool*)((char*)found+16)&&amount==1;
        bool pass=match&&view&&messages;ok=ok&&pass;
        fprintf(f,"{\"phase\":\"retained_conversion_pair\",\"source\":%d,\"target\":%d,\"category\":%d,\"view\":%s,\"original_message_values\":%s,\"pass\":%s,\"exception\":\"%s\"}\n",id,*(int*)((char*)out+0x10),category,view?"true":"false",messages?"true":"false",pass?"true":"false",ex?class_name(object_class(ex)):"none");
    }
    void* card=local_career::skill_card(30);int effect=card?*(int*)((char*)card+0x38):0,cases=0;
    for(int level=0;ok&&level<=11;++level){Roots roots;void* skill=object_new(find_class("com.geargames.aow.entities","Skill"));void* ex=nullptr;
        int max=11,cards=0,base=20,expected=0;bool pass=effect==50&&roots.keep(skill);
        void* a[]={card,&level,&max,&cards};if(pass)invoke_named(skill,".ctor",4,a,&ex);
        void* b[]={&base};void* value=pass&&!ex?invoke_named(skill,"applyDown",1,b,&ex):nullptr;
        int actual=value&&!ex?*(int*)((char*)value+16):-1;
        pass=pass&&!ex&&boost_conversion_policy::price(base,effect,level,expected)&&actual==expected&&actual==20-level;
        ok=ok&&pass;++cases;fprintf(f,"{\"phase\":\"original_conversion_discount\",\"level\":%d,\"price\":%d,\"expected\":%d,\"pass\":%s}\n",level,actual,expected,pass?"true":"false");
    }
    // Exercise the real client inventory handlers on a detached profile only.
    bool publication_ok=false;{
        Roots roots;local_research_runtime::Roots clone_roots;void* ex=nullptr;
        void* source=local_career::lookup("BATTLE","boosts",0),*out=local_boost_conversion::target(source);
        void* profile=local_research_runtime::clone(user_profile,clone_roots);
        void* stock=user_stock?object_new(object_class(user_stock)):nullptr;
        void* wrapper=active?object_new(object_class(active)):nullptr;
        publication_ok=profile&&source&&out&&roots.keep(stock)&&roots.keep(wrapper);
        int fifty=50,two=2;
        if(publication_ok){
            invoke_named(stock,".ctor",0,nullptr,&ex);
            void* a[]={source,&fifty};if(!ex)invoke_named(stock,"Add",2,a,&ex);
            void* b[]={out,&two};if(!ex)invoke_named(stock,"Add",2,b,&ex);
            local_skill::ref(profile,"boosts",stock);
            void* c[]={profile};if(!ex)invoke_named(wrapper,".ctor",1,c,&ex);
        }
        local_boost_conversion::Publication p;
        publication_ok=publication_ok&&!ex&&local_boost_conversion::messages(source,out,20,1,roots,p);
        if(publication_ok){
            void* removed=invoke_named(p.remove,"get_Values",0,nullptr,&ex);
            void* added=!ex?invoke_named(p.add,"get_Values",0,nullptr,&ex):nullptr;
            void* a[]={removed};if(!ex)invoke_named(wrapper,"DeleteBoosts",1,a,&ex);
            void* b[]={added};if(!ex)invoke_named(wrapper,"AddBoosts",1,b,&ex);
        }
        int left=-1,right=-1;
        void* a[]={source,&left};if(publication_ok&&!ex)invoke_named(stock,"TryGetValue",2,a,&ex);
        void* b[]={out,&right};if(publication_ok&&!ex)invoke_named(stock,"TryGetValue",2,b,&ex);
        publication_ok=publication_ok&&!ex&&left==30&&right==3&&stock!=user_stock;
        fprintf(f,"{\"phase\":\"original_detached_inventory_publication\",\"source_before\":50,\"source_after\":%d,\"target_before\":2,\"target_after\":%d,\"pass\":%s,\"exception\":\"%s\"}\n",left,right,publication_ok?"true":"false",ex?class_name(object_class(ex)):"none");
    }
    ok=ok&&publication_ok;
    bool unchanged=!memcmp(&before,&progress,sizeof(before));
    fprintf(f,"{\"summary\":true,\"pass\":%s,\"mapped_pairs\":%d,\"unmapped_non_gold\":%d,\"view_cases\":%d,\"price_levels\":%d,\"player_state_unchanged\":%s,\"player_save_writes\":0,\"player_inventory_debits\":0}\n",ok&&mapped>0&&cases==12&&unchanged?"true":"false",mapped,unmapped,views,cases,unchanged?"true":"false");fclose(f);
}
}
