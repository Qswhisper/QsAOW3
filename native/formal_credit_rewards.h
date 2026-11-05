// local fixed-credit policy. Keep v7 constants immutable: a future
// balance change needs a new ledger policy, never revalue an existing receipt.
#pragma once
namespace formal_credit_rewards {
constexpr int v7_base=2770;
inline bool budget(int level,int& amount){
    if(level<0||level>11)return false;
    amount=v7_base+v7_base*50*level/1000;return true;
}
}
