// Rebase capture-time OWN growth onto the retained release's base prototypes.
// Scenario IDs, availability, producers, script, terrain and other seats stay
// in the captured input. Only explicitly opted-in fresh routes use this path.
#pragma once
namespace battle_growth_baseline {
using local_research_runtime::Roots;
static void* get(void* p,int o){return p?*(void**)((char*)p+o):nullptr;}
static int count(void* p){return p?*(int*)((char*)p+0x18):0;}
static bool reject(int line){LOG("Own growth baseline refused line=%d",line);return false;}
static void span(void* target,void* base,int start,int length){memcpy((char*)target+start,(char*)base+start,length);}
static bool add(void* list,void* value){void* ex=nullptr;void* args[]={value};invoke_named(list,"Add",1,args,&ex);return !ex;}
// First hero acquisition changes only heroClass in the verified original
// service. Accept its reversal only when the ENTIRE ability content graph is
// still canonical (apart from the original seat-scoped entity IDs).
struct SameAbility {
    void* a[128]={},*b[128]={};int seen=0;
    static int width(int code){return code==2||code==4||code==5?1:code==3||code==6||code==7?2:
        code==8||code==9||code==12?4:code==10||code==11||code==13?8:0;}
    bool equal(void* x,void* y,int depth=0){
        if(x==y)return true;if(!x||!y||depth>12||object_class(x)!=object_class(y))return false;
        for(int i=0;i<seen;++i)if(a[i]==x)return b[i]==y;
        if(seen==128)return false;a[seen]=x;b[seen++]=y;
        void* klass=object_class(x);const char* name=class_name(klass);
        if(!strcmp(name,"String")){int n=*(int*)((char*)x+16);
            return n>=0&&n<=4096&&n==*(int*)((char*)y+16)&&!memcmp((char*)x+20,(char*)y+20,n*2);}
        if(!strcmp(name,"List`1")){int n=count(x);if(n<0||n>64||n!=count(y))return false;
            for(int i=0;i<n;++i)if(!equal(list_item(x,i),list_item(y,i),depth+1))return false;return true;}
        size_t len=strlen(name);
        if(len>2&&name[len-1]==']'){size_t n=array_length(x);if(n>256||n!=array_length(y))return false;
            void* element=class_element(klass);if(!element)return false;
            if(class_valuetype(element)){int w=width(type_type(class_type(element)));return w&&!memcmp((char*)x+32,(char*)y+32,n*w);}
            for(size_t i=0;i<n;++i)if(!equal(((void**)((char*)x+32))[i],((void**)((char*)y+32))[i],depth+1))return false;return true;}
        if(strcmp(class_namespace(klass),"com.geargames.aow.entities"))return false;
        for(void* parent=klass;parent;parent=class_parent(parent)){void* iter=nullptr;
            while(void* f=class_fields(parent,&iter)){if(field_flags(f)&(0x10|0x40))continue;
                int o=field_offset(f),code=type_type(field_type(f)),w=width(code);if(o<16)continue;
                if(!strcmp(field_name(f),"id")&&code==8){int i=*(int*)((char*)x+o),j=*(int*)((char*)y+o);
                    if(i!=j&&i!=j+1000)return false;continue;}
                if(w){if(memcmp((char*)x+o,(char*)y+o,w)){LOG("Hero first-acquisition ability differs class=%s field=%s",name,field_name(f));return false;}}
                else if(code==14||code==18||code==20||code==28||code==29||
                        (code==21&&!class_valuetype(class_from_type(field_type(f))))){if(!equal(get(x,o),get(y,o),depth+1))return false;}
                else {LOG("Hero first-acquisition ability unverified field=%s code=%d",field_name(f),code);return false;}
            }}return true;
    }
};
struct Weapons {
    void* sources[2048]={},*bases[2048]={},*copies[2048]={};int size=0;
    void* copy(void* source,void* base,Roots& roots){
        if(!source||!base||*(int16_t*)((char*)source+0x20)!=*(int16_t*)((char*)base+0x20)||
           *(int16_t*)((char*)source+0x24)!=*(int16_t*)((char*)base+0x24))return nullptr;
        for(int i=0;i<size;++i)if(sources[i]==source)return bases[i]==base?copies[i]:nullptr;
        if(size==2048)return nullptr;void* next=local_research_runtime::clone(source,roots);if(!next)return nullptr;
        // Numeric original weapon configuration; preserve content identity and
        // source-owned reference graph (aura, visuals, priority/modifier arrays).
        span(next,base,0x28,0x18);span(next,base,0x48,0x23);span(next,base,0x78,0x38);
        *(bool*)((char*)next+0x14)=false;
        sources[size]=source;bases[size]=base;copies[size++]=next;return next;
    }
    void* list(void* source,void* base,Roots& roots){
        if(!source&&!base)return nullptr;
        int n=count(source);if(!source||!base||n<0||n>32||n!=count(base))return nullptr;
        void* next=local_research_runtime::new_list(source,roots);if(!next)return nullptr;
        for(int i=0;i<n;++i){Roots one;void* weapon=copy(list_item(source,i),list_item(base,i),one);if(!weapon||!add(next,weapon))return nullptr;}
        return next;
    }
};
static void* prototype(void* source,void* base,bool building,Weapons& weapons,Roots& roots){
    if(!source||!base)return nullptr;
    // A captured upgraded hero's ability graph is not an ordinary stat delta.
    // A different class/count needs its own verified ability projection.
    bool first_acquisition=!building&&*(int*)((char*)source+0xa0)==1&&*(int*)((char*)base+0xa0)==0;
    if(!building){
        if(*(int*)((char*)source+0xa4)!=*(int*)((char*)base+0xa4))return nullptr;
        if(*(int*)((char*)source+0xa0)!=*(int*)((char*)base+0xa0)){
            SameAbility graph;
            if(!first_acquisition||!graph.equal(get(source,0x98),get(base,0x98)))return nullptr;
        }
    }
    void* next=local_research_runtime::clone(source,roots);void* old=get(source,0x18),*original=get(base,0x18);
    int n=count(old);if(!next||!old||!original||n<1||n>32||n!=count(original))return nullptr;
    if(first_acquisition){*(int*)((char*)next+0xa0)=0;
        LOG("Captured first hero acquisition removed id=%d canonical_ability_graph=1",*(int*)((char*)source+0x10));}
    if(building){span(next,base,0x31,1);span(next,base,0x34,4);}
    else {span(next,base,0x32,0x1a);span(next,base,0x77,5);
        *(int*)((char*)next+0x3c)=*(int*)((char*)base+0x38);}
    void* rows=local_research_runtime::new_list(old,roots);if(!rows)return nullptr;
    for(int i=0;i<n;++i){Roots one;void* a=list_item(old,i),*b=list_item(original,i);
        if(!a||!b)return nullptr;void* row=local_research_runtime::clone(a,one);if(!row)return nullptr;
        if(building){span(row,b,0x30,0x22);span(row,b,0x60,0x14);span(row,b,0x80,8);
            *(int*)((char*)row+0x44)=*(int*)((char*)b+0x40);
            *(int16_t*)((char*)row+0x4e)=*(int16_t*)((char*)b+0x4c);
            *(int16_t*)((char*)row+0x62)=*(int16_t*)((char*)b+0x60);
            *(int*)((char*)row+0x68)=*(int*)((char*)b+0x64);
        }else{if(*(int8_t*)((char*)a+0x20)!=*(int8_t*)((char*)b+0x20)||*(int8_t*)((char*)a+0x26)!=*(int8_t*)((char*)b+0x26))return nullptr;
            span(row,b,0x22,6);span(row,b,0x48,0x1c);span(row,b,0x70,0x17);span(row,b,0x88,9);
            *(int*)((char*)row+0x50)=*(int*)((char*)b+0x4c);*(int8_t*)((char*)row+0x59)=*(int8_t*)((char*)b+0x58);
            *(int16_t*)((char*)row+0x74)=*(int16_t*)((char*)b+0x72);*(int16_t*)((char*)row+0x7a)=*(int16_t*)((char*)b+0x78);
            *(int16_t*)((char*)row+0x8e)=*(int16_t*)((char*)b+0x8c);
        }
        void* aw=get(a,0x18),*bw=get(b,0x18);
        if(aw||bw){void* list=weapons.list(aw,bw,one);if(!list)return nullptr;field_set(row,class_field_from_name(object_class(row),"weapons"),list);}
        if(!add(rows,row))return nullptr;
    }
    field_set(next,class_field_from_name(object_class(next),building?"levels":"states"),rows);return next;
}
static bool prepare(void* prepared,void* battle,void* source_side){
    // The verified fresh inputs spawn their own armies from GAI after startup.
    // A future snapshot with preplaced own entities needs its HP/cache contract
    // checked before removing captured growth.
    if(!prepared||!local_research_runtime::empty_player_buildings(battle,source_side)||
       !local_research_runtime::empty_player_units(battle,source_side))return reject(__LINE__);
    Weapons weapons;int total[2]={};
    for(int kind=0;kind<2;++kind){bool building=kind==1;Roots roots;uintptr_t handle=0;
        void* old=get(prepared,building?0xf8:0xf0);
        void* values=local_dictionary_values(old,find_class("com.geargames.aow.entities",building?"BuildingType":"UnitType"),handle);
        void* bases=local_career::list("BATTLE",building?"buildingTypes":"unitTypes");
        if(!values||!roots.take(handle)||!bases)return reject(__LINE__);
        void* dictionary=object_new(object_class(old));if(!roots.keep(dictionary))return reject(__LINE__);
        void* ex=nullptr;invoke_named(dictionary,".ctor",0,nullptr,&ex);if(ex)return reject(__LINE__);
        for(size_t i=0;i<array_length(values);++i){Roots one;void* source=((void**)((char*)values+32))[i],*base=nullptr;
            for(int j=0;j<count(bases);++j){void* candidate=list_item(bases,j);
                if(building?building_research::identity(source,candidate):generic_unit_research::identity(source,candidate)){
                    if(base)return reject(__LINE__);base=candidate;}}
            void* next=prototype(source,base,building,weapons,one);if(!next)return reject(__LINE__);
            int id=*(int*)((char*)source+0x10);if(local_dictionary_item(old,id)!=source)return reject(__LINE__);
            void* args[]={&id,next};invoke_named(dictionary,"Add",2,args,&ex);if(ex)return reject(__LINE__);++total[kind];
        }
        field_set(prepared,class_field_from_name(object_class(prepared),building?"$Gg":"$gg"),dictionary);
    }
    if(!local_research_runtime::relink_production_units(prepared))return reject(__LINE__);
    LOG("Own growth baseline prepared units=%d buildings=%d weapons=%d",total[0],total[1],weapons.size);return true;
}
}
