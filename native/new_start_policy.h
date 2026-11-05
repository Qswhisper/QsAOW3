#pragma once
#include <stdint.h>
#include <stdio.h>
#include <string.h>
namespace new_start_policy {
inline bool tutorial_key(const char* key){
    if(!key||!strncmp(key,"art3.local.",11))return false;char lower[192];size_t n=strlen(key);if(n>=sizeof(lower))return false;
    for(size_t i=0;i<=n;++i)lower[i]=key[i]>='A'&&key[i]<='Z'?key[i]+32:key[i];
    return strstr(lower,"tutorial")||!strncmp(lower,"firstmission",12)||!strcmp(lower,"firstgamerun");
}
inline uint32_t scope_hash(const char* root){uint32_t h=2166136261u;for(const unsigned char* p=(const unsigned char*)root;*p;++p)h=(h^*p)*16777619u;return h;}
inline bool scoped_key(const char* root,uint64_t account,bool isolated,const char* key,char* out,size_t size){
    if((!account&&!isolated)||!tutorial_key(key))return false;
    return snprintf(out,size,"art3.local.%08x.%llu.%s",scope_hash(root),(unsigned long long)account,key)<int(size);
}
}
