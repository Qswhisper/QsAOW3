#include "local_history_view.h"
#include <cassert>
#include <string>
using namespace local_history_store;
static local_match_store::State pvp(int version,uint64_t run){
    local_match_store::State s;s.version=version;s.size=version<8?112:136;
    s.next=s.settled=run;s.outcome=local_match_store::Surrender;s.reason=6;s.tick=1408;s.winner=2;s.losses=run;
    s.checksum=local_match_store::hash(s);assert(local_match_store::valid(s));return s;
}
int main(int argc,char** argv){
    assert(argc==2);std::string root=argv[1],f1=root+"/f1",f2=root+"/f2";
    assert(!mkdir(f1.c_str(),0777)&&!mkdir(f2.c_str(),0777));
    std::vector<Record> rows;unsigned bad=99;assert(load(f2.c_str(),2,rows,bad)&&rows.empty()&&!bad);
    Record r,last;
    for(int version:{4,7,8,9,10,11,12}){
        auto s=pvp(version,version);assert(make(2,s,version==4?0:1800000000+version,r)&&valid(r));
        assert(retain(f2.c_str(),r));char name[64];assert(filename(name,sizeof(name),r));
        assert(read((f2+"/battle-history/"+name).c_str(),last)&&same_result(last,r)&&last.ended_utc==r.ended_utc);
        auto before=last;r.ended_utc=1800000500;r.checksum=hash(r);
        assert(retain(f2.c_str(),r)&&read((f2+"/battle-history/"+name).c_str(),last)&&!memcmp(&before,&last,sizeof(last)));
    }
    assert(load(f2.c_str(),2,rows,bad)&&rows.size()==7&&!bad&&match(rows.front()).settled==12&&!rows.back().ended_utc);
    auto old=rows.back();auto changed=match(old);++changed.wins;changed.checksum=local_match_store::hash(changed);
    assert(make(2,changed,0,r)&&!retain(f2.c_str(),r));assert(load(f2.c_str(),2,rows,bad)&&same_result(rows.back(),old));
    auto live=pvp(12,20);assert(make(2,live,0,r)&&merge(rows,r)&&rows.size()==8&&merge(rows,r)&&rows.size()==8);
    auto unchanged=live;assert(local_history_view::select(old)&&local_history_view::state.settled==4&&!memcmp(&live,&unchanged,sizeof(live)));
    local_history_view::clear();assert(!local_history_view::active);
    for(int outcome:{0,5,6}){auto invalid=live;invalid.outcome=outcome;invalid.checksum=local_match_store::hash(invalid);assert(!make(2,invalid,0,r));}
    auto pending=live;++pending.next;pending.checksum=local_match_store::hash(pending);assert(!make(2,pending,0,r));
    assert(!make(0,live,0,r)&&!make(3,live,0,r)&&!make(2,live,253402300800ULL,r));
    assert(make(1,pvp(4,4),0,r)&&retain(f1.c_str(),r));assert(load(f1.c_str(),1,rows,bad)&&rows.size()==1&&!bad);
    cooperative_store::State c;c.match.scenario=973;c.match.map=558;c.match.opponent=1;c.match.mode=3;cooperative_store::seal(c);
    assert(cooperative_store::begin(c)&&cooperative_store::freeze_credits(c,2)&&cooperative_store::settle(c,1,local_match_store::Victory,9200,1,1,nullptr,nullptr,10));
    cooperative_store::seal(c);assert(make(c,1800000100,r)&&retain(f2.c_str(),r));auto wire=cooperative_store::to_wire(cooperative(r));
    assert(!memcmp(&wire,r.payload,176)&&cooperative(r).points==100&&cooperative(r).credits==3324&&match(r).size==112);
    assert(load(f2.c_str(),2,rows,bad)&&rows.size()==8&&!bad&&rows.front().kind==Cooperative);
    assert(local_history_view::select(r)&&local_history_view::coop.credits==3324&&local_history_view::state.settled==1);
    auto latest=pvp(11,13);assert(local_match_store::write(f2.c_str(),latest));assert(make(2,latest,1800000200,r)&&retain(f2.c_str(),r));
    auto next=latest;assert(local_match_store::begin_pvp(next,false)&&local_match_store::write(f2.c_str(),next));
    assert(load(f2.c_str(),2,rows,bad)&&rows.size()==9&&!bad&&match(rows.front()).settled==13);
    local_match_store::State current;assert(local_match_store::load(f2.c_str(),current)==1&&current.next==14&&current.settled==13);
    assert(local_history_view::select(old)&&local_match_store::load(f2.c_str(),current)==1&&current.next==14); // Browse cannot settle a pending run.
    char name[64];filename(name,sizeof(name),r);std::string path=f2+"/battle-history/"+name;
    FILE* f=fopen((path+".tmp").c_str(),"wb");assert(f&&fwrite("torn",1,4,f)==4&&!fclose(f));
    assert(load(f2.c_str(),2,rows,bad)&&rows.size()==9&&!bad);
    f=fopen(path.c_str(),"r+b");assert(f&&!fseek(f,12,SEEK_SET)&&fputc(0,f)!=EOF&&!fclose(f));
    // Force a checksum mismatch even if its first byte happened to be zero.
    f=fopen(path.c_str(),"r+b");assert(f&&!fseek(f,12,SEEK_SET)&&fputc(uint8_t(r.checksum)^255,f)!=EOF&&!fclose(f));
    assert(!retain(f2.c_str(),r)&&load(f2.c_str(),2,rows,bad)&&rows.size()==8&&bad==1);
    assert(merge(rows,r)&&rows.size()==9); // The real latest remains independently readable.
    assert(load(f1.c_str(),2,rows,bad)&&rows.empty()&&bad==1); // No faction mixing.
    std::string blocked=root+"/blocked";assert(!mkdir(blocked.c_str(),0777)&&!mkdir((blocked+"/battle-history").c_str(),0777));
    filename(name,sizeof(name),old);assert(!mkdir((blocked+"/battle-history/"+name).c_str(),0777)&&!retain(blocked.c_str(),old));
    puts("PASS: immutable 216-byte history, v4/7..12 PvP112/136 and cooperative176, duplicate/conflict/cold order/faction/date/torn/corruption/failure paths; browse preserves current pending ledger");
}
