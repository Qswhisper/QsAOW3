// A plan over original contribution counters. Execution remains native.
#pragma once
#include <stdint.h>
namespace ai_tech_dependencies {
constexpr int capacity=100;
constexpr int max_nodes=128;
inline int index(int id){return id<0?-1:id%1000%100;}
struct Requirement {int key=0,count=0;};
struct Node {bool present=false;int type=0,tier=0,price=0,previous=-1,count=0;Requirement needs[16];};
struct Observation {int completed[capacity]={},pending[capacity]={};int limit=capacity;};
enum Status {Satisfied,Waiting,Action,Unsupported};
struct Result {Status status=Unsupported;int key=-1;};
struct Graph {
    Node nodes[max_nodes];int ids[max_nodes]={},size=0;
    int find(int key)const{for(int j=0;j<size;++j)if(ids[j]==key)return j;return -1;}
    bool add(int key,const Node& n){
        if(key<0||size>=max_nodes||find(key)>=0||n.type<1||n.tier<0||n.price<0||n.count<0||n.count>16)return false;
        for(int j=0;j<n.count;++j)if(n.needs[j].key<0||n.needs[j].count<1)return false;
        ids[size]=key;nodes[size]=n;nodes[size++].present=true;return true;
    }
    Result require(int key,int quantity,const Observation& s,bool* path,int depth)const{
        int counter=index(key),position=find(key);
        if(counter<0||counter>=s.limit||quantity<1||depth>=32||s.completed[counter]<0||s.pending[counter]<0)return {};
        if(s.completed[counter]>=quantity)return {Satisfied,key};
        if(s.pending[counter])return {Waiting,key};
        // Original counters alias by modulo, but recipe lookup must retain
        // full IDs: a zero-price scripted HQ alias is not a normal HQ recipe.
        if(position<0||path[position])return {};path[position]=true;
        const auto& node=nodes[position];
        for(int j=0;j<node.count;++j){const auto& need=node.needs[j];auto r=require(need.key,need.count,s,path,depth+1);
            if(r.status!=Satisfied){path[position]=false;return r;}}
        // Do not invent a missing lower tier even if a future definition omits
        // it from its contribution list. Existing structures must be upgraded.
        if(node.tier>0){auto r=require(node.previous,quantity,s,path,depth+1);
            if(r.status!=Satisfied){path[position]=false;return r;}}
        path[position]=false;return {Action,key};
    }
    Result choose(int key,int quantity,const Observation& s)const{bool path[max_nodes]={};return require(key,quantity,s,path,0);}
};
}
