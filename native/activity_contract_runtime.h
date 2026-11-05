// Invoke the pinned original FINAL side activity calculation, not the
// scoreboard's ongoing team maximum. This adapter does not count UI inputs.
#pragma once
namespace activity_contract_runtime {
static bool bound;
static bool available(){return bound;}
static bool read(void* battle,void* side,int& raw,int& percent){
    if(!bound||!battle||!side)return false;
    // Metadata lookup only on the attached original engine/GUI thread.
    void* cls=find_class("com.geargames.aow.entities.battle","BattleAct");
    const void* calculate=cls?class_method(cls,"$xA",4):nullptr;
    const void* normalize=cls?class_method(cls,"$WA",1):nullptr;
    if(!calculate||!normalize||*(uintptr_t*)calculate!=image_base+0x46bf04c||*(uintptr_t*)normalize!=image_base+0x46bff50)return false;
    bool adjust=true,final=true;void* ex=nullptr;void* a[]={battle,side,&adjust,&final};
    void* value=runtime_invoke(calculate,nullptr,a,&ex);if(ex||!value)return false;
    int result=*(int*)((char*)value+16);if(!activity_contract_events::valid(result))return false;
    void* b[]={&result};value=runtime_invoke(normalize,nullptr,b,&ex);if(ex||!value)return false;
    int shown=*(int*)((char*)value+16);if(shown<0||shown>100||(shown>70)!=activity_contract_events::qualifies(result))return false;
    raw=result;percent=shown;return true;
}
static void bind(){
    const uint32_t calculate[]={0xd101c3ff,0xa9017bfd,0xa9026ffc,0xa90367fa};
    const uint32_t normalize[]={0xa9be57fe,0xa9014ff4,0xf002b715,0xf0028294};
    bound=!memcmp((void*)(image_base+0x46bf04c),calculate,sizeof(calculate))&&
        !memcmp((void*)(image_base+0x46bff50),normalize,sizeof(normalize));
}
}
