#pragma once
#include "ai_spawn_plan.h"
namespace control_point_plan {
using ai_spawn_plan::Point;
inline bool distances(const uint8_t* land,int w,int h,Point origin,int* out,int* queue){
    if(origin.x<0||origin.y<0||origin.x>=w||origin.y>=h)return false;int start=origin.y*w+origin.x;if(!land[start])return false;
    for(int i=0;i<w*h;++i)out[i]=-1;int head=0,tail=1;queue[0]=start;out[start]=0;
    while(head<tail){int p=queue[head++],x=p%w,y=p/w;int next[]={x?p-1:-1,x+1<w?p+1:-1,y?p-w:-1,y+1<h?p+w:-1};
        for(int q:next)if(q>=0&&land[q]==land[p]&&out[q]<0){out[q]=out[p]+1;queue[tail++]=q;}}
    return true;
}
inline bool plan(const uint8_t* mask,const uint8_t* land,int w,int h,const Point* bases,Point* out){
    if(!mask||!land||!bases||!out||w<20||h<20||w>128||h>128)return false;
    for(int i=0;i<2;++i)if(bases[i].x<0||bases[i].y<0||bases[i].x>=w||bases[i].y>=h||!mask[bases[i].y*w+bases[i].x])return false;
    int n=w*h;int* data=(int*)malloc(n*3*sizeof(int));if(!data)return false;int* a=data,*b=data+n,*queue=data+2*n;
    bool ok=distances(land,w,h,bases[0],a,queue)&&distances(land,w,h,bases[1],b,queue);Point result[3];
    for(int step=0;step<3&&ok;++step){int chosen=-1;int64_t best=INT64_MIN;
        for(int y=3;y<h-3;++y)for(int x=3;x<w-3;++x){int p=y*w+x;if(a[p]<8||b[p]<8)continue;
            // A flag is a landmark rather than a headquarters. Require its
            // conservative 3x3 ground area; do not demand an HQ build footprint.
            bool ground=true;for(int dy=-1;dy<=1&&ground;++dy)for(int dx=-1;dx<=1;++dx)if(land[(y+dy)*w+x+dx]!=land[p]){ground=false;break;}
            if(!ground)continue;
            int gap=abs(a[p]-b[p]);int span=a[bases[1].y*w+bases[1].x];if(span<1||gap>(span/3>6?span/3:6))continue;
            int separation=INT_MAX;for(int i=0;i<step;++i){int d=ai_spawn_plan::distance(result[i],{x,y});if(d<separation)separation=d;}
            if(step&&separation<64)continue;
            int64_t score=step?int64_t(separation)*4-gap*12-a[p]-b[p]:-int64_t(gap)*16-a[p]-b[p];
            if(chosen<0||score>best){best=score;chosen=p;}}
        if(chosen<0)ok=false;else result[step]={chosen%w,chosen/w};}
    if(ok)for(int i=0;i<3;++i)out[i]=result[i];free(data);return ok;
}
}
