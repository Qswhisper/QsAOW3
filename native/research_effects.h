#pragma once
#include <stdint.h>
#include <limits.h>

namespace research_effects {
struct Delta {
    int hp[5] = {};
    int income_base = 0, income_level = 0;
    int units_base = 0, units_level = 0;
};

inline bool number(const char*& p, int& value) {
    if (*p < '0' || *p > '9') return false;
    int64_t result = 0;
    while (*p >= '0' && *p <= '9') {
        result = result * 10 + (*p++ - '0');
        if (result > INT_MAX) return false;
    }
    value = (int)result;
    return true;
}

// The original UpgradeLevel.Desc embeds localization tokens with exact numeric
// deltas. HQ capacity uses the original base + zero-based HQ tier * increment.
inline bool parse(const char* p, Delta& result) {
    if (!p) return false;
    Delta delta; unsigned mask = 0;
    while (*p) {
        if (*p++ != '{' || *p++ != '#') return false;
        int token, value;
        if (!number(p, token) || *p++ != '<' || !number(p, value) ||
            *p++ != '>' || *p++ != '}') return false;
        int index = token >= 20075 && token <= 20078 ? token - 20075 :
                    token == 20094 ? 4 : token == 20037 ? 5 : token == 20038 ? 6 :
                    token == 20035 ? 7 : token == 20036 ? 8 : -1;
        if (index < 0 || mask & (1u << index)) return false;
        mask |= 1u << index;
        if (index < 5) delta.hp[index] = value;
        else if (index == 5) delta.income_base = value;
        else if(index==6) delta.income_level = value;
        else if(index==7){if(value<=0)return false;delta.units_base=value;}
        else {if(value<=0)return false;delta.units_level=value;}
        if (*p == '?' || *p == '\n' || *p == '\r') {
            if (*p++ == '\r' && *p == '\n') ++p;
            if (!*p) return false;
        }
        else if (*p) return false;
    }
    if (mask != 31 && mask != 96 && mask != 128 && mask != 384) return false;
    result = delta;
    return true;
}

// Exact flat HP token for starter infantry. Mixed-stat levels are rejected.
inline bool unit_health(const char* p, int& result) {
    if (!p) return false;
    const char prefix[] = "{#20000<";
    for (int i = 0; prefix[i]; ++i) if (*p++ != prefix[i]) return false;
    int value;
    if (!number(p, value) || *p++ != '>' || *p++ != '}' || *p || value <= 0) return false;
    result = value;
    return true;
}

inline bool add(const Delta& a, const Delta& b, Delta& result) {
    Delta next;
    for (int i = 0; i < 9; ++i) {
        int x = i < 5 ? a.hp[i] : i == 5 ? a.income_base : i==6?a.income_level:i==7?a.units_base:a.units_level;
        int y = i < 5 ? b.hp[i] : i == 5 ? b.income_base : i==6?b.income_level:i==7?b.units_base:b.units_level;
        if (x < 0 || y < 0 || (int64_t)x + y > INT_MAX) return false;
        int value = x + y;
        if (i < 5) next.hp[i] = value;
        else if (i == 5) next.income_base = value;
        else if(i==6) next.income_level = value;
        else if(i==7) next.units_base=value;
        else next.units_level=value;
    }
    result = next;
    return true;
}
inline bool capacity_values(int base,int increment,const Delta& d,int& next_base,int& next_increment){
    if(base<0||increment<0||base>INT8_MAX||increment>INT8_MAX||d.units_base<0||d.units_level<0)return false;
    int64_t b=int64_t(base)+d.units_base,i=int64_t(increment)+d.units_level;
    if(b>INT8_MAX||i>INT8_MAX)return false;next_base=int(b);next_increment=int(i);return true;
}
} // namespace research_effects
