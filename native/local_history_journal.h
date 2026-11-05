#pragma once
#include "local_history_store.h"
#include <time.h>
namespace local_history_journal {
static pthread_mutex_t mutex=PTHREAD_MUTEX_INITIALIZER;
static uint64_t now(){time_t t=time(nullptr);return t>0&&uint64_t(t)<=253402300799ULL?uint64_t(t):0;}
static bool retain(const local_history_store::Record& r){
    pthread_mutex_lock(&mutex);bool ok=local_history_store::retain(local_faction::root(),r);pthread_mutex_unlock(&mutex);
    LOG("Local history retained=%d faction=%u kind=%u run=%llu dated=%d growth_writes=0",ok,r.faction,r.kind,
        (unsigned long long)local_history_store::match(r).settled,r.ended_utc!=0);return ok;
}
static bool pvp(const local_match_store::State& s,uint64_t utc=0){
    if(!local_history_store::scored(s))return true;local_history_store::Record r;
    return local_history_store::make(local_faction::active,s,utc,r)&&retain(r);
}
static bool coop(const cooperative_store::State& s,uint64_t utc=0){
    if(!local_history_store::scored(s.match))return true;local_history_store::Record r;
    return local_faction::active==2&&local_history_store::make(s,utc,r)&&retain(r);
}
}
