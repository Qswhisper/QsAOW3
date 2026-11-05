// A bounded, checksummed catalogue of real original campaign input files.
#pragma once
namespace campaign_catalog {
struct DiskEntry {int32_t faction,chapter,recorded,previous,map,embedded,size;uint32_t hash;char path[96];};
struct Header {uint32_t magic,version,count,hash;};
static_assert(sizeof(DiskEntry)==128&&sizeof(Header)==16,"Campaign catalogue ABI changed");
inline uint32_t hash(const void* bytes,size_t size,uint32_t value=2166136261u){
    auto p=(const uint8_t*)bytes;for(size_t i=0;i<size;++i)value=(value^p[i])*16777619u;return value;}
struct Entry {int chapter,recorded,previous;char file[512];bool embedded;int faction,map,size;uint32_t hash;bool rebase_growth=false,recorded_presentation=false;};
struct Catalog {
    Entry entries[256]={};int count=0;bool ready=false;
    const Entry* begin()const{return entries;}
    const Entry* end()const{return entries+count;}
    bool verify(const Entry& e)const{
        FILE* f=fopen(e.file,"rb");if(!f)return false;
        uint32_t sum=2166136261u;int total=0;char bytes[8192];size_t n;
        while((n=fread(bytes,1,sizeof(bytes),f))){total+=(int)n;if(total>e.size){fclose(f);return false;}sum=hash(bytes,n,sum);}
        bool ok=!ferror(f)&&total==e.size&&sum==e.hash;fclose(f);return ok;
    }
    bool load(const char* root,const char* catalog="offline-campaign/catalog.bin"){
        if(ready)return true;char filename[512];snprintf(filename,sizeof(filename),"%s%s",root,catalog);
        FILE* f=fopen(filename,"rb");if(!f)return false;Header header{};DiskEntry disk[256];
        bool ok=fread(&header,1,sizeof(header),f)==sizeof(header)&&header.magic==0x43523341&&(header.version==1||header.version==2)&&header.count>0&&header.count<=256;
        if(ok)ok=fread(disk,sizeof(DiskEntry),header.count,f)==header.count&&fgetc(f)==EOF&&!ferror(f)&&hash(disk,sizeof(DiskEntry)*header.count)==header.hash;
        fclose(f);if(!ok)return false;
        Entry prepared[256];
        for(uint32_t i=0;i<header.count;++i){const auto& d=disk[i];
            // Only content-addressed files under the package's own data root.
            if((d.faction!=1&&d.faction!=2)||d.chapter<0||d.recorded<0||d.previous< -1||d.map<0||
               d.embedded<0||d.embedded>(header.version==1?1:7)||((d.embedded&1)&&(d.embedded&6))||d.size<32||d.size>16*1024*1024||d.path[95]||
               strlen(d.path)!=85||strncmp(d.path,"offline-campaign/",17)||strcmp(d.path+81,".ggs"))return false;
            for(int j=17;j<81;++j)if(!((d.path[j]>='0'&&d.path[j]<='9')||(d.path[j]>='a'&&d.path[j]<='f')))return false;
            for(uint32_t j=0;j<i;++j)if(d.faction==disk[j].faction&&(d.chapter==disk[j].chapter||d.recorded==disk[j].recorded))return false;
            prepared[i]={};auto& e=prepared[i];e.chapter=d.chapter;e.recorded=d.recorded;e.previous=d.previous;
            e.embedded=d.embedded&1;e.rebase_growth=d.embedded&2;e.recorded_presentation=d.embedded&4;e.faction=d.faction;e.map=d.map;e.size=d.size;e.hash=d.hash;
            if(snprintf(e.file,sizeof(e.file),"%s%s",root,d.path)>=(int)sizeof(e.file)||!verify(e))return false;
        }
        for(uint32_t i=0;i<header.count;++i){int next=i;unsigned walked=0;
            while(disk[next].previous!=-1){int found=-1;
                for(uint32_t j=0;j<header.count;++j)if(disk[j].faction==disk[next].faction&&disk[j].chapter==disk[next].previous)found=j;
                if(found<0||++walked>=header.count)return false;next=found;}}
        memcpy(entries,prepared,header.count*sizeof(Entry));count=header.count;ready=true;return true;
    }
};
}
