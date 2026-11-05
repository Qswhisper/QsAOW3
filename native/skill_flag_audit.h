// Explicit no-save flag fixture; shared native flag behavior, not new content.
#pragma once
namespace ai_lab {static void* command(const char*,int,local_research_runtime::Roots&,int);static int perform(void*,void*);}
namespace skill_flag_audit {
using Roots=local_research_runtime::Roots;
static bool pending,created,mechanisms;static void* unit,*owner,*flag;static uintptr_t unit_root,flag_root;
static int began,first_progress=-1,max_progress,configured_time;static local_progress::State before;
static int native_capture_calls;static bool eligibility_logged;
static void (*capture_original)(void*,void*,void*);
static const char* path=DATA_ROOT "skill-flag-audit.jsonl";
static void* get(void* p,int o){return p?*(void**)((char*)p+o):nullptr;}
static void log(const char* format,...){FILE* f=fopen(path,"a");if(!f)return;va_list a;va_start(a,format);vfprintf(f,format,a);va_end(a);fputc('\n',f);fclose(f);chmod(path,0644);}
static bool put(void* obj,int offset,void* value){auto barrier=(void(*)(void*,void**,void*))dlsym(il2cpp,"il2cpp_gc_wbarrier_set_field");if(!obj||!barrier)return false;barrier(obj,(void**)((char*)obj+offset),value);return true;}
static int tick(void* battle){void* ex=nullptr;void* v=invoke_named(battle,"$Qo",0,nullptr,&ex);return v&&!ex?*(int*)((char*)v+16):-1;}
static bool coefficient(void* side,int key,int expected){void* ex=nullptr;int value=-1;void* args[]={&key,&value};void* v=invoke_named(get(side,0x3e8),"TryGetValue",2,args,&ex);return v&&!ex&&*(bool*)((char*)v+16)&&value==expected;}
static void capture(void* battle,void* f,void* method){if(pending&&f==flag)++native_capture_calls;capture_original(battle,f,method);}
static bool eligible(void* battle,void* u,void* f){void* ex=nullptr;void* args[]={battle,u,f};void* v=runtime_invoke(class_method(find_class("com.geargames.aow.entities.battle","FlagAct"),"$Ld",3),nullptr,args,&ex);return v&&!ex&&*(bool*)((char*)v+16);}
static bool fixture(local_progress::State& state){
    if(access(DATA_ROOT "skill-flag-audit.request",F_OK))return true;
    FILE* f=fopen(path,"w");if(f)fclose(f);before=progress;
    state=local_progress::seed(1000000,0,0,10000);local_progress::State next;
    bool ok=true;for(int card:{1,2,3,4,5,6,8,9,20,21,23,27})ok=ok&&local_progress::grant_stock(state,8,card,1000);
    for(int node:{111,112,113,114,121,123,132,134,142,143,144,211,221,214}){
        bool bought=ok&&local_skill::plan(state,node,next);ok=ok&&bought;if(bought)state=next;}
    skill_tree_policy::Summary summary;ok=ok&&local_skill::graph.summarize(state,summary)&&summary.stage==2&&summary.level[6]==1&&summary.level[20]==1;
    log("{\"phase\":\"legal_phase2_fixture\",\"pass\":%s,\"nodes\":%u,\"capture_level\":1,\"cp_level\":1,\"unearned_attack7_fixture_only\":true,\"player_save_writes\":0}",ok?"true":"false",state.skill_count);return ok;
}
static bool prepared(void* side){
    if(access(DATA_ROOT "skill-flag-audit.request",F_OK))return true;
    bool ok=coefficient(side,6,5)&&coefficient(side,7,0)&&coefficient(side,20,2);
    log("{\"phase\":\"legal_saved_projection\",\"pass\":%s,\"key6\":5,\"key7\":0,\"key20\":2}",ok?"true":"false");return ok;
}
static bool attack_sample(void* battle,void* born,int level,bool disabled,int& amount){
    Roots roots;void* side=local_research_runtime::clone(get(born,0x18),roots),*copy=local_research_runtime::clone(born,roots),*b=local_research_runtime::clone(battle,roots);
    void* alliances=local_research_runtime::clone(get(battle,0x170),roots);
    int index=*(int8_t*)((char*)born+0x20);if(!side||!copy||!b||!alliances||index<0||index>=(int)array_length(alliances))return false;
    void* alliance=local_research_runtime::clone(((void**)((char*)alliances+32))[index],roots);
    void* f=object_new(find_class("com.geargames.aow.entities","Flag"));if(!alliance||!roots.keep(f))return false;
    void* ex=nullptr;invoke_named(f,".ctor",0,nullptr,&ex);if(ex)return false;
    void* flags=local_research_runtime::new_list(get(alliance,0xb8),roots);if(!flags||!local_skill::append(flags,f))return false;
    if(!put(alliances,32+index*8,alliance)||!put(b,0x170,alliances)||!put(alliance,0xb8,flags)||!put(copy,0x18,side)||!put(f,0x18,side))return false;
    int raw[100]={};raw[6]=50*level;raw[7]=10*level;if(!skill_chest::prepare(side,0,0,raw))return false;
    *(int16_t*)((char*)side+0x20c)=100;*(int16_t*)((char*)side+0x20e)=3;
    *(int*)((char*)f+0x2c)=*(int16_t*)((char*)born+0x48);*(int*)((char*)f+0x30)=*(int16_t*)((char*)born+0x4a);
    *(int*)((char*)f+0x34)=1;*(int*)((char*)f+0x38)=1;*(int*)((char*)f+0x48)=10;*(bool*)((char*)f+0x8c)=disabled;
    bool notify=false;void* args[]={b,copy,&notify};runtime_invoke(class_method(find_class("com.geargames.aow.entities.battle","FlagAct"),"$od",3),nullptr,args,&ex);
    if(ex)return false;amount=*(int16_t*)((char*)copy+0x134);return true;
}
static void finish(bool pass,int now){
    bool unchanged=!memcmp(&before,&progress,sizeof(before));
    log("{\"summary\":true,\"pass\":%s,\"natural_capture\":%s,\"original_attack_levels\":%s,\"configured_capture_ticks\":%d,\"player_state_unchanged\":%s,\"player_save_writes\":0,\"real_player_result\":false}",pass&&unchanged?"true":"false",pass?"true":"false",mechanisms?"true":"false",configured_time,unchanged?"true":"false");
    if(unit_root)gchandle_free(unit_root);if(flag_root)gchandle_free(flag_root);unit_root=flag_root=0;pending=false;
}
static void run(void* battle,void* born){
    if(access(DATA_ROOT "skill-flag-audit.request",F_OK))return;unlink(DATA_ROOT "skill-flag-audit.request");
    mechanisms=true;int cases=0;Roots roots;
    for(int level=0;level<=11;++level){int active=-1,disabled=-1;bool ok=attack_sample(battle,born,level,false,active)&&attack_sample(battle,born,level,true,disabled)&&active==13+level&&disabled==0;
        void* s=object_new(find_class("com.geargames.aow.entities","Skill"));Roots one;int maximum=11,cards=0,base=200;void* ex=nullptr;
        bool time=one.keep(s);void* a[]={local_career::skill_card(6),&level,&maximum,&cards};if(time)invoke_named(s,".ctor",4,a,&ex);
        void* c[]={&base};void* value=time&&!ex?invoke_named(s,"applyDown",1,c,&ex):nullptr;int duration=value&&!ex?*(int*)((char*)value+16):-1;
        ok=ok&&duration==200*(100-5*level)/100;mechanisms=mechanisms&&ok;++cases;
        log("{\"phase\":\"native_flag_levels\",\"level\":%d,\"attack_bonus\":%d,\"expected_attack\":%d,\"disabled_bonus\":%d,\"capture_applydown\":%d,\"pass\":%s}",level,active,13+level,disabled,duration,ok?"true":"false");
    }
    const uint32_t signature[]={0xd10203ff,0xfd000be8,0xa9027bfd,0xa9036ffc};
    capture_original=(decltype(capture_original))install_hook(0x473bc6c,signature,(void*)capture);
    unit=born;owner=get(born,0x18);began=tick(battle);first_progress=-1;max_progress=0;configured_time=190;created=false;eligibility_logged=false;native_capture_calls=0;unit_root=gchandle_new(unit,true);pending=unit_root!=0&&capture_original;
    log("{\"phase\":\"capture_wait_for_deployment\",\"tick\":%d,\"cases\":%d,\"cp\":%d,\"pass\":%s}",began,cases,*(int*)((char*)owner+0x34c),mechanisms?"true":"false");
    if(!pending||!mechanisms)finish(false,began);
}
static void step(void* battle){
    if(!pending)return;int now=tick(battle);
    if(!created){if(now-began<60)return;Roots roots;void* stop=ai_lab::command("AICommUnitsStop",now,roots,1),*ids=array_new(find_class("System","Int32"),1);
        if(!stop||!roots.keep(ids)){log("{\"failure\":\"stop_input\"}");finish(false,now);return;}
        *(int*)((char*)ids+32)=*(int*)((char*)unit+0x10);put(stop,0x30,ids);int code=ai_lab::perform(battle,stop);
        if(code){log("{\"failure\":\"original_stop_refused\",\"code\":%d}",code);finish(false,now);return;}
        void* cmd=object_new(find_class("com.geargames.aow.entities.gai.gaicomm","GAICommandFlag"));if(!roots.keep(cmd)){finish(false,now);return;}
        void* ex=nullptr;invoke_named(cmd,".ctor",0,nullptr,&ex);
        int x=*(int16_t*)((char*)unit+0x48),y=*(int16_t*)((char*)unit+0x4a),fx=x+2,fy=y-1;
        void* probe=object_new(find_class("com.geargames.aow.entities","Flag"));if(!roots.keep(probe)){finish(false,now);return;}
        invoke_named(probe,".ctor",0,nullptr,&ex);if(ex){finish(false,now);return;}
        // Zero retains the canonical side radius. Explicit Flag.CaptureRadius
        // is in logic units, not cells (Flag.XLogic uses cell*100).
        *(int*)((char*)probe+0x34)=3;*(int*)((char*)probe+0x38)=3;*(int*)((char*)probe+0x88)=0;
        const int offsets[][2]={{1,-1},{-3,-1},{-1,1},{-1,-3},{-1,-1}};bool found=false;
        for(const auto& offset:offsets){int px=x+offset[0],py=y+offset[1];if(px<1||py<1)continue;
            *(int*)((char*)probe+0x2c)=px;*(int*)((char*)probe+0x30)=py;
            if(eligible(battle,unit,probe)){fx=px;fy=py;found=true;break;}}
        log("{\"phase\":\"original_flag_position_predicate\",\"eligible\":%s,\"selected\":[%d,%d],\"side_capture_radius\":%d}",found?"true":"false",fx,fy,*(int16_t*)((char*)owner+0x210));
        if(!found){log("{\"failure\":\"original_position_contract_refused\"}");finish(false,now);return;}
        *(int*)((char*)cmd+0x34)=1;*(int*)((char*)cmd+0x3c)=fx;*(int*)((char*)cmd+0x40)=fy;*(int*)((char*)cmd+0x54)=10;*(int*)((char*)cmd+0x90)=0;*(int8_t*)((char*)cmd+0x60)=-1;
        void* args[]={battle,cmd};flag=!ex?runtime_invoke(class_method(find_class("com.geargames.aow.entities.battle","FlagAct"),"$Jd",2),nullptr,args,&ex):nullptr;
        if(!flag||ex){log("{\"failure\":\"original_flag_factory\",\"exception\":\"%s\"}",ex?class_name(object_class(ex)):"none");finish(false,now);return;}
        flag_root=gchandle_new(flag,true);created=flag_root!=0;began=now;
        log("{\"phase\":\"original_flag_created\",\"tick\":%d,\"id\":%d,\"unit_cell\":[%d,%d],\"flag_cell\":[%d,%d],\"size\":[%d,%d],\"capture_radius\":%d,\"native_stop_result\":%d}",now,*(int*)((char*)flag+0x10),x,y,*(int*)((char*)flag+0x2c),*(int*)((char*)flag+0x30),*(int*)((char*)flag+0x34),*(int*)((char*)flag+0x38),*(int*)((char*)flag+0x88),code);
        if(!created)finish(false,now);return;
    }
    if(!eligibility_logged&&now-began>=20){eligibility_logged=true;log("{\"phase\":\"native_flag_update_contract\",\"tick\":%d,\"capture_calls\":%d,\"unit_eligible\":%s}",now,native_capture_calls,eligible(battle,unit,flag)?"true":"false");}
    int progress_now=*(int*)((char*)flag+0x9c);if(progress_now>max_progress)max_progress=progress_now;
    if(first_progress<0&&progress_now>0)first_progress=now;
    if(get(flag,0x18)==owner){int elapsed=first_progress>=0?now-first_progress+1:-1;
        bool pass=first_progress>=0&&elapsed>=configured_time-2&&elapsed<=configured_time+2;
        log("{\"phase\":\"original_natural_flag_capture\",\"tick\":%d,\"first_progress_tick\":%d,\"elapsed_ticks\":%d,\"max_progress\":%d,\"capture_side\":%d,\"pass\":%s}",now,first_progress,elapsed,max_progress,*(int*)((char*)owner+0x10),pass?"true":"false");finish(pass,now);
    }else if(now-began>800){log("{\"failure\":\"natural_flag_timeout\",\"tick\":%d,\"max_progress\":%d,\"capture_calls\":%d,\"capture_side_present\":%s,\"unit_cell\":[%d,%d]}",now,max_progress,native_capture_calls,get(flag,0xa0)?"true":"false",*(int16_t*)((char*)unit+0x48),*(int16_t*)((char*)unit+0x4a));finish(false,now);}
}
}
