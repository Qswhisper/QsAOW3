#include "hero_ability_effects.h"
#include "../tests/fixtures/hero_class_fixture.h"
#include <cassert>
#include <cstring>
#include <cstdio>
#include <string>
int main(){
    assert(hero_effects::supported(173)&&hero_effects::parsable(173));
    hero_effects::Delta total,seraphim,cerberus,wasp,beholder,gatling,solaris;int count=0;
    for(const auto& row:hero_class_rows){hero_effects::Delta d;
        assert(hero_effects::parse(row.description,d,row.hero));
        assert(d.cooldown[0]==row.cooldown_active&&d.cooldown[1]==row.cooldown_passive);
        for(int j=0;j<3;++j)assert(d.damage[j]==row.damage[j]);
        assert(d.explosion==row.radius&&d.range==row.range&&d.shot_start==row.shot&&d.duration==row.duration&&d.reduction==row.reduction);
        assert(d.active_duration==row.active_duration&&d.active_effect==row.active_effect&&d.passive_radius==row.passive_radius);
        for(int j=0;j<3;++j)assert(d.passive_weapon.damage[j]==row.passive_damage[j]);
        assert(d.passive_weapon.explosion==row.passive_explosion&&d.passive_weapon.range==row.passive_range);
        assert(d.aura_duration==row.aura_duration&&d.aura_radius==row.aura_radius&&d.aura_effect==row.aura_effect);
        assert(d.chain_distance==row.chain_distance&&d.chain_loss_reduction==row.chain_loss_reduction);
        assert(hero_effects::add(row.hero==72?solaris:(row.hero==173?gatling:(row.hero==172?total:(row.hero==71?seraphim:(row.hero==70?cerberus:(row.hero==73?beholder:wasp))))),d));++count;
    }
    assert(count==77&&total.cooldown[0]==200&&total.cooldown[1]==100&&total.range==300&&total.reduction==25&&total.duration==15);
    assert(solaris.chain_distance==200&&solaris.chain_loss_reduction==20&&solaris.passive_weapon.damage[0]>0);
    int distance=-1,loss=-1;assert(hero_effects::chain_parameters(300,30,solaris,distance,loss)&&distance==500&&loss==10);
    distance=loss=-1;auto invalid=solaris;invalid.chain_loss_reduction=31;
    assert(!hero_effects::chain_parameters(300,30,invalid,distance,loss)&&distance==-1&&loss==-1);
    invalid=solaris;invalid.chain_distance=INT_MAX;assert(!hero_effects::chain_parameters(300,30,invalid,distance,loss));
    for(const auto& row:hero_class_rows)if(row.hero==72){
        hero_effects::Delta value;value.range=77;auto saved=value;
        for(int other:{70,71,73,170,172,173})assert(!hero_effects::parse(row.description,value,other)&&!std::memcmp(&value,&saved,sizeof(value)));
        std::string duplicate=std::string(row.description)+"{#20347<4>}";
        assert(!hero_effects::parse(duplicate.c_str(),value,72)&&!std::memcmp(&value,&saved,sizeof(value)));
        std::string missing=row.description;auto at=missing.find("{#2034",missing.find('|'));missing.erase(at,missing.find('}',at)-at+1);
        assert(!hero_effects::parse(missing.c_str(),value,72)&&!std::memcmp(&value,&saved,sizeof(value)));
    }
    assert(gatling.passive_weapon.damage[0]>0&&gatling.passive_weapon.range>0&&gatling.aura_duration>0&&gatling.aura_effect>0&&gatling.aura_radius>0);
    assert(!gatling.duration&&!gatling.reduction&&!gatling.passive_radius);
    for(const auto& row:hero_class_rows)if(row.hero==173){
        hero_effects::Delta d;d.range=77;auto before=d;
        std::string duplicate=std::string(row.description)+"{#20341<1>}";
        assert(!hero_effects::parse(duplicate.c_str(),d,173)&&!std::memcmp(&d,&before,sizeof(d)));
        std::string missing=row.description;auto at=missing.find("{#20335<");missing.erase(at,missing.find('}',at)-at+1);
        assert(!hero_effects::parse(missing.c_str(),d,173)&&!std::memcmp(&d,&before,sizeof(d)));
        assert(!hero_effects::parse(row.description,d,172));
    }
    assert(beholder.explosion>0&&beholder.active_duration>0&&beholder.active_effect>0&&beholder.reduction>0&&!beholder.passive_radius&&!beholder.damage[0]);
    for(const auto& row:hero_class_rows)if(row.hero==73){
        hero_effects::Delta value;value.range=77;auto before=value;
        for(int other:{70,71,170,172})assert(!hero_effects::parse(row.description,value,other)&&!std::memcmp(&value,&before,sizeof(value)));
        std::string wrong=row.description;wrong+="{#20004<1>}";
        assert(!hero_effects::parse(wrong.c_str(),value,73)&&!std::memcmp(&value,&before,sizeof(value)));
        wrong=row.description;wrong.insert(0,"{#20010<0,1>}");
        assert(!hero_effects::parse(wrong.c_str(),value,73)&&!std::memcmp(&value,&before,sizeof(value)));
    }
    assert(wasp.active_duration>0&&wasp.active_effect==10&&wasp.passive_radius>0&&wasp.reduction==45);
    for(const auto& row:hero_class_rows)if(row.hero==170){
        hero_effects::Delta value;value.range=77;auto before=value;
        assert(!hero_effects::parse(row.description,value,172)&&!std::memcmp(&value,&before,sizeof(value)));
        std::string wrong=row.description;wrong+="{#20337<1>}";
        assert(!hero_effects::parse(wrong.c_str(),value,170)&&!std::memcmp(&value,&before,sizeof(value)));
    }
    assert(seraphim.reduction==0&&seraphim.cooldown[0]>0&&seraphim.cooldown[1]>0);
    assert(cerberus.reduction>0&&cerberus.shot_start==0&&cerberus.range>0);
    for(const auto& row:hero_class_rows)if(row.hero==71){
        hero_effects::Delta value;
        std::string unknown=std::string(row.description)+"{#20337<1>}";
        assert(!hero_effects::parse(unknown.c_str(),value,71));
        assert(!hero_effects::parse(row.description,value,172));
        assert(!hero_effects::parse(row.description,value,70));
        auto cut=std::string(row.description);cut.erase(cut.find('|'));
        assert(!hero_effects::parse(cut.c_str(),value,71));
    }
    for(const auto& row:hero_class_rows)if(row.hero==70){
        hero_effects::Delta value;
        assert(!hero_effects::parse(row.description,value,71));
        assert(!hero_effects::parse(row.description,value,172));
        auto corrupt=std::string(row.description);corrupt.insert(corrupt.find('|'),"{#20349<0,1>}");
        assert(!hero_effects::parse(corrupt.c_str(),value,70));
    }
    const char* original=hero_class_rows[0].description;
    for(size_t n=0;n<std::strlen(original);++n){hero_effects::Delta d;d.range=77;auto before=d;
        std::string prefix(original,n);assert(!hero_effects::parse(prefix.c_str(),d)&&!std::memcmp(&d,&before,sizeof(d)));}
    for(const char* bad:{"{#99999<1>}","{#20330<0,01>}","{#20330<9999999999999999999999999>}","|","{#20330<1,234>}"}){
        hero_effects::Delta d;assert(!hero_effects::parse(bad,d));}
    std::string duplicate=std::string("{#20330<1>}\n")+original;hero_effects::Delta d;assert(!hero_effects::parse(duplicate.c_str(),d));
    d.cooldown[0]=INT_MAX;auto before=d;hero_effects::Delta one;one.cooldown[0]=1;
    assert(!hero_effects::add(d,one)&&!std::memcmp(&d,&before,sizeof(d)));
    puts("PASS: 77 original hero class rows, independent decimal oracle, Solaris chain parameter bounds, dual ability weapons and independent smoke aura, legacy six routes, unsupported/cross-hero refusal, whole-row rollback and overflow");
}
