// Original skill-node graph; prices and edges are supplied by retained content.
#pragma once
#include "local_progress.h"
namespace skill_tree_policy {
struct Node {int id=0,card=0,stage=0,credits=0,cards=0,edge_count=0,edges[16]={};};
inline int credits(const Node& n){return local_progress::waived_skill_card(n.card)?0:n.credits;}
inline int cards(const Node& n){return local_progress::waived_skill_card(n.card)?0:n.cards;}
struct Stage {int id=0,to_next=0;};
struct Summary {int level[100]={},finished[32]={},stage=0;};
struct Graph {
    Node nodes[512];Stage stages[32];int count=0,stage_count=0,max_level[100]={};
    int index(int id)const{for(int i=0;i<count;++i)if(nodes[i].id==id)return i;return -1;}
    int stage_index(int id)const{for(int i=0;i<stage_count;++i)if(stages[i].id==id)return i;return -1;}
    bool valid()const{
        if(count<1||count>512||stage_count<1||stage_count>32)return false;
        for(int i=0;i<stage_count;++i){if(stages[i].id<=0||stages[i].to_next<0||(i&&stages[i].id<=stages[i-1].id))return false;}
        for(int i=0;i<count;++i){const auto& n=nodes[i];
            if(n.id<=0||n.card<=0||n.card>=100||n.credits<0||n.cards<=0||n.edge_count<0||n.edge_count>16||max_level[n.card]<1||max_level[n.card]>32||stage_index(n.stage)<0)return false;
            for(int j=0;j<i;++j)if(nodes[j].id==n.id)return false;
            for(int j=0;j<n.edge_count;++j){int k=index(n.edges[j]);if(k<0||k==i||nodes[k].stage>n.stage)return false;}
        }return true;
    }
    bool summarize(const local_progress::State& state,Summary& result,int extra=0)const{
        if(state.skill_count>512||stage_count<1)return false;
        Summary next;int unlocked=0;bool done[512]={};
        for(uint32_t i=0;i<state.skill_count+(extra?1:0);++i){int k=index(i<state.skill_count?state.skill_nodes[i]:extra);if(k<0||done[k])return false;
            const auto& n=nodes[k];int stage=stage_index(n.stage);if(stage<0||stage>unlocked)return false;
            for(int j=0;j<n.edge_count;++j){int e=index(n.edges[j]);if(e<0||!done[e])return false;}
            if(next.level[n.card]>=max_level[n.card])return false;
            done[k]=true;++next.level[n.card];++next.finished[stage];
            while(unlocked+1<stage_count&&stages[unlocked].to_next>0&&next.finished[unlocked]>=stages[unlocked].to_next)++unlocked;
        }
        next.stage=stages[unlocked].id;result=next;return true;
    }
    bool purchase(local_progress::State& state,int id)const{
        int i=index(id);Summary next;if(i<0||!summarize(state,next,id))return false;
        const auto& n=nodes[i];return local_progress::buy_skill_node(state,id,credits(n),n.card,cards(n));
    }
};
}
