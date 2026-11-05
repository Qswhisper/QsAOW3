#include "../tests/test_paths.h"
#include "local_progress.h"
#include <assert.h>
#include <sys/stat.h>
#include <initializer_list>

int main() {
    using namespace local_progress;
    char root[256], path[300];
    snprintf(root, sizeof(root), QSAOW3_TEST_TMP "art3-progress-test-%d", getpid());
    assert(mkdir(root, 0700) == 0);
    State s{}, recovered{};
    assert(load(root, s) == 0);
    s = seed(1000, 20, 0, 0);
    assert(write(root, s));
    assert(s.generation == 1 && load(root, recovered) == 1 && !memcmp(&s, &recovered, sizeof(s)));
    ++s.next_run; assert(write(root, s));
    assert(settle(s, 1, 200, true, 2, 18, 30, 3, 7));
    assert(s.credits == 1030 && s.experience == 18 && s.stars == 2);
    assert(write(root, s));
    State before = s;
    assert(!settle(s, 1, 200, true, 3, 28, 50, 3, 7));
    assert(!memcmp(&s, &before, sizeof(s)));
    ++s.next_run; assert(write(root, s));
    assert(settle(s, 2, 200, false, 0, 0, 0, 0, 0)); assert(write(root, s));
    assert(s.credits == 1030 && s.stars == 2 && s.missions[0].losses == 1);
    ++s.next_run; assert(write(root, s));
    assert(settle(s, 3, 200, true, 1, 9, 10, 3, 1)); assert(write(root, s));
    assert(s.credits == 1040 && s.stars == 2 && s.experience == 18);
    ++s.next_run; assert(write(root, s));
    assert(settle(s, 4, 200, true, 3, 28, 50, 3, 7)); assert(write(root, s));
    assert(s.credits == 1090 && s.stars == 3 && s.experience == 28 && s.missions[0].wins == 3);
    assert(load(root, recovered) == 1 && !memcmp(&s, &recovered, sizeof(s)));
    before = s;
    assert(!write("/does-not-exist/art3", s)); assert(!memcmp(&s, &before, sizeof(s)));
    // Truncated newest slot recovers the previous valid generation.
    snprintf(path, sizeof(path), "%s/local-progress.%u", root, unsigned(s.generation & 1));
    FILE* f = fopen(path, "wb"); assert(f); fputs("truncated", f); fclose(f);
    assert(load(root, recovered) == 1 && recovered.generation == s.generation - 1);
    assert(write(root, recovered)); assert(load(root, s) == 1);
    // All invalid slots must report corruption, never silently reseed.
    for (int i = 0; i < 2; ++i) {
        snprintf(path, sizeof(path), "%s/local-progress.%d", root, i);
        f = fopen(path, "wb"); assert(f); fputs("corrupt", f); fclose(f);
    }
    assert(load(root, s) == -1);
    s = seed(0, 0, 0, 0); s.version = 6; s.checksum = checksum(s); assert(!valid(s));
    s = seed(0, 0, 0, 0); s.next_run = 1;
    assert(!settle(s, 2, 200, true, 3, 10, 50, 0, 0));
    assert(!settle(s, 1, 200, true, 4, 10, 50, 0, 0));
    s = seed(1050, 20, 28, 3);
    assert(buy_upgrade(s, 100, 100, 5));
    assert(s.credits == 950 && s.experience == 33 && has_upgrade(s, 100));
    before = s;
    assert(!buy_upgrade(s, 100, 100, 5));
    assert(!buy_upgrade(s, 101, 951, 5));
    assert(!memcmp(&s, &before, sizeof(s)));
    assert(write(root, s)); assert(load(root, recovered) == 1 && has_upgrade(recovered, 100));
    // Migrate the exact original prefix, preserving earned progress/run IDs.
    s = seed(1050, 20, 28, 3); s.next_run = s.settled_run = 1;
    s.version = 1; s.size = kLegacySize; s.checksum = checksum_bytes(s, kLegacySize);
    snprintf(path, sizeof(path), "%s/legacy", root);
    f = fopen(path, "wb"); assert(f); assert(fwrite(&s, 1, kLegacySize, f) == kLegacySize); fclose(f);
    assert(read(path, recovered)); assert(recovered.version == kVersion && recovered.credits == 1050 && recovered.settled_run == 1);
    s = seed(140,20,122,7); s.next_run=s.settled_run=11;
    assert(buy_upgrade(s,10000,100,1));
    s.version=2;s.size=kV2Size;s.checksum=checksum_bytes(s,kV2Size);
    f=fopen(path,"wb");assert(f);assert(fwrite(&s,1,kV2Size,f)==kV2Size);fclose(f);
    assert(read(path,recovered) && recovered.version==kVersion && recovered.next_run==11 && has_upgrade(recovered,10000));
    assert(!recovered.career_count && !recovered.stock_count);
    s=seed(140,20,122,7);s.next_run=s.settled_run=11;
    CareerReward first[]={{21,100,1},{20,21032,1},{0,0,1000},{4,16,2}};
    CareerReward second[]={{22,2,1},{21,101,1},{20,21000,1},{0,0,1000},{1,0,10},{4,1,2},{4,11,2}};
    assert(claim_career(s,10002,3,first,4));
    assert(claim_career(s,10003,6,second,7));
    assert(s.credits==2140 && s.gold==30 && s.stock_count==3 && s.career_count==2 && s.experience==122 && s.next_run==11);
    before=s;assert(!claim_career(s,10002,3,first,4));assert(!memcmp(&s,&before,sizeof(s)));
    assert(!claim_career(s,10004,9,first,4));assert(!memcmp(&s,&before,sizeof(s)));
    CareerReward mixed[]={{0,0,1000},{127,999,1}};
    assert(!claim_career(s,99999,7,mixed,2));assert(!memcmp(&s,&before,sizeof(s)));
    CareerReward huge[]={{0,0,INT_MAX}};
    assert(!claim_career(s,99999,7,huge,1));assert(!memcmp(&s,&before,sizeof(s)));
    assert(write(root,s));assert(load(root,recovered)==1 && !memcmp(&s,&recovered,sizeof(s)));
    before=s;
    for(auto bad: {Stock{4,16,3},Stock{4,999,1},Stock{4,16,0},Stock{0,16,1}}) {
        assert(!consume_stock(s,bad.type,bad.id,bad.amount));assert(!memcmp(&s,&before,sizeof(s)));
    }
    assert(consume_stock(s,4,16,1) && stock_amount(s,4,16)==1);
    assert(write(root,s));assert(load(root,recovered)==1 && stock_amount(recovered,4,16)==1);
    assert(consume_stock(s,4,16,1) && stock_amount(s,4,16)==0 && s.stock_count==2);
    assert(stock_amount(s,4,1)==2 && stock_amount(s,4,11)==2 && s.career_count==2);
    before=s;assert(!consume_stock(s,4,16,1));assert(!memcmp(&s,&before,sizeof(s)));
    assert(!claim_career(s,10002,3,first,4)); // Empty stock is not a new milestone.
    assert(write(root,s));assert(load(root,recovered)==1 && !memcmp(&s,&recovered,sizeof(s)));
    puts("PASS: inventory debit, exhaustion/compaction, non-target stock, no milestone regrant, refusal without mutation, durable consumed stock");
    // One reward definition (10800/credits) is reused by different stones:
    // receipts belong to stones, so both earned milestone payouts are valid.
    puts("PASS: v2 migration retains research/run IDs; milestone star gate, two reused currency payouts, duplicate guard, complete mixed-reward rejection, overflow rejection, inventory round-trip");
    puts("PASS: round-trip, defeat reward policy, repeat rewards, best-score delta, duplicate guard, atomic slots, corrupt recovery, version rejection, upgrade debit/deduplication/insufficient funds, v1 migration");
}
