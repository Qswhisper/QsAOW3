// : explicit original goals, public successors and rank.
// Included inside local_quests after its original model helpers.
struct Rule {void* description=nullptr;int id=-1,rank=0,kind=0,target=-1,minimum=0;};
struct Rules {
    Rule nodes[512];int count=0,root=-1;
    int index(int id)const{for(int i=0;i<count;++i)if(nodes[i].id==id)return i;return -1;}
};
static bool ascii(void* s,char* out,size_t cap){
    if(!s)return false;int n=*(int*)((char*)s+16);if(n<0||(size_t)n>=cap)return false;
    auto c=(uint16_t*)((char*)s+20);for(int i=0;i<n;++i){if(c[i]>127)return false;out[i]=(char)c[i];}out[n]=0;return true;
}
static void* localized_template(void* text){
    char raw[64];int id=-1,used=0;
    if(!ascii(text,raw,sizeof(raw))||sscanf(raw,"{#%d}%n",&id,&used)!=1||!used||raw[used]||id<0)return text;
    void* dict=get(saved("LOCALE"),0x10),*value=nullptr,*ex=nullptr;void* args[]={&id,&value};
    void* found=dict?invoke_named(dict,"TryGetValue",2,args,&ex):nullptr;
    return !ex&&found&&*(bool*)((char*)found+16)?value:nullptr;
}
static int template_id(void* text,int* last_number=nullptr){
    text=localized_template(text);if(!text)return -1;int n=*(int*)((char*)text+16);if(n<5||n>2048)return -1;
    auto c=(uint16_t*)((char*)text+20);if(c[0]!='{'||c[1]!='#')return -1;int id=0,i=2;
    for(;i<n&&c[i]>='0'&&c[i]<='9';++i){if(id>1000000)return -1;id=id*10+c[i]-'0';}
    if(i>=n||c[i]!='<')return -1;
    if(last_number){*last_number=-1;for(;i<n;++i)if(c[i]=='<'){int v=0,j=i+1;bool digit=false;
        for(;j<n&&c[j]>='0'&&c[j]<='9';++j){digit=true;v=v*10+c[j]-'0';if(v>1000)return -1;}
        if(digit&&j<n&&c[j]=='>')*last_number=v;}}
    return id;
}
static bool integer(const char* s,int& n){char* end=nullptr;long v=strtol(s,&end,10);if(!*s||*end||v<0||v>INT_MAX)return false;n=(int)v;return true;}
static void goal(Rule& r,int faction){
    void* loc=get(r.description,0xb0);size_t n=loc?array_length(loc):0;if(n<1||n>8)return;
    char parts[4][64]={};for(size_t i=0;i<n&&i<4;++i)if(!ascii(((void**)((char*)loc+32))[i],parts[i],64))return;
    int number=-1,tid=template_id(get(r.description,0x48),&number),target=-1;
    if(n==3&&!strcmp(parts[0],"battle")&&tid==5999&&integer(parts[2],target)){
        // Only an explicit mission description for this faction, no PvP/count goals.
        // Retained normal routes identify the implemented chapter IDs.
        const auto* route=local_scenario(target);
        if(route&&route->faction==faction){r.kind=1;r.target=target;}return;
    }
    if(n<3||strcmp(parts[0],"upgrade")||!integer(parts[2],target))return;
    bool unit=!strcmp(parts[1],"unit"),building=!strcmp(parts[1],"bld");if(!unit&&!building)return;
    int slot=-2,required=1;
    if(n==4&&tid==5998){if(!integer(parts[3],slot)||slot>19)return;}
    else if(n==3&&building&&tid==5995&&number>=1){slot=-1;required=number;}
    else if(n==3&&building&&tid==5996){slot=-1;required=1;}
    else return;
    void* upgrades=local_career::list(local_career::key(faction),unit?"upgradeUnits":"upgradeBuildings");
    // Several Upgrade objects share a slot: Order is the original phase,
    // while Levels contains the research steps within that phase. Structural
    // building level N is phase N-1, not array index N-1.
    int phase=slot==-1?required-1:0;void* match=nullptr;
    for(int i=0;upgrades&&i<*(int*)((char*)upgrades+0x18);++i){void* u=list_item(upgrades,i),*owner=get(u,0x48);
        if(owner&&*(int*)((char*)owner+0x10)==target&&*(int8_t*)((char*)u+0x21)==slot&&*(int8_t*)((char*)u+0x20)==phase){if(match)return;match=u;}}
    void* levels=get(match,0x18);if(!levels||!array_length(levels))return;
    void* level=((void**)((char*)levels+32))[0];if(!level)return;
    r.kind=2;r.target=*(int*)((char*)level+0x10);r.minimum=required;
}
static bool rules(Rules& out,int faction){
    void* rows=local_career::list(local_career::key(faction),"quests"),*f=local_faction::fraction(faction);
    out.count=rows?*(int*)((char*)rows+0x18):0;if(!f||out.count<1||out.count>512)return false;
    out.root=*(int*)((char*)f+0x108);
    for(int i=0;i<out.count;++i){auto& r=out.nodes[i];r.description=list_item(rows,i);if(!r.description)return false;
        r.id=*(int*)((char*)r.description+0x10);r.rank=*(int*)((char*)r.description+0x78);
        void* own=get(r.description,0xa8),*edges=get(r.description,0x80);
        if(r.id<0||r.rank<0||!own||*(int*)((char*)own+0x10)!=faction||(edges&&array_length(edges)>128))return false;
        for(int j=0;j<i;++j)if(out.nodes[j].id==r.id)return false;goal(r,faction);
    }
    return out.index(out.root)>=0;
}
static void reach(const Rules& g,const local_progress::State& s,bool* reached){
    int rank=local_skill::rank(s),root=g.index(g.root);if(root<0||g.nodes[root].rank>rank)return;reached[root]=true;
    for(int round=0;round<g.count;++round){bool changed=false;
        for(int i=0;i<g.count;++i)if(reached[i]&&local_progress::has_quest(s,g.nodes[i].id)){
            void* edges=get(g.nodes[i].description,0x80);
            for(size_t j=0;edges&&j<array_length(edges);++j){void* d=((void**)((char*)edges+32))[j];int k=d?g.index(*(int*)((char*)d+0x10)):-1;
                if(k>=0&&g.nodes[k].description==d&&g.nodes[k].rank<=rank&&!reached[k]){reached[k]=true;changed=true;}}}
        if(!changed)break;
    }
}
static bool accomplished(const local_progress::State& s,const Rule& r){
    if(r.kind==1){for(uint32_t i=0;i<s.count;++i)if(s.missions[i].id==r.target&&s.missions[i].wins>0)return true;return false;}
    if(r.kind==2){if(local_progress::has_upgrade(s,r.target))return true;
        // Structural career unlocks are real completed upgrades too. Resolve
        // only receipts in this state, never captured profile upgrade lists.
        void* faction=get(r.description,0xa8);int f=faction?*(int*)((char*)faction+0x10):0;
        void* stones=local_career::list(local_career::key(f),"careerStones");
        for(int i=0;stones&&i<*(int*)((char*)stones+0x18);++i){void* stone=list_item(stones,i);
            if(!local_progress::has_career(s,*(int*)((char*)stone+0x10)))continue;void* rewards=get(stone,0x18);
            for(int j=0;rewards&&j<*(int*)((char*)rewards+0x18);++j){void* item=list_item(rewards,j);
                if(item&&*(int*)((char*)item+0x14)==20&&*(int*)((char*)item+0x18)==r.target&&*(int*)((char*)item+0x1c)==1)return true;}}
    }return false;
}
static bool reward_supported(void* d){return d&&*(int*)((char*)d+0x60)>=0&&*(int*)((char*)d+0x64)>=0&&
    *(int*)((char*)d+0x68)==0&&*(int*)((char*)d+0x6c)>=0&&*(int*)((char*)d+0x70)>=0;}
static bool rule_available(const local_progress::State& s,void* d){Rules g;if(!d||!rules(g,local_faction::active))return false;
    int k=g.index(*(int*)((char*)d+0x10));bool reached[512]={};reach(g,s,reached);return k>=0&&reached[k]&&g.nodes[k].description==d;
}
static bool rule_complete(const local_progress::State& s,void* d){Rules g;if(!d||!rules(g,local_faction::active))return false;
    int k=g.index(*(int*)((char*)d+0x10));return k>=0&&g.nodes[k].description==d&&reward_supported(d)&&accomplished(s,g.nodes[k]);
}
static void* rule_list(void* user,const local_progress::State& s,Roots& roots){
    Rules g;if(!rules(g,local_faction::active))return nullptr;bool reached[512]={};reach(g,s,reached);
    void* old=get(get(user,0x28),0xa8),*rows=old?object_new(object_class(old)):nullptr;if(!roots.keep(rows))return nullptr;
    void* ex=nullptr;invoke_named(rows,".ctor",0,nullptr,&ex);if(ex)return nullptr;
    for(int i=0;i<g.count;++i)if(reached[i]){const auto& r=g.nodes[i];Roots one;int state=local_progress::quest_state(s,r.id);
        if(state!=32)state=reward_supported(r.description)&&accomplished(s,r)?16:state==8?8:4;
        void* q=quest(r.description,state,one);if(!q)return nullptr;void* args[]={q};invoke_named(rows,"Add",1,args,&ex);if(ex)return nullptr;
    }return rows;
}
