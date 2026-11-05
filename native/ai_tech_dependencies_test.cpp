#include "ai_tech_dependencies.h"
#include <stdio.h>
#include <assert.h>
#include <initializer_list>
using namespace ai_tech_dependencies;
int main(int argc,char** argv){
    assert(argc==2);FILE* f=fopen(argv[1],"r");assert(f);Graph graphs[2];int faction,type,tier,id,price,previous,count,total=0;
    while(fscanf(f,"%d%d%d%d%d%d%d",&faction,&type,&tier,&id,&price,&previous,&count)==7){
        assert(faction>=1&&faction<=2&&count>=0&&count<=16);Node n;n.type=type;n.tier=tier;n.price=price;n.previous=previous;n.count=count;
        for(int j=0;j<count;++j)assert(fscanf(f,"%d%d",&n.needs[j].key,&n.needs[j].count)==2);
        assert(graphs[faction-1].add(id,n));++total;
    }
    fclose(f);assert(total==120);int cases=0;
    for(int side=0;side<2;++side){auto& g=graphs[side];int base=1000+side*100;assert(g.size==60);Observation s;
        // Normal HQ recipes must not collapse onto the free scripted aliases
        // which intentionally share the engine's modulo counter index.
        auto first=g.choose(base+2,1,s);assert(first.status==Action&&g.nodes[g.find(first.key)].type==base&&g.nodes[g.find(first.key)].price==1000);++cases;
        int alias=1501+side*100;assert(index(alias)==index(base+1)&&g.nodes[g.find(alias)].price==0&&g.nodes[g.find(base+1)].type==base);++cases;
        for(int key:{0,1,10,15,18,20,30,31})s.completed[key]=1;
        auto r=g.choose(base+2,1,s);assert(r.status==Action&&g.nodes[g.find(r.key)].type==base+1&&g.nodes[g.find(r.key)].tier==0);++cases;
        s.pending[10]=1;r=g.choose(base+2,1,s);assert(r.status==Waiting);++cases;
        s.pending[10]=0;s.completed[10]=2;r=g.choose(base+2,1,s);assert(r.status==Action&&g.nodes[g.find(r.key)].type==base+2);++cases;
        s.pending[15]=1;assert(g.choose(base+2,1,s).status==Waiting);++cases;
        s.pending[15]=0;s.completed[15]=2;r=g.choose(base+2,1,s);assert(r.status==Action&&r.key==base+2&&g.nodes[g.find(r.key)].price==800);++cases;
        s.completed[2]=1;assert(g.choose(base+2,1,s).status==Satisfied);++cases;
        assert(g.choose(98777,1,s).status==Unsupported);++cases;
        s.completed[2]=-1;assert(g.choose(base+2,1,s).status==Unsupported);++cases;
    }
    Graph cyclic;Node a;a.type=1;a.count=1;a.needs[0]={6060,1};Node b=a;b.needs[0]={5050,1};assert(cyclic.add(5050,a)&&cyclic.add(6060,b));
    Observation empty;assert(cyclic.choose(5050,1,empty).status==Unsupported);++cases;
    empty.limit=50;assert(cyclic.choose(5050,1,empty).status==Unsupported);++cases;
    empty.limit=0;assert(cyclic.choose(5050,1,empty).status==Unsupported);++cases;
    printf("PASS: %d original tiers, %d dependency cases; both factions need second power and supply, pending work deduplicated, full recipe IDs preserved, unknown/cycle/negative refused\n",total,cases);
}
