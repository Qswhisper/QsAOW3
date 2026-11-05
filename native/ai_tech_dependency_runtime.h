// Included inside local_duel after its original command/placement helpers.
#pragma once
static int producer_site_goal[8]={};
static int tech_plan_notice[8]={};
static int tech_site_notice[8]={};
static int tech_quota_notice[8]={};
struct ProducerSiteProbe {
    Roots roots;void* command=nullptr,*battle=nullptr,*side=nullptr,*prototype=nullptr,*own=nullptr,*other=nullptr;const void* method=nullptr;bool refused=false;int checked=0,accepted=0,last=0;
    bool init(void* c,void* b,void* s,void* p){
        command=c;battle=b;side=s;prototype=p;void* alliances=*(void**)((char*)b+0x170),*exemplar=nullptr;size_t n=alliances?array_length(alliances):0;if(n>8)return false;
        for(size_t i=0;i<n&&!exemplar;++i){void* a=((void**)((char*)alliances+32))[i];for(int offset:{0x78,0x80}){void* list=a?*(void**)((char*)a+offset):nullptr;if(list&&!strcmp(class_name(object_class(list)),"List`1")){exemplar=list;break;}}}
        if(!exemplar)return false;own=object_new(object_class(exemplar));other=object_new(object_class(exemplar));if(!roots.keep(own)||!roots.keep(other))return false;
        void* ex=nullptr;invoke_named(own,".ctor",0,nullptr,&ex);if(!ex)invoke_named(other,".ctor",0,nullptr,&ex);method=class_method(object_class(c),"$zk",9);return !ex&&method;
    }
    bool permits(int x,int y){
        // Original aK passes static[0x14]^static[0x18] == 1: one cell
        // clearance, not zero. Zero accepts sites the actual command rejects.
        void* ex=nullptr;invoke_named(own,"Clear",0,nullptr,&ex);if(!ex)invoke_named(other,"Clear",0,nullptr,&ex);int16_t angle=*(int16_t*)((char*)command+0x44);int extra=1;void* args[]={battle,side,prototype,&x,&y,&angle,own,other,&extra};
        void* result=!ex?runtime_invoke(method,command,args,&ex):nullptr;if(ex||!result){refused=true;return false;}last=*(int16_t*)((char*)result+16);++checked;if(!last)++accepted;return !last;
    }
};
static bool prepare_tech_graph(void* s,Seat& seat){
    if(seat.tech_graph_ready)return true;if(seat.tech_graph_refused)return false;
    producer_site_goal[*(int*)((char*)s+0x10)-1]=0;
    ai_tech_dependencies::Graph graph;uintptr_t handle=0;
    void* values=local_dictionary_values(*(void**)((char*)s+0xf8),find_class("com.geargames.aow.entities","BuildingType"),handle);
    bool ok=values&&handle&&array_length(values)<=64;
    for(size_t j=0;ok&&j<array_length(values);++j){void* type=((void**)((char*)values+32))[j],*levels=type?*(void**)((char*)type+0x18):nullptr;
        int n=levels?*(int*)((char*)levels+0x18):0;if(n<1||n>16){ok=false;break;}int previous=-1;
        for(int k=0;ok&&k<n;++k){void* level=list_item(levels,k),*needs=level?*(void**)((char*)level+0x98):nullptr;
            if(!level){ok=false;break;}void* text=*(void**)((char*)level+0x78);
            int count=needs?*(int*)((char*)needs+0x18):0;
            if((!needs&&text&&*(int*)((char*)text+0x10))||count<0||count>16){ok=false;break;}
            ai_tech_dependencies::Node node;node.type=*(int*)((char*)type+0x10);node.tier=k;node.previous=previous;node.price=*(int*)((char*)level+0x6c);node.count=count;
            for(int i=0;i<count;++i){void* need=list_item(needs,i);if(!need){ok=false;break;}node.needs[i]={*(int*)((char*)need+0x10),*(int8_t*)((char*)need+0x14)};}
            int id=*(int*)((char*)level+0x10);if(ok)ok=graph.add(id,node);previous=id;
        }
    }
    if(handle)gchandle_free(handle);
    if(ok){seat.tech_graph=graph;seat.tech_graph_ready=true;}else seat.tech_graph_refused=true;
    log("TECH_GRAPH side=%d ready=%d original_recipes=%d full_ids=1",*(int*)((char*)s+0x10),ok,graph.size);return ok;
}
static bool observe_tech(void* s,ai_tech_dependencies::Observation& out){
    void* counters=*(void**)((char*)s+0x350);if(!counters||!array_length(counters)||array_length(counters)>1024)return false;
    out.limit=array_length(counters)<100?array_length(counters):100;
    for(int j=0;j<out.limit;++j)out.completed[j]=((int16_t*)((char*)counters+32))[j];
    void* buildings=*(void**)((char*)s+0x148);if(buildings&&array_length(buildings)>512)return false;
    for(size_t j=0;buildings&&j<array_length(buildings);++j){void* b=((void**)((char*)buildings+32))[j];
        if(!b||*((bool*)b+0x139)||*(int*)((char*)b+0x5c)<=0||*((uint8_t*)b+0x47)==1)continue;
        void* type=*(void**)((char*)b+0x38),*levels=type?*(void**)((char*)type+0x18):nullptr;
        int tier=*((int8_t*)b+0x60)+(*((uint8_t*)b+0x47)==3?1:0);
        if(!levels||tier<0||tier>=*(int*)((char*)levels+0x18))return false;
        void* level=list_item(levels,tier);int key=level?ai_tech_dependencies::index(*(int*)((char*)level+0x10)):-1;
        if(key<0)return false;if(key<out.limit)++out.pending[key];
    }
    return true;
}
static bool satisfy_tech_requirements(void* battle,void* s,void* goal,int now,bool construct_goal=false,int quantity=1,const ai_spawn_plan::Point* focus=nullptr,int native_depth=0){
    int id=*(int*)((char*)s+0x10);Seat& seat=seats[id-1];
    bool planned=offline_world_runtime::planner(id);
    if(!prepare_tech_graph(s,seat))return false;ai_tech_dependencies::Observation observed;
    if(!observe_tech(s,observed)){if(now%1000==0)log("TECH_COUNTER_WAIT side=%d original_array=0",id);return false;}
    int key=*(int*)((char*)goal+0x10);auto result=seat.tech_graph.choose(key,quantity,observed);
    if(naval_test&&now>=tech_plan_notice[id-1]){
        tech_plan_notice[id-1]=now+1000;int counter=ai_tech_dependencies::index(key),dependency=ai_tech_dependencies::index(result.key);
        log("TECH_PLAN tick=%d side=%d goal=%d quantity=%d status=%d result=%d completed=%d pending=%d dependency_completed=%d dependency_pending=%d gold=%d next=%d",now,id,key,quantity,(int)result.status,result.key,observed.completed[counter],observed.pending[counter],dependency>=0?observed.completed[dependency]:-1,dependency>=0?observed.pending[dependency]:-1,ai_lab::gold(s),seat.next_tech);
    }
    if(result.status!=ai_tech_dependencies::Action){
        if(planned&&(result.status==ai_tech_dependencies::Waiting||result.status==ai_tech_dependencies::Unsupported)&&now>=tech_plan_notice[id-1]){
            tech_plan_notice[id-1]=now+1000;log("PLAN_BLOCK tick=%d side=%d goal=%d prerequisite=%d status=%s",now,id,key,result.key,result.status==ai_tech_dependencies::Waiting?"original-construction-pending":"recipe-unsupported");}
        return false;
    }
    if(result.key==key&&!construct_goal)return true;
    int position=seat.tech_graph.find(result.key);if(position<0)return false;const auto& node=seat.tech_graph.nodes[position];
    if(node.price<=0)return false; // No scripted/free alias construction.
    void* prototype=local_dictionary_item(*(void**)((char*)s+0xf8),node.type);if(!prototype)return false;
    // Parsed contributions are only the first part of original $Mf. Its
    // $nf tail also checks the level's HqCount limits. Do not send dozens of
    // placement commands when this original prerequisite already refuses.
    void* original_levels=*(void**)((char*)prototype+0x18);
    void* original_level=original_levels?list_item(original_levels,node.tier):nullptr;
    auto native_requirements=reinterpret_cast<bool(*)(void*,void*,void*,void*,void*)>(image_base+0x47ed228);
    if(!original_level)return false;
    if(!native_requirements(battle,s,original_level,prototype,nullptr)){
        if(planned&&now>=tech_quota_notice[id-1]){tech_quota_notice[id-1]=now+1000;log("PLAN_BLOCK tick=%d side=%d goal=%d action=%d status=native-prerequisite-or-hq-quota",now,id,key,result.key);}
        void* limits=*(void**)((char*)original_level+0x88);
        void* hq=ai_lab::building(s,base_id(s)),*hq_type=hq?*(void**)((char*)hq+0x38):nullptr;
        void* hq_levels=hq_type?*(void**)((char*)hq_type+0x18):nullptr;
        int current=hq?*(int8_t*)((char*)hq+0x60):-1,total=hq_levels?*(int*)((char*)hq_levels+0x18):0;
        if(naval_test&&now>=tech_quota_notice[id-1]){tech_quota_notice[id-1]=now+1000;
            log("TECH_NATIVE_REQUIREMENT tick=%d side=%d goal=%d action=%d type=%d hq_tier=%d hq_limits=%d values=%d,%d,%d,%d,%d original_allowed=0",now,id,key,result.key,node.type,current,limits?(int)array_length(limits):0,
                limits&&array_length(limits)>0?((int*)((char*)limits+32))[0]:-1,limits&&array_length(limits)>1?((int*)((char*)limits+32))[1]:-1,limits&&array_length(limits)>2?((int*)((char*)limits+32))[2]:-1,limits&&array_length(limits)>3?((int*)((char*)limits+32))[3]:-1,limits&&array_length(limits)>4?((int*)((char*)limits+32))[4]:-1);}
        // Improving the real HQ is a bounded AI choice, not a quota override.
        // Re-enter the same paid original recipe path for all its inputs.
        if(node.type!=base_id(s)&&limits&&array_length(limits)>0&&array_length(limits)<=16&&native_depth<8&&hq&&*(uint8_t*)((char*)hq+0x47)==1&&current>=0&&current+1<total)
            satisfy_tech_requirements(battle,s,list_item(hq_levels,current+1),now,true,1,nullptr,native_depth+1);
        return false;
    }
    if(!planned){seat.reserve_tech=true;if(ai_lab::gold(s)<node.price||now<seat.next_tech)return false;seat.next_tech=now+100;}
    else if(now<seat.next_tech)return false;
    if(node.tier>0){
        void* target=nullptr,*buildings=*(void**)((char*)s+0x148);
        for(size_t j=0;buildings&&j<array_length(buildings);++j){void* b=((void**)((char*)buildings+32))[j];
            if(b&&*(void**)((char*)b+0x38)==prototype&&!*((bool*)b+0x139)&&*(int*)((char*)b+0x5c)>0&&*((uint8_t*)b+0x47)==1&&*((int8_t*)b+0x60)==node.tier-1){target=b;break;}}
        if(!target){seat.reserve_tech=false;return false;}
        if(planned){seat.reserve_tech=true;if(!budget(s,key,result.key,ai_economy::Upgrade,node.price,now))return false;seat.next_tech=now+100;}
        Roots roots;void* command=ai_lab::command("AICommBuildingUpgrade",now,roots,id);if(!command){fail("tech-dependency-upgrade-command");return false;}
        *(int*)((char*)command+0x30)=*(int*)((char*)target+0x10);int before=ai_lab::gold(s),code=ai_lab::perform(battle,command);seat.reserve_tech=false;
        if(ai_lab::stopped){fail("tech-dependency-upgrade-exception");return false;}
        log("TECH_DEPENDENCY_UPGRADE tick=%d side=%d goal=%d type=%d tier=%d code=%d gold=%d->%d",now,id,key,node.type,node.tier,code,before,ai_lab::gold(s));return false;
    }
    void* hq=ai_lab::building(s,base_id(s));if(!hq){seat.reserve_tech=false;return false;}
    Roots roots;void* command=ai_lab::command("AICommBuSet",now,roots,id);if(!command){fail("tech-dependency-build-command");return false;}
    *(int*)((char*)command+0x30)=node.type;*(int*)((char*)command+0x3c)=++sequence;
    bool water_site=*(int8_t*)((char*)prototype+0x30)==11&&naval_enabled();
    bool observe_water_command=water_site&&naval_test&&!access(DATA_ROOT "ai-advanced-trace.enabled",F_OK);
    if(water_site&&!naval_map(battle)){seat.reserve_tech=false;return false;}
    ProducerSiteProbe complete_site;bool full_probe=advanced_enabled()||naval_enabled()||planned;
    if(full_probe&&!complete_site.init(command,battle,s,prototype)){fail("original-full-site-preflight-init");return false;}
    auto valid=reinterpret_cast<bool(*)(void*,void*,int,int,int16_t,void*)>(image_base+0x4922a7c);
    auto covered=reinterpret_cast<bool(*)(void*,void*,void*,int,int,void*)>(image_base+0x47e7f08);
    int hx=*(int16_t*)((char*)hq+0x40),hy=*(int16_t*)((char*)hq+0x42),tested=0,code=-1;
    int water_distance=INT_MAX,water_outside_area=0;if(water_site)naval_focus_valid[id-1]=false;
    if(focus){hx=focus->x;hy=focus->y;}
    // A successful preceding producer must not lend its search cursor to a
    // different footprint/recipe. Each new goal starts with its nearby cells.
    bool full_search=construct_goal||full_probe;
    if(full_search&&producer_site_goal[id-1]!=result.key){producer_site_goal[id-1]=result.key;seat.producer_site_cursor=0;}
    int stride=full_search?1:2,ordinal=0,start=full_search?seat.producer_site_cursor:0;
    int area_cells=0,geometry_cells=0;
    for(int r=full_search?1:4;r<=24&&tested<32;r+=stride)for(int dy=-r;dy<=r&&tested<32;dy+=stride)for(int dx=-r;dx<=r&&tested<32;dx+=stride){
        if(abs(dx)!=r&&abs(dy)!=r)continue;int x=hx+dx,y=hy+dy;if(x<0||y<0)continue;
        if(full_search&&ordinal++<start)continue;
        if(full_search)seat.producer_site_cursor=ordinal;
        // A building coordinate is an original anchor, not a promise that
        // its centre is water. Let native checks choose shore-side anchors.
        if(water_site&&!naval_water.coastal_site(x,y))continue;
        bool orientations[4]={};constexpr int16_t angles[]={0,90,180,270};bool prefilter=false;
        for(int a=0;a<(water_site?4:1);++a){orientations[a]=valid(battle,prototype,x,y,angles[a],nullptr);prefilter=prefilter||orientations[a];}
        if(!prefilter)continue;++geometry_cells;
        // The original land-area shortcut rejects every sampled shipyard
        // site on map2. Let the complete native water predicate and actual
        // build command own their coverage/shore/orientation checks.
        if(full_search&&!water_site&&!covered(battle,s,prototype,x,y,nullptr))continue;++area_cells;
        *(int16_t*)((char*)command+0x40)=x;*(int16_t*)((char*)command+0x42)=y;
        if(water_site){bool found=false;for(int a=0;a<4;++a){if(!orientations[a])continue;
            *(int16_t*)((char*)command+0x44)=angles[a];if(observe_water_command||complete_site.permits(x,y)){found=true;break;}}
            if(complete_site.refused){fail("original-full-water-site-preflight-call");return false;}if(!found)continue;
            // $zk validates occupancy/terrain. The actual $aK ALSO requires
            // SideAct.$if; the four-call original trace proves this boundary.
            // Expand toward a native-valid anchor, not arbitrary nearest water.
            if(!covered(battle,s,prototype,x,y,nullptr)){
                ++water_outside_area;int dx=x-hx,dy=y-hy,distance=dx*dx+dy*dy;
                if(distance<water_distance){water_distance=distance;naval_focus[id-1]={x,y};naval_focus_valid[id-1]=true;}
                continue;
            }
        }else if(full_probe&&!complete_site.permits(x,y)){if(complete_site.refused){fail("original-full-site-preflight-call");return false;}continue;}
        if(planned){seat.reserve_tech=true;if(!budget(s,key,result.key,ai_economy::Build,node.price,now))return false;seat.next_tech=now+100;}
        int before=ai_lab::gold(s);code=ai_lab::perform(battle,command);++tested;
        if(ai_lab::stopped){fail("tech-dependency-build-exception");return false;}
        if(!code){seat.reserve_tech=false;log("TECH_DEPENDENCY_BUILD tick=%d side=%d goal=%d type=%d cell=%d,%d gold=%d->%d original_count=%d geometry_prefilter=%d native_validator=1",now,id,key,node.type,x,y,before,ai_lab::gold(s),observed.completed[ai_tech_dependencies::index(result.key)],prefilter);
            if(water_site)log("NAVAL_SITE tick=%d side=%d type=%d cell=%d,%d angle=%d component=%d native_validator=1",now,id,node.type,x,y,*(int16_t*)((char*)command+0x44),naval_water.label(x,y));return false;}
        if(planned)break;
    }
    if(full_search&&tested<32)seat.producer_site_cursor=0;
    if(naval_test&&now>=tech_site_notice[id-1]){
        tech_site_notice[id-1]=now+1000;
        log("TECH_SITE_SCAN tick=%d side=%d goal=%d quantity=%d type=%d focus=%d,%d geometry=%d area=%d full_checked=%d full_accepted=%d last=%d commands=%d cursor=%d",now,id,key,quantity,node.type,hx,hy,geometry_cells,area_cells,complete_site.checked,complete_site.accepted,complete_site.last,tested,seat.producer_site_cursor);
    }
    if(water_site&&water_outside_area){naval_need_area[id-1]=true;log("NAVAL_VALID_OUTSIDE_AREA tick=%d side=%d count=%d anchor=%d,%d original_area=0",now,id,water_outside_area,naval_focus[id-1].x,naval_focus[id-1].y);}
    bool no_site=full_probe&&!tested&&!complete_site.accepted&&geometry_cells&&
        (!area_cells||(complete_site.checked&&complete_site.last==-33));
    if(no_site){
        log("PRODUCTION_AREA_FULL tick=%d side=%d type=%d checked=%d native_full_sites=0",now,id,node.type,complete_site.checked);
        if(water_site)naval_need_area[id-1]=true;
        int kind=*(int8_t*)((char*)prototype+0x30);
        // Initial and advanced ground/air producers need the same expansion
        // service. Do not gate it behind a unit born from the blocked factory.
        // A real original yard expands coverage; $Mf still enforces HQ quotas.
        if(kind>=7&&kind<=10&&native_depth<8){
            void* yard=local_dictionary_item(*(void**)((char*)s+0xf8),base_id(s)+3);
            void* levels=yard?*(void**)((char*)yard+0x18):nullptr,*first=levels?list_item(levels,0):nullptr;
            void* limits=first?*(void**)((char*)first+0x88):nullptr;
            int maximum=0;size_t n=limits?array_length(limits):0;
            if(n<=16)for(size_t i=0;i<n;++i){int limit=((int*)((char*)limits+32))[i];if(limit>maximum&&limit<=8)maximum=limit;}
            int counter=first?ai_tech_dependencies::index(*(int*)((char*)first+0x10)):-1;
            int current=counter>=0&&counter<observed.limit?observed.completed[counter]:-1;
            if(current>=1&&current<maximum){
                seat.reserve_tech=false;
                // This scan used the parent's command slot but sent nothing.
                // Allow the paid child action to use it, then keep the normal
                // retry interval even when an existing yard is still pending.
                seat.next_tech=now;
                satisfy_tech_requirements(battle,s,first,now,true,current+1,nullptr,native_depth+1);
                if(seat.next_tech<now+100)seat.next_tech=now+100;
                return false;
            }
        }
    }
    if(planned&&seat.plan.active()&&seat.plan.goal==key){log("PLAN_RELEASE tick=%d side=%d goal=%d reason=no-site-or-requirement",now,id,key);seat.plan.release(ai_economy::Invalid,now);}
    seat.reserve_tech=false;if(now>=seat.next_tech_notice){seat.next_tech_notice=now+1000;log("TECH_DEPENDENCY_WAIT tick=%d side=%d goal=%d type=%d code=%d footprints=%d site_cursor=%d geometry_cells=%d covered_cells=%d full_checked=%d full_accepted=%d full_code=%d native_validator=1",now,id,key,node.type,code,tested,seat.producer_site_cursor,geometry_cells,area_cells,complete_site.checked,complete_site.accepted,complete_site.last);}return false;
}
