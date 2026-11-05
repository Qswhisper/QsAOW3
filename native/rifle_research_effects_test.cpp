#include "rifle_research_effects.h"
#include <assert.h>
#include <stdio.h>
#include <string>
#include <fstream>
#include <sstream>
int main(int argc,char** argv) {
    using namespace research_effects;
    RifleDelta d;
    assert(rifle_parse("{#20000<20>}\n{#20014<1>}",0,d) && d.hp==20 && d.speed==1);
    assert(rifle_parse("{#20001<1>}",1,d) && d.armor==1 && !d.hp);
    assert(rifle_parse("{#20004<2>} {#20005<1>} {#20006<1>}",3,d) && d.damage[0][0]==2 && d.damage[0][1]==1 && !d.damage[1][0]);
    assert(rifle_parse("{#20004<10>}?{#20005<5>}?{#20006<4>}",4,d) && d.damage[1][0]==10 && d.damage[1][2]==4 && !d.damage[0][0]);
    assert(rifle_parse("{#20000<16>}\r\n{#20034<1>}",5,d) && d.hp==16 && d.sight==1);
    for(const char* bad:{"","{","{#20000<0>}","{#20000<-1>}","{#20000<2147483648>}","{#20000<1>} ","{#20000<1>} {#20000<2>}","{#20000<1>} {#20017<2>}"}) {
        RifleDelta before; before.hp=77;
        assert(!rifle_parse(bad,0,before) && before.hp==77);
    }
    assert(!rifle_parse("{#20004<2>}",3,d));
    assert(!rifle_parse("{#20008<1>}",2,d));
    assert(!rifle_parse("{#20001<1>}",0,d));
    RifleDelta x,y,z; x.hp=INT_MAX;y.hp=1;z.hp=77;
    assert(!rifle_add(x,y,z) && z.hp==77);
    assert(argc==2); std::ifstream input(argv[1]); assert(input.good());
    std::string line;int rows=0,supported=0;
    while(std::getline(input,line)) {
        if(!line.empty() && line.back()=='\r') line.pop_back();
        std::istringstream row(line);int id,slot,expected;
        assert(bool(row>>id>>slot>>expected)); std::string desc;std::getline(row,desc);desc.erase(0,1);
        bool parsed=rifle_parse(desc.c_str(),slot,d);
        if(parsed!=bool(expected)) {fprintf(stderr,"catalog mismatch id=%d slot=%d parsed=%d expected=%d\n",id,slot,parsed,expected);return 1;}
        ++rows;supported+=parsed;
    }
    assert(rows==105 && supported==64);
    printf("Rifle family: %d original levels audited; %d supported / %d safely refused; mixed effects, routing and invalid-input checks passed\n",rows,supported,rows-supported);
}
