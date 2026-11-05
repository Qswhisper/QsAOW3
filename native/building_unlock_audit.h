// Original career-granted structural tiers; no player transaction or save.
#pragma once
namespace building_unlock_audit {
using namespace building_research;
static void* get(void* p,int o){return p?*(void**)((char*)p+o):nullptr;}
static bool expected(void* copy,void* base,unsigned mask){
    void* a=get(base,0x18),*b=get(copy,0x18);if(!copy||copy==base||count(a)!=count(b))return false;
    for(int i=0;i<count(a);++i){void* old=list_item(a,i),*row=list_item(b,i);
        int value=(mask&(1u<<i))?2:*(int8_t*)((char*)old+0x28);
        if(!row||row==old||!original_stat_equals(row,"get_Available",1,value)||
           memcmp((char*)old+0x29,(char*)row+0x29,0x7f)||get(old,0x18)!=get(row,0x18)||get(old,0x20)!=get(row,0x20))return false;
    }return true;
}
static uint64_t dictionary_hash(void* dictionary){
    Roots roots;uintptr_t handle=0;void* values=local_dictionary_values(dictionary,find_class("com.geargames.aow.entities","BuildingType"),handle);
    if(!values||!roots.take(handle))return 0;uint64_t h=1469598103934665603ULL;
    for(size_t i=0;i<array_length(values);++i)h=(h^building_research_audit::hash(((void**)((char*)values+32))[i]))*1099511628211ULL;
    return h;
}
static bool projection(int faction,const Plan& plan,int& matched){
    Roots roots;char path[512];snprintf(path,sizeof(path),DATA_ROOT "offline-seeds/fmc%dm.ggs",faction);
    void* config=campaign_seeds::decode(path,roots),*battle=get(config,0x10),*side=local_dictionary_item(get(battle,0x178),1),*enemy=local_dictionary_item(get(battle,0x178),2);
    void* base=get(side,0xf8),*other=get(enemy,0xf8);uint64_t baseline=dictionary_hash(base),enemy_hash=dictionary_hash(other);int again=0;
    bool pass=side&&enemy&&baseline&&enemy_hash&&apply(side,base,plan,matched)&&matched>0&&
        apply(side,base,plan,again)&&again==matched&&baseline==dictionary_hash(base)&&enemy_hash==dictionary_hash(other)&&get(enemy,0xf8)==other;
    void* live=get(side,0xf8);uintptr_t handle=0;void* values=local_dictionary_values(base,find_class("com.geargames.aow.entities","BuildingType"),handle);
    pass=pass&&values&&roots.take(handle);
    for(size_t i=0;pass&&i<array_length(values);++i){void* owner=((void**)((char*)values+32))[i];
        for(int j=0;j<plan.size;++j)if(identity(owner,plan.families[j].owner))
            pass=expected(local_dictionary_item(live,*(int*)((char*)owner+0x10)),owner,plan.families[j].unlocked_levels);}
    return pass;
}
static void run(){
    if(access(DATA_ROOT "building-unlock-audit.request",F_OK))return;
    unlink(DATA_ROOT "building-unlock-audit.request");auto before=progress;
    FILE* f=fopen(DATA_ROOT "building-unlock-audit.jsonl","w");if(!f)return;chmod(DATA_ROOT "building-unlock-audit.jsonl",0644);
    Plan plans[2];bool ok=true;int rows_seen=0,accepted=0,manual_refusals=0,starters=0;void* sample=nullptr;
    for(const char* source:{"CONFEDERATION","RESISTANCE"}){
        int faction=!strcmp(source,"CONFEDERATION")?1:2;void* rows=generic_unit_research::levels(source);
        if(!rows){ok=false;continue;}
        for(int i=0;i<count(rows);++i){void* level=list_item(rows,i),*up=get(level,0x48);
            if(!up||strcmp(class_name(object_class(up)),"UpgradeBuilding")||*(int8_t*)((char*)up+0x21)!=-1)continue;
            ++rows_seen;Effect e;Roots roots;bool pass=resolve(level,e)&&e.unlocked_levels&&shape(e.owner,e);
            if(!pass&&preexisting_starter(level)){++starters;fprintf(f,"{\"phase\":\"preexisting_starter_without_link\",\"faction\":%d,\"id\":%d,\"available\":2,\"projection_changes\":0}\n",faction,*(int*)((char*)level+0x10));continue;}
            uint64_t original=pass?building_research_audit::hash(e.owner):0;void* copy=pass?copy_type(e.owner,e,roots):nullptr;
            pass=pass&&expected(copy,e.owner,e.unlocked_levels)&&original==building_research_audit::hash(e.owner)&&plans[faction-1].add(e);
            if(faction==local_faction::active){pass=pass&&!purchase_supported(level);++manual_refusals;}
            if(pass)++accepted;ok=ok&&pass;int id=*(int*)((char*)level+0x10);if(id==21000)sample=level;
            fprintf(f,"{\"phase\":\"original_building_unlock\",\"faction\":%d,\"id\":%d,\"owner\":%d,\"mask\":%u,\"pass\":%s}\n",faction,id,e.owner?*(int*)((char*)e.owner+0x10):-1,e.unlocked_levels,pass?"true":"false");fflush(f);
        }
    }
    int projections=0,career_cases=0;
    for(int faction=1;faction<=2;++faction){int matched=0;bool pass=projection(faction,plans[faction-1],matched);
        ok=ok&&pass;++projections;fprintf(f,"{\"phase\":\"decoded_building_unlock_projection\",\"faction\":%d,\"matched\":%d,\"pass\":%s}\n",faction,matched,pass?"true":"false");
        void* stones=local_career::list(local_career::key(faction),"careerStones");int required_stars=10000;
        // Factions do not grant their first advanced tier at the same star count.
        // Choose the original earliest reward that actually unlocks a higher tier.
        for(int i=0;stones&&i<count(stones);++i){void* stone=list_item(stones,i),*rewards=get(stone,0x18);
            for(int j=0;j<count(rewards);++j){void* r=list_item(rewards,j);if(*(int*)((char*)r+0x14)!=20)continue;
                Effect e;void* level=local_career::lookup(local_career::key(faction),"upgradeLevels",*(int*)((char*)r+0x18));
                int stars=*(int*)((char*)stone+0x20);
                if(stars>0&&stars<required_stars&&resolve(level,e)&&(e.unlocked_levels&~1u))required_stars=stars;}}
        auto state=local_progress::seed(1000,0,0,required_stars);int expected_credits=1000;bool earned=stones&&required_stars<10000;
        for(int i=0;earned&&i<count(stones);++i){void* stone=list_item(stones,i);int stars=*(int*)((char*)stone+0x20);if(stars>state.stars)continue;
            local_progress::CareerReward rewards[128];int n=0;
            earned=local_career::resolve_stone(saved("UserConfiguration"),state,faction,stone,rewards,128,n);
            for(int j=0;earned&&j<n;++j)if(rewards[j].type==0)expected_credits+=rewards[j].amount;
            earned=earned&&local_progress::claim_career(state,*(int*)((char*)stone+0x10),stars,rewards,n);
        }
        Plan earned_plan,none,low_plan;auto unearned=local_progress::seed(1000,0,0,required_stars),low=state;low.stars=0;
        earned=earned&&state.credits==expected_credits&&state.upgrade_count==0&&collect_structural(earned_plan,state,faction)&&earned_plan.size>0&&
            collect_structural(none,unearned,faction)&&none.size==0&&!collect_structural(low_plan,low,faction);
        char path[]=DATA_ROOT "building-unlock-career-fixture-XXXXXX";local_progress::State loaded;
        earned=earned&&mkdtemp(path)&&local_progress::write(path,state)&&local_progress::load(path,loaded)==1&&!memcmp(&state,&loaded,sizeof(state));
        Plan restored;earned=earned&&collect_structural(restored,loaded,faction)&&restored.size==earned_plan.size&&projection(faction,restored,matched);
        ok=ok&&earned;++career_cases;
        fprintf(f,"{\"phase\":\"earned_career_tier_restore\",\"faction\":%d,\"stars\":%d,\"families\":%d,\"credits\":%d,\"expected_original_rewards_only\":%d,\"paid_upgrade_receipts\":%u,\"pass\":%s}\n",faction,required_stars,earned_plan.size,state.credits,expected_credits,state.upgrade_count,earned?"true":"false");fflush(f);
    }
    bool guards=false;{Roots roots;void* bad=local_research_runtime::clone(sample,roots);if(bad){Effect refused;*(int*)((char*)bad+0x10)=INT_MAX;guards=!resolve(bad,refused);
        *(int*)((char*)bad+0x10)=21000;local_skill::ref(bad,"desc",string_new("{#20000<10>}"));guards=guards&&!resolve(bad,refused);}}
    Plan actual;bool actual_earned=collect_structural(actual,progress,local_faction::active);int actual_families=actual.size;
    bool unchanged=!memcmp(&before,&progress,sizeof(before));
    fprintf(f,"{\"summary\":true,\"pass\":%s,\"original_rows\":%d,\"accepted\":%d,\"preexisting_starter_roots\":%d,\"manual_purchase_refusals\":%d,\"faction_projections\":%d,\"career_restore_cases\":%d,\"actual_earned_families\":%d,\"missing_link_nonempty_description_refused\":%s,\"player_state_unchanged\":%s,\"player_save_writes\":0,\"battle_runs\":0}\n",ok&&rows_seen==accepted+starters&&accepted>0&&guards&&actual_earned&&unchanged?"true":"false",rows_seen,accepted,starters,manual_refusals,projections,career_cases,actual_families,guards?"true":"false",unchanged?"true":"false");fclose(f);
}
}
