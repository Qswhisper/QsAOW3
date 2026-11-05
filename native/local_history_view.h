// A result presentation snapshot, never adopted by either active match ledger.
#pragma once
#include "local_history_store.h"
namespace local_history_view {
static bool active;
static local_history_store::Record selected;
static local_match_store::State state;
static cooperative_store::State coop;
static void clear(){active=false;}
static bool select(const local_history_store::Record& r){
    if(!local_history_store::valid(r))return false;
    selected=r;state=local_history_store::match(r);
    coop=r.kind==local_history_store::Cooperative?local_history_store::cooperative(r):cooperative_store::State{};
    active=true;return true;
}
}
