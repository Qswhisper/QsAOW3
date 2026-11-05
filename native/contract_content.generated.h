// Filtered original definitions; checked-in data described in docs/CONTENT.md.
#pragma once
namespace contract_content {
struct Reward {int type,id,amount;};
struct Definition {int id,faction,type,subtype,flags,rank_min,rank_max,value,param,xp,points,category,difficulty,level_min,level_max,probability,ads_inc,kind,reward_count;Reward rewards[8];const char* description;const char* icon;const char* background;int location_count;const char* location[8];int param_count;int params[8];};
static const Definition definitions[]={
{20000,0,2,0,2,4,6,1,0,0,5,1,1,0,0,100,0,0,1,{{0,0,300}},"{#4268<1>}","achiev_90","",2,{"battle","pvp"},0,{}},
{20010,0,4,1,2,4,6,2,1,0,15,2,1,0,0,100,1,3,1,{{0,0,1000}},"{#4503<2>}","achiev_91","",2,{"battle","pvp"},0,{}},
{20020,0,14,0,2,4,6,3,4,0,10,3,1,0,0,100,1,4,1,{{0,0,600}},"{#4269<3>}","achiev_28","",1,{"shop"},0,{}},
{20080,0,16,25,2,4,6,1,0,0,20,104,1,0,0,100,1,6,1,{{0,0,1200}},"{#4273<1><25>}","achiev_37","",2,{"battle","pvp"},0,{}},
{20060,0,0,0,2,4,6,1,0,0,15,105,1,0,0,100,1,1,1,{{0,0,1200}},"{#4271<1>}","achiev_97","",1,{"upgrade"},0,{}},
{20040,0,5,0,2,4,6,1,0,0,10,106,1,0,0,100,1,2,1,{{0,0,1000}},"{#4261<1>}","achiev_41","",2,{"quest","1"},0,{}},
{20090,0,3,70,3,4,6,1,0,0,15,7,1,0,0,100,1,9,1,{{0,0,1000}},"{#4274<1><70>}","achiev_37","",2,{"battle","pvp"},0,{}},
{21290,0,15,21,1,4,6,1,0,0,15,8,1,0,0,100,1,7,1,{{0,0,1200}},"{#4556<6><4>}","achiev_91","",2,{"battle","pvp"},2,{6,4}},
{20490,0,9,0,1,4,6,1,105,0,10,9,1,0,0,100,2,8,1,{{0,0,300}},"{#4318<1>}","achiev_98","",2,{"battle","pvp"},0,{}},
{20670,0,12,50,1,4,6,4,0,0,20,10,1,0,0,100,1,5,1,{{0,0,1000}},"{#4338<4>}","achiev_16","",2,{"battle","pvp"},0,{}},
{20900,1,12,16,1,4,6,4,0,0,15,11,1,0,0,100,2,5,1,{{0,0,1000}},"{#4361<4>}","achiev_19","",2,{"battle","pvp"},0,{}},
{20460,0,9,0,1,4,6,1,85,0,10,12,1,0,0,100,2,8,1,{{0,0,600}},"{#4315<1>}","achiev_98","",2,{"battle","pvp"},0,{}},
{20880,1,12,68,0,4,6,3,0,0,10,13,1,0,0,100,1,5,1,{{0,0,600}},"{#4359<3>}","achiev_2","",2,{"battle","pvp"},0,{}},
{21020,1,12,64,1,4,6,2,0,0,15,14,1,0,0,100,1,5,1,{{0,0,1000}},"{#4373<2>}","achiev_1","",2,{"battle","pvp"},0,{}},
{20760,0,12,27,0,4,6,10,0,0,15,15,1,0,0,100,4,5,1,{{0,0,600}},"{#4346<10>}","achiev_2","",2,{"battle","pvp"},0,{}},
};
struct BattleRule {int id,subtype,flags,value,faction,rank_min,rank_max;};
static const BattleRule battle_rules[]={
{20670,50,1,4,0,4,6},
{20760,27,0,10,0,4,6},
{20880,68,0,3,1,4,6},
{20900,16,1,4,1,4,6},
{21020,64,1,2,1,4,6},
};
using ResultRule=BattleRule;
static const ResultRule result_rules[]={
{20080,25,2,1,0,4,6},
};
struct TimedRule {int id,subtype,flags,value,faction,rank_min,rank_max,minutes,buildings;};
static const TimedRule timed_rules[]={
{21290,21,1,1,0,4,6,6,4},
};
struct BoostRule {int id,boost,value,flags,faction,rank_min,rank_max;};
static const BoostRule boost_rules[]={
{20460,85,1,1,0,4,6},
{20490,105,1,1,0,4,6},
};
static const ResultRule activity_rules[]={
{20090,70,3,1,0,4,6},
};
}
