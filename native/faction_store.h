// Preserve the existing Resistance slots verbatim. A second career gets its
// own directory and identical v3 slot format; faction choice is atomic.
#pragma once
#include "offline_account_store.h"
namespace local_faction {
static int active=2;
static const char* key(){return active==1?"CONFEDERATION":"RESISTANCE";}
static const char* root(int faction){return offline_accounts::faction_root(faction);}
static const char* root(){return root(active);}
struct Choice {uint32_t magic,faction,check;};
static bool read_choice(){
    char path[760];snprintf(path,sizeof(path),"%slocal-faction.bin",offline_accounts::root());FILE* f=fopen(path,"rb");
    if(!f){if(errno!=ENOENT)return false;active=2;return true;}
    Choice c{};bool ok=fread(&c,1,sizeof(c),f)==sizeof(c)&&fgetc(f)==EOF;fclose(f);
    if(!ok||c.magic!=0x31434146||(c.faction!=1&&c.faction!=2)||c.check!=(c.magic^c.faction^0x52ad731bu))return false;
    active=(int)c.faction;return true;
}
static bool prepare(){
    char fixture[600]={};bool isolated=false;
    if(FILE* f=fopen(DATA_ROOT "new-start-fixture.txt","r")){
        char id[81]={},extra=0;bool ok=fscanf(f,"ART3_NEW_START %80s %c",id,&extra)==1;fclose(f);
        if(strncmp(id,"qa-new-start-",13))ok=false;
        for(const char* p=id;*p;++p)if(!((*p>='a'&&*p<='z')||(*p>='0'&&*p<='9')||*p=='-'))ok=false;
        if(!ok)return false;snprintf(fixture,sizeof(fixture),"%s%s/",DATA_ROOT,id);isolated=true;
        if(mkdir(fixture,0700)&&errno!=EEXIST)return false;
    }
    if(!offline_accounts::initialize(isolated?fixture:DATA_ROOT,isolated))return false;
    if(!read_choice())return false;
    if(active==1&&mkdir(root(),0700)&&errno!=EEXIST)return false;
    struct stat s{};return stat(root(),&s)==0&&S_ISDIR(s.st_mode);
}
static bool choose(int faction){
    if(faction!=1&&faction!=2)return false;
    Choice c{0x31434146,(uint32_t)faction,0};c.check=c.magic^c.faction^0x52ad731bu;
    char path[760],temp[768];snprintf(path,sizeof(path),"%slocal-faction.bin",offline_accounts::root());snprintf(temp,sizeof(temp),"%s.tmp",path);
    FILE* f=fopen(temp,"wb");if(!f)return false;
    bool ok=fchmod(fileno(f),0644)==0&&fwrite(&c,1,sizeof(c),f)==sizeof(c);
    if(fflush(f)||fsync(fileno(f)))ok=false;if(fclose(f))ok=false;
    if(!ok||rename(temp,path))return false;
    int dir=open(offline_accounts::root(),O_RDONLY|O_DIRECTORY);if(dir<0)return false;ok=!fsync(dir);close(dir);return ok;
}
static bool account_gold(const local_progress::State& current,int& gold){
    local_progress::State other{};int status=local_progress::load(root(active==1?2:1),other);
    if(status<0||int64_t(current.gold)+other.gold>INT_MAX)return false;
    // Gold is account-wide in the original UserConfiguration. Each career
    // retains only its earned/imported contribution; no copied initial wallet.
    gold=current.gold+other.gold;return true;
}
}
