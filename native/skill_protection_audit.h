// Opt-in protection mechanism checks. All mutations are to private copies.
#pragma once
namespace skill_protection_audit {
using local_research_runtime::Roots;
static void* get(void* p,int o){return p?*(void**)((char*)p+o):nullptr;}
static bool values(void* source,void* copy,bool building,int bonus){
    void* a=get(source,0x18),*b=get(copy,0x18);int n=a?*(int*)((char*)a+0x18):0;
    if(!a||!b||copy==source||n!=*(int*)((char*)b+0x18))return false;
    int armor=building?0x38:0x22,protection=building?0x3a:0x24;
    for(int i=0;i<n;++i){void* old=list_item(a,i),*now=list_item(b,i);
        if(!old||!now||old==now||*(int16_t*)((char*)old+armor)!=*(int16_t*)((char*)now+armor)||
           *(int16_t*)((char*)now+protection)!=*(int16_t*)((char*)old+protection)+bonus/10||get(old,0x18)!=get(now,0x18))return false;
    }return true;
}
static uint64_t hash(void* dictionary,bool building){
    uintptr_t h=0;void* rows=local_dictionary_values(dictionary,find_class("com.geargames.aow.entities",building?"BuildingType":"UnitType"),h);
    if(!rows||!h)return 0;uint64_t v=1469598103934665603ULL;
    for(size_t i=0;i<array_length(rows);++i){void* type=((void**)((char*)rows+32))[i],*levels=get(type,0x18);
        if(!type||!levels){gchandle_free(h);return 0;}
        for(int j=0;j<*(int*)((char*)levels+0x18);++j){void* s=list_item(levels,j);if(!s){gchandle_free(h);return 0;}
            int offset=building?0x38:0x22;for(int k=0;k<4;++k)v=(v^((uint8_t*)s)[offset+k])*1099511628211ULL;}
    }gchandle_free(h);return v;
}
static bool native_damage(void* battle,void* shooter,void* weapon,void* target,bool building,int& result){
    Roots roots;void* bullet=object_new(find_class("com.geargames.aow.entities","Bullet"));if(!roots.keep(bullet))return false;
    void* ex=nullptr;invoke_named(bullet,".ctor",0,nullptr,&ex);if(ex)return false;
    local_skill::ref(bullet,"we_type",weapon);local_skill::ref(bullet,"side",get(shooter,0x18));
    local_skill::ref(bullet,"subj",shooter);local_skill::ref(bullet,"obj",target);local_skill::ref(bullet,"obj_stat",target);
    *(int16_t*)((char*)bullet+0x14)=*(int16_t*)((char*)weapon+0x24);
    *(int8_t*)((char*)bullet+0x16)=*(int8_t*)((char*)weapon+0x26);
    *(bool*)((char*)bullet+0x95)=true;int scale=100;
    void* args[]={battle,bullet,nullptr,building?nullptr:target,building?target:nullptr,nullptr,&scale};
    void* value=runtime_invoke(class_method(find_class("com.geargames.aow.entities.battle","CheckAndCalc"),"$GC",7),nullptr,args,&ex);
    if(!value||ex)return false;result=*(int*)((char*)value+16);return true;
}
static void run(void* battle,void* born,void* hq){
    if(access(DATA_ROOT "skill-protection-audit.request",F_OK))return;
    unlink(DATA_ROOT "skill-protection-audit.request");auto before=progress;
    FILE* f=fopen(DATA_ROOT "skill-protection-audit.jsonl","w");if(!f)return;chmod(DATA_ROOT "skill-protection-audit.jsonl",0644);
    bool ok=local_skill::initialize();int tested[2]={};
    for(int category=0;category<2;++category){int id=category?8:23;void* card=local_career::skill_card(id);
        int effect=card?*(int*)((char*)card+0x38):0;void* rows=local_career::list("BATTLE",category?"buildingTypes":"unitTypes");
        if(effect<=0||!rows)ok=false;
        for(int level=1;ok&&level<=11;++level)for(int i=0;ok&&i<*(int*)((char*)rows+0x18);++i){void* type=list_item(rows,i);
            if(!category&&!skill_combat::unit_card(type))continue;Roots roots;skill_combat::Weapons weapons;
            void* copy=category?skill_combat::building(type,0,0,weapons,roots,effect*level):skill_combat::unit(type,0,weapons,roots,effect*level);
            ok=values(type,copy,category,effect*level);++tested[category];
        }
    }
    fprintf(f,"{\"phase\":\"protection_levels\",\"pass\":%s,\"ordinary_unit_cases\":%d,\"building_cases\":%d,\"raw_armor_preserved\":true,\"weapon_references_preserved\":true}\n",ok?"true":"false",tested[0],tested[1]);
    auto fixture=local_progress::seed(100000,0,0,10000);local_progress::State next;
    for(int card:{1,2,3,8,23})ok=ok&&local_progress::grant_stock(fixture,8,card,1000);
    for(int node:{111,112,121,132,142}){bool bought=local_skill::plan(fixture,node,next);ok=ok&&bought;if(bought)fixture=next;}
    bool projected=ok;
    for(int faction:{1,2}){Roots roots;char path[512];snprintf(path,sizeof(path),DATA_ROOT "offline-seeds/fmc%dm.ggs",faction);
        void* config=campaign_seeds::decode(path,roots),*b=get(config,0x10),*own=local_dictionary_item(get(b,0x178),1),*enemy=local_dictionary_item(get(b,0x178),2);
        void* units=get(own,0xf0),*buildings=get(own,0xf8);uint64_t own_u=hash(units,false),own_b=hash(buildings,true),enemy_u=hash(get(enemy,0xf0),false),enemy_b=hash(get(enemy,0xf8),true);
        bool pass=own_u&&own_b&&enemy_u&&enemy_b&&skill_combat::project(own,fixture,units,buildings)&&skill_combat::project(own,fixture,units,buildings)&&
            own_u==hash(units,false)&&own_b==hash(buildings,true)&&enemy_u==hash(get(enemy,0xf0),false)&&enemy_b==hash(get(enemy,0xf8),true);
        projected=projected&&pass;fprintf(f,"{\"phase\":\"protection_battle_projection\",\"faction\":%d,\"pass\":%s}\n",faction,pass?"true":"false");
    }
    bool native=true;Roots roots;skill_combat::Weapons weapons;
    void* source=get(born,0x40),*source_weapon=list_item(get(list_item(get(source,0x18),0),0x18),0);
    void* weapon=local_research_runtime::clone(source_weapon,roots),*enemy=local_dictionary_item(get(battle,0x178),2);
    if(!weapon||!enemy)native=false;
    if(weapon)for(int j=0;j<6;++j)*(int*)((char*)weapon+0x28+j*4)=2000;
    for(int category=0;native&&category<2;++category){
        void* original=category?get(hq,0x38):source;
        void* base=category?skill_combat::building(original,0,0,weapons,roots,20):skill_combat::unit(original,0,weapons,roots,10);
        if(!base){native=false;break;}
        void* levels=get(base,0x18);for(int i=0;i<*(int*)((char*)levels+0x18);++i){void* s=list_item(levels,i);
            *(int16_t*)((char*)s+(category?0x38:0x22))=0;*(int16_t*)((char*)s+(category?0x3a:0x24))=0;}
        void* target=local_research_runtime::clone(category?hq:born,roots);if(!target){native=false;break;}
        local_skill::ref(target,"side",enemy);local_skill::ref(target,category?"bu_type":"un_type",base);
        int before_damage=-1;if(!native_damage(battle,born,weapon,target,category,before_damage)){native=false;break;}
        for(int level:{1,11}){Roots one;int raw=(category?20:10)*level;
            void* adapted=category?skill_combat::building(base,0,0,weapons,one,raw):skill_combat::unit(base,0,weapons,one,raw);
            if(!adapted){native=false;break;}local_skill::ref(target,category?"bu_type":"un_type",adapted);int damage=-1;
            bool pass=native_damage(battle,born,weapon,target,category,damage)&&before_damage>0&&damage==before_damage*(100-raw/10)/100&&damage<before_damage;
            native=native&&pass;fprintf(f,"{\"phase\":\"original_native_damage\",\"card\":%d,\"level\":%d,\"private_zero_armor_sample\":true,\"before\":%d,\"after\":%d,\"expected\":%d,\"pass\":%s}\n",category?8:23,level,before_damage,damage,before_damage*(100-raw/10)/100,pass?"true":"false");
        }
    }
    bool unchanged=!memcmp(&before,&progress,sizeof(before));
    fprintf(f,"{\"summary\":true,\"pass\":%s,\"original_native_damage\":%s,\"legal_private_nodes\":%u,\"player_state_unchanged\":%s,\"player_saves_written\":false}\n",ok&&projected&&native&&unchanged?"true":"false",native?"true":"false",fixture.skill_count,unchanged?"true":"false");fclose(f);
}
}
