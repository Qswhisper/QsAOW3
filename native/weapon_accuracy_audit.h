// Compare raw fields and unchanged original UI formula for both representations.
#pragma once
namespace weapon_accuracy_audit {
using namespace generic_unit_research;
static bool near(float a,float b){float d=a-b;return d<0.00001f&&d> -0.00001f;}
static bool values(void* copy,void* source,const Effect& effect,int& count,bool initialized=false){
    if(initialized?!research_batch_audit::getters(copy,source,effect):!weapon_parameters_audit::raw_values(copy,source,effect))return false;
    void* states=weapon_parameters_audit::get(source,0x18),*actual=weapon_parameters_audit::get(copy,0x18);
    const int offsets[]={0x8e,0x92,0x90};const char* names[]={"get_AccuracyStatic","get_AccuracyWalk","get_AccuracyDynamic"};
    auto stat=(float(*)(int16_t,float,int,int,float,float,void*))(image_base+0x80568f8);
    auto walk=(float(*)(int,int,int,int,int,int,int,int,void*))(image_base+0x80569b8);
    for(int i=0;i<*(int*)((char*)states+0x18);++i){void* ws=weapon_parameters_audit::get(list_item(states,i),0x18),*cs=weapon_parameters_audit::get(list_item(actual,i),0x18);
        for(int j=0;ws&&j<*(int*)((char*)ws+0x18);++j){void* old=list_item(ws,j),*w=list_item(cs,j);int target=target_index(old,effect);
            int expected_fields[3];for(int k=0;k<3;++k)expected_fields[k]=*(int16_t*)((char*)old+offsets[k]);
            if(target>=0&&(effect.accuracy[target][0]||effect.accuracy[target][1]||effect.accuracy[target][2])){
                const char* reason=nullptr;if(!accuracy_values(old,effect.accuracy[target],expected_fields,reason))return false;}
            for(int k=0;k<3;++k)if(!original_stat_equals(w,names[k],2,expected_fields[k]))return false;
            if(target<0){if(old!=w)return false;continue;}
            int type=*(int16_t*)((char*)w+0x24),guided=*(int8_t*)((char*)w+0x94),walking=*(int8_t*)((char*)w+0x8a);
            int a=*(int16_t*)((char*)w+0x8e),b=*(int16_t*)((char*)w+0x92),distance=*(int*)((char*)w+(initialized?0x50:0x4c)),radius=*(int*)((char*)w+0x7c),decrease=*(int8_t*)((char*)w+0x84);
            bool probability=research_effects::probability_weapon(type,guided);
            if(!probability&&(!research_effects::dispersion_weapon(type,guided)||radius<=0||distance<=0))return false;
            float expected_static=probability?float(a)/100:float(1.0-double(a)*distance*(100-decrease)/(200000.0*radius));
            float expected=walking?(probability?(guided?float(a)/100:float(a*b)/10000):float(1.0-double(a+b)*distance*(100-decrease)/(200000.0*radius))):0;
            if(!near(stat(type,float(a),guided,distance,float(decrease),float(radius),nullptr),expected_static)||!near(walk(walking,guided,a,type,b,distance,decrease,radius,nullptr),expected))return false;
            ++count;
        }}return true;
}
static void run(){
    if(!progress_ready||access(DATA_ROOT "weapon-accuracy-audit.request",F_OK))return;unlink(DATA_ROOT "weapon-accuracy-audit.request");auto before=progress;
    FILE* f=fopen(DATA_ROOT "weapon-accuracy-audit.jsonl","w");if(!f)return;chmod(DATA_ROOT "weapon-accuracy-audit.jsonl",0644);
    bool ok=true;int candidates=0,accepted=0,refused=0,native=0,getters=0,predebit=0;Plan plans[2];
    for(const char* source:{"CONFEDERATION","RESISTANCE"}){int faction=!strcmp(source,"CONFEDERATION")?1:2;void* rows=levels(source);int n=rows?*(int*)((char*)rows+0x18):0;
        for(int i=0;i<n;++i){void* level=list_item(rows,i),*up=weapon_parameters_audit::get(level,0x48);char text[4096];research_effects::WeaponParameters p;
            if(!up||strcmp(class_name(object_class(up)),"UpgradeUnit")||!local_research_runtime::ascii(weapon_parameters_audit::get(level,0x18),text,sizeof(text))||!research_effects::weapon_parameters_parse(text,p)||(!p.accuracy[0]&&!p.accuracy[1]&&!p.accuracy[2]))continue;
            ++candidates;Effect e;const char* reason=nullptr;if(!resolve(level,e)||!shape(e.owner,e,reason)){++refused;fprintf(f,"{\"phase\":\"refused_accuracy\",\"faction\":%d,\"id\":%d,\"reason\":\"%s\"}\n",faction,*(int*)((char*)level+0x10),reason?reason:e.reason);continue;}
            Roots roots;uint64_t original=weapon_parameters_audit::hash(e.owner);void* copy=copy_type(e.owner,e,roots);
            bool pass=copy&&values(copy,e.owner,e,getters)&&weapon_parameters_audit::native_init(copy,e,native)&&values(copy,e.owner,e,getters,true)&&plans[faction-1].add(e)&&original==weapon_parameters_audit::hash(e.owner);
            if(faction==local_faction::active){pass=pass&&purchase_supported(level);++predebit;}ok=ok&&pass;++accepted;
            fprintf(f,"{\"phase\":\"original_accuracy\",\"faction\":%d,\"id\":%d,\"owner\":%d,\"static_delta\":%d,\"walk_delta\":%d,\"lead_delta\":%d,\"pass\":%s,\"canonical_unchanged\":%s}\n",faction,*(int*)((char*)level+0x10),*(int*)((char*)e.owner+0x10),p.accuracy[0],p.accuracy[1],p.accuracy[2],pass?"true":"false",original==weapon_parameters_audit::hash(e.owner)?"true":"false");
        }}
    int cumulative=0,guarded=0;
    for(int faction=0;faction<2;++faction)for(int i=0;i<plans[faction].count;++i){Effect e=plans[faction].families[i];const char* reason=nullptr;
        if(!shape(e.owner,e,reason)){++guarded;fprintf(f,"{\"phase\":\"cumulative_refused\",\"faction\":%d,\"owner\":%d,\"reason\":\"%s\"}\n",faction+1,*(int*)((char*)e.owner+0x10),reason?reason:"unknown");continue;}
        Roots roots;uint64_t original=weapon_parameters_audit::hash(e.owner);void* a=copy_type(e.owner,e,roots),*b=copy_type(e.owner,e,roots);
        bool pass=a&&b&&values(a,e.owner,e,getters)&&values(b,e.owner,e,getters)&&original==weapon_parameters_audit::hash(e.owner);
        auto bad=e;bad.accuracy[0][0]=INT16_MAX+1;void* rejected=copy_type(e.owner,bad,roots);pass=pass&&!rejected;
        ok=ok&&pass;++cumulative;fprintf(f,"{\"phase\":\"cumulative_accuracy\",\"faction\":%d,\"owner\":%d,\"pass\":%s,\"repeat_and_bound_guard\":%s}\n",faction+1,*(int*)((char*)e.owner+0x10),pass?"true":"false",pass?"true":"false");
    }
    bool isolated=!memcmp(&before,&progress,sizeof(progress));
    fprintf(f,"{\"summary\":true,\"pass\":%s,\"candidates\":%d,\"accepted\":%d,\"refused\":%d,\"native_initializers\":%d,\"original_ui_checks\":%d,\"active_faction_predebit\":%d,\"cumulative_families\":%d,\"cumulative_guarded\":%d,\"player_state_unchanged\":%s,\"player_save_writes\":0,\"battle_runs\":0}\n",ok&&accepted>0&&cumulative>0&&isolated?"true":"false",candidates,accepted,refused,native,getters,predebit,cumulative,guarded,isolated?"true":"false");fclose(f);
}
}
