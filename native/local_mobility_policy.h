#pragma once
#include "local_mobility_content.generated.h"
#include <string.h>
#include <stdint.h>
#include <algorithm>
namespace local_mobility {
inline const Row* find(int faction,int owner,int level){for(const auto& r:rows)if(r.faction==faction&&r.owner==owner&&r.level==level)return &r;return nullptr;}
inline bool whitespace(char c){return c==' '||c=='\n'||c=='\r'||c=='?';}
inline bool strip(const Row& r,const char* source,char* out,size_t capacity){
 if(!source||strcmp(source,r.description))return false;const char* start=strstr(source,"{#20013<");if(!start||strstr(start+1,"{#20013<"))return false;
 const char* end=strstr(start,">}");if(!end)return false;end+=2;
 size_t prefix=start-source,suffix=strlen(end);if(prefix+suffix>=capacity)return false;
 memcpy(out,source,prefix);memcpy(out+prefix,end,suffix+1);
 size_t n=strlen(out);while(n&&whitespace(out[n-1]))out[--n]=0;
 size_t left=0;while(out[left]&&whitespace(out[left]))++left;if(left)memmove(out,out+left,n-left+1);return true;
}
inline int apply(int base,int levels){return base>0?std::min(INT16_MAX,base+std::clamp(levels,0,6)):base;}
}
