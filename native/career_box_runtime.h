#pragma once
#include "career_box_content.generated.h"
namespace career_boxes {
static bool resolve(int id,int quantity,local_progress::CareerReward* out,int capacity,int& count){
    count=0;if(quantity!=1||!out)return false;
    void* shared=saved("SHARED"),*dictionary=shared?*(void**)((char*)shared+0xa8):nullptr,*rows=nullptr,*ex=nullptr;
    void* args[]={&id,&rows};void* found=dictionary?invoke_named(dictionary,"TryGetValue",2,args,&ex):nullptr;
    if(ex||!found||!*(bool*)((char*)found+16)||!rows)return false;
    int n=*(int*)((char*)rows+0x18),expected=0;for(const auto& item:items)expected+=item.box==id;
    if(!expected||expected!=n||capacity<expected)return false;
    for(const auto& item:items)if(item.box==id){void* source=nullptr;for(int j=0;j<n;++j){void* v=list_item(rows,j);if(v&&*(int*)((char*)v+0x10)==item.id)source=v;}
        if(!source||*(int*)((char*)source+0x14)!=id||*(int*)((char*)source+0x1c)!=100||*(int*)((char*)source+0x20)!=item.type||*(int*)((char*)source+0x24)||*(int*)((char*)source+0x40)!=item.amount||*(int*)((char*)source+0x44)!=item.amount)return false;
        out[count++]={item.type==1?0:item.type==2?1:2,0,item.amount};}
    return true;
}
}
