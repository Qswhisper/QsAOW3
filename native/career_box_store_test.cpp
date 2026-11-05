#include "local_progress.h"
#include "career_box_content.generated.h"
#include <cassert>
int main(){
    for(int id=11030;id<=11036;++id){auto state=local_progress::seed(3,2,0,360);local_progress::CareerReward grants[3];int count=0,credits=3,gold=2,tokens=0;
        for(const auto& i:career_boxes::items)if(i.box==id){int type=i.type==1?0:i.type==2?1:2;grants[count++]={type,0,i.amount};if(type==0)credits+=i.amount;else if(type==1)gold+=i.amount;else tokens+=i.amount;}
        assert(local_progress::claim_career(state,id,0,grants,count)&&state.credits==credits&&state.gold==gold&&state.supply.tokens==tokens);auto before=state;assert(!local_progress::claim_career(state,id,0,grants,count)&&!memcmp(&state,&before,sizeof(state)));
    }
    auto state=local_progress::seed(0,0,0,360);state.supply.tokens=INT_MAX;auto before=state;local_progress::CareerReward bad[]={{0,0,100},{2,0,1}};assert(!local_progress::claim_career(state,1,0,bad,2)&&!memcmp(&state,&before,sizeof(state)));
    state.supply.tokens=0;bad[1]={2,1,1};before=state;assert(!local_progress::claim_career(state,1,0,bad,2)&&!memcmp(&state,&before,sizeof(state)));
    puts("PASS: seven original fixed career boxes, exact credits/gold/tokens, once-only receipt, overflow/unknown-token atomic refusal");
}
