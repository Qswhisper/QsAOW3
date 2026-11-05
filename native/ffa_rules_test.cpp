#include "local_battle_format.h"
#include "local_history_store.h"
#include <cassert>
#include <string>
using namespace local_battle_format;
int main(int argc,char** argv){
    assert(argc==2);std::string root=argv[1];
    for(int mode=7;mode<=10;++mode){int count=seats(mode);assert(valid(mode)&&free_for_all(mode)&&teams(mode)==count);
        assert(lobby_mode(mode-1,team_format(mode))==mode&&!lobby_mode(mode-1,11));
        std::string directory=root+"/ffa"+std::to_string(count);assert(mkdir(directory.c_str(),0777)==0);
        local_match_store::State state;state.mode=mode;int buildings[6]={},units[6]={};
        assert(ffa_winner(buildings,units,count)==0);
        for(int id=1;id<=count;++id){assert(team(mode,id)==id-1);buildings[id-1]=1;
            assert(ffa_winner(buildings,units,count)==id);units[id%count]=1;assert(ffa_winner(buildings,units,count)== -1);units[id%count]=0;buildings[id-1]=0;
            assert(local_match_store::begin_pvp(state,false)&&local_match_store::freeze_career_stars(state,false,0,0)&&
                local_match_store::freeze_victory_credits(state,0)&&local_match_store::freeze_rules(state));
            assert(state.version==14&&state.size==136&&local_match_store::write(directory.c_str(),state));
            assert(local_match_store::settle(state,state.next,id==1?local_match_store::Victory:local_match_store::Defeat,999,1,id,nullptr,nullptr));
            assert(local_match_store::write(directory.c_str(),state));local_match_store::State read;
            assert(local_match_store::load(directory.c_str(),read)==1&&!memcmp(&state,&read,sizeof(state)));
            local_history_store::Record record;assert(local_history_store::make(2,state,0,record)&&record.version==3&&local_history_store::valid(record));
            assert(local_history_store::retain(directory.c_str(),record)&&local_history_store::retain(directory.c_str(),record));
            auto wrong=record;wrong.version=2;wrong.checksum=local_history_store::hash(wrong);assert(!local_history_store::valid(wrong));
            auto malformed=state;malformed.version=13;malformed.checksum=local_match_store::hash(malformed);assert(!local_match_store::valid(malformed));
            malformed=state;malformed.winner=count+1;malformed.checksum=local_match_store::hash(malformed);assert(!local_match_store::valid(malformed));
            assert(!local_match_store::formal(state)&&!local_match_store::career_stars(state)&&!local_match_store::victory_credits(state));
        }
        std::vector<local_history_store::Record> records;unsigned damaged;
        assert(local_history_store::load(directory.c_str(),2,records,damaged)&&records.size()==size_t(count)&&!damaged);
        auto switch_mode=state;switch_mode.mode=1;
        assert(local_match_store::begin_pvp(switch_mode,true)&&switch_mode.outcome==local_match_store::None&&switch_mode.winner== -1&&!switch_mode.reason&&switch_mode.wins==state.wins);
        assert(local_match_store::freeze_career_stars(switch_mode,true,2,10)&&local_match_store::freeze_victory_credits(switch_mode,0)&&
            local_match_store::freeze_contracts(switch_mode,0)&&local_match_store::freeze_rules(switch_mode)&&local_match_store::write(directory.c_str(),switch_mode));
        assert(switch_mode.version==12&&local_match_store::formal(switch_mode));
        units[count-1]=-1;assert(ffa_winner(buildings,units,count)== -2);
    }
    assert(!valid(3)&&!valid(11)&&!free_for_all(6)&&!free_for_all(11));
    assert(ffa_winner(nullptr,nullptr,3)== -2);
    puts("PASS: four original FFA formats, arbitrary sole survivor including seats3..6, alive/invalid/tie decisions, independent teams; v14/136 dual-slot and v3/216 immutable history with true winner, old-version rejection and no rewards");
}
