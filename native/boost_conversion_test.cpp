#include "boost_conversion_policy.h"
#include <cassert>
#include <cstdio>
#include <initializer_list>
int main(int argc,char** argv){
    int cost=0;for(int level=0;level<=11;++level){assert(boost_conversion_policy::price(20,50,level,cost)&&cost==20-level);}
    assert(!boost_conversion_policy::price(0,50,1,cost)&&!boost_conversion_policy::price(20,50,12,cost));
    assert(!boost_conversion_policy::price(1,50,1,cost)); // refuse a zero-input conversion
    auto state=local_progress::seed(100,10,15,9);assert(local_progress::grant_stock(state,4,0,45));
    auto original=state;
    for(int input:{0,-1,19,21,60}){auto before=state;assert(!boost_conversion_policy::convert(state,0,1,input,20));assert(!memcmp(&before,&state,sizeof(state)));}
    assert(boost_conversion_policy::convert(state,0,1,40,20));
    assert(local_progress::stock_amount(state,4,0)==5&&local_progress::stock_amount(state,4,1)==2);
    assert(state.credits==100&&state.gold==10&&state.experience==15&&state.stars==9);
    auto before=state;assert(!boost_conversion_policy::convert(state,1,1,1,1)&&!memcmp(&before,&state,sizeof(state)));
    assert(!boost_conversion_policy::convert(state,0,1,20,20)&&!memcmp(&before,&state,sizeof(state)));
    assert(local_progress::grant_stock(state,4,1,INT_MAX-2));before=state;
    assert(!boost_conversion_policy::convert(state,0,1,1,1)&&!memcmp(&before,&state,sizeof(state)));
    auto discounted=original;assert(boost_conversion_policy::convert(discounted,0,1,38,19));
    assert(local_progress::stock_amount(discounted,4,0)==7&&local_progress::stock_amount(discounted,4,1)==2);
    if(argc>1){assert(local_progress::write(argv[1],discounted));local_progress::State read{};assert(local_progress::load(argv[1],read)==1&&!memcmp(&read,&discounted,sizeof(read)));}
    puts("PASS: original20-piece prices/12 skill levels, whole multiples, atomic no-op refusals, no currency/xp/star change, overflow refusal, independent stock/save");
}
