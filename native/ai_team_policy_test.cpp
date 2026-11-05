#include "ai_team_policy.h"
#include <assert.h>
#include <stdio.h>
int main(){
    using namespace ai_team_policy;
    assert(seats(1)==2&&seats(2)==4&&seats(4)==6&&team(0)==-1&&team(7)==-1);
    for(int a=1;a<=6;++a)for(int b=1;b<=6;++b){
        assert(allied(a,b)==((a%2)==(b%2)));
        assert(opponent(a,b)==((a%2)!=(b%2)));
    }
    int b[]={0,1,1,1},u[]={0,0,0,0};assert(winner(b,u,4)==-1);
    b[2]=0;assert(winner(b,u,4)==2);
    b[1]=b[3]=0;u[2]=1;assert(winner(b,u,4)==1);
    u[2]=0;assert(winner(b,u,4)==-1);
    b[0]=1;assert(winner(b,u,2)==1);
    int six_b[]={0,0,0,0,1,1},six_u[]={0,0,0,0,0,0};
    assert(winner(six_b,six_u,6)==-1);six_b[4]=0;assert(winner(six_b,six_u,6)==2);
    six_b[5]=0;six_u[4]=1;assert(winner(six_b,six_u,6)==1);
    six_u[4]=0;assert(winner(six_b,six_u,6)==-1);assert(winner(six_b,six_u,5)==-1);
    puts("PASS team policy: 36 friend/enemy pairs, fifth/sixth-seat survival, whole-team elimination and legacy modes");
}
