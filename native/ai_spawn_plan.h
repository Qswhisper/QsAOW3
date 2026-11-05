// Conservative local AI layout: HQ footprints and land transit are separate.
#pragma once
#include <stdint.h>
#include <stdlib.h>
#include <limits.h>
namespace ai_spawn_plan {
struct Point {int x=0,y=0;};
inline int distance(Point a,Point b){int x=a.x-b.x,y=a.y-b.y;return x*x+y*y;}
inline bool plan(const uint8_t* mask,int w,int h,int count,Point* out,const uint8_t* transit=nullptr,bool free_for_all=false){
    if(!mask||!out||w<20||h<20||w>128||h>128||(free_for_all?(count<3||count>6):(count!=2&&count!=4&&count!=6)))return false;
    int n=w*h;int* labels=(int*)calloc(n,sizeof(int));int* queue=(int*)malloc(n*sizeof(int));
    uint8_t* roomy=(uint8_t*)calloc(n,1);if(!labels||!queue||!roomy){free(labels);free(queue);free(roomy);return false;}
    if(!transit)transit=mask;int serial=0,best=0,size=0,best_eligible=0;
    for(int i=0;i<n;++i)if(transit[i]&&!labels[i]){
        int head=0,tail=1;queue[0]=i;labels[i]=++serial;
        while(head<tail){int p=queue[head++],x=p%w,y=p/w;
            const int next[]={x?p-1:-1,x+1<w?p+1:-1,y?p-w:-1,y+1<h?p+w:-1};
            for(int q:next)if(q>=0&&transit[q]==transit[p]&&!labels[q]){labels[q]=serial;queue[tail++]=q;}}
        int eligible=0;for(int q=0;q<tail;++q)if(mask[queue[q]])++eligible;
        if(eligible>best_eligible||(eligible==best_eligible&&tail>size)){size=tail;best=serial;best_eligible=eligible;}
    }
    int candidates=0;
    // Nearby buildable centers provide development room without demanding a
    // solid rectangle or making every transit corridor headquarters-wide.
    for(int y=0;y<h;++y)for(int x=0;x<w;++x){int p=y*w+x;if(!mask[p]||labels[p]!=best||!best)continue;
        int room=0;for(int dy=-4;dy<=4;++dy)for(int dx=-4;dx<=4;++dx){int px=x+dx,py=y+dy;
            if(px>=0&&py>=0&&px<w&&py<h&&mask[py*w+px]&&labels[py*w+px]==best)++room;}
        if(room>=16){roomy[p]=1;++candidates;}}
    Point result[6];bool ok=size>=200&&candidates>=count;
    if(ok){
        int chosen=-1,score=INT_MAX;
        // Stable low-corner anchor, then the farthest point on that same land.
        for(int i=0;i<n;++i)if(roomy[i]){int v=i%w+i/w;if(v<score){score=v;chosen=i;}}
        result[0]={chosen%w,chosen/w};chosen=-1;score=-1;
        for(int i=0;i<n;++i)if(roomy[i]){int v=distance(result[0],{i%w,i/w});if(v>score){score=v;chosen=i;}}
        result[1]={chosen%w,chosen/w};ok=score>=256;
        for(int seat=2;seat<count&&ok;++seat){chosen=-1;score=INT_MAX;int team=seat%2;
            if(free_for_all){score=-1;
                for(int i=0;i<n;++i)if(roomy[i]){Point p{i%w,i/w};int nearest=INT_MAX;
                    for(int old=0;old<seat;++old){int d=distance(p,result[old]);if(d<nearest)nearest=d;}
                    // Preserve the existing ten-cell minimum HQ clearance;
                    // maximize separation rather than hard-code map positions.
                    if(nearest>=100&&nearest>score){score=nearest;chosen=i;}}
                if(chosen<0)ok=false;else result[seat]={chosen%w,chosen/w};continue;
            }
            for(int i=0;i<n;++i)if(roomy[i]){Point p{i%w,i/w};int own=distance(p,result[team]),enemy=distance(p,result[1-team]);
                if(own>=enemy)continue;bool separated=true;
                for(int old=0;old<seat;++old)if(distance(p,result[old])<(old%2==team?100:256)){separated=false;break;}
                if(separated&&own<score){score=own;chosen=i;}}
            if(chosen<0)ok=false;else result[seat]={chosen%w,chosen/w};
        }
    }
    if(ok)for(int i=0;i<count;++i)out[i]=result[i];
    free(labels);free(queue);free(roomy);return ok;
}
}
