#include "hero_ability_effects.h"
#include "../tests/fixtures/atlant_class_fixture.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <string>
static void equal(const hero_effects::Delta& d,const int* v){
    assert(d.cooldown[0]==v[0]&&d.cooldown[1]==v[1]);
    for(int i=0;i<3;++i)assert(d.damage[i]==v[i+2]);
    assert(d.explosion==v[5]&&d.range==v[6]&&d.shot_start==v[7]&&d.duration==v[8]);
    assert(d.passive_absorption==v[9]&&d.shield_half_angle==v[10]);
    assert(!d.reduction&&!d.passive_radius&&!d.salamander_speed&&!d.passive_recovery&&!d.chain_distance&&!d.chain_loss_reduction);
}
int main(){
    assert(hero_effects::supported(75));hero_effects::Delta total;int n=0;
    for(const auto& row:atlant_rows){hero_effects::Delta d;assert(hero_effects::parse(row.description,d,75));equal(d,row.value);assert(hero_effects::add(total,d));++n;
        hero_effects::Delta guard;guard.range=77;auto before=guard;
        for(int other:{70,71,72,73,74,170,171,172,173,174,175})assert(!hero_effects::parse(row.description,guard,other)&&!memcmp(&guard,&before,sizeof(before)));
        for(const char* tail:{"{#20365<1>}","{#20364<0,1>}","{#99999<1>}"}){std::string bad=std::string(row.description)+tail;assert(!hero_effects::parse(bad.c_str(),guard,75)&&!memcmp(&guard,&before,sizeof(before)));}
        std::string missing=row.description;auto p=missing.find("{#20365<");missing.erase(p,missing.find('}',p)-p+1);
        assert(!hero_effects::parse(missing.c_str(),guard,75)&&!memcmp(&guard,&before,sizeof(before)));
    }
    assert(n==11);equal(total,atlant_totals);int angle=-1;
    assert(hero_effects::shield_angle(75,total,angle)&&angle*2==170);
    auto invalid=total;invalid.shield_half_angle=106;angle=-1;assert(!hero_effects::shield_angle(75,invalid,angle)&&angle==-1);
    invalid.shield_half_angle=-1;assert(!hero_effects::shield_angle(75,invalid,angle));
    auto text=std::string(atlant_rows[0].description);auto p=text.find("{#20368<");text.replace(p,text.find('}',p)-p+1,"{#20368<1>}");
    assert(!hero_effects::parse(text.c_str(),invalid,75)); // Half-degree raw field is an integer.
    invalid.passive_absorption=INT_MAX;auto before=invalid;hero_effects::Delta one;one.passive_absorption=1;
    assert(!hero_effects::add(invalid,one)&&!memcmp(&invalid,&before,sizeof(before)));
    printf("PASS: 11 original Atlant rows, independent Decimal points and doubled display angle150->170, absorption400->%d, full-row/precision/overflow/isolation checks\n",400+total.passive_absorption);
}
