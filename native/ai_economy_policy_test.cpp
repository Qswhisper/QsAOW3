#include "ai_economy_policy.h"
#include "ai_replay_config.h"
#include <assert.h>
#include <stdio.h>
int main(){
    ai_economy::Power power;for(int e:{-80,-105,40,40,10,15})power.add(e);
    assert(power.supplied==185&&power.used==105&&!power.needs());power.add(80);assert(power.needs());
    ai_economy::Plan p;
    assert(p.propose(1102,1110,ai_economy::Build,250,100,50));
    assert(p.reserved(120)==120&&p.spendable(300)==50&&p.until==900);
    assert(p.propose(1102,1110,ai_economy::Build,250,600,50)&&p.until==900);
    assert(!p.propose(1102,1110,ai_economy::Build,250,900,50)&&!p.active());
    assert(!p.propose(1102,1110,ai_economy::Build,250,1000,50));
    assert(p.propose(1102,1110,ai_economy::Build,250,1200,50));p.release(ai_economy::Rejected,1210);
    assert(p.propose(1107,1130,ai_economy::Build,160,1220,50));p.release(ai_economy::Completed,1220);
    assert(p.completed==1&&p.canceled==2);
    assert(p.propose(1,1,ai_economy::Upgrade,800,1300,0)&&p.until==3700);
    p.release(ai_economy::Threat,1400);assert(p.spendable(900)==900);
    ai_economy::Money m;m.observe(1200,30);m.observe(1250,100);m.order(1250,950,100);m.observe(940,110);m.order(940,945,120);
    assert(m.income==50&&m.spent==300&&m.outflow==10&&m.refund==5&&m.reconciled());
    const char* valid="ART3_AI_BATCH 1 test_1 201 1 2 18000 0 42 2\n1 0 1 1\n2 1 2 0\n";
    FILE* f=tmpfile();assert(f);fputs(valid,f);rewind(f);ai_replay::Job job;assert(ai_replay::read(f,job)&&job.seats[0].controller==0&&job.policy_seed==42);fclose(f);
    for(const char* bad:{"ART3_AI_BATCH 1 ../bad 201 1 2 18000 0 1 2\n1 0 1 1\n2 1 1 1\n", "ART3_AI_BATCH 1 ok 201 4 2 18000 0 1 2\n1 0 1 1\n2 1 1 1\n", "ART3_AI_BATCH 1 ok 201 1 2 18000 0 1 2\n1 0 1 1\n2 1 1 1\nextra"}){
        f=tmpfile();assert(f);fputs(bad,f);rewind(f);assert(!ai_replay::read(f,job));fclose(f);}
    puts("PASS: bounded plans, cash reconciliation, strict private job parser");
}
