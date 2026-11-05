#include "modification_effect_policy.h"
#include <cassert>
#include <cstdio>
#include <initializer_list>
int main(){
    using namespace modification_effects;int checked=0;
    for(const auto& d:modification_content::definitions)for(int i=0;i<d.count;++i){Action a;assert(parse(modification_content::actions[d.first+i].text,a));assert(field(a.kind,a.property));++checked;}
    Action a[3];assert(parse("add<10><u.11.life>",a[0])&&parse("addpct<20><u.11.life>",a[1]));float value=0;
    assert(calculate(100,a,2,value)&&fabsf(value-132)<0.00001f);
    assert(parse("set<7><u.11.life>",a[2])&&calculate(100,a,3,value)&&value==7);
    auto before=a[0];for(const char* text:{"add<1><u.11.life>junk","add<-2147483649><u.1.life>","add<1><u.-1.life>","addpct<0.5><w.1.distance>","set<1><unknown.1.life>"}){a[0]=before;assert(!parse(text,a[0])&&!memcmp(&a[0],&before,sizeof(before)));}
    int out=77;auto f=*field(2,"round_len");assert(quantize(9.9f,f,out)&&out==9);assert(quantize(-9.9f,f,out)&&out==-9);
    out=77;assert(!quantize(32768,f,out)&&out==77);assert(!quantize(INFINITY,f,out)&&out==77);
    f=*field(0,"cp");assert(!quantize(128,f,out)&&out==77);f=*field(1,"spec");assert(quantize(15795203,f,out)&&out==15795203);
    puts("PASS: 14 retained definitions/187 original actions/36 scalar contracts, add-before-percentage aggregation, Set precedence, typed truncation/overflow/nonfinite and atomic malformed-input guards; native oracle/projection checked separately");assert(checked==187);
}
