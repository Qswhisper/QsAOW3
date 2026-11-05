#include "contract_box_store.h"
#include "contract_policy.h"
#include <cassert>
#include <initializer_list>
static void claim(local_progress::State& s,int track,int rank=4){
    const auto* b=contract_box_policy::next(s.contract_boxes,track,rank);assert(b);
    supply_policy::Eligibility e[128];for(auto& v:e)v=supply_policy::Eligible;supply_policy::Draw d;
    assert(supply_policy::draw_pool(*b,contract_box_content::items,e,b->count,s.supply.claims+s.contract_boxes.claims+1,d));
    supply_types::Grant g[]={{0,0,10},{1,0,1},{4,1,1}};
    assert(contract_box_store::claim(s,track,rank,b->id,d,g,3));
    assert(s.contract_boxes.pending);s.contract_boxes.pending=0;
}
int main(int argc,char** argv){
    assert(argc==2);auto s=local_progress::seed(0,0,0,0);s.contract_points=777;
    assert(contract_box_policy::initialize(s.contract_boxes,s.contract_points));
    assert(!contract_box_policy::can_claim(s.contract_boxes,s.contract_points,0,4,100401));
    assert(contract_policy::initialize(s,20060,0));
    assert(contract_policy::claim(s,20060,1,1,100,0,0,15));
    assert(contract_box_policy::balance(s.contract_boxes,0,s.contract_points)==15&&contract_box_policy::balance(s.contract_boxes,1,s.contract_points)==15);
    assert(!contract_policy::claim(s,20060,1,1,100,0,0,15));
    assert(contract_policy::claim(s,20060,2,1,100,0,0,15));
    claim(s,0);assert(s.contract_boxes.tracks[0].step==1&&s.contract_points==807);
    assert(!contract_box_policy::can_claim(s.contract_boxes,s.contract_points,0,4,100401));
    assert(!contract_box_policy::next(s.contract_boxes,1,3)&&contract_box_policy::next(s.contract_boxes,1,5));
    for(int rank:{4,5,6,7,20,30}){assert(contract_policy::rank_allowed(20010,rank,4,6));assert(contract_box_policy::next(s.contract_boxes,1,rank));}
    assert(!contract_policy::rank_allowed(20010,3,4,6)&&!contract_policy::rank_allowed(20000,7,4,6));
    assert(contract_policy::published_rank_max(20010,4,6)==0&&contract_policy::published_rank_max(20000,4,6)==6);
    // A started group remains original rank4 until its own final box.
    s.contract_points+=1320;
    for(int i=0;i<4;++i)claim(s,0,5);
    assert(s.contract_boxes.tracks[0].cycles==1&&s.contract_boxes.tracks[0].spent==200&&!s.contract_boxes.tracks[0].group);
    assert(contract_box_policy::next(s.contract_boxes,0,5));
    assert(contract_box_policy::balance(s.contract_boxes,0,s.contract_points)==1150&&contract_box_policy::balance(s.contract_boxes,1,s.contract_points)==1350);
    claim(s,1);assert(s.contract_boxes.tracks[1].step==1);claim(s,1);
    assert(s.contract_boxes.tracks[1].cycles==1&&contract_box_policy::balance(s.contract_boxes,1,s.contract_points)==50);
    for(int round=0;round<5;++round)for(int i=0;i<5;++i)claim(s,0);
    assert(s.contract_boxes.tracks[0].cycles==6&&contract_box_policy::balance(s.contract_boxes,0,s.contract_points)==150);
    assert(local_progress::write(argv[1],s));local_progress::State restored;assert(local_progress::load(argv[1],restored)==1&&!memcmp(&s,&restored,sizeof(s)));
    const auto* b=contract_box_policy::next(s.contract_boxes,0,4);supply_policy::Eligibility e[128];for(auto& x:e)x=supply_policy::Eligible;supply_policy::Draw d;
    assert(supply_policy::draw_pool(*b,contract_box_content::items,e,b->count,42,d));supply_types::Grant g[]={{0,0,5}};
    auto transaction=s;assert(contract_box_store::claim(transaction,0,4,b->id,d,g,1));
    assert(!local_progress::write("/proc/art3-no-contract-write",transaction));assert(local_progress::load(argv[1],restored)==1&&!memcmp(&s,&restored,sizeof(s)));
    assert(local_progress::write(argv[1],transaction));assert(local_progress::load(argv[1],restored)==1&&restored.contract_boxes.pending);
    auto before=restored;assert(!contract_box_store::claim(restored,0,4,b->id,d,g,1)&&!memcmp(&before,&restored,sizeof(before)));
    restored.contract_boxes.pending=0;restored.credits=INT_MAX;before=restored;
    const auto* next=contract_box_policy::next(restored.contract_boxes,0,4);assert(supply_policy::draw_pool(*next,contract_box_content::items,e,next->count,77,d));
    assert(!contract_box_store::claim(restored,0,4,next->id,d,g,1)&&!memcmp(&before,&restored,sizeof(before)));
    auto old=local_progress::seed(123,45,6,7);old.contract_points=500;old.version=14;old.size=local_progress::kV14Size;old.checksum=local_progress::checksum_bytes(old,old.size);
    char path[640];snprintf(path,sizeof(path),"%s/legacy14",argv[1]);FILE* f=fopen(path,"wb");assert(f&&fwrite(&old,1,old.size,f)==old.size);fclose(f);
    assert(local_progress::read(path,restored)&&restored.version==local_progress::kVersion&&restored.credits==123&&restored.contract_points==500&&!restored.contract_boxes.initialized);
    assert(contract_box_policy::initialize(restored.contract_boxes,500)&&!contract_box_policy::balance(restored.contract_boxes,0,500));
    int draws=0;for(const auto& box:contract_box_content::boxes)for(int k=1;k<=100;++k){assert(supply_policy::draw_pool(box,contract_box_content::items,e,box.count,k,d)&&supply_policy::valid_draw_pool(box,contract_box_content::items,d));++draws;}
    puts("PASS: real new contract points; two original rows/cycles; rank4 gate and approved rank5..30 continuity; unsupported contract range preserved; carry/frozen/pending/duplicate integrity;700 original draws; atomic wallet/journal/save failure;read-only v14 migration");
}
