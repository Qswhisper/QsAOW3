// Reward percentages use the original SkillsController integer formula. The
// base draw stays canonical; its final grants are journaled with the bonus.
#pragma once
#include "contract_policy.h"
namespace local_reward_skills {
static bool installed;
static bool cooperative_credit_installed;
static bool apply(const local_progress::State& state,int card_id,int base,int& amount){
    if(!installed||(card_id!=24&&card_id!=36&&card_id!=37)||base<0)return false;
    if(!state.skill_count){amount=base;return true;}
    if(!local_skill::initialize())return false;skill_tree_policy::Summary summary;
    if(!local_skill::graph.summarize(state,summary))return false;
    void* card=local_career::skill_card(card_id);int effect=card?*(int*)((char*)card+0x38):-1;
    if(effect!=(card_id==24?30:50))return false;
    return contract_policy::credits_bonus(base,effect,summary.level[card_id],amount);
}
static bool box_credits(const local_progress::State& state,int base,int& amount){return apply(state,36,base,amount);}
static bool credit_level(const local_progress::State& state,int id,int effect,int& level){
    if(!installed||!local_skill::initialize())return false;skill_tree_policy::Summary summary;
    void* card=local_career::skill_card(id);if(!card||*(int*)((char*)card+0x38)!=effect||!local_skill::graph.summarize(state,summary))return false;
    level=summary.level[id];return level>=0&&level<=11;
}
static bool pvp_level(const local_progress::State& state,int& level){return credit_level(state,37,50,level);}
static bool cooperative_level(const local_progress::State& state,int& level){return cooperative_credit_installed&&credit_level(state,25,100,level);}
static void install(){local_career::box_credit_bonus=box_credits;installed=true;}
}
