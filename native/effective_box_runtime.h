// The original preview reads ClientMainContext.BoxItems directly (static150).
// Publish a separate presentation dictionary; SHARED remains original so old
// draw journals can still resolve their exact former items and amounts.
#pragma once
namespace effective_boxes {
using Roots=local_faction::Roots;
static void* source;static void* view;static uintptr_t source_root,view_root;
static bool installed,reported_failure;static int removed_rows,changed_boxes;
static bool local_box(int id){return id==12000||contract_box_content::box(id)||supply_policy::box(id)||competition_track_content::box(id);}
static void* context_items(){
    void* cls=find_class("com.geargames.aow.scenes.main","ClientMainContext"),*ex=nullptr;
    void* result=cls?runtime_invoke(class_method(cls,"get_BoxItems",0),nullptr,nullptr,&ex):nullptr;
    return ex?nullptr:result;
}
static bool publish(){
    void* current=context_items();if(!current)return false;if(view&&current==view)return true;
    if(strcmp(class_name(object_class(current)),"Dictionary`2"))return false;
    void* entries=local_supply::get(current,0x18);int n=*(int*)((char*)current+0x20);
    if(n<1||n>1024||!entries||array_length(entries)<size_t(n))return false;
    Roots roots;void* next=object_new(object_class(current)),*ex=nullptr;
    if(!roots.keep(next))return false;invoke_named(next,".ctor",0,nullptr,&ex);if(ex)return false;
    int removed=0,changed=0;
    for(int i=0;i<n;++i){char* row=(char*)entries+32+i*24;if(*(int*)row<0)continue;
        int id=*(int*)(row+8);void* original=*(void**)(row+16),*projected=original;Roots one;
        if(local_box(id)){
            int count=original?*(int*)((char*)original+0x18):0;if(count<0||count>128)return false;
            int discard=0;for(int j=0;j<count;++j){void* item=list_item(original,j);if(!item)return false;
                discard+=effective_box_policy::removed(*(int*)((char*)item+0x20),*(int*)((char*)item+0x24));}
            if(discard){projected=object_new(object_class(original));if(!one.keep(projected))return false;
                invoke_named(projected,".ctor",0,nullptr,&ex);if(ex)return false;
                for(int j=0;j<count;++j){void* item=list_item(original,j);
                    if(effective_box_policy::removed(*(int*)((char*)item+0x20),*(int*)((char*)item+0x24)))continue;
                    void* args[]={item};invoke_named(projected,"Add",1,args,&ex);if(ex)return false;}
                removed+=discard;++changed;
            }
        }
        void* args[]={&id,projected};invoke_named(next,"Add",2,args,&ex);if(ex)return false;
    }
    uintptr_t pinned_source=gchandle_new(current,true),pinned_view=gchandle_new(next,true);
    if(!pinned_source||!pinned_view){if(pinned_source)gchandle_free(pinned_source);if(pinned_view)gchandle_free(pinned_view);return false;}
    void* cls=find_class("com.geargames.aow.scenes.main","ClientMainContext"),*args[]={next};
    runtime_invoke(class_method(cls,"set_BoxItems",1),nullptr,args,&ex);
    if(ex||context_items()!=next){gchandle_free(pinned_source);gchandle_free(pinned_view);return false;}
    if(source_root)gchandle_free(source_root);if(view_root)gchandle_free(view_root);
    source=current;view=next;source_root=pinned_source;view_root=pinned_view;
    removed_rows=removed;changed_boxes=changed;reported_failure=false;
    LOG("Local effective box previews published boxes=%d removed_rows=%d canonical_source_preserved=1",changed,removed);return true;
}
static void (*create_original)(void*,void*,void*);
static void create(void* self,void* box,void* method){
    if(offline_network&&box&&local_box(*(int*)((char*)box+0x10))&&!publish()&&!reported_failure){
        reported_failure=true;LOG("Local effective box preview projection unavailable; original preview fallback");}
    create_original(self,box,method);
}
static void install(){
    const uint32_t signature[]={0xd10203ff,0xa90467fe,0xa9055ff8,0xa90657f6};
    create_original=(decltype(create_original))install_hook(0x7f2da48,signature,(void*)create);
    installed=create_original;LOG("Local effective box preview hook=%d future_gifts_filtered=1 reroll_disabled=1",installed);
}
}
