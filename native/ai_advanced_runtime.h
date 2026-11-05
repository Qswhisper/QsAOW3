// Inside local_duel: original kind8 recipes, paid native production/movement.
#pragma once
struct AdvancedSeat {void* side=nullptr;int tier=-1,ready_tier=-1,id=0,x=0,y=0;bool ordered=false,moved=false;};
static AdvancedSeat advanced_seats[8];
// Shared original recipes, paid construction/Hire and native birth/move passed
// the isolated NPC sample. The player seat remains outside this controller.
static bool advanced_enabled(){return offline_world_runtime::gameplay_scope()||advanced_test;}
static void reset_advanced(){for(auto& a:advanced_seats)a=AdvancedSeat{};}
static AdvancedSeat& advanced_state(void* s){auto& a=advanced_seats[*(int*)((char*)s+0x10)-1];if(a.side!=s){a=AdvancedSeat{};a.side=s;}return a;}
static int producer_unit_tier(void* s,void* unit,int kind){
    void* type=local_dictionary_item(*(void**)((char*)s+0xf8),base_id(s)+kind),*levels=type?*(void**)((char*)type+0x18):nullptr;
    int count=levels?*(int*)((char*)levels+0x18):0;if(!type||*(int8_t*)((char*)type+0x30)!=kind||count<1||count>16)return -1;
    for(int i=0;i<count;++i){void* level=list_item(levels,i),*list=level?*(void**)((char*)level+0x20):nullptr;int n=list?*(int*)((char*)list+0x18):0;if(n<0||n>32)return -1;
        for(int j=0;j<n;++j)if(list_item(list,j)==unit)return i;}
    return -1;
}
static int advanced_production_tier(void* s,void* unit){return producer_unit_tier(s,unit,8);}
static bool producer_capacity(void* battle,void* s,int now,void* factory,int tier,int (*unit_tier)(void*,void*),const char* label){
    int id=*(int*)((char*)s+0x10);auto& seat=seats[id-1];void* type=*(void**)((char*)factory+0x38),*levels=type?*(void**)((char*)type+0x18):nullptr;
    if(!levels||tier<0||tier>=*(int*)((char*)levels+0x18))return fail("producer-original-cp-tier");
    void* produced=*(void**)((char*)list_item(levels,tier)+0x20);int n=produced?*(int*)((char*)produced+0x18):0,min_cp=INT_MAX,used=0;
    if(n<1||n>32)return fail("producer-original-cp-list");
    for(int i=0;i<n;++i){void* u=list_item(produced,i);if(unit_tier(s,u)==tier){int cp=*(int8_t*)((char*)u+0x44);if(cp<0)return fail("producer-original-unit-cp");if(cp<min_cp)min_cp=cp;}}
    void* army=*(void**)((char*)s+0x140);size_t size=army?array_length(army):0;if(size>512||min_cp==INT_MAX)return fail("producer-original-cp-input");
    for(size_t i=0;i<size;++i){void* u=((void**)((char*)army+32))[i];if(u&&*(int*)((char*)u+0x184)>0){void* t=*(void**)((char*)u+0x40);int cp=t?*(int8_t*)((char*)t+0x44):-1;if(cp<0)return fail("producer-original-live-cp");used+=cp;}}
    int capacity=*(int*)((char*)s+0x34c);
    if(capacity-used<min_cp){
        void* hq=ai_lab::building(s,base_id(s)),*hq_type=hq?*(void**)((char*)hq+0x38):nullptr,*hq_levels=hq_type?*(void**)((char*)hq_type+0x18):nullptr;
        int current=hq?*(int8_t*)((char*)hq+0x60):-1,total=hq_levels?*(int*)((char*)hq_levels+0x18):0;
        if(hq&&*(uint8_t*)((char*)hq+0x47)==1&&current>=0&&current+1<total){
            if(now>=seat.next_tech_notice){seat.next_tech_notice=now+1000;log("%s_CP_EXPAND tick=%d side=%d used=%d capacity=%d needed=%d hq_tier=%d original_recipe=1",label,now,id,used,capacity,min_cp,current);}
            satisfy_tech_requirements(battle,s,list_item(hq_levels,current+1),now,true);
        }
    }
    return !stopped;
}
static bool advance_advanced(void* battle,void* s,int now){
    if(!advanced_enabled())return true;int id=*(int*)((char*)s+0x10);auto& seat=seats[id-1];auto& observed=advanced_state(s);
    if(seat.reserve_rebuild||seat.reserve_tech||seat.guard.defending||seat.ground_tier_born<2||!seat.air_id||alive(s,0x140)<3)return true;
    void* type=local_dictionary_item(*(void**)((char*)s+0xf8),base_id(s)+8),*levels=type?*(void**)((char*)type+0x18):nullptr;
    int count=levels?*(int*)((char*)levels+0x18):0;if(!type||*(int8_t*)((char*)type+0x30)!=8||count<1||count>16)return true;
    void* factory=ai_lab::building(s,base_id(s)+8);int tier=factory?*(int8_t*)((char*)factory+0x60):-1;
    if(factory){if(*(uint8_t*)((char*)factory+0x47)!=1)return true;if(tier<0||tier>=count)return fail("advanced-factory-tier");
        if(observed.ready_tier!=tier){observed.ready_tier=tier;log("ADVANCED_FACTORY_READY tick=%d side=%d type=%d tier=%d original_recipe=1",now,id,base_id(s)+8,tier);}
        // Produce at the current tier before progressing to the next one.
        if(observed.tier<tier)return producer_capacity(battle,s,now,factory,tier,advanced_production_tier,"ADVANCED");
        if(tier==count-1)return true;
    }
    void* goal=list_item(levels,tier+1);if(goal)satisfy_tech_requirements(battle,s,goal,now,true);return !stopped;
}
static bool observe_advanced(void* s,void* unit,void* type,int hp,int now){
    if(!advanced_enabled()||hp<=0)return true;auto& a=advanced_state(s);int tier=advanced_production_tier(s,type);
    if(tier<0)return true;int id=*(int*)((char*)unit+0x10);
    if(tier>a.tier){a.tier=tier;log("ADVANCED_TIER_BORN tick=%d side=%d tier=%d id=%d type=%d native_birth=1",now,*(int*)((char*)s+0x10),tier,id,*(int*)((char*)type+0x10));}
    if(!a.id){a.id=id;a.x=*(int16_t*)((char*)unit+0x48);a.y=*(int16_t*)((char*)unit+0x4a);}
    if(id==a.id&&!a.moved&&(a.x!=*(int16_t*)((char*)unit+0x48)||a.y!=*(int16_t*)((char*)unit+0x4a))){a.moved=true;log("ADVANCED_MOVED tick=%d side=%d id=%d from=%d,%d to=%d,%d native_pathing=1",now,*(int*)((char*)s+0x10),id,a.x,a.y,*(int16_t*)((char*)unit+0x48),*(int16_t*)((char*)unit+0x4a));}return true;
}
static int advanced_recruit_budget(void* s){
    if(!advanced_enabled())return 0;auto& a=advanced_state(s);void* factory=ai_lab::building(s,base_id(s)+8);if(!factory||*(uint8_t*)((char*)factory+0x47)!=1)return 0;
    int tier=*(int8_t*)((char*)factory+0x60);if(a.tier>=tier)return 0;void* type=*(void**)((char*)factory+0x38),*levels=type?*(void**)((char*)type+0x18):nullptr;
    if(tier<0||!levels||tier>=*(int*)((char*)levels+0x18))return 0;void* list=*(void**)((char*)list_item(levels,tier)+0x20);int n=list?*(int*)((char*)list+0x18):0,price=INT_MAX;if(n<0||n>32)return 0;
    for(int i=0;i<n;++i){void* unit=list_item(list,i);int cost=unit?*(int*)((char*)unit+0x48):0;if(cost>0&&cost<price&&advanced_production_tier(s,unit)==tier)price=cost;}
    return price==INT_MAX?0:price;
}
static void advanced_order(void* s,const int* ids,int count,int now){
    if(!advanced_enabled())return;auto& a=advanced_state(s);for(int i=0;i<count;++i)if(a.id&&ids[i]==a.id&&!a.ordered){a.ordered=true;log("ADVANCED_ORDER tick=%d side=%d id=%d native_command=1",now,*(int*)((char*)s+0x10),a.id);}
}
static bool advanced_fixture_result(void* battle,int now){
    void* s=side(battle,2);auto& a=advanced_state(s);bool unchanged=!memcmp(&tech_before,&progress,sizeof(progress));bool pass=a.ready_tier>=0&&a.id&&a.ordered&&a.moved&&unchanged;
    if(!pass&&now<29500)return false;FILE* f=fopen(DATA_ROOT "ai-advanced-production-fixture.jsonl","w");if(!f){fail("advanced-fixture-report");return true;}
    fprintf(f,"{\"summary\":true,\"pass\":%s,\"tick\":%d,\"side\":2,\"factory\":%d,\"factory_tier\":%d,\"advanced_tier_born\":%d,\"unit\":%d,\"native_order\":%s,\"native_moved\":%s,\"player_state_unchanged\":%s,\"player_match\":false,\"career_writes\":false}\n",pass?"true":"false",now,base_id(s)+8,a.ready_tier,a.tier,a.id,a.ordered?"true":"false",a.moved?"true":"false",unchanged?"true":"false");fclose(f);chmod(DATA_ROOT "ai-advanced-production-fixture.jsonl",0644);stopped=true;return true;
}
