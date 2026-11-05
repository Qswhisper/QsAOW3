// Included inside local_duel after the side/alliance helpers. Original FlagAct
// owns creation, capture, recapture and economic/vision bonuses. The local
// rule adapter observes ownership and sends original progress/result commands.
#pragma once
namespace control_points {
struct Order {int unit=0,target=0,tick=-10000;bool holding=false;};
struct Session {int mode=1,scores[2]={},owner[3]={-1,-1,-1},since[3]={};void* flags[3]={};uintptr_t roots[3]={};Order orders[8][3];bool ready=false,draw=false;};
static Session session;
static void* get(void* p,int o){return p?*(void**)((char*)p+o):nullptr;}
static bool enabled(){return session.mode==5||session.mode==6;}
static void reset(int mode){for(auto h:session.roots)if(h)gchandle_free(h);session=Session{};session.mode=mode;}
static bool original_command(void* battle,const char* name,int n,void** args){void* out=nullptr;return local_bot_runtime::call(get(battle,0x260),name,n,args,out);}
static bool setup(void* battle,const ai_spawn_runtime::Layout& map,const ai_spawn_plan::Point* bases,int now){
    if(!enabled())return true;if(seat_count()!=2||session.ready)return false;ai_spawn_plan::Point positions[3];
    if(!control_point_plan::plan(map.mask,map.transit,map.width,map.height,bases,positions))return false;
    for(int i=0;i<3;++i){Roots roots;void* c=object_new(find_class("com.geargames.aow.entities.gai.gaicomm","GAICommandFlag"));if(!roots.keep(c))return false;
        void* ex=nullptr;invoke_named(c,".ctor",0,nullptr,&ex);if(ex)return false;
        *(int*)((char*)c+0x18)=1;*(int*)((char*)c+0x3c)=positions[i].x;*(int*)((char*)c+0x40)=positions[i].y;
        *(int*)((char*)c+0x44)=control_point_rules::flag_bonus;*(int8_t*)((char*)c+0x48)=control_point_rules::flag_sight;*(int8_t*)((char*)c+0x60)=-1;
        void* args[]={battle,c};void* flag=runtime_invoke(class_method(find_class("com.geargames.aow.entities.battle","FlagAct"),"$Jd",2),nullptr,args,&ex);
        if(!flag||ex)return false;session.flags[i]=flag;session.roots[i]=gchandle_new(flag,true);session.since[i]=now;if(!session.roots[i])return false;
        log("CONTROL_POINT_CREATED mode=%d index=%d id=%d cell=%d,%d bonus=%d sight=%d original_capture=1",session.mode,i,*(int*)((char*)flag+0x10),positions[i].x,positions[i].y,*(int*)((char*)flag+0x3c),*(int8_t*)((char*)flag+0x54));}
    if(session.mode==5){int target=control_point_rules::score_target;void* a=side(battle,1),*b=side(battle,2);void* args[]={&target,a,b};
        if(!original_command(battle,"addCommandProgressBarFlagsCreate",3,args))return false;}
    session.ready=true;return true;
}
static bool observe(void* battle,int now){
    if(!enabled())return true;if(!session.ready)return false;
    for(int i=0;i<3;++i){void* flag=session.flags[i];int owner=-1;void* s=get(flag,0x18);if(s)owner=alliance_of(battle,s);
        if(owner< -1||owner>1||now<session.since[i])return false;
        if(owner!=session.owner[i]){log("CONTROL_POINT_OWNER tick=%d id=%d previous=%d owner=%d native_capture_progress=%d",now,*(int*)((char*)flag+0x10),session.owner[i],owner,*(int*)((char*)flag+0x9c));session.owner[i]=owner;session.since[i]=now;}
        if(session.mode==5&&owner>=0){int steps=(now-session.since[i])/control_point_rules::score_interval_ticks;
            if(steps){int64_t next=int64_t(session.scores[owner])+int64_t(steps)*control_point_rules::score_per_flag;
                if(next>INT_MAX)return false;session.scores[owner]=int(next);session.since[i]+=steps*control_point_rules::score_interval_ticks;}}}
    if(session.mode==5){int zero=0;void* args[]={&session.scores[0],&session.scores[1],&zero,&zero};
        if(!original_command(battle,"addCommandProgressBarFlagsUpdate",4,args))return false;
        if(now%100==0)log("CONTROL_POINT_SCORE tick=%d scores=%d,%d target=%d actual_native_ownership=1",now,session.scores[0],session.scores[1],control_point_rules::score_target);}
    return true;
}
static bool order(void* battle,void* s,void* u,void* type,int now,int& scouts,bool& reserved){
    reserved=false;if(!enabled()||!session.ready||scouts>=3||*(int8_t*)((char*)type+0x30)<1||*(int8_t*)((char*)type+0x30)>2)return true;
    int id=*(int*)((char*)s+0x10);if(id<1||id>8||(!private_mode&&id==1)||seats[id-1].guard.defending)return true;
    int slot=scouts++,team=alliance_of(battle,s),uid=*(int*)((char*)u+0x10);auto& previous=session.orders[id-1][slot];
    control_point_policy::Point points[3];
    for(int i=0;i<3;++i){void* f=session.flags[i];points[i]={*(int*)((char*)f+0x10),*(int*)((char*)f+0x2c)+*(int*)((char*)f+0x34)/2,
        *(int*)((char*)f+0x30)+*(int*)((char*)f+0x38)/2,session.owner[i],true};
        for(int k=0;k<slot;++k)if(session.orders[id-1][k].target==points[i].id)points[i].active=false;}
    control_point_policy::Target chosen;int x=*(int16_t*)((char*)u+0x48),y=*(int16_t*)((char*)u+0x4a);
    if(!control_point_policy::choose(points,3,team,x,y,control_point_rules::capture_radius,chosen))return false;
    if(chosen.index<0){previous=Order{};return true;}reserved=true;
    if(chosen.hold){void* ex=nullptr;void* args[]={battle,u,session.flags[chosen.index]};
        void* value=runtime_invoke(class_method(find_class("com.geargames.aow.entities.battle","FlagAct"),"$Ld",3),nullptr,args,&ex);
        if(!value||ex)return false;chosen.hold=*(bool*)((char*)value+16);}
    if(previous.unit==uid&&previous.target==chosen.id&&((chosen.hold&&previous.holding)||(!chosen.hold&&now-previous.tick<100)))return true;
    Roots roots;void* command=ai_lab::command(chosen.hold?"AICommUnitsStop":"AICommUnitsMove",now,roots,id),*ids=array_new(find_class("System","Int32"),1);
    if(!command||!roots.keep(ids))return false;*(int*)((char*)ids+32)=uid;
    if(chosen.hold){if(!put(command,0x30,ids))return false;}
    else{*(int16_t*)((char*)command+0x2e)=points[chosen.index].x;*(int16_t*)((char*)command+0x30)=points[chosen.index].y;if(!put(command,0x40,ids))return false;}
    int code=ai_lab::perform(battle,command);if(ai_lab::stopped)return false;
    if(code){reserved=false;return true;}previous={uid,chosen.id,now,chosen.hold};
    log("CONTROL_POINT_ORDER tick=%d side=%d unit=%d flag=%d hold=%d original_command=1",now,id,uid,chosen.id,chosen.hold);return true;
}
}
