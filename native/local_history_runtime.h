// Adapt the original profile battle list to the real retained local results.
// Archived rows and the actual latest ledgers share the original scroll list.
#pragma once
namespace local_history {
using Roots=local_research_runtime::Roots;
struct Visible {local_history_store::Record record;void* model;uintptr_t root;};
static std::vector<Visible> visible;
static int64_t next_id=-22001;
static int owner;
static bool ours(void* result){
    return result&&!strcmp(class_name(object_class(result)),"PublicBattleResults")&&*(int*)((char*)result+0x10)<=-22001;
}
static const Visible* lookup(void* result){
    if(!ours(result))return nullptr;
    for(const auto& value:visible)if(value.model==result)return &value;return nullptr;
}
static void clear_rows(){
    for(const auto& value:visible)if(value.root)gchandle_free(value.root);visible.clear();
}
static void ref(void* object,const char* field,void* value){field_set(object,class_field_from_name(object_class(object),field),value);}
static void* create(const char* name,Roots& roots,void** ex){
    void* value=object_new(find_class("com.geargames.aow.entities",name));
    if(!roots.keep(value))return nullptr;invoke_named(value,".ctor",0,nullptr,ex);return *ex?nullptr:value;
}
static void* row(int k,const local_match_store::State& s,void* profile,Roots& roots,int id){
    bool cooperative=k==1;void* ex=nullptr;
    const auto* route=cooperative?nullptr:local_scenario(s.scenario);
    const auto* map=cooperative?nullptr:local_match_maps::find(s.scenario);
    if(!cooperative&&(!route||!map||map->faction!=owner||map->map!=s.map))return nullptr;
    void* source=local_mission_description(cooperative?973:(route?route->recorded:-1));
    local_faction::Roots clones;void* description=local_faction::clone(source,clones);
    void* result=create("PublicBattleResults",roots,&ex);if(!result||!description)return nullptr;
    void* alliances=local_campaign::field_list(object_class(result),"alliances",roots);
    void* headers=local_campaign::field_list(object_class(result),"headers",roots);
    if(!alliances||!headers)return nullptr;
    bool free_for_all=!cooperative&&local_battle_format::free_for_all(s.mode);
    int seats=cooperative?3:local_battle_format::seats(s.mode);
    if(!cooperative){
        *(int*)((char*)description+0x20)=local_battle_format::team_format(s.mode);
        *(int*)((char*)description+0x24)=3;
    }
    *(int*)((char*)description+0x30)=owner;
    char title[128];
    if(cooperative)snprintf(title,sizeof(title),"南瓜狩猎 · 本地合作第%llu局",(unsigned long long)s.settled);
    else snprintf(title,sizeof(title),"本地 AI %s · 第%llu局",local_match::mode_name(s.mode),(unsigned long long)s.settled);
    ref(description,"name",string_new(title));
    if(!local_campaign::append(headers,string_new(title)))return nullptr;
    if(!local_campaign::append(headers,string_new(local_match::outcome_name(s.outcome))))return nullptr;
    *(int*)((char*)result+0x10)=id;
    *(int*)((char*)result+0x14)=cooperative?5:3;
    *(int*)((char*)result+0x48)=s.tick/10;
    *(bool*)((char*)result+0x4c)=false; // There is no saved replay.
    ref(result,"description",description);ref(result,"headers",headers);ref(result,"alliances",alliances);
    offline_world::Snapshot identities;bool named=offline_world_runtime::historical(owner,s,identities);
    for(int team=0;team<(free_for_all?seats:2);++team){
        void* alliance=create("PublicAllianceResults",roots,&ex);if(!alliance)return nullptr;
        void* sides=local_campaign::field_list(object_class(alliance),"sides",roots);if(!sides)return nullptr;
        // PublicBattleResultsExtensions uses presentation alliance IDs 0/1,
        // independently of the battle/store winner values 1/2.
        *(int*)((char*)alliance+0x10)=team;*(bool*)((char*)alliance+0x14)=s.winner==team+1;
        ref(alliance,"sides",sides);
        int players=free_for_all?1:cooperative?(team?1:2):seats/2;
        for(int i=0;i<players;++i){
            Roots side_roots;local_faction::Roots profile_roots;
            void* side=create("PublicBattleSideResults",side_roots,&ex);if(!side)return nullptr;
            int id=free_for_all?team+1:team?2+i*2:1+i*2;void* display=profile;
            if(id!=1){
                if(named&&id<=int(identities.actor_count))display=offline_world_runtime::profile(profile,identities.actors[id-1],profile_roots);
                else{display=local_faction::clone(profile,profile_roots);if(!display)return nullptr;
                    *(int*)((char*)display+0x10)=-100-id;ref(display,"name",string_new(team?"AI 对手":"AI 队友"));
                    ref(display,"nameAlias",string_new(""));ref(display,"fraction",local_faction::fraction(team?s.opponent:owner));ref(display,"clan",nullptr);}
                if(!display)return nullptr;
            }
            // Even the original AI panel requires a non-null profile seed;
            // InitiateInfoAlliancesAI applies its normal BOT presentation.
            ref(side,"profile",display);
            ref(side,"rank",*(void**)((char*)display+0x60));ref(side,"level",*(void**)((char*)display+0x20));
            *(int*)((char*)side+0x30)=id;*(int*)((char*)side+0x34)=team;
            *(int*)((char*)side+0x54)=s.tick/10;*(bool*)((char*)side+0x59)=id==1&&s.outcome==local_match_store::Surrender;
            // No per-player kills/builds are fabricated. The original list only
            // uses identity/rank/outcome; its detail action opens our real result.
            if(!local_campaign::append(sides,side))return nullptr;
        }
        if(!local_campaign::append(alliances,alliance))return nullptr;
    }
    return ex?nullptr:result;
}
static void (*show_original)(void*,void*);
static void show(void* self,void* method){
    if(!offline_network){show_original(self,method);return;}
    owner=local_faction::active;clear_rows();local_history_view::clear();
    std::vector<local_history_store::Record> records;unsigned damaged=0;
    pthread_mutex_lock(&local_history_journal::mutex);
    bool loaded=local_history_store::load(local_faction::root(),owner,records,damaged);
    pthread_mutex_unlock(&local_history_journal::mutex);
    if(!loaded){LOG("Local history directory unreadable; current results retained");records.clear();}
    local_match_store::State pvp{};cooperative_store::State coop{};
    int pvp_status=local_match_store::load(local_faction::root(),pvp);
    int coop_status=owner==2?cooperative_store::load(offline_accounts::cooperative_root(),coop):0;
    local_history_store::Record latest;
    if(pvp_status>0&&local_history_store::scored(pvp)&&local_history_store::make(owner,pvp,0,latest)&&!local_history_store::merge(records,latest))++damaged;
    if(coop_status>0&&local_history_store::scored(coop.match)&&local_history_store::make(coop,0,latest)&&!local_history_store::merge(records,latest))++damaged;
    Roots roots;void* ex=nullptr;
    void* answer=object_new(find_class("com.geargames.aow.messages","BattleListAnswer"));if(!roots.keep(answer))return;
    invoke_named(answer,".ctor",0,nullptr,&ex);
    void* list=local_campaign::field_list(object_class(answer),"battles",roots);
    void* user=saved("UserConfiguration");void* profile=user?*(void**)((char*)user+0x28):nullptr;
    if(ex||!list||!profile)return;
    for(const auto& record:records){
        if(next_id<=INT32_MIN){LOG("Local history presentation identity exhausted");return;}
        // The completed model is retained by visible/list. Temporary roots
        // belong to one record; a growing journal must not exhaust one batch's
        // fixed32-root guard and hide all previously saved results.
        Roots record_roots;
        void* value=row(record.kind,local_history_store::match(record),profile,record_roots,int(next_id--));
        uintptr_t root=value?gchandle_new(value,false):0;
        if(!root||!local_campaign::append(list,value)){if(root)gchandle_free(root);clear_rows();LOG("Local history model rejected kind=%u",record.kind);return;}
        visible.push_back({record,value,root});
    }
    ref(answer,"battles",list);void* a[]={answer};invoke_named(self,"OnBattleListAnswerReceived",1,a,&ex);
    LOG("Local original history faction=%d rows=%zu damaged=%u pvp_status=%d coop_status=%d exception=%s read_only=1",owner,visible.size(),damaged,pvp_status,coop_status,ex?class_name(object_class(ex)):"none");
}
static void (*date_original)(void*,void*,void*);
static void date(void* self,void* result,void* method){
    if(!offline_network||!ours(result)){date_original(self,result,method);return;}
    const auto* entry=lookup(result);if(!entry)return;
    // Keep the two original labels. Old ledgers have no recorded calendar date.
    int seconds=*(int*)((char*)result+0x48);char duration[32];snprintf(duration,sizeof(duration),"%02d:%02d",seconds/60,seconds%60);
    char stamp[48];snprintf(stamp,sizeof(stamp),"%s · 日期未记录",entry->record.kind?"合作":"对战");
    if(entry->record.ended_utc){time_t t=time_t(entry->record.ended_utc);tm parts{};
        if(localtime_r(&t,&parts))strftime(stamp,sizeof(stamp),"%Y/%m/%d %H:%M",&parts);}
    void* ex=nullptr;
    for(int i=0;i<2;++i){void* label=*(void**)((char*)self+0x20+i*8);
        void* a[]={string_new(i?duration:stamp)};if(label)invoke_named(label,"set_text",1,a,&ex);}
}
static void (*open_original)(void*,void*,void*);
static void open(void* self,void* result,void* method){
    if(!offline_network||!ours(result)){open_original(self,result,method);return;}
    const auto* entry=lookup(result);
    if(!entry||owner!=local_faction::active||local_match::playing||local_coop::playing||local_run_active)return;
    const auto record=entry->record;const auto s=local_history_store::match(record);
    const auto* route=record.kind?nullptr:local_scenario(s.scenario);
    if(!record.kind&&!route)return;
    if(!local_history_view::select(record))return;
    auto previous=local_campaign_results::latest;auto& view=local_campaign_results::latest;
    view={};view.pending=true;view.win=s.outcome==local_match_store::Victory;view.run=s.settled;
    view.chapter=record.kind?-1002:-1001;view.credits=record.kind?local_history_view::coop.credits:0;
    char title[192];snprintf(title,sizeof(title),"%s · 第%llu局 · %s（历史记录）",record.kind?"本地合作 PVE":"本地 AI 对战",
        (unsigned long long)s.settled,local_match::outcome_name(s.outcome));
    bool ok=local_campaign_results::publish(true,title,record.kind?973:route->recorded);
    local_campaign_results::latest=previous;
    if(!ok)local_history_view::clear();
    LOG("Local original history detail kind=%u run=%llu published=%d read_only=1 reward_sync=0 ledger_adopt=0",record.kind,(unsigned long long)s.settled,ok);
}
static void install(){
    const uint32_t s[]={0xa9be57fe,0xa9014ff4,0xd00100f4,0xf000ce95};
    const uint32_t d[]={0xf81c0ffe,0xa9015ff8,0xa90257f6,0xa9034ff4};
    const uint32_t o[]={0xa9be57fe,0xa9014ff4,0x90023f15,0xaa0103f4};
    show_original=(decltype(show_original))install_hook(0x7d93228,s,(void*)show);
    date_original=(decltype(date_original))install_hook(0x7d89b54,d,(void*)date);
    open_original=(decltype(open_original))install_hook(0x55c485c,o,(void*)open);
    LOG("Local original history hooks=%d",show_original&&date_original&&open_original);
}
}
