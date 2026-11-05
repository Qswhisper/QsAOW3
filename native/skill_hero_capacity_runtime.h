// Capacity is an out-of-battle service value, not a second native skill bonus.
#pragma once
namespace skill_hero_capacity {
static bool calculate(int base,int effect,int level,int& result){
    if(base<1||effect<0||level<0||level>11)return false;
    int64_t value=int64_t(base)*(1000+int64_t(effect)*level)/1000;
    if(value<1||value>INT16_MAX)return false;result=(int)value;return true;
}
static bool project(void* configuration,const local_progress::State& state){
    if(!configuration||!local_skill::initialize())return false;
    skill_tree_policy::Summary summary;if(!local_skill::graph.summarize(state,summary))return false;
    void* parameter=local_career::list("BATTLE","heroParam"),*card=local_career::skill_card(28);
    if(!parameter||!card||strcmp(class_name(object_class(parameter)),"HeroParam"))return false;
    int base=*(int*)((char*)parameter+0xac),effect=*(int*)((char*)card+0x38),capacity=0;
    // Same retained Effect/1000 and positive integer rounding as other skill
    // increases. Always derive from the original parameter, never the last cap.
    if(!calculate(base,effect,summary.level[28],capacity))return false;
    *(int*)((char*)configuration+0x28)=capacity;
    // Capacity changes do not refill the hall or grant batteries. Battle entry
    // separately applies the existing fill-to-current-cap policy.
    int& balance=*(int*)((char*)configuration+0x24);if(balance>capacity)balance=capacity;
    return balance>=0;
}
}
