#include "hero_ability_effects.h"
#include "../tests/fixtures/salamander_class_fixture.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <string>
static void equal(const hero_effects::Delta& d,const int* v){
    assert(d.cooldown[0]==v[0]&&d.cooldown[1]==v[1]);
    for(int i=0;i<3;++i)assert(d.damage[i]==v[2+i]);
    assert(d.explosion==v[5]&&d.range==v[6]&&d.shot_start==v[7]&&d.duration==v[8]);
    assert(d.passive_recovery==v[9]&&d.salamander_speed==v[10]);
    assert(!d.reduction&&!d.active_duration&&!d.active_effect&&!d.passive_radius&&!d.chain_distance&&!d.chain_loss_reduction);
}
int main(){
    assert(hero_effects::supported(174));hero_effects::Delta total;int count=0;
    for(const auto& row:salamander_rows){
        hero_effects::Delta d;assert(hero_effects::parse(row.description,d,174));equal(d,row.value);
        assert(hero_effects::add(total,d));++count;
        hero_effects::Delta sentinel;sentinel.range=99;auto before=sentinel;
        for(int hero:{70,71,72,73,170,171,172,173,175})assert(!hero_effects::parse(row.description,sentinel,hero)&&!memcmp(&sentinel,&before,sizeof(before)));
        for(const char* suffix:{"{#20364<0,1>}","{#20337<1>}","{#99999<1>}"}){
            std::string bad=std::string(row.description)+suffix;
            assert(!hero_effects::parse(bad.c_str(),sentinel,174)&&!memcmp(&sentinel,&before,sizeof(before)));
        }
        std::string missing=row.description;auto p=missing.find("{#20364<");missing.erase(p,missing.find('}',p)-p+1);
        assert(!hero_effects::parse(missing.c_str(),sentinel,174)&&!memcmp(&sentinel,&before,sizeof(before)));
    }
    assert(count==11);equal(total,salamander_totals);
    int speed=-1;assert(hero_effects::salamander_speed(25,total,speed)&&speed==35);
    auto invalid=total;invalid.salamander_speed=75;speed=-1;
    assert(!hero_effects::salamander_speed(25,invalid,speed)&&speed==-1);
    invalid.salamander_speed=-1;assert(!hero_effects::salamander_speed(25,invalid,speed));
    invalid=total;invalid.passive_recovery=INT_MAX;auto before=invalid;hero_effects::Delta one;one.passive_recovery=1;
    assert(!hero_effects::add(invalid,one)&&!memcmp(&invalid,&before,sizeof(before)));
    std::string overflow=salamander_rows[0].description;auto p=overflow.find("{#20364<");overflow.replace(p,overflow.find('}',p)-p+1,"{#20364<21474836>}");
    assert(!hero_effects::parse(overflow.c_str(),invalid,174)&&!memcmp(&invalid,&before,sizeof(before)));
    printf("PASS: 11 original Salamander rows, independent Decimal x600 recovery, shared speed25->35, cumulative effect=%d, full-row/cross-hero/overflow/rollback checks\n",2400+total.passive_recovery);
}
