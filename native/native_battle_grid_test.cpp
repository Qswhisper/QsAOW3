#include "native_battle_grid.h"
#include <assert.h>
#include <stdio.h>
struct Array {uintptr_t header[3]={};uintptr_t size=0;void* items[4]={};};
static_assert(offsetof(Array,items)==32,"original IL2CPP array layout");
int main(){
    int values[4][3]={{10,11,12},{20,21,22},{30,31,32},{40,41,42}};
    Array rows,lines[4];rows.size=4;
    for(int y=0;y<4;++y){rows.items[y]=&lines[y];lines[y].size=3;for(int x=0;x<3;++x)lines[y].items[x]=&values[y][x];}
    native_battle_grid::View grid;assert(grid.init_rows(&rows,3,4));
    assert(grid.cell(2,1)==&values[1][2]&&grid.cell(1,2)==&values[2][1]&&grid.cell(2,1)!=grid.cell(1,2));
    assert(*(int*)grid.cell(2,3)==42&&*(int*)grid.cell(0,0)==10);
    assert(!grid.cell(-1,0)&&!grid.cell(0,-1)&&!grid.cell(3,0)&&!grid.cell(0,4));
    assert(!grid.init_rows(&rows,4,3)&&!grid.rows&&!grid.cell(0,0));
    lines[2].size=2;assert(!grid.init_rows(&rows,3,4));lines[2].size=3;
    rows.items[2]=nullptr;assert(!grid.init_rows(&rows,3,4));
    assert(!grid.init(nullptr)&&!grid.rows);
    puts("PASS original row[y][x] layout; asymmetric 3x4 coordinates; transposed dimensions, truncated/null rows and out-of-range access refused");
}
