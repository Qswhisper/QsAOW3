// Card39 uses the native Hero classification and the existing damage mechanism.
// Covers shared state/ability weapon aliases without invoking player abilities.
#pragma once
namespace skill_hero_attack_audit {
using Roots=local_research_runtime::Roots;
static void* get(void* p,int o){return p?*(void**)((char*)p+o):nullptr;}
struct Pairs {
    void* old[512]={},*adapted[512]={};int count=0,aliases=0,checks=0;void* skill=nullptr;
    bool add(void* source,void* copy){
        if(!source)return !copy;if(!copy||source==copy||count==512)return false;
        for(int i=0;i<count;++i)if(old[i]==source){++aliases;return adapted[i]==copy;}
        for(int i=0;i<3;++i){int base=*(int*)((char*)source+0x28+i*4);int expected=reinterpret_cast<int(*)(int,void*,bool,void*)>(image_base+0x7f37f88)(base,skill,true,nullptr);
            if(*(int*)((char*)copy+0x28+i*4)!=expected||*(int*)((char*)copy+0x34+i*4)!=expected)return false;++checks;}
        old[count]=source;adapted[count++]=copy;return true;
    }
    bool unit(void* source,void* copy){
        void* states=get(source,0x18),*next=get(copy,0x18);int n=states?*(int*)((char*)states+0x18):0;
        if(!next||n<1||n>32||*(int*)((char*)next+0x18)!=n)return false;
        for(int i=0;i<n;++i){void* a=list_item(states,i),*b=list_item(next,i),*wa=get(a,0x18),*wb=get(b,0x18);int m=wa?*(int*)((char*)wa+0x18):0;
            if(!a||!b||(wa&&!wb)||(wb?*(int*)((char*)wb+0x18):0)!=m||m<0||m>64)return false;
            if(memcmp((char*)a+0x20,(char*)b+0x20,0x88))return false;
            for(int j=0;j<m;++j)if(!add(list_item(wa,j),list_item(wb,j)))return false;
        }
        void* a=get(source,0x98),*b=get(copy,0x98);int m=a?*(int*)((char*)a+0x18):0;
        if(!b||a==b||m<1||m>32||*(int*)((char*)b+0x18)!=m)return false;
        for(int j=0;j<m;++j){void* ability=list_item(a,j),*next=list_item(b,j);
            if(!ability||!next||ability==next||memcmp((char*)ability+0x10,(char*)next+0x10,0x40)||memcmp((char*)ability+0x58,(char*)next+0x58,0x28)||!add(get(ability,0x50),get(next,0x50)))return false;
        }
        return true;
    }
};
static uint64_t fingerprint(void* dictionary){
    uintptr_t handle=0;void* rows=local_dictionary_values(dictionary,find_class("com.geargames.aow.entities","UnitType"),handle);if(!rows||!handle)return 0;
    uint64_t hash=1469598103934665603ULL;auto bytes=[&](void* p,int size){if(!p)return;for(int i=0;i<size;++i)hash=(hash^((uint8_t*)p)[i])*1099511628211ULL;};
    auto weapon=[&](void* w){if(w)bytes((char*)w+0x20,0x44);};
    for(size_t i=0;i<array_length(rows);++i){void* type=((void**)((char*)rows+32))[i];if(!type){hash=0;break;}bytes((char*)type+0x10,0xb0);
        void* states=get(type,0x18);int n=states?*(int*)((char*)states+0x18):0;if(n<0||n>32){hash=0;break;}
        for(int j=0;j<n;++j){void* state=list_item(states,j);bytes((char*)state+0x10,0x40);void* list=get(state,0x18);int m=list?*(int*)((char*)list+0x18):0;if(m<0||m>64){hash=0;break;}for(int k=0;k<m;++k)weapon(list_item(list,k));}
        void* abilities=get(type,0x98);int m=abilities?*(int*)((char*)abilities+0x18):0;if(m<0||m>32){hash=0;break;}
        for(int j=0;j<m;++j){void* a=list_item(abilities,j);bytes((char*)a+0x10,0x68);weapon(get(a,0x50));}
    }
    gchandle_free(handle);return hash;
}
static void run(){
    if(access(DATA_ROOT "skill-hero-attack-audit.request",F_OK))return;unlink(DATA_ROOT "skill-hero-attack-audit.request");
    FILE* f=fopen(DATA_ROOT "skill-hero-attack-audit.jsonl","w");if(!f)return;chmod(DATA_ROOT "skill-hero-attack-audit.jsonl",0644);
    local_progress::StateBuffer before,earned,next;if(!before.data||!earned.data||!next.data){fclose(f);return;}*before.data=progress;
    bool ok=local_skill::initialize()&&skill_combat::supported(39);void* rows=local_career::list("BATTLE","unitTypes"),*card=local_career::skill_card(39);int effect=card?*(int*)((char*)card+0x38):0;
    int heroes=0,others=0,cases=0,aliases=0,damages=0;ok=ok&&rows&&effect==15;
    for(int i=0;ok&&i<*(int*)((char*)rows+0x18);++i){void* type=list_item(rows,i),*ex=nullptr,*value=invoke_named(type,"get_Hero",0,nullptr,&ex);bool hero=value&&!ex&&*(bool*)((char*)value+16);
        ok=value&&!ex&&(skill_combat::attack_card(type)==39)==hero;if(!ok)break;if(!hero){++others;continue;}++heroes;
        bool all=true;for(int level=1;all&&level<=11;++level){Roots roots;skill_combat::Weapons weapons;void* skill=object_new(find_class("com.geargames.aow.entities","Skill"));int max=11,stock=0;all=roots.keep(skill);void* args[]={card,&level,&max,&stock};if(all)invoke_named(skill,".ctor",4,args,&ex);
            void* copy=all&&!ex?skill_combat::unit(type,effect*level,weapons,roots):nullptr;Pairs pairs;pairs.skill=skill;
            all=copy&&!ex&&pairs.unit(type,copy)&&skill_combat::protection_card(type)==40;aliases+=pairs.aliases;damages+=pairs.checks;++cases;
        }
        fprintf(f,"{\"phase\":\"original_hero_weapons\",\"unit\":%d,\"pass\":%s,\"levels\":11}\n",*(int*)((char*)type+0x10),all?"true":"false");ok=ok&&all;
    }
    *earned.data=local_progress::seed(100000000,0,0,50);void* cards[128];int n=local_career::skill_cards(cards,128);ok=ok&&n>0;
    for(int i=0;ok&&i<n;++i)ok=local_progress::grant_stock(*earned.data,8,*(int*)((char*)cards[i]+0x10),10000);
    for(int round=0;ok&&!local_progress::has_skill_node(*earned.data,184)&&round<32;++round){bool changed=false;for(int i=0;i<local_skill::graph.count;++i){int id=local_skill::graph.nodes[i].id;if(local_progress::has_skill_node(*earned.data,id))continue;if(local_skill::plan(*earned.data,id,*next.data)){*earned.data=*next.data;changed=true;}if(local_progress::has_skill_node(*earned.data,184))break;}if(!changed)break;}
    bool legal=local_progress::has_skill_node(*earned.data,184)&&!local_skill::plan(*earned.data,184,*next.data);int projections=0,projected_heroes=0;
    for(int faction:{1,2}){Roots roots;char path[512];snprintf(path,sizeof(path),DATA_ROOT "offline-seeds/fmc%dm.ggs",faction);void* config=campaign_seeds::decode(path,roots),*battle=get(config,0x10),*own=local_dictionary_item(get(battle,0x178),1),*enemy=local_dictionary_item(get(battle,0x178),2);
        void* units=get(own,0xf0),*buildings=get(own,0xf8);uint64_t source_hash=fingerprint(units),enemy_hash=fingerprint(get(enemy,0xf0));
        bool pass=ok&&legal&&source_hash&&enemy_hash&&skill_combat::project(own,*earned.data,units,buildings);uint64_t first=fingerprint(get(own,0xf0));
        pass=pass&&first&&source_hash==fingerprint(units)&&enemy_hash==fingerprint(get(enemy,0xf0));
        // New object addresses differ; compare both projections against the
        // same immutable source with the same skill, not pointer-byte hashes.
        void* first_units=get(own,0xf0);auto keep=gchandle_new(first_units,true);pass=pass&&keep&&skill_combat::project(own,*earned.data,units,buildings);
        uintptr_t values_handle=0;void* actual_types=local_dictionary_values(units,find_class("com.geargames.aow.entities","UnitType"),values_handle);pass=pass&&actual_types&&roots.take(values_handle)&&array_length(actual_types)<=512;
        // Decoded battle prototypes have seat-scoped IDs (e.g.1071), not the
        // canonical catalogue key71. Inspect the actual input dictionary.
        for(size_t i=0;pass&&i<array_length(actual_types);++i){void* type=((void**)((char*)actual_types+32))[i];if(skill_combat::attack_card(type)!=39)continue;int id=*(int*)((char*)type+0x10);void* a=local_dictionary_item(first_units,id),*b=local_dictionary_item(get(own,0xf0),id);
            auto weapon_damage=[&](void* x,void* y){return (!x&&!y)||(x&&y&&!memcmp((char*)x+0x28,(char*)y+0x28,24));};
            ++projected_heroes;void* aa=get(a,0x98),*bb=get(b,0x98);int m=aa?*(int*)((char*)aa+0x18):0;pass=a&&b&&bb&&*(int*)((char*)bb+0x18)==m;for(int j=0;pass&&j<m;++j)pass=weapon_damage(get(list_item(aa,j),0x50),get(list_item(bb,j),0x50));
            aa=get(a,0x18);bb=get(b,0x18);m=aa?*(int*)((char*)aa+0x18):0;pass=pass&&bb&&*(int*)((char*)bb+0x18)==m;
            for(int j=0;pass&&j<m;++j){void* aw=get(list_item(aa,j),0x18),*bw=get(list_item(bb,j),0x18);int count=aw?*(int*)((char*)aw+0x18):0;pass=(bw?*(int*)((char*)bw+0x18):0)==count;for(int k=0;pass&&k<count;++k)pass=weapon_damage(list_item(aw,k),list_item(bw,k));}
        }
        pass=pass&&source_hash==fingerprint(units)&&enemy_hash==fingerprint(get(enemy,0xf0));if(keep)gchandle_free(keep);ok=ok&&pass;++projections;
        fprintf(f,"{\"phase\":\"repeat_original_battle_input\",\"faction\":%d,\"pass\":%s,\"canonical_and_enemy_unchanged\":%s}\n",faction,pass?"true":"false",pass?"true":"false");
    }
    bool unchanged=!memcmp(before.data,&progress,sizeof(progress));ok=ok&&legal&&heroes==14&&others==45&&cases==154&&aliases>0&&damages>0&&projected_heroes>=12&&unchanged;
    fprintf(f,"{\"summary\":true,\"pass\":%s,\"hero_types\":%d,\"other_types\":%d,\"level_cases\":%d,\"native_damage_getters\":%d,\"preserved_weapon_aliases\":%d,\"battle_input_projections\":%d,\"projected_hero_types\":%d,\"legal_node184\":%s,\"player_state_unchanged\":%s,\"player_save_writes\":0,\"battle_runs\":0}\n",ok?"true":"false",heroes,others,cases,damages,aliases,projections,projected_heroes,legal?"true":"false",unchanged?"true":"false");fclose(f);
}
}
