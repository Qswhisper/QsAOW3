// Local career content adapter; diagnostic export contains canonical data only.
#pragma once
#include "hero_card_policy.h"
namespace local_career {
static bool (*extra_inventory_supported)(const local_progress::CareerReward&);
static bool (*modification_reward_supported)(const local_progress::CareerReward&,int);
static bool (*extra_mechanic_supported)(int);
static bool obsolete_premium_gifts;
static bool obsolete_speedup_gifts;
static bool (*box_rewards)(int,int,local_progress::CareerReward*,int,int&);
static bool (*box_credit_bonus)(const local_progress::State&,int,int&);
static void* list(const char* key, const char* name) {
    void* owner=saved(key);
    void* field=owner?class_field_from_name(object_class(owner),name):nullptr;
    return field?*(void**)((char*)owner+field_offset(field)):nullptr;
}
static void* lookup(const char* key, const char* name, int id) {
    void* values=list(key,name);
    int n=values?*(int*)((char*)values+0x18):0;
    for(int i=0;i<n;++i) { void* item=list_item(values,i); if(item && *(int*)((char*)item+0x10)==id) return item; }
    return nullptr;
}
static const char* key(int faction){return faction==1?"CONFEDERATION":"RESISTANCE";}
static int skill_cards(void** out,int capacity){
    void* milestones=list("SHARED","milestones");int used=0;
    for(int i=0;milestones&&i<*(int*)((char*)milestones+0x18);++i){
        void* milestone=list_item(milestones,i);void* map=milestone?*(void**)((char*)milestone+0x20):nullptr;
        size_t n=map?array_length(map):0;if(n>128)return -1;
        for(size_t j=0;j<n;++j){void* node=((void**)((char*)map+32))[j];void* card=node?*(void**)((char*)node+0x20):nullptr;
            if(!card)return -1;int id=*(int*)((char*)card+0x10);if(id<1||id>=100)return -1;
            bool found=false;for(int k=0;k<used;++k)if(*(int*)((char*)out[k]+0x10)==id){if(out[k]!=card)return -1;found=true;break;}
            if(!found){if(used==capacity)return -1;out[used++]=card;}
        }
    }return used;
}
static void* skill_card(int id){void* cards[128];int n=skill_cards(cards,128);for(int i=0;i<n;++i)if(*(int*)((char*)cards[i]+0x10)==id)return cards[i];return nullptr;}
static void* league(int id){
    void* dictionary=list("SHARED","leagues"),*value=nullptr,*ex=nullptr;void* args[]={&id,&value};
    void* found=dictionary?invoke_named(dictionary,"TryGetValue",2,args,&ex):nullptr;
    return !ex&&found&&*(bool*)((char*)found+16)&&value&&!strcmp(class_name(object_class(value)),"League")&&*(int*)((char*)value+0x10)==id?value:nullptr;
}
static bool supported(void* user, const local_progress::CareerReward& r,int stars,int faction) {
    if(r.amount<=0 || r.id<0) return false;
    if(r.type==0 || r.type==1) return true;
    if(r.type==2)return r.id==0; // Existing original token wallet/source.
    if(r.type==6)return obsolete_premium_gifts&&(r.id==1||r.id==3);
    if(r.type==4) return lookup("BATTLE","boosts",r.id)!=nullptr;
    if(r.type==8)return skill_card(r.id)!=nullptr;
    if(r.type==7&&!obsolete_speedup_gifts)return false;
    if(r.type==7||r.type==10||r.type==11)return extra_inventory_supported&&extra_inventory_supported(r);
    if(r.type==17)return modification_reward_supported&&modification_reward_supported(r,faction);
    if(r.type==9){void* card=lookup("SHARED","heroCards",r.id);void* fraction=card?*(void**)((char*)card+0x38):nullptr;
        // Random rarity rewards require an actual resolved grant; do not invent
        // the official server's missing random distribution or award a guest's cards.
        return fraction&&*(int*)((char*)fraction+0x10)==faction&&r.id<200&&*(int*)((char*)card+0x50)>0;}
    if(r.amount!=1) return false;
    if(r.type==18)return lookup(key(faction),"gameMechanics",r.id)!=nullptr;
    if(r.type==21) return lookup("BATTLE","unitTypes",r.id)!=nullptr;
    if(r.type==22) {
        void* rank=lookup("SHARED","profileRanks",r.id);
        return rank && *(int*)((char*)rank+0x14)<=stars;
    }
    if(r.type==20) {
        void* level=lookup(key(faction),"upgradeLevels",r.id);
        void* owner=level?*(void**)((char*)level+0x48):nullptr;
        // A career grant completes the structural unlock directly. Its normal
        // research cost/time (e.g. barracks II: 20/30) is not a claim cost.
        return owner && !strcmp(class_name(object_class(owner)),"UpgradeBuilding") &&
            *(int8_t*)((char*)owner+0x21)==-1 && !*(void**)((char*)level+0x18) &&
            !*(int*)((char*)level+0x28) && !*(int*)((char*)level+0x54);
    }
    void* mechanics=*(void**)((char*)user+0x210);
    if(r.type==19){void* episode=lookup(key(faction),"episodes",r.id);
        void* fraction=episode?*(void**)((char*)episode+0x18):nullptr;
        return mechanics&&fraction&&*(int*)((char*)fraction+0x10)==faction;}
    // Original LeagueProvider derives the badge from ProfileRank.League.
    // Publishing the earned canonical rank already supplies this structural
    // reward; no rating, scheduled match or league-box payout is invented.
    return r.type==23 && league(r.id);
}
static bool resolve_hero_reward(int stone,int reward,int card_id,int amount,int stars,int faction,local_progress::CareerReward* out,int capacity,int& count){
    count=0;void* definition=lookup("SHARED","heroCards",card_id);if(!definition||amount<=0)return false;
    if(card_id<200){void* f=*(void**)((char*)definition+0x38);if(!f||*(int*)((char*)f+0x10)!=faction||capacity<1||*(int*)((char*)definition+0x50)<=0)return false;
        out[0]={9,card_id,amount};count=1;return true;}
    if(card_id<200||card_id>202)return false;
    void* ranks=list("SHARED","profileRanks");int rank=0;
    for(int i=0;ranks&&i<*(int*)((char*)ranks+0x18);++i){void* r=list_item(ranks,i);if(r&&*(int*)((char*)r+0x14)<=stars&&*(int*)((char*)r+0x10)>rank)rank=*(int*)((char*)r+0x10);}
    void* cards=list("SHARED","heroCards");int n=cards?*(int*)((char*)cards+0x18):0;if(n<1||n>128)return false;
    hero_card_policy::Candidate candidates[128];int used=0;
    for(int i=0;i<n;++i){void* c=list_item(cards,i);if(!c||*(int*)((char*)c+0x10)>=200||*(int*)((char*)c+0x50)<=0)continue;
        void* f=*(void**)((char*)c+0x38),*from=*(void**)((char*)c+0x40);if(!f||!from)return false;
        // The local rule uses the unlock rank. Retained RankTo is
        // not a verified ownership/drop ceiling (common first hero is 6..6,
        // while the next common hero only opens at rank 9).
        candidates[used++]={*(int*)((char*)c+0x10),*(int*)((char*)f+0x10),*(int8_t*)((char*)c+0x30),*(int*)((char*)from+0x10),0};}
    hero_card_policy::Grant grants[128];int size=0;
    uint64_t receipt=(uint64_t(uint32_t(stone))<<32)|uint32_t(reward);
    if(!hero_card_policy::distribute(candidates,used,faction,rank,card_id-199,amount,receipt,grants,capacity,size))return false;
    for(int i=0;i<size;++i)out[count++]={9,grants[i].id,grants[i].amount};return true;
}
static bool resolve_skill_reward(int stone,int reward,int card_id,int amount,int faction,local_progress::CareerReward* out,int capacity,int& count){
    count=0;if(card_id<100){if(!skill_card(card_id)||amount<=0||capacity<1)return false;out[0]={8,card_id,amount};count=1;return true;}
    if(card_id>102)return false;void* cards[128];int n=skill_cards(cards,128);if(n<1)return false;
    hero_card_policy::Candidate candidates[128];for(int i=0;i<n;++i)candidates[i]={*(int*)((char*)cards[i]+0x10),faction,*(int*)((char*)cards[i]+0x28),1,0};
    uint64_t receipt=((uint64_t(uint32_t(stone))<<32)|uint32_t(reward))^(uint64_t(8)<<56);
    hero_card_policy::Grant grants[128];int size=0;
    if(!hero_card_policy::distribute(candidates,n,faction,1,card_id-99,amount,receipt,grants,capacity,size))return false;
    for(int i=0;i<size;++i)out[count++]={8,grants[i].id,grants[i].amount};return true;
}
static bool resolve_stone(void* user,const local_progress::State& state,int faction,void* stone,local_progress::CareerReward* out,int capacity,int& count){
    count=0;if(!stone||!out||capacity<1||capacity>128)return false;
    int id=*(int*)((char*)stone+0x10),stars=*(int*)((char*)stone+0x20);if(stars>state.stars)return false;
    void* values=*(void**)((char*)stone+0x18);int n=values?*(int*)((char*)values+0x18):0;if(n<1||n>128)return false;
    for(int j=0;j<n;++j){void* r=list_item(values,j);if(!r)return false;
        local_progress::CareerReward item{*(int*)((char*)r+0x14),*(int*)((char*)r+0x18),*(int*)((char*)r+0x1c)};int added=0;
        if(item.type==23){bool linked=false;for(int k=0;k<n;++k){void* gift=list_item(values,k);if(gift&&*(int*)((char*)gift+0x14)==22){void* rank=lookup("SHARED","profileRanks",*(int*)((char*)gift+0x18));if(rank&&*(int*)((char*)rank+0x88)==item.id)linked=true;}}
            if(!linked){LOG("Local career league is not represented by its original rank stone=%d league=%d",id,item.id);return false;}}
        bool ok=item.type==14?(box_rewards&&box_rewards(item.id,item.amount,out+count,capacity-count,added)):item.type==9?resolve_hero_reward(id,*(int*)((char*)r+0x10),item.id,item.amount,state.stars,faction,out+count,capacity-count,added):
            item.type==8?resolve_skill_reward(id,*(int*)((char*)r+0x10),item.id,item.amount,faction,out+count,capacity-count,added):supported(user,item,state.stars,faction);
        if(!ok){LOG("Local career unsupported reward stone=%d type=%d id=%d amount=%d",id,item.type,item.id,item.amount);return false;}
        if(item.type==14)for(int k=0;k<added;++k){auto& reward=out[count+k];if(reward.type==0&&box_credit_bonus&&!box_credit_bonus(state,reward.amount,reward.amount))return false;}
        if(item.type==8||item.type==9||item.type==14)count+=added;else{if(count==capacity)return false;out[count++]=item;}
    }return true;
}
using Roots=local_faction::Roots;
static void* (*skill_projector)(void*,const local_progress::State&,Roots&);
static bool add_unique(void* values, void* value) {
    if(!values || !value) return false;
    int id=*(int*)((char*)value+0x10), n=*(int*)((char*)values+0x18);
    for(int i=0;i<n;++i){void* old=list_item(values,i);if(old && *(int*)((char*)old+0x10)==id)return true;}
    void* exception=nullptr;void* args[]={value};invoke_named(values,"Add",1,args,&exception);return !exception;
}
static void* skill_info(void* user,const local_progress::State& state,Roots& roots){
    if(skill_projector)return skill_projector(user,state,roots);
    void* source=user?*(void**)((char*)user+0x1e8):nullptr;
    if(!source)return nullptr;void* config=local_faction::clone(source,roots);if(!config)return nullptr;
    // This bridge owns card inventory only. Do not import the capture guest's
    // learned skills or clear an already implemented skill-level state.
    void* finished=*(void**)((char*)source+0x10);if(finished&&*(int*)((char*)finished+0x18))return nullptr;
    if(!local_faction::empty_field(config,"skills",roots)||!local_faction::empty_field(config,"skillMapFinished",roots)||!local_faction::empty_field(config,"milestoneBoxOpened",roots))return nullptr;
    void* rows=*(void**)((char*)config+0x20);if(!rows)return nullptr;
    void* original_rows=*(void**)((char*)source+0x20);if(!original_rows)return nullptr;
    void* cards[128];int n=skill_cards(cards,128);if(n<1)return nullptr;
    for(uint32_t i=0;i<state.stock_count;++i)if(state.stocks[i].type==8&&!skill_card(state.stocks[i].id))return nullptr;
    for(int i=0;i<n;++i){Roots one;void* value=object_new(find_class("com.geargames.aow.entities","Skill"));if(!one.keep(value))return nullptr;
        int level=0,max_level=0,amount=local_progress::stock_amount(state,8,*(int*)((char*)cards[i]+0x10));void* ex=nullptr;
        for(int j=0;j<*(int*)((char*)original_rows+0x18);++j){void* original=list_item(original_rows,j);void* original_card=original?*(void**)((char*)original+0x10):nullptr;
            if(original_card&&*(int*)((char*)original_card+0x10)==*(int*)((char*)cards[i]+0x10)){
                if(*(int*)((char*)original+0x18)!=0)return nullptr;max_level=*(int*)((char*)original+0x1c);break;}}
        if(max_level<1||max_level>32)return nullptr;
        void* args[]={cards[i],&level,&max_level,&amount};invoke_named(value,".ctor",4,args,&ex);if(ex)return nullptr;
        void* add_args[]={value};invoke_named(rows,"Add",1,add_args,&ex);if(ex)return nullptr;
    }return config;
}
static void* mechanic_info(void* user,const local_progress::State& state,int faction,Roots& roots){
    void* source=user?*(void**)((char*)user+0x210):nullptr;
    void* config=local_faction::clone(source,roots);if(!config||!local_faction::empty_field(config,"unlockedGameMechanics",roots))return nullptr;
    void* unlocked=*(void**)((char*)config+0x10);if(!unlocked)return nullptr;
    // Keep only initial mechanics without a career reward gate. Gated mechanics
    // are rebuilt exclusively from this faction's persisted stone receipts.
    void* old=*(void**)((char*)source+0x10);void* stones=list(key(faction),"careerStones");int count=stones?*(int*)((char*)stones+0x18):0;
    for(int i=0;old&&i<*(int*)((char*)old+0x18);++i){void* item=list_item(old,i);if(!item)return nullptr;int id=*(int*)((char*)item+0x10);bool gated=false;
        for(int j=0;j<count&&!gated;++j){void* stone=list_item(stones,j);void* rewards=*(void**)((char*)stone+0x18);
            for(int k=0;rewards&&k<*(int*)((char*)rewards+0x18);++k){void* reward=list_item(rewards,k);if(*(int*)((char*)reward+0x14)==18&&*(int*)((char*)reward+0x18)==id)gated=true;}}
        if(!gated&&!add_unique(unlocked,item))return nullptr;
    }
    for(int i=0;i<count;++i){void* stone=list_item(stones,i);if(!local_progress::has_career(state,*(int*)((char*)stone+0x10)))continue;
        if(*(int*)((char*)stone+0x20)>state.stars)return nullptr;void* rewards=*(void**)((char*)stone+0x18);
        for(int j=0;rewards&&j<*(int*)((char*)rewards+0x18);++j){void* r=list_item(rewards,j);if(*(int*)((char*)r+0x14)!=18)continue;
            void* mechanic=lookup(key(faction),"gameMechanics",*(int*)((char*)r+0x18));if(!mechanic)return nullptr;
            int type=*(int*)((char*)mechanic+0x38);
            // The receipt retains every earned entitlement. Online-only or
            // unimplemented mechanics keep their original disabled entry.
            if((type>=1&&type<=4)||type==8||type==12||type==20||(extra_mechanic_supported&&extra_mechanic_supported(type)))if(!add_unique(unlocked,mechanic))return nullptr;}
    }return config;
}
static void mixed_audit(){
    if(!progress_ready||access(DATA_ROOT "career-mixed-audit.request",F_OK))return;
    unlink(DATA_ROOT "career-mixed-audit.request");auto before=progress;void* user=saved("UserConfiguration");
    void* old_skills=*(void**)((char*)user+0x1e8),*old_mechanics=*(void**)((char*)user+0x210);bool ok=true;int factions=0;
    for(int faction:{1,2}){Roots roots;auto fixture=local_progress::seed(0,0,0,120);void* stones=list(key(faction),"careerStones");
        for(int stars:{18,75,120}){void* stone=nullptr;for(int i=0;stones&&i<*(int*)((char*)stones+0x18);++i){void* value=list_item(stones,i);if(*(int*)((char*)value+0x20)==stars)stone=value;}
            local_progress::CareerReward rewards[128];int n=0;bool resolved=resolve_stone(user,fixture,faction,stone,rewards,128,n);
            if(!resolved||!local_progress::claim_career(fixture,*(int*)((char*)stone+0x10),stars,rewards,n)){ok=false;break;}
            auto claimed=fixture;ok=ok&&!local_progress::claim_career(fixture,*(int*)((char*)stone+0x10),stars,rewards,n)&&!memcmp(&fixture,&claimed,sizeof(fixture));
        }
        void* skills=ok?skill_info(user,fixture,roots):nullptr;void* mechanics=ok?mechanic_info(user,fixture,faction,roots):nullptr;
        int skill_total=0,hero_total=0;for(uint32_t i=0;i<fixture.stock_count;++i){if(fixture.stocks[i].type==8)skill_total+=fixture.stocks[i].amount;if(fixture.stocks[i].type==9)hero_total+=fixture.stocks[i].amount;}
        bool hero_gate=false;void* unlocked=mechanics?*(void**)((char*)mechanics+0x10):nullptr;
        for(int i=0;unlocked&&i<*(int*)((char*)unlocked+0x18);++i)if(*(int*)((char*)list_item(unlocked,i)+0x38)==20)hero_gate=true;
        void* ex=nullptr;void* logger=object_new(find_class("com.geargames.aow.logger.wrapper","LoggerWrapper"));
        void* source=object_new(find_class("com.geargames.aow.assets.scenes.main.network","SkillInfoDataSource"));
        if(!roots.keep(logger)||!roots.keep(source)||!skills){ok=false;break;}invoke_named(logger,".ctor",0,nullptr,&ex);
        void* ctor_args[]={logger};if(!ex)invoke_named(source,".ctor",1,ctor_args,&ex);void* set_args[]={skills};if(!ex)invoke_named(source,"SetSkillInfo",1,set_args,&ex);
        void* actual=!ex?invoke_named(source,"get_Configuration",0,nullptr,&ex):nullptr;void* rows=actual?*(void**)((char*)actual+0x20):nullptr;int actual_total=0;
        for(int i=0;rows&&i<*(int*)((char*)rows+0x18);++i){void* row=list_item(rows,i);actual_total+=*(int*)((char*)row+0x20);if(*(int*)((char*)row+0x18)!=0||*(int*)((char*)row+0x1c)!=11)ok=false;}
        bool pass=!ex&&rows&&*(int*)((char*)rows+0x18)==34&&actual_total==skill_total&&skill_total==18&&hero_total==25&&hero_gate&&fixture.credits==6000&&fixture.gold==10;
        LOG("Local mixed reward audit faction=%d pass=%d skills=%d heroes=%d credits=%d gold=%d hero_gate=%d exception=%s",faction,pass,skill_total,hero_total,fixture.credits,fixture.gold,hero_gate,ex?class_name(object_class(ex)):"none");
        ok=ok&&pass;++factions;
    }
    bool isolated=!memcmp(&before,&progress,sizeof(progress))&&old_skills==*(void**)((char*)user+0x1e8)&&old_mechanics==*(void**)((char*)user+0x210);
    FILE* f=fopen(DATA_ROOT "career-mixed-audit.json","w");if(f){fprintf(f,"{\"pass\":%s,\"factions\":%d,\"original_skill_data_source\":true,\"private_stones\":[18,75,120],\"player_state_unchanged\":%s,\"save_written\":false,\"battle_started\":false}\n",ok&&isolated?"true":"false",factions,isolated?"true":"false");fclose(f);chmod(DATA_ROOT "career-mixed-audit.json",0644);}
}
static bool apply(void* user) {
    if(!progress_ready || !user) return false;
    void* stones=list(local_faction::key(),"careerStones");
    void* profile=*(void**)((char*)user+0x28);
    void* config=*(void**)((char*)user+0x100);
    void* old_levels=config?*(void**)((char*)config+0x40):nullptr;
    void* old_stock=profile?*(void**)((char*)profile+0x98):nullptr;
    if(!stones || !profile || !old_levels || !old_stock) return false;
    if(!progress.career_count && !progress.stock_count && *(int*)((char*)old_stock+0x20)>0) {
        LOG("Local career refuses to replace an unimported nonempty seed inventory");return false;
    }
    auto next=progress; int newly_claimed=0;
    int count=*(int*)((char*)stones+0x18);
    for(int i=0;i<count;++i) {
        void* stone=list_item(stones,i); if(!stone)return false;
        int id=*(int*)((char*)stone+0x10), stars=*(int*)((char*)stone+0x20);
        if(stars>progress.stars || local_progress::has_career(next,id)) continue;
        local_progress::CareerReward rewards[128];int resolved_count=0;bool ready=resolve_stone(user,next,local_faction::active,stone,rewards,128,resolved_count);
        if(!ready) { LOG("Local career stone deferred without partial grant id=%d stars=%d",id,stars);continue; }
        if(!local_progress::claim_career(next,id,stars,rewards,resolved_count)) return false;
        ++newly_claimed;
    }
    // Prepare fresh publication objects before committing the wallet/receipts.
    Roots roots;void* levels=object_new(object_class(old_levels));void* stock=object_new(object_class(old_stock));
    if(!roots.keep(levels) || !roots.keep(stock)) return false;
    void* exception=nullptr;
    invoke_named(levels,".ctor",0,nullptr,&exception);if(exception)return false;
    invoke_named(stock,".ctor",0,nullptr,&exception);if(exception)return false;
    int n=*(int*)((char*)old_levels+0x18);
    for(int i=0;i<n;++i)if(!add_unique(levels,list_item(old_levels,i)))return false;
    void* rank=nullptr;int rank_stars=-1, unlocks=0, episode_id=0;
    for(int i=0;i<count;++i) {
        void* stone=list_item(stones,i);if(!local_progress::has_career(next,*(int*)((char*)stone+0x10)))continue;
        if(*(int*)((char*)stone+0x20)>progress.stars)return false;
        void* rewards=*(void**)((char*)stone+0x18);int rn=*(int*)((char*)rewards+0x18);
        for(int j=0;j<rn;++j) {
            void* reward=list_item(rewards,j);int type=*(int*)((char*)reward+0x14),id=*(int*)((char*)reward+0x18);
            if(type==19){void* episode=lookup(local_faction::key(),"episodes",id);void* faction=episode?*(void**)((char*)episode+0x18):nullptr;
                if(!faction||*(int*)((char*)faction+0x10)!=local_faction::active)return false;if(id>episode_id)episode_id=id;}
            if(type==20){if(!add_unique(levels,local_upgrade_level(id)))return false;++unlocks;}
            if(type==22){void* candidate=lookup("SHARED","profileRanks",id);if(!candidate)return false;int stars=*(int*)((char*)candidate+0x14);if(stars>rank_stars){rank=candidate;rank_stars=stars;}}
        }
    }
    if(!rank)return false;
    void* skills=skill_info(user,next,roots);void* mechanics=mechanic_info(user,next,local_faction::active,roots);
    if(!skills||!mechanics){LOG("Local career refused unsupported skill/mechanic publication");return false;}
    for(uint32_t i=0;i<next.stock_count;++i) {
        const auto& item=next.stocks[i];if(item.type!=4)continue; // Published to their original typed sources.
        void* boost=lookup("BATTLE","boosts",item.id);
        if(!boost || item.type!=4)return false;
        void* args[]={boost,(void*)&item.amount};invoke_named(stock,"Add",2,args,&exception);if(exception)return false;
    }
    if(newly_claimed) {
        if(!local_progress::write(local_faction::root(),next)) { LOG("Local career save failed; no publication");return false; }
        progress=next;
    }
    field_set(config,class_field_from_name(object_class(config),"finishUpgrades"),levels);
    field_set(profile,class_field_from_name(object_class(profile),"boosts"),stock);
    field_set(profile,class_field_from_name(object_class(profile),"rank"),rank);
    if(episode_id>0)*(int*)((char*)mechanics+0x18)=episode_id;
    field_set(user,class_field_from_name(object_class(user),"skillInfo"),skills);
    field_set(user,class_field_from_name(object_class(user),"gameMechanicsConfiguration"),mechanics);
    LOG("Local career restored new=%d receipts=%u rank=%d freeUnlocks=%d stockTypes=%u credits=%d gold=%d",
        newly_claimed,progress.career_count,*(int*)((char*)rank+0x10),unlocks,progress.stock_count,progress.credits,progress.gold);
    return true;
}
}
static void export_local_career_catalog() {
    if (access(DATA_ROOT "career-catalog.enabled", F_OK)) return;
    void* base = saved(local_faction::key());
    void* field = base ? class_field_from_name(object_class(base), "careerStones") : nullptr;
    void* stones = field ? *(void**)((char*)base + field_offset(field)) : nullptr;
    int n = stones ? *(int*)((char*)stones + 0x18) : 0;
    if (n < 1 || n > 1000) return;
    FILE* f = fopen(DATA_ROOT "career-catalog.json", "w");
    FILE* definitions = fopen(DATA_ROOT "career-unlock-definitions.txt", "w");
    if (!f || !definitions) { if (f) fclose(f); if (definitions) fclose(definitions); return; }
    fputs("[\n", f);
    for (int i = 0; i < n; ++i) {
        void* stone = list_item(stones, i);
        void* rewards = *(void**)((char*)stone + 0x18);
        int count = rewards ? *(int*)((char*)rewards + 0x18) : 0;
        fprintf(f, "%s{\"id\":%d,\"stars\":%d,\"order\":%d,\"rewards\":[", i ? ",\n" : "",
            *(int*)((char*)stone + 0x10), *(int*)((char*)stone + 0x20), *(int*)((char*)stone + 0x24));
        for (int j = 0; j < count; ++j) {
            void* reward = list_item(rewards, j);
            int type = *(int*)((char*)reward + 0x14), id = *(int*)((char*)reward + 0x18);
            fprintf(f, "%s{\"id\":%d,\"type\":%d,\"type_id\":%d,\"amount\":%d}", j ? "," : "",
                *(int*)((char*)reward + 0x10), type, id, *(int*)((char*)reward + 0x1c));
            if (type == 20 && *(int*)((char*)stone + 0x20) <= 12) {
                fprintf(definitions, "stone=%d requiredStars=%d upgrade=%d\n", *(int*)((char*)stone + 0x10), *(int*)((char*)stone + 0x20), id);
                visited_count = 0; dump_object(definitions, local_upgrade_level(id), 0, 3);
            }
        }
        fputs("]}", f);
    }
    fputs("\n]\n", f); fclose(f); fclose(definitions);
    chmod(DATA_ROOT "career-catalog.json", 0644); chmod(DATA_ROOT "career-unlock-definitions.txt", 0644);
    LOG("Local career catalog exported stones=%d; no transaction or battle", n);
}
