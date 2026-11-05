// Observe original consumption semantics on detached objects, never a run.
#pragma once
namespace battery_native_audit {
using Roots=local_items::Roots;
static void* get(void* p,int o){return p?*(void**)((char*)p+o):nullptr;}
static int amount(void* dictionary,void* key){int value=-1;void* ex=nullptr;void* args[]={key,&value};void* found=invoke_named(dictionary,"TryGetValue",2,args,&ex);
    return found&&!ex&&*(bool*)((char*)found+16)?value:-999;}
static void run(){
    if(!progress_ready||access(DATA_ROOT "battery-native-audit.request",F_OK))return;
    unlink(DATA_ROOT "battery-native-audit.request");auto before=progress;Roots roots;
    FILE* f=fopen(DATA_ROOT "battery-native-audit.jsonl","w");if(!f)return;chmod(DATA_ROOT "battery-native-audit.jsonl",0644);
    const auto* source=local_scenario(202);void* input=source?read_scenario_message(source->file,5040,"@battery-native-audit"):nullptr;
    void* battle=get(input,0x20),*side=local_dictionary_item(get(battle,0x178),1),*ex=nullptr;
    bool reset=false;void* init_args[]={&reset};if(side)invoke_named(side,"$hU",1,init_args,&ex);
    void* commands=object_new(find_class("com.geargames.aow.scenes.battle.logicservice","ClientCommandAdd"));
    bool ready=side&&!ex&&roots.keep(commands)&&local_hero_battle::add_original&&local_items::initialize();
    if(ready){invoke_named(commands,".ctor",0,nullptr,&ex);if(!ex){field_set(battle,class_field_from_name(object_class(battle),"$EB"),commands);field_set(battle,class_field_from_name(object_class(battle),"$pb"),side);
        // Original command dispatch establishes the sender. Calling a command
        // override without that context is an accepted no-op, not a use.
        // Getter Battle.$Ro/RVA4674fa4 reads this exact field (offset208).
        field_set(battle,class_field_from_name(object_class(battle),"$xb"),side);}}
    ready=ready&&!ex;int cases=0,exceptions=0;
    if(ready)for(int owned:{0,1,2})for(int id:{1,2,3})for(int energy:{0,1,8,14,15}){
        Roots row_roots;auto state=before;state.stock_count=3;for(int j=0;j<3;++j)state.stocks[j]={10,j+1,owned};
        void* old=get(get(saved("UserConfiguration"),0x110),0x18);
        void* stock=local_items::dictionary(old,state,10,row_roots);
        if(!stock){ready=false;break;}field_set(side,class_field_from_name(object_class(side),"$qF"),stock);
        *(int*)((char*)side+0x40)=energy;*(int*)((char*)side+0x48)=15;
        void* command=object_new(find_class("com.geargames.aow.entities.battle.aicomm","AICommHeroBatteryBuy"));
        if(!row_roots.keep(command)){ready=false;break;}ex=nullptr;invoke_named(command,".ctor",0,nullptr,&ex);
        *(int*)((char*)command+0x28)=1;*(int*)((char*)command+0x30)=id;
        local_hero_battle::capture_begin();void* args[]={battle};
        void* result=!ex?runtime_invoke(class_method(object_class(command),"$dKA",1),command,args,&ex):nullptr;
        int code=result&&!ex?*(int16_t*)((char*)result+16):-999;if(ex)++exceptions;++cases;
        fprintf(f,"{\"battery\":%d,\"owned_before\":%d,\"energy_before\":%d,\"cap\":15,\"code\":%d,\"exception\":\"%s\",\"energy_after\":%d,\"owned_after\":%d,\"energy_events\":%d,\"event_value\":%d,\"events\":[",id,owned,energy,code,ex?class_name(object_class(ex)):"none",*(int*)((char*)side+0x40),amount(stock,local_items::battery(id)),local_hero_battle::captured_energy_count,local_hero_battle::captured_energy);
        for(int j=0;j<local_hero_battle::captured_count;++j)fprintf(f,"%s\"%s\"",j?",":"",class_name(object_class(local_hero_battle::captured_objects[j])));
        fputs("]}\n",f);local_hero_battle::capture_end();
    }
    int transactions=0,passed=0;
    if(ready)for(int id:{1,2,3})for(int variant=0;variant<10;++variant){
        Roots row_roots;auto state=before;state.stock_count=3;
        for(int j=0;j<3;++j)state.stocks[j]={10,j+1,2};
        int energy=variant==0?0:variant==1?14:15;
        if(variant==3)state.stocks[id-1].amount=1;
        if(variant==4){state.stock_count=0;}
        if(variant==7)energy=INT_MAX;
        void* old=get(get(saved("UserConfiguration"),0x110),0x18);
        void* stock=local_items::dictionary(old,state,10,row_roots);if(!stock){ready=false;break;}
        field_set(side,class_field_from_name(object_class(side),"$qF"),stock);
        *(int*)((char*)side+0x40)=energy;*(int*)((char*)side+0x48)=15;
        int count=local_progress::stock_amount(state,10,id);
        if(variant==5)local_item_guards::set_amount(stock,local_items::battery(id),count+1);
        field_set(battle,class_field_from_name(object_class(battle),"$xb"),variant==8?nullptr:side);
        void* command=object_new(find_class("com.geargames.aow.entities.battle.aicomm","AICommHeroBatteryBuy"));
        if(!row_roots.keep(command)){ready=false;break;}ex=nullptr;invoke_named(command,".ctor",0,nullptr,&ex);
        *(int*)((char*)command+0x28)=1;*(int*)((char*)command+0x30)=variant==9?99:id;
        char root[640];snprintf(root,sizeof(root),DATA_ROOT "battery-private-%d-%d",id,variant);mkdir(root,0700);
        char failed_root[680];snprintf(failed_root,sizeof(failed_root),"%s/not-created",root);
        auto initial=state;int before_stock=amount(stock,local_items::battery(id));
        int code=ex?-999:local_item_guards::execute(command,battle,nullptr,state,variant==6?failed_root:root);
        bool expect=variant<=3;
        local_progress::StateBuffer loaded;int load=local_progress::load(root,*loaded.data);
        bool pass=expect?(code==0&&*(int*)((char*)side+0x40)==energy+*(int*)((char*)local_items::battery(id)+0x14)&&
            amount(stock,local_items::battery(id))==count-1&&local_progress::stock_amount(state,10,id)==count-1&&state.generation==initial.generation+1&&
            load==1&&!memcmp(loaded.data,&state,sizeof(state))):
            (code!=0&&!memcmp(&initial,&state,sizeof(state))&&*(int*)((char*)side+0x40)==energy&&amount(stock,local_items::battery(id))==before_stock&&load==0);
        if(expect&&variant==3){int next=local_item_guards::execute(command,battle,nullptr,state,root);pass&=next!=0&&local_progress::stock_amount(state,10,id)==0;}
        if(expect&&variant==2){
            void* saved_battle=local_hero_battle::prepared_battle;int saved_side=local_hero_battle::prepared_side;
            local_hero_battle::prepared_battle=battle;local_hero_battle::prepared_side=1;
            int remaining=energy+*(int*)((char*)local_items::battery(id)+0x14)-1;
            local_hero_battle::energy(commands,side,remaining,nullptr);pass&=*(int*)((char*)side+0x40)==remaining;
            local_hero_battle::prepared_battle=saved_battle;local_hero_battle::prepared_side=saved_side;
        }
        ++transactions;passed+=pass;
        fprintf(f,"{\"transaction\":true,\"battery\":%d,\"variant\":%d,\"pass\":%s,\"code\":%d,\"energy_after\":%d,\"stock_after\":%d,\"cold_load\":%d}\n",id,variant,pass?"true":"false",code,*(int*)((char*)side+0x40),amount(stock,local_items::battery(id)),load);
    }
    bool unchanged=!memcmp(&before,&progress,sizeof(before));
    fprintf(f,"{\"summary\":true,\"complete\":true,\"fixture_ready\":%s,\"cases\":%d,\"exceptions\":%d,\"transactions\":%d,\"passed\":%d,\"player_unchanged\":%s,\"player_save_writes\":0,\"scene_runs\":0}\n",ready?"true":"false",cases,exceptions,transactions,passed,unchanged?"true":"false");fclose(f);
}
}
