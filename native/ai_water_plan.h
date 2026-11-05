// Water topology only; construction/movement remain original native commands.
#pragma once
#include <stdlib.h>
#include <limits.h>
#include <stdint.h>
namespace ai_water_plan {
struct Plan {
    Plan()=default;Plan(const Plan&)=delete;Plan& operator=(const Plan&)=delete;
    int width=0,height=0,count=0,largest=0;int* labels=nullptr;int* sizes=nullptr;
    ~Plan(){reset();}void reset(){free(labels);free(sizes);labels=sizes=nullptr;width=height=count=largest=0;}
    bool init(const uint8_t* terrain,int w,int h){
        reset();if(!terrain||w<1||h<1||w>128||h>128)return false;
        int n=w*h;labels=(int*)calloc(n,sizeof(int));sizes=(int*)calloc(n+1,sizeof(int));int* queue=(int*)malloc(n*sizeof(int));
        if(!labels||!sizes||!queue){free(queue);reset();return false;}width=w;height=h;
        for(int p=0;p<n;++p)if(terrain[p]==2&&!labels[p]){
            ++count;int head=0,tail=0;queue[tail++]=p;labels[p]=count;
            while(head<tail){int q=queue[head++],x=q%w,y=q/w;
                for(int k=0;k<4;++k){int xx=x+(k==0)-(k==1),yy=y+(k==2)-(k==3);
                    if(xx<0||yy<0||xx>=w||yy>=h)continue;int next=yy*w+xx;
                    if(terrain[next]==2&&!labels[next]){labels[next]=count;queue[tail++]=next;}}
            }sizes[count]=tail;if(tail>sizes[largest])largest=count;
        }free(queue);return true;
    }
    int label(int x,int y)const{return labels&&x>=0&&y>=0&&x<width&&y<height?labels[y*width+x]:0;}
    bool site(int x,int y)const{int component=label(x,y);return component&&sizes[component]>=64;}
    bool footprint(int x,int y,int w,int h)const{
        if(w<1||h<1||w>32||h>32||w>width||h>height||x<0||y<0||x>width-w||y>height-h||!site(x,y))return false;
        int component=label(x,y);for(int yy=y;yy<y+h;++yy)for(int xx=x;xx<x+w;++xx)if(label(xx,yy)!=component)return false;
        return true;
    }
    bool coastal_site(int x,int y)const{for(int dy=-4;dy<=4;++dy)for(int dx=-4;dx<=4;++dx)if(site(x+dx,y+dy))return true;return false;}
    bool nearest(int tx,int ty,int& out_x,int& out_y)const{
        int best=INT_MAX,bx=-1,by=-1;
        for(int y=1;y<height-1;++y)for(int x=1;x<width-1;++x){if(!site(x,y))continue;bool clear=true;
            for(int dy=-1;dy<=1&&clear;++dy)for(int dx=-1;dx<=1;++dx)if(label(x+dx,y+dy)!=label(x,y)){clear=false;break;}
            if(!clear)continue;int dx=x-tx,dy=y-ty,d=dx*dx+dy*dy;if(d<best){best=d;bx=x;by=y;}}
        if(bx<0)return false;out_x=bx;out_y=by;return true;
    }
    bool target(int x,int y,int tx,int ty,int& out_x,int& out_y)const{
        int component=label(x,y);if(!component)return false;
        int best=INT_MAX,bx=-1,by=-1;
        for(int yy=1;yy<height-1;++yy)for(int xx=1;xx<width-1;++xx){
            bool clear=true;for(int dy=-1;dy<=1&&clear;++dy)for(int dx=-1;dx<=1;++dx)if(label(xx+dx,yy+dy)!=component){clear=false;break;}
            if(!clear)continue;int dx=xx-tx,dy=yy-ty,distance=dx*dx+dy*dy;
            if(distance<best){best=distance;bx=xx;by=yy;}}
        if(bx<0)return false;out_x=bx;out_y=by;return true;
    }
};
}
