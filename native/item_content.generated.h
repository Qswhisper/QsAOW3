// Original definition SHA256 25ce63a83c70057a6ddf3d75fd914611b78085693ce025eb9bf102992ab8b998
#pragma once
namespace item_content {
struct Battery {int id,capacity,value;const char* name;const char* description;const char* icon;};
struct SpeedUp {int id,time;const char* name;const char* description;const char* icon;};
static const Battery batteries[]={
{1,2,20,"{#40384}","{#40390}","ico_battary_01"},
{2,7,70,"{#40385}","{#40390}","ico_battary_02"},
{3,15,150,"{#40386}","{#40390}","ico_battary_03"},
};
static const SpeedUp speedups[]={
{1,500,"{#2432}",nullptr,"ico_speedup_blue"},
{2,1000,"{#2433}",nullptr,"ico_speedup_red"},
{3,1625,"{#2434}",nullptr,"ico_speedup_gold"},
{4,250,"{#2488}",nullptr,"ico_speedup_green"},
};
}
