#include "../tests/test_paths.h"
#include "warpoint_store.h"
#include "cooperative_store.h"
#include <cassert>
#include <cstdlib>
#include <pthread.h>
#include <initializer_list>
using namespace local_match_store;
static State result(int mode,bool formal,int level,int outcome,int reason,int stars=4){
    State s;s.mode=mode;assert(begin_pvp(s,formal)&&freeze_career_stars(s,formal,formal?2:0,formal?stars:0)&&freeze_victory_credits(s,formal?level:0));
    assert(settle(s,1,(Outcome)outcome,19000,reason,outcome==Victory?1:2,nullptr,nullptr));s.reserved=formal&&reason==1&&(outcome==Victory||outcome==Defeat)?100:0;s.checksum=hash(s);assert(valid(s));return s;
}
static void clear_root(const char* root){char p[640];for(const char* kind:{"local-progress","local-ai"})for(int slot:{0,1}){snprintf(p,sizeof(p),"%s/%s.%d",root,kind,slot);unlink(p);}assert(!rmdir(root));}
static void* small_stack(void* arg){
    char root[]=QSAOW3_TEST_TMP "art3-credit-small-XXXXXX";assert(mkdtemp(root));local_progress::StateBuffer a,b;assert(a.data&&b.data);*a.data=local_progress::seed(1690,30,197,9);assert(local_progress::write(root,*a.data));
    auto match=result(1,true,11,Victory,1);assert(write(root,match));bool changed=false;assert(warpoint_store::reconcile(*a.data,match,changed)&&changed&&a.data->credits==5983&&local_progress::write(root,*a.data));
    State cold;assert(load(root,cold)==1&&local_progress::load(root,*b.data)==1&&warpoint_store::reconcile(*b.data,cold,changed)&&!changed&&b.data->credits==5983&&b.data->stars==13&&b.data->quest_warpoints==100);
    clear_root(root);*(bool*)arg=true;return nullptr;
}
int main(int argc,char** argv){
    assert(argc==2&&sizeof(State)==136&&sizeof(cooperative_store::WireState)==176&&sizeof(cooperative_store::State)==200);int cases=0;
    local_progress::StateBuffer a,b,c;assert(a.data&&b.data&&c.data);
    for(int mode:{1,2,4})for(bool formal:{false,true})for(int level:{0,1,11})for(int outcome=1;outcome<=6;++outcome)for(int reason:{0,1,6,7}){
        auto match=result(mode,formal,level,outcome,reason);*a.data=local_progress::seed(1690,30,197,9);bool changed=false;
        int credits=formal&&outcome==Victory&&reason==1?(level==0?2770:level==1?2908:4293):0;
        assert(victory_credits(match)==credits&&credit_level(match)==(formal?level:0)&&start_rank(match)==(formal?2:0)&&star_budget(match)==(formal?4:0));
        assert(warpoint_store::reconcile(*a.data,match,changed)&&changed==formal&&a.data->credits==1690+credits&&a.data->gold==30&&a.data->experience==197&&a.data->stars==9+(credits?4:0));
        assert(warpoint_store::reconcile(*a.data,match,changed)&&!changed);++cases;
    }
    auto match=result(1,true,11,Victory,1);auto frozen=match;assert(!freeze_victory_credits(match,0)&&!memcmp(&match,&frozen,sizeof(match))); // Settled awards cannot be refrozen.
    auto highest=result(4,true,0,Victory,1,0);assert(!career_stars(highest)&&victory_credits(highest)==2770);
    for(int v:{5,6}){State old;assert(begin_pvp(old,true));if(v==6)assert(freeze_career_stars(old,true,2,4));assert(settle(old,1,Victory,1000,1,1,nullptr,nullptr));old.checksum=hash(old);assert(valid(old)&&!victory_credits(old));*a.data=local_progress::seed(1690,30,197,9);bool changed=false;assert(warpoint_store::reconcile(*a.data,old,changed)&&changed&&a.data->credits==1690);}
    for(int version:{5,6}){auto bad=match;bad.version=version;bad.checksum=hash(bad);assert(!valid(bad));}
    for(int flags:{0,1,1<<26,12<<22}){auto bad=match;if(flags==0)bad.reserved2&=~(1<<30);else if(flags==1)bad.reserved2&=~1;else if(flags==(1<<26))bad.reserved2|=flags;else bad.reserved2=(bad.reserved2&~(15<<22))|flags;bad.checksum=hash(bad);assert(!valid(bad));}
    State pending;assert(begin_pvp(pending,true)&&freeze_career_stars(pending,true,2,4));auto unchanged=pending;assert(!freeze_victory_credits(pending,12)&&!memcmp(&pending,&unchanged,sizeof(pending)));
    pending.mode=3;assert(!freeze_victory_credits(pending,0));
    for(int field:{0,1,2}){*a.data=local_progress::seed(1690,30,197,9);if(field==0)a.data->credits=INT_MAX-10;else if(field==1)a.data->stars=INT_MAX-1;else a.data->quest_warpoints=INT_MAX-1;*b.data=*a.data;bool changed=true;assert(!warpoint_store::reconcile(*a.data,match,changed)&&!changed&&!memcmp(a.data,b.data,sizeof(*a.data)));}
    char root[]=QSAOW3_TEST_TMP "art3-credit-store-XXXXXX";assert(mkdtemp(root));*a.data=local_progress::seed(1690,30,197,9);assert(local_progress::write(root,*a.data)&&write(root,match));*b.data=*a.data;bool changed=false;assert(warpoint_store::reconcile(*b.data,match,changed)&&changed&&b.data->credits==5983);
    char missing[640];snprintf(missing,sizeof(missing),"%s/missing",root);assert(!local_progress::write(missing,*b.data));State cold;assert(load(root,cold)==1&&credit_level(cold)==11&&victory_credits(cold)==4293);
    assert(local_progress::load(root,*c.data)==1&&c.data->credits==1690&&warpoint_store::reconcile(*c.data,cold,changed)&&changed&&c.data->credits==5983&&local_progress::write(root,*c.data));
    assert(local_progress::load(root,*a.data)==1&&warpoint_store::reconcile(*a.data,cold,changed)&&!changed&&a.data->credits==5983);
    FILE* out=fopen(argv[1],"wb");assert(out&&fwrite(&cold,1,cold.size,out)==cold.size);fclose(out);clear_root(root);
    pthread_attr_t attr;assert(!pthread_attr_init(&attr)&&!pthread_attr_setstacksize(&attr,65536));pthread_t t;bool small=false;assert(!pthread_create(&t,&attr,small_stack,&small)&&!pthread_join(t,nullptr)&&small);pthread_attr_destroy(&attr);
    printf("PASS: %d mode/source/level/outcome/reason cases; frozen2770/2908/4293, no legacy credits, PvP112/cooperative176 ABI, combined receipt replay/failed-write retry, atomic three-wallet overflow, invalid policy/mask refusal, complete cold receipt on64KiB thread\n",cases);
}
