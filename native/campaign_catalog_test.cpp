#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "campaign_catalog.h"
using namespace campaign_catalog;
int main(int argc,char** argv){
    assert(argc==2);const char* root=argv[1];char filename[512];
    snprintf(filename,sizeof(filename),"%soffline-campaign/catalog.bin",root);
    FILE* file=fopen(filename,"rb");assert(file);fseek(file,0,SEEK_END);long size=ftell(file);rewind(file);
    auto original=(char*)malloc(size);assert(original&&fread(original,1,size,file)==(size_t)size);fclose(file);
    auto bytes=(char*)malloc(size);assert(bytes);
    auto write=[&](const char* data){FILE* f=fopen(filename,"wb");assert(f);assert(fwrite(data,1,size,f)==(size_t)size);fclose(f);};
    Catalog good;assert(good.load(root)&&good.count>=5);assert(good.verify(good.entries[0]));
    int cases=0;
    auto reject=[&](){write(bytes);Catalog bad;assert(!bad.load(root)&&!bad.ready&&bad.count==0);++cases;};
    auto checksum=[&](){auto h=(Header*)bytes;h->hash=hash(bytes+sizeof(Header),size-sizeof(Header));};
    memcpy(bytes,original,size);((Header*)bytes)->count=257;reject();
    memcpy(bytes,original,size);((Header*)bytes)->hash^=1;reject();
    memcpy(bytes,original,size);auto entries=(DiskEntry*)(bytes+sizeof(Header));entries[1]=entries[0];checksum();reject();
    memcpy(bytes,original,size);entries[0].previous=entries[0].chapter;checksum();reject();
    memcpy(bytes,original,size);entries[0].previous=entries[1].chapter;checksum();reject();
    memcpy(bytes,original,size);strcpy(entries[0].path,"../local-progress.0");checksum();reject();
    memcpy(bytes,original,size);entries[0].size+=1;checksum();reject();
    memcpy(bytes,original,size);entries[0].hash^=1;checksum();reject();
    memcpy(bytes,original,size);((Header*)bytes)->version=3;reject();
    memcpy(bytes,original,size);entries[0].embedded=8;checksum();reject();
    memcpy(bytes,original,size);entries[0].embedded=3;checksum();reject();
    write(original);Catalog restored;assert(restored.load(root)&&restored.count==good.count);
    puts("PASS: valid catalogue fixtures; 11 malformed/corrupt/catalogue-source/flags cases refused atomically; original catalogue restored");
    free(bytes);free(original);return cases==11?0:1;
}
