// A bounded single-goal reservation. It never changes native cash or prices.
#pragma once
#include <algorithm>
#include <stdint.h>
namespace ai_economy {
enum Kind {None,Build,Upgrade,Unit};
enum Release {Completed=1,Rejected=2,Expired=3,Threat=4,Invalid=5,Replaced=6};
struct Power {
    int supplied=0,used=0;
    void add(int native_energy){if(native_energy<0)supplied-=native_energy;else used+=native_energy;}
    bool needs(int margin=20)const{return supplied<used+margin;}
};
struct Plan {
    int goal=0,action=0,kind=None,price=0,since=0,until=0;
    int canceled_goal=0,retry_at=0,last_release=0;
    uint32_t started=0,completed=0,canceled=0;
    bool active()const{return kind!=None;}
    void release(int why,int now){
        if(!active())return;
        if(why==Completed)++completed;else {++canceled;canceled_goal=goal;retry_at=now+300;}
        last_release=why;goal=action=price=since=until=0;kind=None;
    }
    bool propose(int g,int a,int k,int cost,int now,int income_per_100,bool emergency=false){
        if(cost<=0||g<=0||a<=0||k<Build||k>Unit)return false;
        if(active()&&(goal!=g||action!=a||kind!=k))release(Replaced,now);
        if(!active()){
            if(g==canceled_goal&&now<retry_at&&!emergency)return false;
            // One observed earning interval per income chunk, plus construction
            // scheduling slack; no unlimited reservation on a dead economy.
            int wait=std::clamp(cost*100/std::max(10,income_per_100)+300,400,2400);
            goal=g;action=a;kind=k;price=cost;since=now;until=now+wait;++started;
        }else price=cost;
        if(now>=until){release(Expired,now);return false;}
        return true;
    }
    int reserved(int cash)const{return active()?std::min(std::max(0,cash),price):0;}
    int spendable(int cash)const{return std::max(0,cash-reserved(cash));}
};
struct Money {
    int last=-1,first=0,last_tick=0,last_spend=0;
    int64_t income=0,outflow=0,spent=0,refund=0;
    void observe(int cash,int now){
        if(last<0){first=cash;last=cash;last_tick=now;return;}
        int delta=cash-last;if(delta>=0)income+=delta;else outflow-=delta;
        last=cash;last_tick=now;
    }
    void order(int before,int after,int now){
        observe(before,now);int delta=before-after;
        if(delta>0){spent+=delta;last_spend=now;}else if(delta<0)refund-=delta;
        last=after;
    }
    bool reconciled()const{return last==first+income+refund-outflow-spent;}
};
}
