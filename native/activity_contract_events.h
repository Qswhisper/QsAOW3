// Original locale4274 says strictly greater than the configured percentage.
#pragma once
#include "battle_contract_events.h"
namespace activity_contract_events {
static constexpr uint32_t eligible=0x1000,completed=0x2000;
inline bool valid(int raw){return raw>=0&&raw<=1000;}
inline bool qualifies(int raw){
    const auto& rule=contract_content::activity_rules[0];
    return valid(raw)&&rule.subtype>=0&&rule.subtype<=100&&raw>rule.subtype*10;
}
}
