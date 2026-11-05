#include <assert.h>
#include <stdio.h>
#include <initializer_list>
#include "building_research_effects.h"
using namespace research_effects;
int main(){
    BuildingStats a,b,c;assert(building_parse("{#20041<1>}",a)&&a.flat_power&&a.power[0]==1);
    assert(building_parse("{#20047<1>}",b)&&b.training==1);
    assert(building_add(a,b,c)&&c.power[0]==1&&c.training==1);
    assert(building_parse("{#20079<2>}\n{#20080<3>}\n{#20081<4>}\n{#20082<5>}\n{#20095<7>}",c)&&c.power_mask==31);
    for(const char* bad:{"{#20047<100>}","{#20047<1>} {#20048<2>}","{#20079<1>}","{#20041<1>} {#20079<1>}",
        "{#20041<2147483648>}","{#20041<-1>}","{#20042<3>}","{#20047<1>}?"}){
        auto old=c;assert(!building_parse(bad,c)&&building_equal(old,c));}
    assert(building_parse("{#20047<60>}",a)&&building_parse("{#20047<40>}",b));auto old=c;
    assert(!building_add(a,b,c)&&building_equal(old,c));int time=123;
    assert(training_time(160,1,time)&&time==158);assert(training_time(450,20,time)&&time==360);
    assert(!training_time(1,99,time)&&time==360);assert(!training_time(INT_MAX,100,time));
    assert(training_time(INT_MAX,1,time)&&time==2126008810);
    puts("PASS: power/training parsing, cumulative limits, atomic refusal and integer time bounds");
}
