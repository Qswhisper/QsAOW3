#include "../tests/test_paths.h"
#include "supply_store.h"
#include <cassert>
#include <cstdlib>
#include <cstring>
using namespace local_progress;
int main(int argc,char** argv){
    assert(argc==3);State migrated;assert(read(argv[1],migrated)&&migrated.version==kVersion&&migrated.supply.pending&&migrated.quest_warpoints==20&&!migrated.modification_equipment_count);
    auto s=seed(20,3,50,18);assert(grant_stock(s,17,110,7)&&grant_stock(s,17,111,3)&&grant_stock(s,17,151,4));
    assert(equip_modification(s,11,110)&&equip_modification(s,15,151)&&equipped_modification(s,11)==110&&equipped_modification(s,15)==151);
    auto before=s;assert(!equip_modification(s,11,110)&&!memcmp(&s,&before,sizeof(s)));assert(!equip_modification(s,11,999)&&!memcmp(&s,&before,sizeof(s)));
    assert(!consume_stock(s,17,110,1)&&!memcmp(&s,&before,sizeof(s)));assert(grant_stock(s,17,110,4)&&stock_amount(s,17,110)==11);
    assert(equip_modification(s,11,111)&&stock_amount(s,17,110)==11&&stock_amount(s,17,111)==3&&s.modification_equipment_count==2);
    char root[]=QSAOW3_TEST_TMP "art3-mod-store-XXXXXX";assert(mkdtemp(root));assert(write(root,s));State loaded;assert(load(root,loaded)==1&&!memcmp(&s,&loaded,sizeof(s)));
    before=s;assert(equip_modification(s,11,0));auto proposed=s;char missing[640];snprintf(missing,sizeof(missing),"%s/missing",root);assert(!write(missing,s)&&!memcmp(&s,&proposed,sizeof(s)));
    assert(load(root,s)==1&&!memcmp(&s,&before,sizeof(s)));assert(equip_modification(s,11,0)&&write(root,s)&&load(root,loaded)==1&&equipped_modification(loaded,11)==0&&equipped_modification(loaded,15)==151&&stock_amount(loaded,17,110)==11);
    auto bad=loaded;bad.modification_equipment[0].modification=999;bad.checksum=checksum(bad);assert(!valid(bad));bad=loaded;bad.modification_equipment[1]=bad.modification_equipment[0];bad.modification_equipment_count=2;bad.checksum=checksum(bad);assert(!valid(bad));
    char path[640];snprintf(path,sizeof(path),"%s/legacy",root);bad=seed(3,1,0,9);assert(grant_stock(bad,17,110,7));bad.version=11;bad.size=kV11Size;bad.checksum=checksum_bytes(bad,bad.size);FILE* f=fopen(path,"wb");assert(f&&fwrite(&bad,1,bad.size,f)==bad.size);fclose(f);assert(!read(path,loaded));unlink(path);
    f=fopen(argv[2],"wb");assert(f&&fwrite(&s,1,sizeof(s),f)==sizeof(s));fclose(f);
    for(int slot=0;slot<2;++slot){snprintf(path,sizeof(path),"%s/local-progress.%d",root,slot);unlink(path);}assert(!rmdir(root));
    puts("PASS: v11 read-only migration, permanent cumulative days, no consumption, one equipment per unit, free switch/base restore, other-unit isolation, failed-write cold restore, owned/duplicate/corrupt/backdated guards; no runtime gate/effect bypass");
}
