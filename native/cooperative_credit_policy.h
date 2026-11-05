// fixed local policy v2; never revalue existing v1 results.
#pragma once
namespace cooperative_credit_policy {
constexpr int v2_base=2770;
inline bool budget(int level,int& amount){if(level<0||level>11)return false;amount=v2_base+v2_base*100*level/1000;return true;}
}
