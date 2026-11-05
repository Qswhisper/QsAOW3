// Original reward pools + local uniform subtype/card selection.
#pragma once
#include "supply_store.h"
namespace local_supply_rewards {
struct Candidates {int type=0,ids[128]={},count=0;};
static supply_policy::Eligibility (*extra_candidates)(const supply_content::Item&,const local_progress::State&,int,Candidates&);
static bool add(Candidates& c,int id){
    if(id<0||c.count==128)return false;int p=c.count;
    while(p&&c.ids[p-1]>id){c.ids[p]=c.ids[p-1];--p;}
    if((p&&c.ids[p-1]==id)||(p<c.count&&c.ids[p]==id))return false;c.ids[p]=id;++c.count;return true;
}
static supply_policy::Eligibility candidates(const supply_content::Item& item,const local_progress::State& s,int faction,Candidates& out){
    Candidates c;int rank=local_skill::rank(s);if(rank<1||faction<1||faction>2)return supply_policy::Unsupported;
    if(item.type==25&&item.type_id==1&&item.id==12019){c.type=15;c.ids[c.count++]=1;out=c;return supply_policy::Eligible;}
    if(item.type==26)return extra_candidates?extra_candidates(item,s,faction,out):supply_policy::Unsupported;
    if(item.type==1||item.type==2||item.type==7){if(item.type_id)return supply_policy::Unsupported;c.type=item.type==1?0:item.type==2?1:2;c.ids[c.count++]=0;out=c;return supply_policy::Eligible;}
    if((item.type>=8&&item.type<=10)||item.type==20){int id=local_items::speedup_id(item.type);if(item.type_id||!local_career::obsolete_speedup_gifts||!local_items::supported({7,id,1}))return supply_policy::Unsupported;c.type=7;c.ids[c.count++]=id;
    }else if(item.type>=17&&item.type<=19){int id=item.type-16;if(item.type_id||!local_items::supported({10,id,1}))return supply_policy::Unsupported;c.type=10;c.ids[c.count++]=id;
    }else if(item.type==21){if(item.type_id||!local_items::supported({11,0,1}))return supply_policy::Unsupported;c.type=11;c.ids[c.count++]=0;
    }else if(item.type>=3&&item.type<=5){c.type=4;void* rows=local_career::list("BATTLE","boosts");int n=rows?*(int*)((char*)rows+0x18):0;
        if(n<1||n>512)return supply_policy::Unsupported;
        for(int i=0;i<n;++i){void* card=list_item(rows,i);if(!card)return supply_policy::Unsupported;int id=*(int*)((char*)card+0x10);
            if((item.type_id&&item.type_id!=id)||*(int8_t*)((char*)card+0x2a)!=item.type-3||*(int8_t*)((char*)card+0x2c)>rank)continue;
            if(!add(c,id))return supply_policy::Unsupported;}
    }else if(item.type>=11&&item.type<=13){c.type=8;void* cards[128];int n=local_career::skill_cards(cards,128);if(n<1)return supply_policy::Unsupported;
        for(int i=0;i<n;++i){int id=*(int*)((char*)cards[i]+0x10);if((item.type_id&&item.type_id!=id)||*(int*)((char*)cards[i]+0x28)!=item.type-10)continue;
            if(!add(c,id))return supply_policy::Unsupported;}
    }else if(item.type>=14&&item.type<=16){c.type=9;void* rows=local_career::list("SHARED","heroCards");int n=rows?*(int*)((char*)rows+0x18):0;if(n<1||n>128)return supply_policy::Unsupported;
        for(int i=0;i<n;++i){void* card=list_item(rows,i);if(!card)return supply_policy::Unsupported;int id=*(int*)((char*)card+0x10);
            if(id>=200||*(int*)((char*)card+0x50)<=0)continue;
            void* fraction=*(void**)((char*)card+0x38),*from=*(void**)((char*)card+0x40);if(!fraction||!from)return supply_policy::Unsupported;
            if((item.type_id&&item.type_id!=id)||*(int*)((char*)fraction+0x10)!=faction||*(int8_t*)((char*)card+0x30)!=item.type-13||*(int*)((char*)from+0x10)>rank)continue;
            if(!add(c,id))return supply_policy::Unsupported;}
    }else return supply_policy::Unsupported;
    out=c;return c.count?supply_policy::Eligible:supply_policy::Ineligible;
}
static bool supported_pool(const supply_content::Box& box,const supply_content::Item* items,const local_progress::State& s,int faction){
    if(box.count<1||box.count>128)return false;int eligible=0,mandatory=0;
    for(int i=0;i<box.count;++i){const auto& item=items[box.first+i];if(item.prob<=0||effective_box_policy::removed(item.type,item.type_id))continue;
        Candidates pool;auto status=candidates(item,s,faction,pool);if(status==supply_policy::Unsupported)return false;
        if(status==supply_policy::Eligible){++eligible;mandatory+=item.prob_txt==100;}else if(item.prob_txt==100)return false;}
    return mandatory<=box.minimum&&eligible>=box.maximum;
}
static bool grant(supply_types::Grant* out,int capacity,int& count,int type,int id,int amount){
    if(!out||amount<=0)return false;
    for(int i=0;i<count;++i)if(out[i].type==type&&out[i].id==id){if(int64_t(out[i].amount)+amount>INT_MAX)return false;out[i].amount+=amount;return true;}
    if(count==capacity)return false;out[count++]={type,id,amount};return true;
}
static bool supported(const supply_content::Box& b,const local_progress::State& s,int f){return supported_pool(b,supply_content::items,s,f);}
static bool prepare_pool(const supply_content::Box& box,const supply_content::Item* items,uint64_t claims,const local_progress::State& s,int faction,supply_policy::Draw& drawn,supply_types::Grant* out,int capacity,int& count){
    if(box.count<1||box.count>128||claims==UINT64_MAX||!out||capacity<1||capacity>128)return false;
    supply_policy::Eligibility eligible[128];
    for(int i=0;i<box.count;++i){const auto& item=items[box.first+i];Candidates pool;
        eligible[i]=item.prob>0&&!effective_box_policy::removed(item.type,item.type_id)?candidates(item,s,faction,pool):supply_policy::Ineligible;}
    uint64_t receipt=supply_policy::seed(faction,claims+1,box.id);supply_policy::Draw result;
    if(!supply_policy::draw_pool(box,items,eligible,box.count,receipt,result))return false;
    supply_types::Grant grants[128];int used=0;
    for(int i=0;i<result.count;++i){const supply_content::Item* item=nullptr;
        for(int j=0;j<box.count;++j)if(items[box.first+j].id==result.rows[i].item)item=&items[box.first+j];
        Candidates pool;if(!item||candidates(*item,s,faction,pool)!=supply_policy::Eligible||result.rows[i].amount>1000000)return false;
        if(pool.type<=2){int amount=result.rows[i].amount;
            if(pool.type==0&&!local_reward_skills::box_credits(s,amount,amount))return false;
            if(!grant(grants,capacity,used,pool.type,0,amount))return false;continue;}
        uint64_t state=receipt^(uint64_t(uint32_t(item->id))*0x9e3779b97f4a7c15ULL),selected=0;
        // A modification reward's amount is collected days for ONE selected
        // modification; card quantities below are still distributed per card.
        if(pool.type==17){if(!supply_policy::uniform(state,pool.count,selected)||!grant(grants,capacity,used,17,pool.ids[selected],result.rows[i].amount))return false;continue;}
        for(int unit=0;unit<result.rows[i].amount;++unit){if(!supply_policy::uniform(state,pool.count,selected)||!grant(grants,capacity,used,pool.type,pool.ids[selected],1))return false;}
    }
    for(int i=0;i<used;++i)out[i]=grants[i];count=used;drawn=result;return true;
}
static bool prepare(const supply_content::Box& b,const local_progress::State& s,int f,supply_policy::Draw& d,supply_types::Grant* g,int cap,int& n){return prepare_pool(b,supply_content::items,s.supply.claims,s,f,d,g,cap,n);}

}
