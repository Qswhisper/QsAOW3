#include "../tests/test_paths.h"
#include "supply_store.h"
#include <cassert>
#include <cstring>
#include <cstdlib>
using namespace supply_policy;
static Draw original_draw(const Box& b,uint64_t receipt){
    Eligibility available[128];assert(b.count<=128);for(int i=0;i<b.count;++i)available[i]=Eligible;
    Draw a,bis;assert(draw(b,available,b.count,receipt,a)&&draw(b,available,b.count,receipt,bis));
    assert(a.count==bis.count&&!memcmp(a.rows,bis.rows,a.count*sizeof(a.rows[0]))&&valid_draw(b,a));return a;
}
int main(int argc,char** argv){
    int sequences=0,boxes_checked=0;
    for(const auto& g:groups){auto state=local_progress::seed(100,10,0,0);state.quest_warpoints=box(g.boxes[g.count-1])->points+777;
        int paid=0;
        for(int step=0;step<g.count;++step){const Box* b=nullptr;int cost=0;
            assert(next(state.supply,g.rank,b,cost)&&b->id==g.boxes[step]);auto before=state;
            auto drawn=original_draw(*b,seed(1,state.supply.claims+1,b->id));
            // Core transaction fixture only; runtime reward mapping is checked separately.
            supply_types::Grant grants[]={{0,0,100},{1,0,3},{2,0,2},{4,1,1},{8,1,2},{9,70,1}};
            assert(!supply_store::claim(state,g.rank,b->id+1,drawn,grants,6)&&!memcmp(&state,&before,sizeof(state)));
            auto bad=drawn;bad.rows[0].amount=INT_MAX;assert(!supply_store::claim(state,g.rank,b->id,bad,grants,6)&&!memcmp(&state,&before,sizeof(state)));
            assert(supply_store::claim(state,g.rank,b->id,drawn,grants,6));paid+=cost;
            assert(state.quest_warpoints==before.quest_warpoints-cost&&state.credits==before.credits+100&&state.supply.tokens==before.supply.tokens+2);
            assert(state.supply.claims==uint64_t(step+1)&&state.supply.last_cost==cost&&state.supply.last_box==b->id&&state.supply.pending);
            auto committed=state;assert(!supply_store::claim(state,g.rank,b->id,drawn,grants,6)&&!memcmp(&state,&committed,sizeof(state)));
            if(step+1<g.count){assert(state.supply.group==g.rank&&state.supply.step==step+1);const Box* frozen=nullptr;int delta=0;
                state.supply.pending=0;assert(next(state.supply,29,frozen,delta)&&frozen->rank==g.rank);}
            else state.supply.pending=0;
            ++boxes_checked;
        }
        assert(paid==box(g.boxes[g.count-1])->points&&state.quest_warpoints==777&&state.supply.cycle==1&&!state.supply.group&&!state.supply.step);
        const Box* b=nullptr;int cost=0;assert(next(state.supply,29,b,cost)&&b->rank==29&&b->id==group(29)->boxes[0]);++sequences;
    }
    const auto* first=box(401);Eligibility e[128];for(int i=0;i<first->count;++i)e[i]=Eligible;
    Draw unchanged;unchanged.count=13;auto sentinel=unchanged;e[2]=Unsupported;
    assert(!draw(*first,e,first->count,1,unchanged)&&!memcmp(&unchanged,&sentinel,sizeof(unchanged)));
    for(int i=0;i<first->count;++i)e[i]=items[first->first+i].prob_txt==100?Eligible:Ineligible;
    assert(!draw(*first,e,first->count,1,unchanged));
    auto state=local_progress::seed(10,3,0,0);state.quest_warpoints=199;auto before=state;auto d=original_draw(*first,7);supply_types::Grant grant{0,0,1};
    assert(!supply_store::claim(state,2,401,d,&grant,1)&&!memcmp(&state,&before,sizeof(state)));
    state.quest_warpoints=200;state.credits=INT_MAX;before=state;assert(!supply_store::claim(state,2,401,d,&grant,1)&&!memcmp(&state,&before,sizeof(state)));
    char root[]=QSAOW3_TEST_TMP "art3-supply-test-XXXXXX";assert(mkdtemp(root));state=local_progress::seed(10,3,0,0);state.quest_warpoints=220;
    assert(local_progress::write(root,state)&&supply_store::claim(state,2,401,d,&grant,1));auto claimed=state;
    char missing[640];snprintf(missing,sizeof(missing),"%s/missing",root);assert(!local_progress::write(missing,state)&&!memcmp(&state,&claimed,sizeof(state)));
    assert(local_progress::load(root,state)==1&&state.quest_warpoints==220&&!state.supply.claims);
    assert(supply_store::claim(state,2,401,d,&grant,1)&&local_progress::write(root,state));assert(local_progress::load(root,state)==1&&state.supply.pending&&state.quest_warpoints==20&&state.credits==11);
    assert(state.supply.draw_count==unsigned(d.count)&&!memcmp(state.supply.draws,d.rows,d.count*sizeof(d.rows[0])));
    if(argc==2){FILE* fixture=fopen(argv[1],"wb");assert(fixture&&fwrite(&state,1,sizeof(state),fixture)==sizeof(state));fclose(fixture);}
    // The complete v9 prefix is read-only migrated, preserving its point cursor.
    char legacy[640];snprintf(legacy,sizeof(legacy),"%s/v9",root);auto old=local_progress::seed(17,4,50,9);old.version=9;old.size=local_progress::kV9Size;old.warpoint_pvp_cursor=27;old.checksum=local_progress::checksum_bytes(old,old.size);
    FILE* f=fopen(legacy,"wb");assert(f&&fwrite(&old,1,old.size,f)==old.size);fclose(f);
    assert(local_progress::read(legacy,state)&&state.version==local_progress::kVersion&&state.size==sizeof(state)&&state.warpoint_pvp_cursor==27&&!state.supply.claims&&state.credits==17);
    f=fopen(legacy,"rb");uint32_t header[4];assert(f&&fread(header,1,16,f)==16&&header[1]==9&&header[2]==52344);fclose(f);unlink(legacy);
    for(int slot=0;slot<2;++slot){char path[640];snprintf(path,sizeof(path),"%s/local-progress.%d",root,slot);unlink(path);}assert(rmdir(root)==0);
    printf("PASS: %d original rank sequences/%d box steps; cumulative threshold debit, frozen rank, manual cycle rollover/carry, deterministic weighted mandatory draws, unsupported/ineligible/invalid/insufficient/overflow atomic refusals, private cold-save replay, v9 read-only migration; runtime reward mapping not included\n",sequences,boxes_checked);
}
