#include "research_effects.h"
#include <assert.h>
#include <stdio.h>
#include <initializer_list>

int main() {
    using namespace research_effects;
    Delta hp, income, sum;
    assert(parse("{#20075<178>}?{#20076<207>}?{#20077<278>}?{#20078<343>}?{#20094<435>}", hp));
    assert(parse("{#20037<1>}?{#20038<1>}", income));
    assert(parse("{#20037<1>}\n{#20038<1>}", income));
    assert(parse("{#20037<1>}\r\n{#20038<1>}", income));
    assert(add(hp, income, sum));
    const int before[] = {7700, 9900, 12100, 14300, 17600};
    const int after[] = {7878, 10107, 12378, 14643, 18035};
    for (int i = 0; i < 5; ++i) assert(before[i] + sum.hp[i] == after[i]);
    assert(60 + sum.income_base == 61 && 10 + sum.income_level == 11);
    // Refuse partial, unknown, repeated, negative or overflowing descriptions;
    // failure must not leave a partially applied upgrade.
    for (const char* bad : {"", "{#20075<178>}", "{#99999<1>}", "{#20037<1>}?{#20037<2>}",
                           "{#20037<-1>}?{#20038<1>}", "{#20037<2147483648>}?{#20038<1>}",
                           "{#20037<1>}?{#20038<1>}?", "{#20037<1"}) {
        Delta unchanged = sum;
        assert(!parse(bad, unchanged));
        for (int i = 0; i < 5; ++i) assert(unchanged.hp[i] == sum.hp[i]);
        assert(unchanged.income_base == sum.income_base && unchanged.income_level == sum.income_level);
    }
    Delta overflow = sum; overflow.income_base = INT_MAX;
    assert(!add(overflow, income, sum));
    puts("research effects: original HP/income deltas and invalid-input transaction checks passed");
    int hp_delta = 0;
    assert(unit_health("{#20000<14>}", hp_delta) && 840 + hp_delta == 854);
    assert(unit_health("{#20000<16>}", hp_delta) && hp_delta == 16);
    for (const char* bad : {"", "{", "{#20000<", "{#20000<14", "{#20000<14>",
                           "{#20000<-1>}", "{#20000<0>}", "{#20000<2147483648>}",
                           "{#20000<20>}?{#20014<1>}", "{#20001<1>}"}) {
        int unchanged = 77;
        assert(!unit_health(bad, unchanged) && unchanged == 77);
    }
    puts("unit health: exact original deltas and unsupported/mixed-input checks passed");
}
