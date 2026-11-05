// Original kind11 recipes and native Hire; water targets are topology-filtered.
#pragma once
constexpr int naval_max_yards=5; // Original HqCount is 1/2/3/4/5 for this AI's one HQ.
struct NavalSeat {void* side=nullptr;int tier=-1,ready=-1,unit=0,type=0,x=0,y=0,yards=0,area_checked_yards=-1,area_cells=-1;bool ordered=false,moved=false;int ids[128]={},next[128]={},count=0;
    int scout=0,scout_next=0,scout_x=0,scout_y=0,scout_failures=0;ai_coastal_recon_plan::Point vantage;
    bool scout_planned=false,scout_ordered=false,scout_moved=false,scout_explored=false;
};
static NavalSeat naval_seats[8];
static void reset_naval(){naval_map_battle=nullptr;naval_water.reset();for(auto& s:naval_seats)s=NavalSeat{};for(auto& b:naval_need_area)b=false;for(auto& b:naval_focus_valid)b=false;for(auto& b:naval_layout_checked)b=false;for(auto& b:naval_layout_valid)b=false;}
static NavalSeat& naval_state(void* side){auto& a=naval_seats[*(int*)((char*)side+0x10)-1];if(a.side!=side){a=NavalSeat{};a.side=side;}return a;}
static bool naval_first_pending(void* s){return naval_enabled()&&naval_state(s).scout_explored&&naval_state(s).tier<0;}
static bool naval_anchor_explored(void* battle,void* s,ai_spawn_plan::Point anchor,int w,int h){
    void* alliances=*(void**)((char*)battle+0x170);int id=*(int8_t*)((char*)s+0x2f8);
    if(!alliances||id<0||(size_t)id>=array_length(alliances))return false;
    void* a=((void**)((char*)alliances+32))[id],*matrix=a?*(void**)((char*)a+0x38):nullptr;
    for(int y=anchor.y;y<anchor.y+h;++y)for(int x=anchor.x;x<anchor.x+w;++x){
        if(!matrix||x<0||y<0||(size_t)(x>>5)>=array_length(matrix))return false;
        void* column=((void**)((char*)matrix+32))[x>>5];
        if(!column||(size_t)y>=array_length(column)||!((((uint32_t*)((char*)column+32))[y]>>(x&31))&1))return false;
    }return w>0&&h>0;
}
static bool naval_recon_unit(void* battle,void* s,void* unit,void* type,int now,bool& reserved){
    reserved=false;if(!naval_enabled())return true;int owner=*(int*)((char*)s+0x10)-1;
    if(owner<0||owner>=8||seats[owner].guard.defending)return true;
    auto& a=naval_state(s);if(a.scout_explored||a.scout_failures>=3)return true;
    if(a.scout&&*(int*)((char*)unit+0x10)==a.scout)
        a.scout_moved|=*(int16_t*)((char*)unit+0x48)!=a.scout_x||*(int16_t*)((char*)unit+0x4a)!=a.scout_y;
    ai_spawn_plan::Point anchor;if(!naval_base_focus(battle,s,anchor))return true;
    void* yard=local_dictionary_item(*(void**)((char*)s+0xf8),base_id(s)+11);
    int w=yard?*(int8_t*)((char*)yard+0x29):0,h=yard?*(int8_t*)((char*)yard+0x2a):0;
    if(w<1||h<1||w>16||h>16)return true;
    if(naval_anchor_explored(battle,s,anchor,w,h)){
        a.scout_explored=true;log("NAVAL_RECON_EXPLORED tick=%d side=%d anchor=%d,%d original_matrix38=1 scout=%d native_orders=%d native_moved=%d",now,owner+1,anchor.x,anchor.y,a.scout,a.scout_ordered,a.scout_moved);return true;}
    // One original infantry unit scouts; all other units retain normal orders.
    if(*(int8_t*)((char*)type+0x30)!=1)return true;
    int uid=*(int*)((char*)unit+0x10),x=*(int16_t*)((char*)unit+0x48),y=*(int16_t*)((char*)unit+0x4a);
    if(a.scout&&a.scout!=uid){void* units=*(void**)((char*)s+0x140);bool alive=false;
        for(size_t i=0;units&&i<array_length(units);++i){void* u=((void**)((char*)units+32))[i];
            if(u&&*(int*)((char*)u+0x10)==a.scout&&*(int*)((char*)u+0x184)>0){alive=true;break;}}
        if(alive)return true;a.scout=0;a.scout_planned=false;
    }
    if(!a.scout){a.scout=uid;a.scout_x=x;a.scout_y=y;}
    if(!a.scout_planned){
        native_battle_grid::View grid;if(!grid.init(battle))return false;
        uint8_t* transit=(uint8_t*)calloc(grid.width*grid.height,1);if(!transit)return false;
        for(int yy=0;yy<grid.height;++yy)for(int xx=0;xx<grid.width;++xx){void* c=grid.cell(xx,yy);int level=*(int8_t*)((char*)c+0x22);
            if(*(uint8_t*)((char*)c+0x20)==0&&level>=0&&level<127)transit[yy*grid.width+xx]=(uint8_t)(level+1);}
        bool ok=false;int best=INT_MAX;auto geometry=reinterpret_cast<bool(*)(void*,void*,int,int,int16_t,void*)>(image_base+0x4922a7c);
        for(int yy=1;yy+h<grid.height;++yy)for(int xx=1;xx+w<grid.width;++xx){
            if(!naval_water.footprint(xx,yy,w,h))continue;bool buildable=true;
            for(int py=yy;py<yy+h&&buildable;++py)for(int px=xx;px<xx+w;++px)
                if(!*(bool*)((char*)grid.cell(px,py)+0x40)){buildable=false;break;}
            ai_coastal_recon_plan::Point vantage;
            if(!buildable||!ai_coastal_recon_plan::select(transit,grid.width,grid.height,{x,y},xx,yy,w,h,vantage))continue;
            bool exit=false;for(int16_t angle:{0,90,180,270})exit|=geometry(battle,yard,xx,yy,angle,nullptr);
            if(!exit)continue;int dx=vantage.x-x,dy=vantage.y-y,score=dx*dx+dy*dy;
            if(score<best){best=score;anchor={xx,yy};a.vantage=vantage;ok=true;}
        }free(transit);
        if(!ok){log("NAVAL_RECON_REFUSED tick=%d side=%d scout=%d reason=no-joint-reachable-coast original_map_preserved=1",now,owner+1,uid);a.scout=0;++a.scout_failures;return true;}
        naval_layout_focus[owner]=anchor;naval_layout_valid[owner]=true;a.scout_planned=true;
        log("NAVAL_RECON_PLAN tick=%d side=%d scout=%d from=%d,%d vantage=%d,%d anchor=%d,%d terrain_flags_preserved=1",now,owner+1,uid,x,y,a.vantage.x,a.vantage.y,anchor.x,anchor.y);
    }
    a.scout_moved|=x!=a.scout_x||y!=a.scout_y;reserved=true;
    if(now<a.scout_next)return true;a.scout_next=now+150;
    Roots roots;void* command=ai_lab::command("AICommUnitsMove",now,roots,owner+1),*ids=array_new(find_class("System","Int32"),1);
    if(!command||!roots.keep(ids))return false;*(int*)((char*)ids+32)=uid;
    *(int16_t*)((char*)command+0x2e)=a.vantage.x;*(int16_t*)((char*)command+0x30)=a.vantage.y;if(!put(command,0x40,ids))return false;
    int code=ai_lab::perform(battle,command);if(ai_lab::stopped)return false;
    if(!code)a.scout_ordered=true;else{++a.scout_failures;reserved=false;}
    log("NAVAL_RECON_ORDER tick=%d side=%d scout=%d cell=%d,%d target=%d,%d code=%d native_moved=%d",now,owner+1,uid,x,y,a.vantage.x,a.vantage.y,code,a.scout_moved);return true;
}
static int naval_tier(void* side,void* unit){return producer_unit_tier(side,unit,11);}
static void naval_area_probe(void* battle,void* s,NavalSeat& a,int yards,int now){
    if(!naval_test||a.area_checked_yards==yards)return;a.area_checked_yards=yards;
    auto allowed=reinterpret_cast<bool(*)(void*,void*,void*,int,int,void*)>(image_base+0x47e7f08);
    int totals[2]={},xmin=INT_MAX,ymin=INT_MAX,xmax=-1,ymax=-1;
    for(int k=0;k<2;++k){void* type=local_dictionary_item(*(void**)((char*)s+0xf8),base_id(s)+(k?3:11));if(!type)continue;
        for(int y=0;y<naval_water.height;++y)for(int x=0;x<naval_water.width;++x)if(allowed(battle,s,type,x,y,nullptr)){
            ++totals[k];if(!k){if(x<xmin)xmin=x;if(y<ymin)ymin=y;if(x>xmax)xmax=x;if(y>ymax)ymax=y;}}}
    a.area_cells=totals[0];log("NAVAL_AREA_MAP tick=%d side=%d yards=%d shipyard_allowed=%d yard_allowed=%d bounds=%d,%d,%d,%d original_read_only_predicate=1",now,*(int*)((char*)s+0x10),yards,totals[0],totals[1],xmin,ymin,xmax,ymax);
}
static bool advance_naval(void* battle,void* s,int now){
    if(!naval_enabled())return true;auto& seat=seats[*(int*)((char*)s+0x10)-1];auto& a=naval_state(s);
    if(!naval_map(battle))return fail("naval-original-map");
    ai_spawn_plan::Point layout;if(!naval_base_focus(battle,s,layout))return true;
    if(!naval_water.largest||naval_water.sizes[naval_water.largest]<64||seat.reserve_rebuild||seat.reserve_tech||seat.guard.defending||seat.ground_tier_born<1)return true;
    int owner=*(int*)((char*)s+0x10);
    if(naval_need_area[owner-1]){
        void* yard=local_dictionary_item(*(void**)((char*)s+0xf8),base_id(s)+3),*levels=yard?*(void**)((char*)yard+0x18):nullptr,*goal=levels?list_item(levels,0):nullptr;
        ai_tech_dependencies::Observation counts;if(!goal||!observe_tech(s,counts))return fail("naval-area-original-yard");
        int current=counts.completed[ai_tech_dependencies::index(*(int*)((char*)goal+0x10))];
        naval_area_probe(battle,s,a,current,now);
        if(!a.yards){if(current<1||current>=naval_max_yards)return true;a.yards=current+1;}
        if(current<a.yards){ai_spawn_plan::Point focus;
            if(naval_focus_valid[owner-1])focus=naval_focus[owner-1];
            else focus=layout;
            if(now>=seat.next_tech_notice){seat.next_tech_notice=now+1000;log("NAVAL_AREA_GOAL tick=%d side=%d target=%d,%d validated_anchor=%d yards=%d",now,owner,focus.x,focus.y,naval_focus_valid[owner-1],a.yards);}
            satisfy_tech_requirements(battle,s,goal,now,true,a.yards,&focus);return !stopped;}
        log("NAVAL_AREA_EXPANDED tick=%d side=%d yards=%d native_recipe=1",now,owner,current);naval_need_area[owner-1]=false;a.yards=0;seat.producer_site_cursor=0;
    }
    void* type=local_dictionary_item(*(void**)((char*)s+0xf8),base_id(s)+11),*levels=type?*(void**)((char*)type+0x18):nullptr;
    int n=levels?*(int*)((char*)levels+0x18):0;if(!type||*(int8_t*)((char*)type+0x30)!=11||n<1||n>16)return true;
    void* yard=ai_lab::building(s,base_id(s)+11);int tier=yard?*(int8_t*)((char*)yard+0x60):-1;
    if(yard){if(*(uint8_t*)((char*)yard+0x47)!=1)return true;if(tier<0||tier>=n)return fail("naval-original-tier");
        if(a.ready!=tier){a.ready=tier;log("NAVAL_READY tick=%d side=%d type=%d tier=%d original_recipe=1",now,*(int*)((char*)s+0x10),base_id(s)+11,tier);}
        if(a.tier<tier)return producer_capacity(battle,s,now,yard,tier,naval_tier,"NAVAL");
        if(tier+1==n)return true;
    }
    satisfy_tech_requirements(battle,s,list_item(levels,tier+1),now,true);return !stopped;
}
static int naval_budget(void* s){
    if(!naval_enabled())return 0;auto& a=naval_state(s);void* yard=ai_lab::building(s,base_id(s)+11);
    if(!yard||*(uint8_t*)((char*)yard+0x47)!=1)return 0;int tier=*(int8_t*)((char*)yard+0x60);if(a.tier>=tier)return 0;
    void* type=*(void**)((char*)yard+0x38),*levels=type?*(void**)((char*)type+0x18):nullptr;if(!levels||tier<0||tier>=*(int*)((char*)levels+0x18))return 0;
    void* list=*(void**)((char*)list_item(levels,tier)+0x20);int n=list?*(int*)((char*)list+0x18):0,price=INT_MAX;if(n<1||n>32)return 0;
    for(int i=0;i<n;++i){void* u=list_item(list,i);int cost=u?*(int*)((char*)u+0x48):0;if(cost>0&&cost<price&&naval_tier(s,u)==tier)price=cost;}return price==INT_MAX?0:price;
}
static bool naval_unit(void* battle,void* side,void* enemy,void* unit,void* type,int now){
    auto& a=naval_state(side);int id=*(int*)((char*)unit+0x10),x=*(int16_t*)((char*)unit+0x48),y=*(int16_t*)((char*)unit+0x4a),tier=naval_tier(side,type);
    if(tier>a.tier)a.tier=tier;
    if(!a.unit){a.unit=id;a.type=*(int*)((char*)type+0x10);a.x=x;a.y=y;log("NAVAL_BORN tick=%d side=%d id=%d type=%d tier=%d cell=%d,%d water_component=%d native_birth=1",now,*(int*)((char*)side+0x10),id,a.type,tier,x,y,naval_water.label(x,y));}
    if(id==a.unit&&!a.moved&&(x!=a.x||y!=a.y)){a.moved=true;log("NAVAL_MOVED tick=%d side=%d id=%d from=%d,%d to=%d,%d native_pathing=1",now,*(int*)((char*)side+0x10),id,a.x,a.y,x,y);}
    int slot=0;while(slot<a.count&&a.ids[slot]!=id)++slot;if(slot==a.count){if(a.count==128)return true;a.ids[a.count++]=id;}
    if(now<a.next[slot])return true;a.next[slot]=now+150;
    void* target=ai_lab::building(enemy,base_id(enemy));int tx=target?*(int16_t*)((char*)target+0x40):a.x,ty=target?*(int16_t*)((char*)target+0x42):a.y,gx=0,gy=0;
    if(!naval_map(battle)||!naval_water.target(x,y,tx,ty,gx,gy))return true;
    Roots roots;void* command=ai_lab::command("AICommUnitsMove",now,roots,*(int*)((char*)side+0x10)),*ids=array_new(find_class("System","Int32"),1);
    if(!command||!roots.keep(ids))return fail("naval-move-allocation");*(int*)((char*)ids+32)=id;
    *(int16_t*)((char*)command+0x2e)=gx;*(int16_t*)((char*)command+0x30)=gy;if(!put(command,0x40,ids))return fail("naval-move-ids");
    int code=ai_lab::perform(battle,command);if(ai_lab::stopped)return fail("naval-move-exception");
    if(!code){if(id==a.unit)a.ordered=true;log("NAVAL_ORDER tick=%d side=%d id=%d cell=%d,%d target=%d,%d component=%d code=0 native_command=1",now,*(int*)((char*)side+0x10),id,x,y,gx,gy,naval_water.label(x,y));}
    return true;
}
static bool naval_fixture_result(void* battle,int now){
    void* s=side(battle,2);auto& a=naval_state(s);bool unchanged=!memcmp(&tech_before,&progress,sizeof(progress)),pass=a.ready>=0&&a.tier>=0&&a.unit&&a.ordered&&a.moved&&unchanged;
    int completed_yards=0;void* array=*(void**)((char*)s+0x148);
    for(size_t i=0;array&&i<array_length(array);++i){void* b=((void**)((char*)array+32))[i],*t=b?*(void**)((char*)b+0x38):nullptr;
        if(t&&*(int*)((char*)t+0x10)==base_id(s)+3&&*(uint8_t*)((char*)b+0x47)==1&&*(int*)((char*)b+0x5c)>0)++completed_yards;}
    bool diagnosed_no_site=now>=10000&&a.ready<0&&naval_need_area[1]&&completed_yards>=naval_max_yards;
    if(!pass&&!diagnosed_no_site&&now<44500)return false;FILE* f=fopen(DATA_ROOT "ai-naval-production-fixture.jsonl","w");if(!f){fail("naval-fixture-report");return true;}
    fprintf(f,"{\"summary\":true,\"pass\":%s,\"tick\":%d,\"side\":2,\"shipyard\":%d,\"ready_tier\":%d,\"born_tier\":%d,\"unit\":%d,\"type\":%d,\"native_order\":%s,\"native_moved\":%s,\"player_state_unchanged\":%s,\"player_match\":false,\"career_writes\":false}\n",pass?"true":"false",now,base_id(s)+11,a.ready,a.tier,a.unit,a.type,a.ordered?"true":"false",a.moved?"true":"false",unchanged?"true":"false");fclose(f);chmod(DATA_ROOT "ai-naval-production-fixture.jsonl",0644);stopped=true;return true;
}
