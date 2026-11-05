#include "weapon_group_effects.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <initializer_list>
using namespace research_effects;
int main(){
    WeaponGroups g;
    // Retained alliance rifle40/41/42/45/85: independent expected effects.
    assert(weapon_groups_parse("{#20053<2>}",g)&&g.used[0]&&!g.used[1]&&g.weapons[0].accuracy[0]==2);
    assert(weapon_groups_parse("{#20012<1>}\n{#20092<0,5>}",g)&&g.used[0]&&!g.used[1]&&g.weapons[0].range==50&&g.weapons[0].accuracy[2]==1);
    assert(weapon_groups_parse("{#20009<2>}\n{#20093<0,5>}",g)&&!g.used[0]&&g.used[1]&&g.weapons[1].range==50&&g.weapons[1].accuracy[1]==2);
    assert(weapon_groups_parse("{#20053<3>}\n{#20054<12>}",g)&&g.used[0]&&g.used[1]&&g.weapons[0].accuracy[0]==3&&g.weapons[1].accuracy[0]==12);
    assert(weapon_groups_parse("{#20053<1>}\n{#20054<3>}\n{#20000<9>}\n{#20001<1>}",g)&&g.scalar.hp==9&&g.scalar.armor==1&&g.weapons[0].accuracy[0]==1&&g.weapons[1].accuracy[0]==3);
    assert(weapon_groups_parse("{#20053<2>}\n{#20012<1>}\n{#20009<3>}\n{#20054<4>}\n{#20012<2>}\n{#20009<5>}",g)&&g.weapons[0].accuracy[2]==1&&g.weapons[1].accuracy[2]==2&&g.weapons[0].accuracy[1]==3&&g.weapons[1].accuracy[1]==5);
    assert(weapon_groups_parse("{#20009<2>}\n{#20093<0,5>}\n{#20011<0,1>}",g)&&g.weapons[1].reload==1);
    for(const char* bad:{"", "{#20008<1>}","{#20053<1>}\n{#20013<1>}","{#20053<1>}\n{#20054<2>}\n{#20011<0,1>}",
        "{#20012<1>}\n{#20053<1>}\n{#20054<2>}","{#20053<1>}\n{#20053<2>}","{#20054<1,5>}","{#20092<0,001>}",
        "{#20053<101>}","{#20053<0>}","{#20053<1>} ","{#20053<1>}\n{#20054<2>}\n{#20024<1>}"}){
        WeaponGroups before;before.scalar.hp=777;auto expected=before;
        assert(!weapon_groups_parse(bad,before)&&!memcmp(&before,&expected,sizeof(before)));
    }
    // The two original weapon types use different representations. One group
    // must not apply the other's sign/scale or consume its accuracy budget.
    const char* why=nullptr;int rifle[]={53,73,50},grenade[]={50,30,0},delta_a[]={3,0,0},delta_b[]={12,0,0},a[3],b[3];
    assert(weapon_accuracy_apply(10,0,1,rifle,delta_a,a,why)&&a[0]==56&&a[1]==73);
    assert(weapon_accuracy_apply(20,0,1,grenade,delta_b,b,why)&&b[0]==38&&b[1]==30);
    puts("PASS: original explicit I/II rifle rows, mixed scalar effects, separate probability/dispersion fields, label scope, comma units and atomic whole-row refusals");
}
