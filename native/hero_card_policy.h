#pragma once
#include <stdint.h>
#include <limits.h>
namespace hero_card_policy {
struct Candidate {int id,faction,rarity,rank_from,rank_to;};
struct Grant {int id,amount;};
inline uint64_t draw(uint64_t& state){state+=0x9e3779b97f4a7c15ULL;uint64_t z=state;z=(z^(z>>30))*0xbf58476d1ce4e5b9ULL;z=(z^(z>>27))*0x94d049bb133111ebULL;return z^(z>>31);}
// Offline hero draw rule; official server weights are unknown.
// Receipt identity determines the draw stream; retrying cannot reroll a grant.
inline bool distribute(const Candidate* cards,int count,int faction,int rank,int rarity,int amount,uint64_t receipt,Grant* out,int capacity,int& size){
    size=0;if(!cards||!out||count<1||count>128||capacity<1||capacity>128||faction<1||faction>2||rank<1||rarity<1||rarity>3||amount<=0||amount>1000000||!receipt)return false;
    int ids[128],n=0;
    for(int i=0;i<count;++i){const auto& c=cards[i];if(c.id<0||c.id>=200||c.faction!=faction||c.rarity!=rarity||c.rank_from>rank||(c.rank_to>0&&rank>c.rank_to))continue;
        for(int j=0;j<n;++j)if(ids[j]==c.id)return false;
        int j=n++;while(j&&ids[j-1]>c.id){ids[j]=ids[j-1];--j;}ids[j]=c.id;}
    if(!n||n>capacity)return false;
    for(int i=0;i<n;++i)out[i]={ids[i],0};
    uint64_t state=receipt^(uint64_t(faction)<<48)^(uint64_t(rarity)<<40);uint64_t threshold=-uint64_t(n)%uint64_t(n);
    for(int i=0;i<amount;++i){uint64_t value;do{value=draw(state);}while(value<threshold);++out[value%uint64_t(n)].amount;}
    for(int i=0;i<n;++i)if(out[i].amount)out[size++]=out[i];
    return true;
}
}
