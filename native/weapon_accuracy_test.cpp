#include "weapon_research_effects.h"
#include "weapon_accuracy_cases.generated.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <initializer_list>
int main(){
    int checked=0;for(const auto& row:accuracy_cases){research_effects::WeaponParameters p;assert(research_effects::weapon_parameters_parse(row.text,p));assert(p.accuracy[0]||p.accuracy[1]||p.accuracy[2]);++checked;}
    for(const char* text:{"{#20008<0>}","{#20009<-1>}","{#20012<101>}","{#20008<1>} {#20008<2>}","{#20012<1>} {#20092<1>}","{#20009<0,5>}"}){
        research_effects::WeaponParameters p;p.accuracy[0]=77;auto before=p;assert(!research_effects::weapon_parameters_parse(text,p)&&!memcmp(&p,&before,sizeof(p)));}
    assert(research_effects::probability_weapon(10,0)&&research_effects::probability_weapon(40,0)&&research_effects::probability_weapon(30,1));
    assert(!research_effects::probability_weapon(20,0)&&!research_effects::probability_weapon(27,0));
    printf("PASS: %d original whole accuracy candidates; static/walk/lead token identity, integer points, mixed range/reload parsing, duplicate/unknown/zero/overflow/precision atomic refusals; probability and dispersion paths distinguished\n",checked);
}
