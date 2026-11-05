// Original kind9 cumulative producer lists, birth-gated upgrades and native Hire.
#pragma once
struct AdvancedAirSeat {void* side=nullptr;int tier=-1,ready=-1,id=0,type=0,x=0,y=0;bool ordered=false,moved=false;};
static AdvancedAirSeat advanced_air_seats[8];
static bool advanced_air_enabled(){return offline_world_runtime::gameplay_scope()||advanced_air_test;}
static void reset_advanced_air(){for(auto& a:advanced_air_seats)a=AdvancedAirSeat{};}
static AdvancedAirSeat& advanced_air_state(void* s){auto& a=advanced_air_seats[*(int*)((char*)s+0x10)-1];if(a.side!=s){a=AdvancedAirSeat{};a.side=s;}return a;}
static int advanced_air_tier(void* s,void* unit){return producer_unit_tier(s,unit,9);}
static bool advance_air_tiers(void* battle,void* s,int now){
    if(!advanced_air_enabled())return true;int id=*(int*)((char*)s+0x10);auto& seat=seats[id-1];auto& a=advanced_air_state(s);
    if(seat.reserve_rebuild||seat.reserve_tech||seat.guard.defending||advanced_state(s).tier<0||alive(s,0x140)<3)return true;
    void* field=ai_lab::building(s,base_id(s)+9);if(!field||*(uint8_t*)((char*)field+0x47)!=1)return true;
    int tier=*(int8_t*)((char*)field+0x60);void* type=*(void**)((char*)field+0x38),*levels=type?*(void**)((char*)type+0x18):nullptr;int n=levels?*(int*)((char*)levels+0x18):0;
    if(!type||*(int8_t*)((char*)type+0x30)!=9||n<1||n>16||tier<0||tier>=n)return fail("advanced-air-original-tier");
    if(a.ready!=tier){a.ready=tier;log("AIR_TECH_READY tick=%d side=%d tier=%d type=%d original_recipe=1",now,id,tier,base_id(s)+9);}
    // Fixed-wing aircraft also require the original kind10 airport. The
    // kind9 producer's level list alone does not supply a landing base.
    void* airport=ai_lab::building(s,base_id(s)+10);
    if(!airport){
        void* airport_type=local_dictionary_item(*(void**)((char*)s+0xf8),base_id(s)+10),*airport_levels=airport_type?*(void**)((char*)airport_type+0x18):nullptr;
        if(!airport_type||*(int8_t*)((char*)airport_type+0x30)!=10||!airport_levels||*(int*)((char*)airport_levels+0x18)<1)return fail("advanced-air-original-airport");
        satisfy_tech_requirements(battle,s,list_item(airport_levels,0),now,true);return !stopped;
    }
    if(*(uint8_t*)((char*)airport+0x47)!=1)return true;
    if(a.tier<tier)return producer_capacity(battle,s,now,field,tier,advanced_air_tier,"AIR_TECH");
    if(tier+1<n)satisfy_tech_requirements(battle,s,list_item(levels,tier+1),now,true);
    return !stopped;
}
static bool observe_advanced_air(void* s,void* unit,void* type,int hp,int now){
    if(!advanced_air_enabled()||hp<=0)return true;auto& a=advanced_air_state(s);int tier=advanced_air_tier(s,type);if(tier<0)return true;int id=*(int*)((char*)unit+0x10);
    if(tier>a.tier){a.tier=tier;log("AIR_TECH_BORN tick=%d side=%d tier=%d id=%d type=%d native_birth=1",now,*(int*)((char*)s+0x10),tier,id,*(int*)((char*)type+0x10));}
    if(tier>0&&!a.id){a.id=id;a.type=*(int*)((char*)type+0x10);a.x=*(int16_t*)((char*)unit+0x48);a.y=*(int16_t*)((char*)unit+0x4a);}
    if(id==a.id&&!a.moved&&(a.x!=*(int16_t*)((char*)unit+0x48)||a.y!=*(int16_t*)((char*)unit+0x4a))){a.moved=true;log("AIR_TECH_MOVED tick=%d side=%d id=%d from=%d,%d to=%d,%d native_pathing=1",now,*(int*)((char*)s+0x10),id,a.x,a.y,*(int16_t*)((char*)unit+0x48),*(int16_t*)((char*)unit+0x4a));}return true;
}
static int advanced_air_budget(void* s){
    if(!advanced_air_enabled())return 0;auto& a=advanced_air_state(s);void* field=ai_lab::building(s,base_id(s)+9);if(!field||*(uint8_t*)((char*)field+0x47)!=1)return 0;int tier=*(int8_t*)((char*)field+0x60);
    if(tier<=0||a.tier>=tier)return 0;void* type=*(void**)((char*)field+0x38),*levels=type?*(void**)((char*)type+0x18):nullptr;if(!levels||tier>=*(int*)((char*)levels+0x18))return 0;
    void* list=*(void**)((char*)list_item(levels,tier)+0x20);int n=list?*(int*)((char*)list+0x18):0,price=INT_MAX;if(n<1||n>32)return 0;
    for(int i=0;i<n;++i){void* u=list_item(list,i);int cost=u?*(int*)((char*)u+0x48):0;if(cost>0&&cost<price&&advanced_air_tier(s,u)==tier)price=cost;}return price==INT_MAX?0:price;
}
static void advanced_air_order(void* s,const int* ids,int count,int now){
    if(!advanced_air_enabled())return;auto& a=advanced_air_state(s);for(int i=0;i<count;++i)if(a.id&&ids[i]==a.id&&!a.ordered){a.ordered=true;log("AIR_TECH_ORDER tick=%d side=%d id=%d native_command=1",now,*(int*)((char*)s+0x10),a.id);}
}
static bool advanced_air_fixture_result(void* battle,int now){
    void* s=side(battle,2);auto& a=advanced_air_state(s);bool unchanged=!memcmp(&tech_before,&progress,sizeof(progress)),pass=a.ready>=1&&a.tier>=1&&a.id&&a.ordered&&a.moved&&unchanged;
    if(!pass&&now<44500)return false;FILE* f=fopen(DATA_ROOT "ai-advanced-air-production-fixture.jsonl","w");if(!f){fail("advanced-air-fixture-report");return true;}
    fprintf(f,"{\"summary\":true,\"pass\":%s,\"tick\":%d,\"side\":2,\"factory\":%d,\"factory_tier\":%d,\"air_tier_born\":%d,\"unit\":%d,\"unit_type\":%d,\"native_order\":%s,\"native_moved\":%s,\"player_state_unchanged\":%s,\"player_match\":false,\"career_writes\":false}\n",pass?"true":"false",now,base_id(s)+9,a.ready,a.tier,a.id,a.type,a.ordered?"true":"false",a.moved?"true":"false",unchanged?"true":"false");fclose(f);chmod(DATA_ROOT "ai-advanced-air-production-fixture.jsonl",0644);stopped=true;return true;
}
