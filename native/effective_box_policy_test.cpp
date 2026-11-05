#include "shop_box_store.h"
#include "contract_box_store.h"
#include "supply_store.h"
#include <cassert>
#include <cstdio>
using namespace supply_policy;
static int checks,empty_placeholders;
static void batch(const Box& box,const Item* pool){
    Eligibility e[128];for(int i=0;i<box.count;++i)e[i]=effective_box_policy::removed(pool[box.first+i].type,pool[box.first+i].type_id)?Unsupported:Eligible;
    if(!box.count){Draw d;d.count=7;auto before=d;assert(!draw_pool(box,pool,e,0,1,d)&&!memcmp(&d,&before,sizeof(d)));++empty_placeholders;return;}
    for(int seed=1;seed<=64;++seed){Draw d;assert(draw_pool(box,pool,e,box.count,seed,d)&&valid_draw_pool(box,pool,d,true));
        for(int j=0;j<d.count;++j){const Item* item=nullptr;for(int i=0;i<box.count;++i)if(pool[box.first+i].id==d.rows[j].item)item=&pool[box.first+i];
            assert(item&&!effective_box_policy::removed(item->type,item->type_id));}++checks;}
}
int main(int argc,char** argv){
    assert(argc==2);for(const auto& b:supply_content::boxes)batch(b,supply_content::items);
    batch(shop_box_content::box,shop_box_content::items);for(const auto& b:contract_box_content::boxes)batch(b,contract_box_content::items);
    Item custom[]={{1,1000,100,1,0,4,4},{2,999,100,8,0,1,1},{3,10,1,3,0,1,1},{4,20,2,7,0,1,1},{5,999,100,21,0,1,1}};
    Box b{9000,1,0,0,2,2,0,5};Eligibility e[]={Eligible,Unsupported,Eligible,Eligible,Unsupported};int boost=0,xp=0;
    for(int seed=1;seed<=12000;++seed){Draw d;assert(draw_pool(b,custom,e,5,seed,d)&&d.count==2&&valid_draw_pool(b,custom,d,true));
        assert(d.rows[0].item==1);boost+=d.rows[1].item==3;xp+=d.rows[1].item==4;}
    assert(boost+xp==12000&&xp>boost*17/10&&xp<boost*23/10);
    // Unknown IDs in an obsolete family are not silently deleted.
    auto old_id=custom[1].type_id;custom[1].type_id=999;Draw unchanged;unchanged.count=7;auto sentinel=unchanged;
    assert(!draw_pool(b,custom,e,5,1,unchanged)&&!memcmp(&unchanged,&sentinel,sizeof(unchanged)));custom[1].type_id=old_id;
    e[2]=Ineligible;e[3]=Ineligible;assert(!draw_pool(b,custom,e,5,1,unchanged)&&!memcmp(&unchanged,&sentinel,sizeof(unchanged)));
    // Old saved results and inventory are read, never redrawn or erased.
    auto legacy=local_progress::seed(300,20,0,9);assert(local_progress::grant_stock(legacy,11,0,5)&&local_progress::grant_stock(legacy,7,1,6));
    legacy.shop.claims=1;legacy.shop.pending=1;legacy.shop.draw_count=legacy.shop.grant_count=2;
    legacy.shop.draws[0]={12000,300};legacy.shop.draws[1]={12010,1};legacy.shop.grants[0]={0,0,300};legacy.shop.grants[1]={11,0,1};
    Draw historical;historical.count=2;historical.rows[0]=legacy.shop.draws[0];historical.rows[1]=legacy.shop.draws[1];
    assert(valid_draw_pool(shop_box_content::box,shop_box_content::items,historical)&&!valid_draw_pool(shop_box_content::box,shop_box_content::items,historical,true));
    assert(local_progress::write(argv[1],legacy));auto disk=legacy;local_progress::State restored;
    assert(local_progress::load(argv[1],restored)==1&&!memcmp(&disk,&restored,sizeof(disk))&&local_progress::stock_amount(restored,11,0)==5&&local_progress::stock_amount(restored,7,1)==6);
    restored.shop.pending=0;restored.shop.games=3;auto before=restored;
    Eligibility all[128];for(auto& x:all)x=Eligible;Draw d;assert(draw_pool(shop_box_content::box,shop_box_content::items,all,33,7,d));
    supply_types::Grant bad[]={{11,0,1}};assert(!shop_box_store::claim(restored,d,bad,1)&&!memcmp(&restored,&before,sizeof(before)));
    auto supply=local_progress::seed(0,0,0,9);supply.quest_warpoints=200;auto supply_before=supply;const auto* first=box(401);
    assert(draw(*first,all,first->count,1,d)&&!supply_store::claim(supply,2,401,d,bad,1)&&!memcmp(&supply,&supply_before,sizeof(supply)));
    auto contract=local_progress::seed(0,0,0,0);assert(contract_box_policy::initialize(contract.contract_boxes,0));contract.contract_points=20;auto contract_before=contract;
    const auto& cb=contract_box_content::boxes[0];assert(draw_pool(cb,contract_box_content::items,all,cb.count,1,d));bad[0]={7,1,1};
    assert(!contract_box_store::claim(contract,0,4,cb.id,d,bad,1)&&!memcmp(&contract,&contract_before,sizeof(contract)));
    printf("PASS %d original retained-table future draws; %d original empty disabled slots refused; excluded mandatory/unsupported obsolete rows; original10:20 relative weights; unknown ID and insufficient effective pool atomic refusal; old key result/cold save and key/speed inventory unchanged; three stores reject new obsolete grants atomically\n",checks,empty_placeholders);
}
