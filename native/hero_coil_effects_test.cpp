#include "hero_ability_effects.h"
#include "../tests/fixtures/coil_class_fixture.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <string>
static void equal(const hero_effects::Delta& d,const int* v){
    assert(d.cooldown[0]==v[0]&&d.cooldown[1]==v[1]);
    for(int i=0;i<3;++i)assert(d.damage[i]==v[i+2]&&d.passive_weapon.damage[i]==v[i+10]);
    assert(d.explosion==v[5]&&d.range==v[6]&&d.shot_start==v[7]);
    assert(d.active_duration==v[8]&&d.active_effect==v[9]&&d.passive_weapon.explosion==v[13]&&d.passive_stun==v[14]);
    assert(!d.reduction&&!d.duration&&!d.passive_radius&&!d.passive_recovery&&!d.salamander_speed&&!d.shield_half_angle&&!d.passive_absorption);
}
int main(){
    assert(hero_effects::supported(175)&&!hero_effects::supported(74));hero_effects::Delta total;int n=0;
    for(const auto& row:coil_rows){hero_effects::Delta d;assert(hero_effects::parse(row.description,d,175));equal(d,row.value);assert(hero_effects::add(total,d));++n;
        hero_effects::Delta sentinel;sentinel.range=77;auto before=sentinel;
        for(int hero:{70,71,72,73,74,75,170,171,172,173,174})assert(!hero_effects::parse(row.description,sentinel,hero)&&!memcmp(&sentinel,&before,sizeof(before)));
        for(const char* suffix:{"{#20004<1>}","{#20364<1>}","{#20367<1>}","{#99999<1>}"}){
            auto bad=std::string(row.description)+suffix;assert(!hero_effects::parse(bad.c_str(),sentinel,175)&&!memcmp(&sentinel,&before,sizeof(before)));
        }
        std::string missing=row.description;auto at=missing.find("{#20004<");missing.erase(at,missing.find('}',at)-at+1);
        assert(!hero_effects::parse(missing.c_str(),sentinel,175)&&!memcmp(&sentinel,&before,sizeof(before)));
    }
    assert(n==11);equal(total,coil_totals);assert(60+total.active_duration==100&&40+total.active_effect==70&&20+total.passive_stun==35);
    auto text=std::string(coil_rows[0].description);auto at=text.find("{#20366<");text.replace(at,text.find('}',at)-at+1,"{#20366<0,01>}");
    hero_effects::Delta d;assert(!hero_effects::parse(text.c_str(),d,175));
    d.passive_stun=INT_MAX;auto before=d;hero_effects::Delta one;one.passive_stun=1;
    assert(!hero_effects::add(d,one)&&!memcmp(&d,&before,sizeof(before)));
    puts("PASS: 11 original Coil Tank rows, independent Decimal tick/percent units, building stun60->100, energy40->70, passive stun20->35, complete dual-weapon rows, cross-hero/precision/overflow/rollback");
}
