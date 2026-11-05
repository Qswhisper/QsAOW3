#include "weapon_research_effects.h"
#include "../tests/fixtures/weapon_reload_fixture.h"
#include <cassert>
#include <cstdio>
int main(){
    int count=0;
    for(const auto& row:reload_rows){research_effects::WeaponParameters p;
        assert(research_effects::weapon_parameters_parse(row.text,p));
        assert(p.reload==row.ticks&&p.range==row.range&&p.explosion==row.radius);
        for(int i=0;i<3;++i)assert(p.damage[i]==row.damage[i]);++count;
    }
    assert(count==146);printf("PASS: %d whole original reload rows, independent Decimal ticks/damage/range/radius oracle\n",count);
}
