#include "control_point_policy.h"
#include <cassert>
#include <cstdio>
#include <initializer_list>
#include <string>
#include <vector>
#include "local_history_store.h"
#include "control_point_plan.h"
using namespace control_point_policy;
int main(int argc,char** argv){
    assert(argc==2);std::string root=argv[1];
    assert(lobby_mode(1,11)==1&&lobby_mode(2,11)==5&&lobby_mode(3,11)==6&&lobby_mode(4,22)==2&&lobby_mode(5,33)==4);
    assert(!lobby_mode(2,22)&&!lobby_mode(3,33)&&!lobby_mode(99,11));
    for(int mode:{1,2,4,5,6})assert(valid_mode(mode));
    assert(!valid_mode(0)&&!valid_mode(3)&&!valid_mode(11));
    assert(team_mode(5)==1&&team_mode(6)==1&&team_mode(4)==4);
    // Holding points and elimination are distinct observable terminal paths.
    assert(terminal(Rule::Domination,4,7,500,499,500)==Result::First);
    assert(terminal(Rule::Domination,4,7,499,500,500)==Result::Second);
    assert(terminal(Rule::Domination,4,7,500,500,500)==Result::Draw);
    assert(terminal(Rule::Domination,4,7,499,499,500)==Result::Pending);
    assert(terminal(Rule::Maneuvers,4,7,9000,0,500)==Result::Pending);
    assert(terminal(Rule::Elimination,4,7,9000,0,500)==Result::Pending);
    for(auto rule:{Rule::Elimination,Rule::Domination,Rule::Maneuvers}){
        assert(terminal(rule,0,1,0,0,500)==Result::Second);
        assert(terminal(rule,1,0,0,0,500)==Result::First);
        assert(terminal(rule,0,0,0,0,500)==Result::Draw);
        assert(terminal(rule,-1,1,0,0,500)==Result::Invalid);
    }
    assert(terminal(Rule::Domination,4,7,0,0,0)==Result::Invalid);
    assert(terminal(Rule::Domination,4,7,-1,0,500)==Result::Invalid);
    Point points[]={{20,12,10,0,true},{3,10,12,-1,true},{5,8,10,1,true},{1,10,10,1,false}};
    Target t;assert(choose(points,4,0,10,10,2,t)&&t.id==3&&t.hold);
    assert(choose(points,4,1,10,10,1,t)&&t.id==3&&!t.hold);
    points[1].team=0;assert(choose(points,4,0,10,10,2,t)&&t.id==5);
    points[2].team=0;assert(choose(points,4,0,10,10,2,t)&&t.index==-1);
    points[2].team=3;t={77,88,true};assert(!choose(points,4,0,10,10,2,t)&&t.id==77&&t.index==88&&t.hold);
    assert(!choose(points,65,0,10,10,2,t));
    for(int mode:{5,6})for(auto result:{local_match_store::Victory,local_match_store::Draw,local_match_store::Surrender,local_match_store::Interrupted}){
        std::string directory=root+"/mode"+std::to_string(mode)+"-result"+std::to_string(int(result));assert(mkdir(directory.c_str(),0777)==0);
        local_match_store::State s;s.mode=mode;
        assert(!local_match_store::valid(s));
        assert(local_match_store::begin_pvp(s,false)&&local_match_store::freeze_career_stars(s,false,0,0)&&
            local_match_store::freeze_victory_credits(s,0)&&local_match_store::freeze_rules(s));
        assert(s.version==13&&s.size==136&&!local_match_store::formal(s));
        assert(local_match_store::write(directory.c_str(),s));auto written=s;local_match_store::State loaded;
        assert(local_match_store::load(directory.c_str(),loaded)==1&&!memcmp(&written,&loaded,sizeof(written)));
        int reason=result==local_match_store::Surrender?6:result==local_match_store::Interrupted?7:1;
        int winner=result==local_match_store::Draw?-1:result==local_match_store::Victory?1:2;
        assert(local_match_store::settle(s,s.next,result,300,reason,winner,nullptr,nullptr));
        assert(!local_match_store::settle(s,s.next,result,300,reason,winner,nullptr,nullptr));
        s.checksum=local_match_store::hash(s);assert(local_match_store::valid(s));
        assert(local_match_store::write(directory.c_str(),s)&&local_match_store::load(directory.c_str(),loaded)==1&&!memcmp(&s,&loaded,sizeof(s)));
        assert(local_match_store::victory_credits(s)==0&&local_match_store::career_stars(s)==0);
        local_history_store::Record record;
        if(result!=local_match_store::Interrupted){
            assert(local_history_store::make(2,s,0,record)&&record.version==2&&local_history_store::match(record).mode==mode);
            assert(local_history_store::retain(directory.c_str(),record)&&local_history_store::retain(directory.c_str(),record));
            std::vector<local_history_store::Record> rows;unsigned damaged=99;
            assert(local_history_store::load(directory.c_str(),2,rows,damaged)&&!damaged&&rows.size()==1&&local_history_store::same_result(rows[0],record));
            auto old=record;old.version=1;old.checksum=local_history_store::hash(old);assert(!local_history_store::valid(old));
        }else assert(!local_history_store::make(2,s,0,record));
        auto malformed=s;malformed.version=12;malformed.checksum=local_match_store::hash(malformed);assert(!local_match_store::valid(malformed));
    }
    // An independent breadth-first search checks the planner's output on
    // ground, a narrow bridge, two heights and disconnected terrain.
    for(int scenario=0;scenario<4;++scenario){
        constexpr int w=64,h=64;std::vector<uint8_t> land(w*h,1),mask(w*h,1);ai_spawn_plan::Point bases[]={{6,6},{57,57}},flags[3]={{99,99},{98,98},{97,97}};
        if(scenario)for(int y=0;y<h;++y)for(int x=28;x<35;++x)if(y<29||y>34||scenario==3)land[y*w+x]=0;
        if(scenario==2)for(int y=0;y<12;++y)for(int x=35;x<w;++x)land[y*w+x]=2;
        for(int y=0;y<h;++y)for(int x=0;x<w;++x){bool ground=x>=3&&y>=3&&x+3<w&&y+3<h;
            for(int dy=-3;ground&&dy<=3;++dy)for(int dx=-3;dx<=3;++dx)if(land[(y+dy)*w+x+dx]!=1){ground=false;break;}mask[y*w+x]=ground;}
        bool ok=control_point_plan::plan(mask.data(),land.data(),w,h,bases,flags);assert(ok==(scenario!=3));
        if(!ok){assert(flags[0].x==99&&flags[1].x==98&&flags[2].x==97);continue;}
        for(auto base:bases){std::vector<int> d(w*h,-1),q{base.y*w+base.x};d[q[0]]=0;
            for(size_t pos=0;pos<q.size();++pos){int p=q[pos],x=p%w,y=p/w;for(int v:{x?p-1:-1,x+1<w?p+1:-1,y?p-w:-1,y+1<h?p+w:-1})
                if(v>=0&&d[v]<0&&land[v]==land[p]){d[v]=d[p]+1;q.push_back(v);}}
            for(auto flag:flags){assert(land[flag.y*w+flag.x]&&d[flag.y*w+flag.x]>=8);
                for(int dy=-1;dy<=1;++dy)for(int dx=-1;dx<=1;++dx)assert(land[(flag.y+dy)*w+flag.x+dx]==land[flag.y*w+flag.x]);}}
        for(int i=0;i<3;++i)for(int j=0;j<i;++j)assert(ai_spawn_plan::distance(flags[i],flags[j])>=64);
    }
    puts("PASS: lobby rules, distinct native-observation victory/AI decisions; connected original-terrain planner; version13 two-slot cold read, real outcome identity, zero rewards, deduped version2 history");
}
