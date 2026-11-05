// Startup-only entity migration. Stage every reference/value before publishing
// a private research dictionary. Keep the original entity/weapon identities so
// script targets, grids, queues and formations continue to point to them.
#pragma once
namespace preplaced_research {
using local_research_runtime::Roots;
static void* pointer(void* p,int offset){return p?*(void**)((char*)p+offset):nullptr;}
static int list_size(void* p){return p?*(int*)((char*)p+0x18):0;}
static bool reject(int line){LOG("Preplaced research rejected at line=%d",line);return false;}
struct Patch {void* object=nullptr;void* field=nullptr;void* reference=nullptr;int offset=0,width=0,value=0;};
struct Migration {
    static constexpr int capacity=16384,entity_capacity=4096;
    Patch* patches=(Patch*)calloc(capacity,sizeof(Patch));int count=0,units=0,buildings=0,weapon_links=0,queue_links=0;
    void** seen=(void**)calloc(entity_capacity,sizeof(void*));int seen_count=0;
    bool allow_mod_penalties=false;void** mine_seen=nullptr;int mine_seen_count=0,mine_links=0;
    ~Migration(){free(patches);free(seen);free(mine_seen);}
    bool ref(void* object,const char* name,void* value){
        if(!object||!value||!patches)return reject(__LINE__);
        void* field=class_field_from_name(object_class(object),name);if(!field)return reject(__LINE__);
        int offset=field_offset(field);if(pointer(object,offset)==value)return true;
        for(int i=0;i<count;++i)if(patches[i].object==object&&patches[i].offset==offset)
            return patches[i].width==0&&patches[i].reference==value;
        if(count==capacity)return reject(__LINE__);
        patches[count++]={object,field,value,offset,0,0};return true;
    }
    bool number(void* object,int offset,int width,int value){
        if(!patches||count==capacity)return reject(__LINE__);
        patches[count++]={object,nullptr,nullptr,offset,width,value};return true;
    }
    bool life(void* object,int old_max,int new_max,int short_offset,int int_offset,bool allow_decrease=false){
        int value=*(int*)((char*)object+int_offset);
        if(old_max==new_max)return true; // No HP research: preserve special/sentinel HP exactly.
        if(old_max<=0||new_max<=0||(new_max<old_max&&!allow_decrease)||new_max>INT16_MAX||value>old_max||
           value<INT16_MIN||*(int16_t*)((char*)object+short_offset)!=value)return reject(__LINE__);
        // Preserve absolute damage and corpses; never heal existing damage or
        // resurrect a dead entity merely because its maximum HP increased.
        int next=value>0?value+(new_max-old_max):value;
        if(value>0&&next<1)next=1; // Lowering a loaded maximum cannot kill or resurrect an existing entity.
        return number(object,short_offset,2,next)&&number(object,int_offset,4,next);
    }
    bool weapons(void* runtime,void* old_types,void* new_types){
        int n=list_size(runtime),a=list_size(old_types),b=list_size(new_types);
        if(n<0||n>64||a<0||a>64||a!=b)return reject(__LINE__);
        // Null/empty means no runtime weapons have been constructed yet.
        if(!n)return true;
        if(n!=a)return reject(__LINE__);
        for(int i=0;i<n;++i){void* w=list_item(runtime,i);void* old=list_item(old_types,i);void* next=list_item(new_types,i);
            if(!w||strcmp(class_name(object_class(w)),"Weapon")||!old||!next||pointer(w,0x10)!=old||
               *(int16_t*)((char*)old+0x20)!=*(int16_t*)((char*)next+0x20)||
               *(int16_t*)((char*)old+0x24)!=*(int16_t*)((char*)next+0x24))return reject(__LINE__);
            if(old!=next)++weapon_links;
            if(!ref(w,"we_type",next))return false;
        }
        return true;
    }
    bool unit_link(void* object,const char* field_name,int offset,void* old_dictionary,void* new_dictionary){
        void* old=pointer(object,offset);if(!old)return true;
        int id=*(int*)((char*)old+0x10);void* next=local_dictionary_item(new_dictionary,id);
        if(local_dictionary_item(old_dictionary,id)!=old||!next||!generic_unit_research::identity(old,next))return reject(__LINE__);
        if(old!=next)++queue_links;
        return ref(object,field_name,next);
    }
    bool mine_type(void* object,const char* name,int offset,void* side,void* prepared){
        void* old=pointer(object,offset);if(!old)return true;int id=*(int*)((char*)old+0x10);
        void* next=local_dictionary_item(pointer(prepared,0x100),id);
        if(local_dictionary_item(pointer(side,0x100),id)!=old||!next||!mine_research::same(old,next))return reject(__LINE__);
        return ref(object,name,next);
    }
    bool mine_entity(void* mine,void* side,void* prepared){
        if(!mine||pointer(mine,0x30)!=side)return true;
        if(strcmp(class_name(object_class(mine)),"Mine"))return reject(__LINE__);
        void* old=pointer(mine,0x18);int id=old?*(int*)((char*)old+0x10):-1;
        void* next=local_dictionary_item(pointer(prepared,0x100),id);
        if(!old||!next||local_dictionary_item(pointer(side,0x100),id)!=old||!mine_research::same(old,next))return reject(__LINE__);
        for(int i=0;i<count;++i)if(patches[i].object==mine&&patches[i].offset==0x18)
            return patches[i].width==0&&patches[i].reference==next;
        int old_max=*(int16_t*)((char*)old+0x38),new_max=*(int16_t*)((char*)next+0x38);
        if(old_max!=new_max){int current=*(int16_t*)((char*)mine+0x2a);
            if(old_max<1||new_max<1||current<0||current>old_max)return reject(__LINE__);
            int value=current>0?current+new_max-old_max:current;if(current>0&&value<1)value=1;
            if(!number(mine,0x2a,2,value))return false;}
        if(old!=next)++mine_links;return ref(mine,"type",next);
    }
    bool mine_queues(void* unit,void* side,void* prepared,bool protect=false){
        if(!protect&&pointer(side,0x100)==pointer(prepared,0x100))return true;
        auto command=[&](void* value){if(!value)return true;
            if(strcmp(class_name(object_class(value)),"MineLaying"))return reject(__LINE__);
            if(protect){for(int i=0;i<count;++i)if(patches[i].object==value)return reject(__LINE__);return true;}
            return mine_type(value,"$jJ",0x18,side,prepared)&&mine_entity(pointer(value,0x20),side,prepared);};
        auto set=[&](void* value){if(!value)return true;
            if(strcmp(class_name(object_class(value)),"MineLayingSet"))return reject(__LINE__);
            void* commands=pointer(value,0x10);int n=list_size(commands);if(n<0||n>1024)return reject(__LINE__);
            for(int i=0;i<n;++i)if(!command(list_item(commands,i)))return false;return true;};
        if(!command(pointer(unit,0x208))||!set(pointer(unit,0x210)))return false;
        void* sets=pointer(unit,0x218);int n=list_size(sets);if(n<0||n>1024)return reject(__LINE__);
        for(int i=0;i<n;++i)if(!set(list_item(sets,i)))return false;return true;
    }
    bool entity(void* entity,void* side,void* prepared,bool building){
        if(!entity||pointer(entity,0x18)!=side)return true;
        if(!seen||!patches)return reject(__LINE__);
        for(int i=0;i<seen_count;++i){if(seen[i]==entity)return true;
            if(*(int*)((char*)seen[i]+0x10)==*(int*)((char*)entity+0x10))return reject(__LINE__);}
        if(seen_count==entity_capacity)return reject(__LINE__);seen[seen_count++]=entity;
        if(strcmp(class_name(object_class(entity)),building?"Building":"Unit"))return reject(__LINE__);
        int dictionary_offset=building?0xf8:0xf0,type_offset=building?0x38:0x40;
        void* old=pointer(entity,type_offset);int id=old?*(int*)((char*)old+0x10):-1;
        void* next=local_dictionary_item(pointer(prepared,dictionary_offset),id);
        if(!old||!next||local_dictionary_item(pointer(side,dictionary_offset),id)!=old)return reject(__LINE__);
        if(building){
            ++buildings;int level=*(int8_t*)((char*)entity+0x60);
            void* old_levels=pointer(old,0x18);void* new_levels=pointer(next,0x18);
            if(level<0||level>=list_size(old_levels)||list_size(old_levels)!=list_size(new_levels))return reject(__LINE__);
            void* a=list_item(old_levels,level);void* b=list_item(new_levels,level);
            if(!a||!b||!life(entity,*(int*)((char*)a+0x34),*(int*)((char*)b+0x34),0x58,0x5c)||
               !weapons(pointer(entity,0x28),pointer(a,0x18),pointer(b,0x18))||!ref(entity,"bu_type",next))return false;
            for(auto link:{"curr_train","curr_move_out","nuclear_missile"}){
                int offset=!strcmp(link,"curr_train")?0xa0:!strcmp(link,"curr_move_out")?0xa8:0x150;
                if(!unit_link(entity,link,offset,pointer(side,0xf0),pointer(prepared,0xf0)))return false;
            }
            void* queue=pointer(entity,0x98);int n=list_size(queue);if(n<0||n>256)return reject(__LINE__);
            for(int i=0;i<n;++i){void* item=list_item(queue,i);if(!item||!unit_link(item,"un_type",0x10,pointer(side,0xf0),pointer(prepared,0xf0)))return false;}
        }else{
            ++units;if(!mine_queues(entity,side,prepared))return false;if(old==next)return true;
            if(!generic_unit_research::identity(old,next))return reject(__LINE__);
            int state=*(int8_t*)((char*)entity+0x6b);void* a_states=pointer(old,0x18);void* b_states=pointer(next,0x18);
            if(state<0||state>=list_size(a_states)||list_size(a_states)!=list_size(b_states))return reject(__LINE__);
            void* a=list_item(a_states,state);void* b=list_item(b_states,state);
            bool penalty=allow_mod_penalties&&local_progress::equipped_modification(progress,*(int*)((char*)old+0x10))>0;
            if(!a||!b||!life(entity,*(int*)((char*)old+0x34),*(int*)((char*)next+0x34),0x182,0x184,penalty)||
               !weapons(pointer(entity,0x28),pointer(a,0x18),pointer(b,0x18))||!ref(entity,"un_type",next))return false;
            // Retain cached additive modifiers (forest/aura/boost/current pace)
            // and change only the underlying state's verified base difference.
            int64_t sight=(int64_t)*(int*)((char*)entity+0xbc)+*(int*)((char*)b+0x4c)-*(int*)((char*)a+0x4c);
            int speed=*(int8_t*)((char*)entity+0xba)+*(int8_t*)((char*)b+0x58)-*(int8_t*)((char*)a+0x58);
            if(sight<0||sight>INT_MAX||speed<0||speed>INT8_MAX)return reject(__LINE__);
            if(!number(entity,0xbc,4,(int)sight)||!number(entity,0xba,1,speed))return false;
        }
        return true;
    }
    bool protected_entity(void* entity,void* side){
        if(!entity||pointer(entity,0x18)==side)return true;
        for(int i=0;i<count;++i)if(patches[i].object==entity)return reject(__LINE__);
        void* weapons=pointer(entity,0x28);int n=list_size(weapons);if(n<0||n>64)return reject(__LINE__);
        for(int j=0;j<n;++j)for(int i=0;i<count;++i)if(patches[i].object==list_item(weapons,j))return reject(__LINE__);
        if(!strcmp(class_name(object_class(entity)),"Building")){
            void* queue=pointer(entity,0x98);int n=list_size(queue);if(n<0||n>256)return reject(__LINE__);
            for(int j=0;j<n;++j)for(int i=0;i<count;++i)if(patches[i].object==list_item(queue,j))return reject(__LINE__);
        }
        else if(!strcmp(class_name(object_class(entity)),"Unit")&&!mine_queues(entity,nullptr,nullptr,true))return false;
        return true;
    }
    bool mines(void* battle,void* side,void* prepared){
        void* old_dictionary=pointer(side,0x100),*next_dictionary=pointer(prepared,0x100);if(old_dictionary==next_dictionary)return true;
        if(!old_dictionary||!next_dictionary)return reject(__LINE__);void* grid=pointer(battle,0x288);if(!grid)return true;
        size_t rows=array_length(grid);if(rows>1024)return reject(__LINE__);mine_seen=(void**)calloc(entity_capacity,sizeof(void*));if(!mine_seen)return reject(__LINE__);
        for(size_t x=0;x<rows;++x){void* row=((void**)((char*)grid+32))[x];if(!row)continue;size_t columns=array_length(row);if(columns>1024)return reject(__LINE__);
            for(size_t y=0;y<columns;++y){void* mine=((void**)((char*)row+32))[y];int chain=0;
                for(;mine;mine=pointer(mine,0x48)){if(++chain>entity_capacity||strcmp(class_name(object_class(mine)),"Mine"))return reject(__LINE__);
                    bool known=false;for(int i=0;i<mine_seen_count;++i)if(mine_seen[i]==mine)known=true;if(known)continue;
                    if(mine_seen_count==entity_capacity)return reject(__LINE__);mine_seen[mine_seen_count++]=mine;
                    if(pointer(mine,0x30)!=side){for(int i=0;i<count;++i)if(patches[i].object==mine)return reject(__LINE__);continue;}
                    if(!mine_entity(mine,side,prepared))return false;
                }}}
        return true;
    }
    bool prepare(void* battle,void* side,void* prepared){
        if(!battle||!side||!prepared||!patches||!seen)return reject(__LINE__);
        auto array=[&](void* value,bool building,bool protect){
            size_t n=value?array_length(value):0;if(n>entity_capacity)return reject(__LINE__);
            for(size_t i=0;i<n;++i){void* item=((void**)((char*)value+32))[i];
                if(!item)continue;
                if(protect){if(!protected_entity(item,side))return false;}
                else if(pointer(item,0x18)!=side||!entity(item,side,prepared,building))return reject(__LINE__);}
            return true;};
        if(!array(pointer(side,0x140),false,false)||!array(pointer(side,0x148),true,false))return false;
        void* alliances=pointer(battle,0x170);size_t n=alliances?array_length(alliances):0;
        if(!n||n>8)return reject(__LINE__);
        for(size_t i=0;i<n;++i){void* alliance=((void**)((char*)alliances+32))[i];if(!alliance)return reject(__LINE__);
            for(int offset:{0x78,0x80,0x88,0xa0}){void* list=pointer(alliance,offset);int count=list_size(list);
                if(count<0||count>entity_capacity)return reject(__LINE__);
                for(int j=0;j<count;++j)if(!entity(list_item(list,j),side,prepared,offset>=0x88))return false;}}
        // Verify every other seat's arrays and alliance lists do not share a
        // runtime weapon/queue object that is about to receive a patch.
        void* members=nullptr;
        for(size_t i=0;i<n;++i){void* alliance=((void**)((char*)alliances+32))[i];
            for(int offset:{0x78,0x80,0x88,0xa0}){void* list=pointer(alliance,offset);for(int j=0;j<list_size(list);++j)if(!protected_entity(list_item(list,j),side))return false;}
            members=pointer(alliance,0x18);int count=list_size(members);if(count<0||count>16)return reject(__LINE__);
            for(int j=0;j<count;++j){void* other=list_item(members,j);if(!other)return reject(__LINE__);
                if(other!=side&&(!array(pointer(other,0x140),false,true)||!array(pointer(other,0x148),true,true)))return false;}}
        return mines(battle,side,prepared);
    }
    void commit(){for(int i=0;i<count;++i){const auto& p=patches[i];char* at=(char*)p.object+p.offset;
        if(!p.width)field_set(p.object,p.field,p.reference);
        else if(p.width==1)*(int8_t*)at=(int8_t)p.value;
        else if(p.width==2)*(int16_t*)at=(int16_t)p.value;
        else *(int*)at=p.value;}}
};
}
