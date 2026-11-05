// Original parser/calculator and detached scalar projection. No reward/equip IO.
#pragma once
#include "modification_effect_policy.h"
namespace local_mod_effects {
using Roots=local_research_runtime::Roots;
using Action=modification_effects::Action;
struct Group {Action key;void* list=nullptr;uintptr_t root=0;int count=0;};
struct Compiled {bool attempted=false,valid=false;void* definition=nullptr;uintptr_t root=0;Group groups[128];int count=0;};
static Compiled cache[14];
static void* get(void* p,int o){return p?*(void**)((char*)p+o):nullptr;}
static void ref(void* p,const char* name,void* value){field_set(p,class_field_from_name(object_class(p),name),value);}
static const modification_content::Definition* definition(int id){for(const auto& d:modification_content::definitions)if(d.id==id)return &d;return nullptr;}
static void* original(int id){
    void* manager=saved("@dictionary-reader"),*context=get(manager,0x10),*ex=nullptr;
    if(!context)return nullptr;void* name=string_new("com.geargames.aow.entities.UnitModificationType");void* args[]={name,&id};
    void* result=invoke_named(context,"getProcessed",2,args,&ex);
    return !ex&&result&&!strcmp(class_name(object_class(result)),"UnitModificationType")?result:nullptr;
}
static void clear(Compiled& c){
    for(int i=0;i<c.count;++i)if(c.groups[i].root)gchandle_free(c.groups[i].root);
    if(c.root)gchandle_free(c.root);c=Compiled{};c.attempted=true;
}
static Compiled* compiled(int id){
    const auto* d=definition(id);if(!d||!progress_ready)return nullptr;int index=int(d-modification_content::definitions);auto& c=cache[index];
    if(c.attempted)return c.valid?&c:nullptr;
    void* v=original(id),*unit=get(v,0x38),*faction=get(unit,0x20);char visual[160];
    if(!v||!unit||!faction||*(int*)((char*)unit+0x10)!=d->unit||*(int*)((char*)faction+0x10)!=d->faction||*(int*)((char*)v+0x48)!=d->rarity||
       *(int*)((char*)v+0x40)!=d->feature||*(int*)((char*)v+0x44)!=d->event||!local_research_runtime::ascii(get(v,0x30),visual,sizeof(visual))||strcmp(visual,d->visual))return nullptr;
    c.attempted=true;Roots roots;void* rows=get(v,0x50);if(!rows||array_length(rows)!=size_t(d->count)){clear(c);return nullptr;}
    for(int i=0;i<d->count;++i){char text[256];if(!local_research_runtime::ascii(((void**)((char*)rows+32))[i],text,sizeof(text))||strcmp(text,modification_content::actions[d->first+i].text)){clear(c);return nullptr;}}
    void* klass=find_class("com.geargames.aow.ugui.army.modifications","ModificationParser");const void* method=klass?class_method(klass,"ParseLine",1):nullptr;void* ex=nullptr;void* args[]={rows};
    void* parsed=method?runtime_invoke(method,nullptr,args,&ex):nullptr;if(ex||!roots.keep(parsed)||*(int*)((char*)parsed+0x18)!=d->count){clear(c);return nullptr;}
    for(int i=0;i<d->count;++i){Action a;if(!modification_effects::parse(modification_content::actions[d->first+i].text,a)){clear(c);return nullptr;}
        void* p=list_item(parsed,i);char property[48];if(!p||!local_research_runtime::ascii(get(p,0x20),property,sizeof(property))||strcmp(property,a.property)||
            *(int*)((char*)p+0x10)!=a.operation||*(float*)((char*)p+0x14)!=a.value||*(int*)((char*)p+0x18)!=a.kind||*(int*)((char*)p+0x1c)!=a.entity){clear(c);return nullptr;}
        int group=0;while(group<c.count&&!modification_effects::same(a,c.groups[group].key))++group;
        if(group==c.count){if(c.count==128){clear(c);return nullptr;}auto& g=c.groups[c.count++];g.key=a;g.list=object_new(object_class(parsed));
            if(!roots.keep(g.list)){clear(c);return nullptr;}invoke_named(g.list,".ctor",0,nullptr,&ex);g.root=!ex?gchandle_new(g.list,true):0;if(!g.root){clear(c);return nullptr;}}
        void* add[]={p};invoke_named(c.groups[group].list,"Add",1,add,&ex);if(ex){clear(c);return nullptr;}++c.groups[group].count;
    }
    c.root=gchandle_new(v,true);c.definition=v;c.valid=c.root!=0;
    LOG("Local original modification compiled id=%d actions=%d fields=%d valid=%d",id,d->count,c.count,c.valid);return c.valid?&c:nullptr;
}
static bool calculate_float(const Group& group,int base,float& out){
    void* klass=find_class("com.geargames.aow.ugui.army.modifications","ModificationCalculator");const void* method=klass?class_method(klass,"Apply",2):nullptr;
    float value=float(base);void* ex=nullptr;void* args[]={&value,group.list};void* boxed=method?runtime_invoke(method,nullptr,args,&ex):nullptr;
    if(ex||!boxed)return false;out=*(float*)((char*)boxed+16);return isfinite(out);
}
static bool calculate(const Group& group,int base,int& out){
    const auto* schema=modification_effects::field(group.key.kind,group.key.property);float result=0;
    return schema&&calculate_float(group,base,result)&&modification_effects::quantize(result,*schema,out);
}
static bool read(void* object,int kind,const char* property,int& out){
    const auto* schema=modification_effects::field(kind,property);const char* classes[]={"UnitType","UnitStateType","WeaponType","Fraction","MineType"};
    if(!object||!schema||kind<0||kind>4||strcmp(class_name(object_class(object)),classes[kind]))return false;
    void* f=class_field_from_name(object_class(object),property);int code=f?type_type(field_type(f)):0;
    int expected=schema->width==1?(schema->signed_value?4:5):schema->width==2?(schema->signed_value?6:7):(schema->signed_value?8:9);
    if(!f||field_offset(f)!=schema->offset||code!=expected)return false;
    char* p=(char*)object+schema->offset;
    out=schema->width==1?(schema->signed_value?int(*(int8_t*)p):int(*(uint8_t*)p)):schema->width==2?(schema->signed_value?int(*(int16_t*)p):int(*(uint16_t*)p)):*(int32_t*)p;
    return kind!=0||strcmp(property,"life")||out==*(int32_t*)((char*)object+0x34);
}
static bool domain(int kind,const char* name,int value){
    if(!strcmp(name,"flags")||!strcmp(name,"spec"))return true;
    if(value<0)return false;
    // Canonical time_train_init is legitimately zero for all 14 retained
    // prototypes. Keep that raw sentinel; the original action on zero is zero.
    if((kind==0&&(!strcmp(name,"life")||!strcmp(name,"cp")||!strcmp(name,"time_train")))||
       (kind==2&&(!strcmp(name,"round_len")||!strcmp(name,"shot_count")||!strcmp(name,"velocity")||!strcmp(name,"distance"))))return value>0;
    return true;
}
static bool apply(void* source,void* copy,int kind,const Group& group){
    int base=0,value=0;if(!source||!copy||source==copy||*(int*)((char*)source+0x10)!=group.key.entity||!read(source,kind,group.key.property,base)||!calculate(group,base,value)||!domain(kind,group.key.property,value))return false;
    const auto* f=modification_effects::field(kind,group.key.property);char* target=(char*)copy+f->offset;
    if(f->width==1){int8_t n=int8_t(value);memcpy(target,&n,1);}else if(f->width==2){int16_t n=int16_t(value);memcpy(target,&n,2);}else memcpy(target,&value,4);
    if(kind==0&&!strcmp(group.key.property,"life"))*(int32_t*)((char*)copy+0x34)=value;
    return true;
}
static void* copy_scope(void* source,int kind,Compiled& c,Roots& roots,int& fields){
    if(!source||kind<0||kind>4)return nullptr;void* copy=local_research_runtime::clone(source,roots);if(!copy)return nullptr;int count=0;
    for(int i=0;i<c.count;++i){const auto& group=c.groups[i];if(group.key.kind!=kind||group.key.entity!=*(int*)((char*)source+0x10))continue;
        if(!apply(source,copy,kind,group))return nullptr;++count;}
    if(!count)return nullptr;fields=count;return copy;
}
// Copies preserve aliases within a unit. Global fraction/mine scopes are
// projected separately onto an unpublished player side, never canonical BATTLE.
static void* copy_unit(void* source,int id,Roots& roots,int& projected,int& global_groups){
    const auto* d=definition(id);auto* c=compiled(id);if(!d||!c||!source||*(int*)((char*)source+0x10)!=d->unit)return nullptr;
    void* copy=local_research_runtime::clone(source,roots),*states=get(source,0x18);void* next=local_research_runtime::new_list(states,roots);if(!copy||!next)return nullptr;
    int n=states?*(int*)((char*)states+0x18):0;if(n<1||n>32)return nullptr;
    void* state_source[32]={},*state_copy[32]={},*weapon_source[1024]={},*weapon_copy[1024]={};int state_count=0,weapon_count=0;void* ex=nullptr;
    for(int i=0;i<n;++i){void* s=list_item(states,i);if(!s)return nullptr;int index=0;while(index<state_count&&state_source[index]!=s)++index;void* clone=index<state_count?state_copy[index]:nullptr;
        if(!clone){clone=local_research_runtime::clone(s,roots);if(!clone)return nullptr;state_source[state_count]=s;state_copy[state_count++]=clone;
            void* ws=get(s,0x18),*list=local_research_runtime::new_list(ws,roots);if(!list)return nullptr;int count=ws?*(int*)((char*)ws+0x18):0;if(count<0||count>32)return nullptr;
            for(int j=0;j<count;++j){void* w=list_item(ws,j);if(!w||*(bool*)((char*)w+0x14))return nullptr;int k=0;while(k<weapon_count&&weapon_source[k]!=w)++k;void* wc=k<weapon_count?weapon_copy[k]:nullptr;
                if(!wc){if(weapon_count==1024)return nullptr;wc=local_research_runtime::clone(w,roots);if(!wc)return nullptr;weapon_source[weapon_count]=w;weapon_copy[weapon_count++]=wc;}
                void* a[]={wc};invoke_named(list,"Add",1,a,&ex);if(ex)return nullptr;}
            ref(clone,"weapons",list);}
        void* a[]={clone};invoke_named(next,"Add",1,a,&ex);if(ex)return nullptr;
    }
    int done=0,global=0;
    for(int i=0;i<c->count;++i){const auto& g=c->groups[i];int hits=0;
        if(g.key.kind>=3){++global;continue;}
        if(g.key.kind==0){if(!apply(source,copy,0,g))return nullptr;++hits;}
        if(g.key.kind==1)for(int j=0;j<state_count;++j)if(*(int*)((char*)state_source[j]+0x10)==g.key.entity){if(!apply(state_source[j],state_copy[j],1,g))return nullptr;++hits;}
        if(g.key.kind==2)for(int j=0;j<weapon_count;++j)if(*(int*)((char*)weapon_source[j]+0x10)==g.key.entity){if(!apply(weapon_source[j],weapon_copy[j],2,g))return nullptr;++hits;}
        if(!hits)return nullptr;done+=hits;
    }
    ref(copy,"states",next);ref(copy,"visualId",get(c->definition,0x30));projected=done;global_groups=global;return copy;
}
static void* clone_dictionary(void* original,void* values,Roots& roots){
    if(!original||!values)return nullptr;void* result=object_new(object_class(original));if(!roots.keep(result))return nullptr;void* ex=nullptr;invoke_named(result,".ctor",0,nullptr,&ex);return ex?nullptr:result;
}
static bool add(void* dictionary,int id,void* value){void* ex=nullptr;void* args[]={&id,value};invoke_named(dictionary,"Add",2,args,&ex);return !ex;}
// Called only on the unpublished side shell between research and skill
// projection. Canonical dictionaries and all other seats stay untouched.
static bool prepare_side(void* side,const local_progress::State& state){
    if(!side||state.modification_equipment_count>64)return false;if(!state.modification_equipment_count)return true;
    Roots roots;void* old_fraction=get(side,0xd8),*fraction=old_fraction,*old_mines=get(side,0x100);if(!old_fraction||!old_mines)return false;
    int faction=*(int*)((char*)old_fraction+0x10);Compiled* equipped[64]={};const modification_content::Definition* definitions[64]={};
    Action global_keys[64];int global_count=0;bool fraction_changed=false,mines_changed=false;
    for(uint32_t i=0;i<state.modification_equipment_count;++i){const auto& e=state.modification_equipment[i];const auto* d=definition(e.modification);auto* c=compiled(e.modification);
        if(!d||!c||d->faction!=faction||d->unit!=e.unit||local_progress::stock_amount(state,17,e.modification)<=0)return false;definitions[i]=d;equipped[i]=c;
        for(int j=0;j<c->count;++j){const auto& g=c->groups[j];if(g.key.kind<3)continue;
            for(int k=0;k<global_count;++k)if(modification_effects::same(global_keys[k],g.key))return false; // Future overlapping globals need combined native groups.
            if(global_count==64)return false;global_keys[global_count++]=g.key;
            if(g.key.kind==3){if(g.key.entity!=faction)return false;fraction_changed=true;}else if(g.key.kind==4)mines_changed=true;else return false;
        }}
    // Research can already have private Fraction aliases for the side and
    // its prototypes. Global actions address faction identity, not one alias.
    // Preserve every alias's unrelated fields while applying each action once.
    void* fraction_sources[128]={},*fraction_copies[128]={};int fraction_count=0;
    auto project_fraction=[&](void* source)->void*{
        if(!source||*(int*)((char*)source+0x10)!=faction)return nullptr;
        for(int i=0;i<fraction_count;++i)if(fraction_sources[i]==source)return fraction_copies[i];
        if(fraction_count==128)return nullptr;void* copy=local_research_runtime::clone(source,roots);if(!copy)return nullptr;
        for(uint32_t i=0;i<state.modification_equipment_count;++i)for(int j=0;j<equipped[i]->count;++j){const auto& g=equipped[i]->groups[j];if(g.key.kind==3&&!apply(source,copy,3,g))return nullptr;}
        fraction_sources[fraction_count]=source;fraction_copies[fraction_count++]=copy;return copy;
    };
    if(fraction_changed){fraction=project_fraction(old_fraction);if(!fraction)return false;}
    void* mines=old_mines;
    if(mines_changed){uintptr_t handle=0;void* values=local_dictionary_values(old_mines,find_class("com.geargames.aow.entities","MineType"),handle);if(!values||!roots.take(handle))return false;
        mines=clone_dictionary(old_mines,values,roots);if(!mines)return false;int found=0,wanted=0;
        for(int j=0;j<global_count;++j)wanted+=global_keys[j].kind==4;
        for(size_t j=0;j<array_length(values);++j){void* type=((void**)((char*)values+32))[j],*published=type;Roots item_roots;if(!type)return false;int id=*(int*)((char*)type+0x10);
            for(uint32_t i=0;i<state.modification_equipment_count;++i)for(int k=0;k<equipped[i]->count;++k){const auto& g=equipped[i]->groups[k];if(g.key.kind!=4||g.key.entity!=id)continue;
                if(published==type){published=local_research_runtime::clone(type,item_roots);if(!published)return false;}if(!apply(type,published,4,g))return false;++found;}
            if(!add(mines,id,published))return false;}
        if(found!=wanted)return false;
    }
    uintptr_t units_handle=0;void* old_units=get(side,0xf0),*unit_values=local_dictionary_values(old_units,find_class("com.geargames.aow.entities","UnitType"),units_handle);
    if(!unit_values||!roots.take(units_handle))return false;void* units=clone_dictionary(old_units,unit_values,roots);if(!units)return false;int modified=0;
    for(size_t j=0;j<array_length(unit_values);++j){void* type=((void**)((char*)unit_values+32))[j],*published=type;Roots item_roots;if(!type)return false;int id=*(int*)((char*)type+0x10);
        for(uint32_t i=0;i<state.modification_equipment_count;++i)if(definitions[i]->unit==id){int fields=0,globals=0;published=copy_unit(type,definitions[i]->id,item_roots,fields,globals);if(!published)return false;++modified;break;}
        void* type_fraction=get(type,0x20);
        if(fraction_changed&&type_fraction&&*(int*)((char*)type_fraction+0x10)==faction){
            void* projected_fraction=project_fraction(type_fraction);if(!projected_fraction)return false;
            if(published==type)published=local_research_runtime::clone(type,item_roots);if(!published)return false;ref(published,"fraction",projected_fraction);}
        if(!add(units,id,published))return false;
    }
    void* buildings=get(side,0xf8);
    if(fraction_changed){uintptr_t handle=0;void* values=local_dictionary_values(buildings,find_class("com.geargames.aow.entities","BuildingType"),handle);if(!values||!roots.take(handle))return false;
        void* next=clone_dictionary(buildings,values,roots);if(!next)return false;
        for(size_t j=0;j<array_length(values);++j){void* type=((void**)((char*)values+32))[j],*published=type;Roots item_roots;if(!type)return false;
            void* field=class_field_from_name(object_class(type),"fraction");if(!field)return false;
            void* type_fraction=get(type,field_offset(field));
            if(type_fraction&&*(int*)((char*)type_fraction+0x10)==faction){void* projected_fraction=project_fraction(type_fraction);if(!projected_fraction)return false;
                published=local_research_runtime::clone(type,item_roots);if(!published)return false;ref(published,"fraction",projected_fraction);}
            if(!add(next,*(int*)((char*)type+0x10),published))return false;}buildings=next;
    }
    ref(side,"$gg",units);ref(side,"$Gg",buildings);ref(side,"$Eg",fraction);ref(side,"$hg",mines);
    if(!local_research_runtime::relink_production_units(side))return false;
    LOG("Local modifications staged side=%d equipped=%u unit_types=%d absent_types=%u globals=%d fraction_private=%d mines_private=%d",*(int*)((char*)side+0x10),state.modification_equipment_count,modified,state.modification_equipment_count-modified,global_count,fraction_changed,mines_changed);return true;
}
}
