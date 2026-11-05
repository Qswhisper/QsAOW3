// Local training adapter for the original control-point rule identities.
// This is explicit local match configuration, not recovered server GAI input.
#pragma once
namespace control_point_rules {
constexpr int flag_count=3;
constexpr int flag_bonus=20,flag_sight=3; // Original 6.10.9 missions14/214.
constexpr int score_target=300,score_interval_ticks=10,score_per_flag=1;
constexpr int capture_ticks=200,capture_radius=2; // Original Flag constants.
}
