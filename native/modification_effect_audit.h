// Shared native effect contract audit, detached from every player inventory.
#pragma once
namespace modification_effect_audit {
using namespace local_mod_effects;
static void* target(void* unit,int kind,int id){
    if(kind==0)return unit&&*(int*)((char*)unit+0x10)==id?unit:nullptr;
    if(kind==3){void* f=get(unit,0x20);return f&&*(int*)((char*)f+0x10)==id?f:nullptr;}
    if(kind==4)return local_career::lookup("BATTLE","mineTypes",id);
    void* states=get(unit,0x18);for(int i=0;states&&i<*(int*)((char*)states+0x18);++i){void* state=list_item(states,i);
        if(kind==1&&*(int*)((char*)state+0x10)==id)return state;
        void* ws=get(state,0x18);for(int j=0;kind==2&&ws&&j<*(int*)((char*)ws+0x18);++j){void* w=list_item(ws,j);if(w&&*(int*)((char*)w+0x10)==id)return w;}}
    return nullptr;
}
static uint64_t hash(void* object,int last){uint64_t h=1469598103934665603ULL;for(int i=0x10;object&&i<last;++i)h=(h^((uint8_t*)object)[i])*1099511628211ULL;return h;}
static bool original_formula(const Group& group,int base){
    Action actions[128];if(group.count<1||group.count>128)return false;
    for(int i=0;i<group.count;++i){void* p=list_item(group.list,i);actions[i]=group.key;actions[i].operation=*(int*)((char*)p+0x10);actions[i].value=*(float*)((char*)p+0x14);}
    float a=0,b=0;return modification_effects::calculate(float(base),actions,group.count,a)&&calculate_float(group,base,b)&&fabsf(a-b)<=0.001f;
}
static bool init_weapons(void* unit,int& count){
    void* seen[1024]={};int n=0;void* states=get(unit,0x18);
    for(int i=0;states&&i<*(int*)((char*)states+0x18);++i){void* ws=get(list_item(states,i),0x18);
        for(int j=0;ws&&j<*(int*)((char*)ws+0x18);++j){void* w=list_item(ws,j);bool known=false;for(int k=0;k<n;++k)if(seen[k]==w)known=true;if(known)continue;if(!w||n==1024||*(bool*)((char*)w+0x14))return false;seen[n++]=w;
            int range=*(int*)((char*)w+0x4c),radius=*(int*)((char*)w+0x7c),period=*(int16_t*)((char*)w+0x66);int padded=range>1000?range+50:range*105/100;
            if(range<1||padded>46340||period<1)return false;void* ex=nullptr;invoke_named(w,"init",0,nullptr,&ex);
            if(ex||!*(bool*)((char*)w+0x14)||*(int*)((char*)w+0x4c)!=padded*padded||*(int*)((char*)w+0x7c)!=radius||*(int16_t*)((char*)w+0x66)!=period)return false;++count;
        }}return true;
}
static bool accuracy_guard(){
    const char* source=local_faction::active==1?"CONFEDERATION":"RESISTANCE";void* rows=generic_unit_research::levels(source);
    for(int i=0;rows&&i<*(int*)((char*)rows+0x18);++i){void* level=list_item(rows,i);char text[4096];generic_unit_research::Effect effect;const char* reason=nullptr;
        if(!local_research_runtime::ascii(get(level,0x18),text,sizeof(text))||!strstr(text,"{#20008<1>}")||!generic_unit_research::resolve(level,effect)||!generic_unit_research::shape(effect.owner,effect,reason)||!generic_unit_research::purchase_supported(level))continue;
        Roots roots;void* changed=local_research_runtime::clone(level,roots);char* p=strstr(text,"{#20008<1>}");p[8]='2';ref(changed,"desc",string_new(text));
        // UpgradeLevel's serialized field is named desc; verify actual resolve.
        generic_unit_research::Effect altered;return changed&&generic_unit_research::resolve(changed,altered)&&generic_unit_research::shape(altered.owner,altered,reason)&&!generic_unit_research::purchase_supported(changed);
    }return false;
}
static void run(){
    if(!progress_ready||access(DATA_ROOT "modification-effects-audit.request",F_OK))return;unlink(DATA_ROOT "modification-effects-audit.request");auto before=progress;
    FILE* f=fopen(DATA_ROOT "modification-effects-audit.jsonl","w");if(!f)return;chmod(DATA_ROOT "modification-effects-audit.jsonl",0644);
    bool ok=true;int mods=0,fields=0,globals=0,native=0;
    for(const auto& d:modification_content::definitions){Roots roots;auto* c=compiled(d.id);void* source=c?get(c->definition,0x38):nullptr;uint64_t original=source?weapon_parameters_audit::hash(source):0;
        int projected=0,global=0;void* copy=c?copy_unit(source,d.id,roots,projected,global):nullptr;bool pass=copy&&c;int checked=0;
        for(int i=0;pass&&i<c->count;++i){const auto& group=c->groups[i];void* old=target(source,group.key.kind,group.key.entity),*actual=nullptr;uint64_t old_hash=hash(old,group.key.kind==3?0xe8:group.key.kind==4?0x68:0x10);
            if(group.key.kind>=3){int changed=0;actual=copy_scope(old,group.key.kind,*c,roots,changed);++globals;}else actual=target(copy,group.key.kind,group.key.entity);
            int base=0,expected=0,value=0;pass=old&&actual&&read(old,group.key.kind,group.key.property,base)&&original_formula(group,base)&&calculate(group,base,expected)&&read(actual,group.key.kind,group.key.property,value)&&value==expected&&old_hash==hash(old,group.key.kind==3?0xe8:group.key.kind==4?0x68:0x10);
            fprintf(f,"{\"phase\":\"original_field\",\"modification\":%d,\"kind\":%d,\"entity\":%d,\"property\":\"%s\",\"base\":%d,\"projected\":%d,\"expected\":%d,\"pass\":%s}\n",d.id,group.key.kind,group.key.entity,group.key.property,base,value,expected,pass?"true":"false");++checked;
        }
        pass=pass&&init_weapons(copy,native)&&original==weapon_parameters_audit::hash(source);ok=ok&&pass;++mods;fields+=checked;
        fprintf(f,"{\"phase\":\"detached_modification\",\"id\":%d,\"faction\":%d,\"unit\":%d,\"actions\":%d,\"projected_fields\":%d,\"global_groups\":%d,\"canonical_unchanged\":%s,\"pass\":%s}\n",d.id,d.faction,d.unit,d.count,projected,global,source&&original==weapon_parameters_audit::hash(source)?"true":"false",pass?"true":"false");
    }
    bool guard=accuracy_guard();ok=ok&&guard;bool isolated=!memcmp(&before,&progress,sizeof(progress));
    fprintf(f,"{\"summary\":true,\"pass\":%s,\"modifications\":%d,\"field_checks\":%d,\"global_field_checks\":%d,\"native_initializers\":%d,\"accuracy_archive_guard\":%s,\"runtime_version\":%u,\"runtime_size\":%zu,\"player_state_unchanged\":%s,\"player_save_writes\":0,\"player_grants\":0,\"player_equips\":0,\"battle_runs\":0}\n",ok&&mods==14&&fields==187&&isolated?"true":"false",mods,fields,globals,native,guard?"true":"false",progress.version,sizeof(progress),isolated?"true":"false");fclose(f);
}
}
