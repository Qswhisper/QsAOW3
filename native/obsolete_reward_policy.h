// Research is instant; acknowledge canonical original speed-up
// rewards in their original receipts, without adding or converting inventory.
#pragma once
namespace obsolete_reward_policy {
inline bool speedup(int type,int id){return type==7&&id>=1&&id<=4;}
}
