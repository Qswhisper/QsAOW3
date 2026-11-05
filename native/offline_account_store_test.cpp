#include "../tests/test_paths.h"
#include "offline_account_store.h"
#include "new_start_policy.h"
#include <assert.h>
#include <stdlib.h>
int main(){
    char p[]=QSAOW3_TEST_TMP "art3-account-test-XXXXXX";assert(mkdtemp(p));char file[768];
    snprintf(file,sizeof(file),"%s/local-progress.0",p);FILE* f=fopen(file,"wb");assert(f);fputs("LEGACY-PROTECTED",f);fclose(f);
    assert(offline_accounts::initialize(p));assert(!offline_accounts::fresh_account());auto original=offline_accounts::choice;
    assert(offline_accounts::create());assert(offline_accounts::fresh_account()&&offline_accounts::choice.has_previous);
    auto first=offline_accounts::choice.active;assert(offline_accounts::initialize(p));assert(offline_accounts::choice.active==first);
    assert(offline_accounts::previous()&&!offline_accounts::fresh_account());assert(offline_accounts::previous()&&offline_accounts::choice.active==first);
    assert(offline_accounts::create());assert(offline_accounts::choice.active!=first);assert(offline_accounts::previous()&&offline_accounts::choice.active==first);
    assert(offline_accounts::cycle());assert(offline_accounts::cycle()&&!offline_accounts::fresh_account());
    assert(offline_accounts::cycle()&&offline_accounts::choice.active==first);
    f=fopen(file,"rb");char s[32]={};assert(f&&fread(s,1,16,f)==16&&!strcmp(s,"LEGACY-PROTECTED"));fclose(f);
    // Corrupt only the newest selector, verify A/B fallback; never alter a career.
    snprintf(file,sizeof(file),"%s/local-accounts.%d",p,int(offline_accounts::choice.generation&1));f=fopen(file,"wb");assert(f);fputc(0,f);fclose(f);
    assert(offline_accounts::initialize(p));assert(offline_accounts::choice.generation>original.generation);
    char clean[]=QSAOW3_TEST_TMP "art3-account-clean-XXXXXX";assert(mkdtemp(clean));assert(offline_accounts::initialize(clean,true));
    assert(offline_accounts::fresh_account()&&!offline_accounts::choice.has_previous&&offline_accounts::private_session);
    assert(!offline_accounts::previous());assert(!offline_accounts::valid(offline_accounts::Choice{}));
    char key1[256],key2[256];
    assert(!new_start_policy::scoped_key("/real",0,false,"FirstMissionState",key1,sizeof(key1)));
    assert(new_start_policy::scoped_key("/real",1,false,"FirstMissionState",key1,sizeof(key1)));
    assert(new_start_policy::scoped_key("/qa",1,true,"FirstMissionState",key2,sizeof(key2))&&strcmp(key1,key2));
    assert(new_start_policy::scoped_key("/real",2,false,"FirstMissionState",key2,sizeof(key2))&&strcmp(key1,key2));
    assert(new_start_policy::tutorial_key(".11851.TutorialVisited"));
    assert(!new_start_policy::tutorial_key(key1)&&!new_start_policy::tutorial_key("MasterVolume")&&!new_start_policy::tutorial_key("GoogleUserId"));
    char fail[]=QSAOW3_TEST_TMP "art3-account-failure-XXXXXX";assert(mkdtemp(fail));
    snprintf(file,sizeof(file),"%s/local-progress.0",fail);f=fopen(file,"wb");assert(f);fputs("LEGACY-PROTECTED",f);fclose(f);
    assert(offline_accounts::initialize(fail));auto before=offline_accounts::choice;
    snprintf(file,sizeof(file),"%s/local-accounts.tmp",fail);assert(!mkdir(file,0700));
    assert(!offline_accounts::create());assert(!memcmp(&before,&offline_accounts::choice,sizeof(before)));
    assert(offline_accounts::initialize(fail)&&!offline_accounts::choice.active);
    snprintf(file,sizeof(file),"%s/local-progress.0",fail);f=fopen(file,"rb");memset(s,0,sizeof(s));assert(f&&fread(s,1,16,f)==16&&!strcmp(s,"LEGACY-PROTECTED"));fclose(f);
    snprintf(file,sizeof(file),"%s/local-accounts.1",fail);f=fopen(file,"wb");assert(f);fputc(0,f);fclose(f);
    assert(!offline_accounts::initialize(fail)&&!offline_accounts::root());
    printf("PASS independent accounts, legacy bytes, A/B fallback, switch, empty fresh account, scoped tutorial keys, write failure, corrupt selectors refused; evidence %s %s %s\n",p,clean,fail);
}
