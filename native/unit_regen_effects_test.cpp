#include "unit_stat_effects.h"
#include "../tests/fixtures/unit_regen_fixture.h"
#include <cassert>
#include <cstring>
#include <climits>
#include <cstdio>
#include <string>
int main(){
    using namespace research_effects;int count=0;
    for(const auto& row:unit_regen_rows){UnitStats d;int unknown=0;
        assert(unit_stats_parse(row.text,d,unknown)==UnitParse::ok);
        assert(d.hp==row.hp&&d.armor==row.armor&&d.speed==row.speed&&d.sight==row.sight&&d.regen==row.regen);
        for(size_t n=0;n<std::strlen(row.text);++n){UnitStats partial;partial.hp=77;
            std::string prefix(row.text,n);if(unit_stats_parse(prefix.c_str(),partial,unknown)!=UnitParse::ok)assert(partial.hp==77);}
        ++count;
    }
    assert(count==486);
    for(const char* text:{"{#20016<0>}","{#20017<-1>}","{#20016<1,2>}","{#20016<2147483648>}",
                         "{#20016<1>} {#20017<2>}","{#20017<1>} {#20017<2>}","{#20016<2>} {#20099<1>}"}){
        UnitStats d;d.regen=77;auto before=d;int unknown=0;
        assert(unit_stats_parse(text,d,unknown)!=UnitParse::ok&&!memcmp(&d,&before,sizeof(d)));
    }
    UnitStats a,b,c;a.regen=INT_MAX;b.regen=1;c.hp=77;
    assert(!rifle_add(a,b,c)&&c.hp==77&&!c.regen);
    a.regen=100;b.regen=27;assert(rifle_add(a,b,c)&&c.regen==127&&rifle_any(c));
    a=c;assert(rifle_equal(a,c));++a.regen;assert(!rifle_equal(a,c));
    puts("PASS: 486 original repair/recovery rows, independent integer oracle, mixed flat stats, duplicate/unknown/precision/overflow refusal, atomic accumulation");
}
