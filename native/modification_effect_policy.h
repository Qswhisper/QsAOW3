// Original client modification formula; scalar field conversion is explicit.
#pragma once
#include <stdint.h>
#include <math.h>
#include <string.h>
#include <limits.h>
#include "modification_content.generated.h"
namespace modification_effects {
struct Action {int operation=0,kind=0,entity=0;float value=0;char property[48]={};};
inline bool integer(const char*& p,int& out,bool signed_value){
    bool negative=signed_value&&*p=='-';if(negative)++p;if(*p<'0'||*p>'9')return false;int64_t value=0;
    while(*p>='0'&&*p<='9'){value=value*10+(*p++-'0');if(value>int64_t(INT_MAX)+negative)return false;}
    out=negative?int(-value):int(value);return true;
}
inline bool parse(const char* p,Action& output){
    if(!p)return false;Action next;int n=0;
    if(!strncmp(p,"addpct<",7)){next.operation=2;p+=7;}
    else if(!strncmp(p,"add<",4)){next.operation=0;p+=4;}
    else if(!strncmp(p,"set<",4)){next.operation=5;p+=4;}
    else return false;
    if(!integer(p,n,true)||*p++!='>'||*p++!='<')return false;next.value=float(n);
    const char* scopes[]={"u.","us.","w.","f.","mine."};bool found=false;
    for(int k=0;k<5;++k){size_t len=strlen(scopes[k]);if(!strncmp(p,scopes[k],len)){next.kind=k;p+=len;found=true;break;}}
    if(!found||!integer(p,next.entity,false)||*p++!='.')return false;
    int count=0;while((*p>='a'&&*p<='z')||*p=='_'){
        if(count==int(sizeof(next.property))-1)return false;next.property[count++]=*p++;}
    if(!count||*p++!='>'||*p)return false;output=next;return true;
}
inline const modification_content::Field* field(int kind,const char* name){for(const auto& f:modification_content::fields)if(f.kind==kind&&!strcmp(f.name,name))return &f;return nullptr;}
inline bool same(const Action& a,const Action& b){return a.kind==b.kind&&a.entity==b.entity&&!strcmp(a.property,b.property);}
inline bool calculate(float base,const Action* actions,int count,float& output){
    if(!isfinite(base)||!actions||count<1||count>128)return false;float sum=0,percent=0,override_value=0;bool override=false;
    for(int i=0;i<count;++i){const auto& a=actions[i];if(!same(actions[0],a)||!isfinite(a.value))return false;
        if(a.operation==0)sum+=a.value;else if(a.operation==2)percent+=a.value;else if(a.operation==5){override=true;override_value=a.value;}else return false;}
    // Native ModificationCalculator.Apply: last Set overrides, otherwise add
    // deltas first and sum percentages before one multiplication.
    float result=override?override_value:((base+sum)*(1.0f+percent/100.0f));if(!isfinite(result))return false;output=result;return true;
}
inline bool quantize(float value,const modification_content::Field& field,int& output){
    if(!isfinite(value)||(field.width!=1&&field.width!=2&&field.width!=4))return false;
    int bits=field.width*8;int64_t low=field.signed_value?-(int64_t(1)<<(bits-1)):0,high=(int64_t(1)<<(bits-field.signed_value))-1;
    if(double(value)<double(low)||double(value)>double(high)||double(value)>INT_MAX)return false;
    // Match the original client integer helper boundaries (FCVTZS). Never
    // reinterpret float bits as serialized integer fields or wrap overflow.
    int next=int(value);output=next;return true;
}
}
