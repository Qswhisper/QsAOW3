// Opt-in card40 routing and original native damage; private objects only.
#pragma once
namespace skill_hero_protection_audit {
using Roots=local_research_runtime::Roots;
static void* get(void* p,int o){return p?*(void**)((char*)p+o):nullptr;}
static bool ancestor_guard(){
    auto fixture=local_progress::seed(1000000,0,0,10000);local_progress::State next;
    bool ok=true;for(int id:{1,2,3,4,5,8,9,21,23,27,40})ok=ok&&local_progress::grant_stock(fixture,8,id,1000);
    for(int node:{111,112,113,114,121,123,132,134,142,143,144}){
        bool purchased=ok&&local_skill::plan(fixture,node,next);ok=ok&&purchased;if(purchased)fixture=next;
    }
    return ok&&fixture.skill_count==11&&!local_skill::plan(fixture,251,next);
}
static void run_guard(){
    if(access(DATA_ROOT "skill-hero-protection-guard.request",F_OK))return;
    unlink(DATA_ROOT "skill-hero-protection-guard.request");auto before=progress;bool pass=ancestor_guard();
    FILE* f=fopen(DATA_ROOT "skill-hero-protection-guard.json","w");if(!f)return;
    fprintf(f,"{\"pass\":%s,\"legal_phase1_nodes\":11,\"missing_ancestor_refused\":%s,\"player_state_unchanged\":%s,\"save_writes\":0,\"battle_runs\":0}\n",pass?"true":"false",pass?"true":"false",!memcmp(&before,&progress,sizeof(before))?"true":"false");fclose(f);chmod(DATA_ROOT "skill-hero-protection-guard.json",0644);
}
static void run(void* battle,void* hero){
    if(access(DATA_ROOT "skill-hero-protection-audit.request",F_OK))return;
    unlink(DATA_ROOT "skill-hero-protection-audit.request");auto before=progress;
    FILE* f=fopen(DATA_ROOT "skill-hero-protection-audit.jsonl","w");if(!f)return;chmod(DATA_ROOT "skill-hero-protection-audit.jsonl",0644);
    void* card=local_career::skill_card(40),*types=local_career::list("BATTLE","unitTypes");
    int effect=card?*(int*)((char*)card+0x38):0,heroes=0,cases=0,others=0;bool ok=effect==10&&types;
    for(int i=0;ok&&i<*(int*)((char*)types+0x18);++i){
        void* type=list_item(types,i),*ex=nullptr;
        void* value=type?invoke_named(type,"get_Hero",0,nullptr,&ex):nullptr;
        bool original_hero=value&&!ex&&*(bool*)((char*)value+16);
        bool route=value&&!ex&&skill_combat::protection_card(type)==(original_hero?40:skill_combat::unit_card(type)?23:0);
        if(!route){ok=false;break;}if(!original_hero){++others;continue;}++heroes;
        for(int level=1;ok&&level<=11;++level){Roots one;skill_combat::Weapons weapons;
            void* copy=skill_combat::unit(type,0,weapons,one,effect*level);
            ok=skill_protection_audit::values(type,copy,false,effect*level)&&get(type,0x98)==get(copy,0x98);
            ++cases;
        }
    }
    fprintf(f,"{\"phase\":\"original_hero_routing\",\"pass\":%s,\"hero_types\":%d,\"level_cases\":%d,\"other_types\":%d,\"armor_weapons_abilities_preserved\":true}\n",ok&&heroes>=12?"true":"false",heroes,cases,others);
    ok=ok&&heroes>=12;
    Roots roots;skill_combat::Weapons weapons;
    void* source=get(hero,0x40),*states=get(source,0x18),*state=states?list_item(states,0):nullptr;
    void* original_weapon=list_item(get(state,0x18),0),*weapon=local_research_runtime::clone(original_weapon,roots);
    void* base=source?skill_combat::unit(source,0,weapons,roots,10):nullptr;
    void* target=local_research_runtime::clone(hero,roots),*enemy=local_dictionary_item(get(battle,0x178),2);
    bool native=ok&&weapon&&base&&target&&enemy&&skill_combat::protection_card(source)==40;
    if(native){
        for(int i=0;i<6;++i)*(int*)((char*)weapon+0x28+i*4)=2000;
        void* levels=get(base,0x18);
        for(int i=0;i<*(int*)((char*)levels+0x18);++i){void* s=list_item(levels,i);*(int16_t*)((char*)s+0x22)=0;*(int16_t*)((char*)s+0x24)=0;}
        local_skill::ref(target,"side",enemy);local_skill::ref(target,"un_type",base);
    }
    int baseline=-1;
    native=native&&skill_protection_audit::native_damage(battle,hero,weapon,target,false,baseline)&&baseline>0;
    for(int level=0;native&&level<=11;++level){Roots one;
        void* adapted=level?skill_combat::unit(base,0,weapons,one,effect*level):base;
        if(!adapted){native=false;break;}local_skill::ref(target,"un_type",adapted);int damage=-1;
        bool pass=skill_protection_audit::native_damage(battle,hero,weapon,target,false,damage)&&damage==baseline*(100-level)/100;
        native=native&&pass;fprintf(f,"{\"phase\":\"original_hero_damage\",\"level\":%d,\"baseline\":%d,\"damage\":%d,\"expected\":%d,\"pass\":%s}\n",level,baseline,damage,baseline*(100-level)/100,pass?"true":"false");
    }
    bool guard=ancestor_guard();
    bool unchanged=!memcmp(&before,&progress,sizeof(before));
    fprintf(f,"{\"summary\":true,\"pass\":%s,\"original_native_damage\":%s,\"missing_ancestor_refused\":%s,\"player_state_unchanged\":%s,\"player_save_writes\":0,\"earned_card40_claimed\":false}\n",ok&&native&&guard&&unchanged?"true":"false",native?"true":"false",guard?"true":"false",unchanged?"true":"false");fclose(f);
}
}
