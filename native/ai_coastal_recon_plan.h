// Select a reachable land vantage; native movement and visibility own effects.
#pragma once
#include <stdint.h>
#include <stdlib.h>
#include <limits.h>
namespace ai_coastal_recon_plan {
struct Point {int x=0,y=0;};
inline bool select(const uint8_t* transit,int w,int h,Point start,int ax,int ay,int bw,int bh,Point& out){
    if(!transit||w<1||h<1||w>128||h>128||start.x<0||start.y<0||start.x>=w||start.y>=h||bw<1||bh<1||bw>16||bh>16||bw>w||bh>h||ax<0||ay<0||ax>w-bw||ay>h-bh)return false;
    int n=w*h,origin=start.y*w+start.x;uint8_t level=transit[origin];if(!level)return false;
    int* distance=(int*)malloc(n*sizeof(int));int* queue=(int*)malloc(n*sizeof(int));
    if(!distance||!queue){free(distance);free(queue);return false;}
    for(int i=0;i<n;++i)distance[i]=-1;
    int head=0,tail=1;queue[0]=origin;distance[origin]=0;
    while(head<tail){int p=queue[head++],x=p%w,y=p/w;
        const int next[]={x?p-1:-1,x+1<w?p+1:-1,y?p-w:-1,y+1<h?p+w:-1};
        for(int q:next)if(q>=0&&distance[q]<0&&transit[q]==level){distance[q]=distance[p]+1;queue[tail++]=q;}}
    int best=-1,score=INT_MAX,steps=INT_MAX;
    // Twice the rectangle center avoids asymmetric integer rounding.
    int cx=2*ax+bw-1,cy=2*ay+bh-1;
    for(int i=0;i<n;++i)if(distance[i]>=0){int dx=2*(i%w)-cx,dy=2*(i/w)-cy,d=dx*dx+dy*dy;
        if(d<score||(d==score&&distance[i]<steps)){best=i;score=d;steps=distance[i];}}
    // A remote island is not a coastal vantage for this footprint.
    bool ok=best>=0&&score<=36;
    if(ok)out={best%w,best/w};free(distance);free(queue);return ok;
}
}
