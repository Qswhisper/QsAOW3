// Original WeaponStatic/DynamicAccuracy and ShotAct consume two different
// representations: hit probabilities, or additive ballistic dispersion.
#pragma once
#include <stdint.h>
#include <limits.h>
namespace research_effects {
inline bool probability_weapon(int type,int guided){return type==10||type==40||guided>0;}
inline bool dispersion_weapon(int type,int guided){return !guided&&(type==20||type==21||type==23||type==30);}
// Order: static, firing while walking, moving-target lead. Upgrade descriptions
// contain positive improvements. Smaller ballistic static/walk values reduce
// spread; lead remains an increasing percentage. Never clamp a whole level.
inline bool weapon_accuracy_apply(int type,int guided,int walking,const int (&base)[3],
                                  const int (&improvement)[3],int (&output)[3],const char*& reason){
    bool probability=probability_weapon(type,guided);
    if(!probability&&!dispersion_weapon(type,guided)){reason="accuracy_weapon_formula_unverified";return false;}
    if((improvement[1]&&(guided||!walking))||(improvement[2]&&guided)){
        reason="accuracy_inactive_field_or_guided_lead_unverified";return false;}
    int next[3];
    for(int k=0;k<3;++k){
        if(base[k]<0||base[k]>INT16_MAX||improvement[k]<0||improvement[k]>100){reason="accuracy_field_or_delta_bounds";return false;}
        next[k]=base[k];if(!improvement[k])continue;
        int64_t value=int64_t(base[k])+((probability||k==2)?improvement[k]:-improvement[k]);
        int maximum=(probability||k==2)?100:INT16_MAX;
        if(value<0||value>maximum){reason=probability||k==2?"accuracy_probability_bounds":"accuracy_dispersion_underflow";return false;}
        next[k]=int(value);
    }
    for(int k=0;k<3;++k)output[k]=next[k];return true;
}
}
