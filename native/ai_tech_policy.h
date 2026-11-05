// Select the next observed ground-production upgrade. Native validators still
// own every contribution, price, level and duration; no free tech or units.
#pragma once
namespace ai_tech_policy {
enum Upgrade {None, Headquarters, Factory};
inline bool extended(bool player_match,bool private_fixture){return player_match||private_fixture;}
inline bool recruit(bool expanded_army,bool defending,int troops,int new_unit_price,int gold){
    return expanded_army||defending||troops<3||new_unit_price<=0||gold>=new_unit_price;
}
inline Upgrade next(int hq_level,bool factory_ready,int factory_level,int troops,bool defending,bool expanded_army=false){
    if(!factory_ready||troops<3||defending)return None;
    // Try the cheaper production tier before saving for another HQ tier.
    // The original command can refuse an unmet contribution without spending.
    if(hq_level>=1&&factory_level==0)return Factory;
    if(hq_level==1&&factory_level>=1&&expanded_army)return Headquarters;
    if(hq_level>=2&&factory_level==1&&expanded_army)return Factory;
    return None;
}
inline int profile(bool extended_scope,int hq_level,int factory_level){
    // Select an existing native weight column, never manufacture unit weights.
    return extended_scope&&hq_level>=2&&factory_level>=2?3:1;
}
inline bool air(bool extended_scope,int hq_level,bool factory_ready,int factory_level,int ground_tier,int troops,bool defending){
    return extended_scope&&hq_level>=2&&factory_ready&&factory_level>=2&&ground_tier>=2&&troops>=3&&!defending;
}
}
