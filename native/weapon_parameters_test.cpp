#include "weapon_research_effects.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <initializer_list>
int main(){
    using namespace research_effects;WeaponParameters result;
    assert(weapon_parameters_parse("{#20004<10>}\n{#20005<4>}\n{#20006<3>}\n{#20010<0,05>}",result));
    assert(result.damage[0]==10&&result.damage[1]==4&&result.damage[2]==3&&result.explosion==5&&!result.range);
    assert(weapon_parameters_parse("{#20007<0,5>} {#20010<0,01>}",result)&&result.range==50&&result.explosion==1);
    assert(weapon_parameters_parse("{#20007<0,25>}",result)&&result.range==25);
    assert(weapon_parameters_parse("{#20004<2147483647>}",result)&&result.damage[0]==INT_MAX);
    assert(weapon_parameters_parse("{#20011<0,1>}",result)&&result.reload==1&&!result.range&&!result.explosion);
    assert(weapon_parameters_parse("{#20004<50>} {#20011<0,2>} {#20007<0,25>}",result)&&result.damage[0]==50&&result.reload==2&&result.range==25);
    assert(weapon_parameters_parse("{#20011<1>} {#20019<0,1>}",result)&&result.reload==10&&result.shot_start==1);
    for(const char* bad:{"{#20011<0,01>}","{#20011<0>}","{#20011<-1>}","{#20011<1>} {#20011<1>}","{#20011<1>} {#20024<1>}"}){
        WeaponParameters before;before.reload=77;auto same=before;
        assert(!weapon_parameters_parse(bad,before)&&!memcmp(&before,&same,sizeof(before)));
    }
    assert(weapon_parameters_parse("{#20007<0,5>} {#20008<1>} {#20009<2>} {#20012<3>}",result)&&result.range==50&&result.accuracy[0]==1&&result.accuracy[1]==2&&result.accuracy[2]==3);
    for(const char* bad:{"","{#20007<0>}","{#20007<-0,5>}","{#20007<0.5>}","{#20007<0,001>}","{#20007<1,>}","{#20007<2147483647>}","{#20004<1,5>}","{#20010<0,05>} ","{#20010<0,05>} {#20010<0,05>}","{#20008<101>}","{#20008<1>} {#20008<1>}","{#20009<0,5>}"}){
        WeaponParameters before;before.range=77;auto same=before;
        assert(!weapon_parameters_parse(bad,before)&&!memcmp(&before,&same,sizeof(before)));
    }
    puts("PASS: whole original damage/range/explosion/reload/accuracy tokens, exact comma hundredths and decisecond ticks, mixed levels, INT_MAX damage, duplicate/unknown/precision/zero/overflow atomic refusals");
}
