// One opt-in read of the original retained GAI control-point definitions.
// Does not initialize a battle or use/alter player progression.
#pragma once
namespace control_point_content_audit {
using local_research_runtime::Roots;
static void* get(void* p,int o){return p?*(void**)((char*)p+o):nullptr;}
static void coordinates(FILE* f,void* array){
    fputc('[',f);size_t n=array?array_length(array):0;
    for(size_t i=0;i<n&&i<128;++i){if(i)fputc(',',f);void* p=((void**)((char*)array+32))[i];fputc('{',f);bool comma=false;
        if(p)for(void* k=object_class(p);k;k=class_parent(k)){void* iter=nullptr;while(void* field=class_fields(k,&iter)){
            if(field_flags(field)&(0x10|0x40))continue;int offset=field_offset(field),code=type_type(field_type(field));
            if(offset<16||offset>128)continue;int value=0;if(code==2)value=*(bool*)((char*)p+offset);
            else if(code==4)value=*(int8_t*)((char*)p+offset);else if(code==6)value=*(int16_t*)((char*)p+offset);
            else if(code==8)value=*(int*)((char*)p+offset);else continue;
            if(comma)fputc(',',f);fprintf(f,"\"%s\":%d",field_name(field),value);comma=true;}}
        fputc('}',f);}
    fputc(']',f);
}
static bool inspect(FILE* f,void* battle,int faction,int mission,int& flags,int& bars){
    void* gai=get(battle,0x140),*commands=get(gai,0x30);int n=commands?*(int*)((char*)commands+0x18):0;if(n<1||n>100000)return false;
    for(int i=0;i<n;++i){void* command=list_item(commands,i);if(!command)return false;const char* name=class_name(object_class(command));
        bool flag=!strcmp(name,"GAICommandFlag"),bar=!strcmp(name,"GAICommandProgressBarFlags");if(!flag&&!bar)continue;
        fprintf(f,"{\"phase\":\"original_control_point\",\"faction\":%d,\"mission\":%d,\"index\":%d,\"class\":\"%s\",\"side\":%d",faction,mission,i,name,*(int*)((char*)command+0x18));
        if(flag){++flags;for(auto pair:{std::pair<const char*,int>{"type",0x30},{"model",0x34},{"x",0x3c},{"y",0x40},{"bonus",0x44},{"sight",0x48},{"cp",0x4c},{"xp",0x50},{"attack",0x54},{"train",0x58},{"constr",0x5c},{"taken",0x60},{"title",0x64},{"hint",0x68},{"hintInactive",0x6c},{"points",0x70},{"totalPoints",0x74},{"interval",0x78},{"captureRadius",0x90},{"attackBonusDisabled",0x94}}){
            int o=pair.second;int v=o==0x30||o==0x48||o==0x60?*(int8_t*)((char*)command+o):o==0x94?*(bool*)((char*)command+o):*(int*)((char*)command+o);fprintf(f,",\"%s\":%d",pair.first,v);}
            fprintf(f,",\"counters\":");coordinates(f,get(command,0x80));
        }else{++bars;for(auto pair:{std::pair<const char*,int>{"amount",0x30},{"showLeader",0x40},{"ignoreFlags",0x41},{"ignoreAmount",0x42},{"hint",0x44},{"header",0x48},{"incrementMode",0x4c}}){
            int o=pair.second,v=o>=0x40&&o<=0x42?*(bool*)((char*)command+o):*(int*)((char*)command+o);fprintf(f,",\"%s\":%d",pair.first,v);}
            fprintf(f,",\"alliances\":");coordinates(f,get(command,0x38));}
        fprintf(f,"}\n");
    }return true;
}
static __attribute__((noinline)) void run(){
    if(!progress_ready||access(DATA_ROOT "control-point-content-audit.request",F_OK))return;unlink(DATA_ROOT "control-point-content-audit.request");
    local_progress::StateBuffer before;if(!before.data)return;*before.data=progress;
    FILE* f=fopen(DATA_ROOT "control-point-content-audit.jsonl","w");if(!f)return;chmod(DATA_ROOT "control-point-content-audit.jsonl",0644);
    bool ok=true;int candidates=0,accepted=0,flags=0,bars=0;
    for(const auto& route:local_scenarios){Roots roots;void* input=nullptr,*battle=nullptr;
        if(route.embedded){input=campaign_seeds::decode(route.file,roots);battle=get(input,0x10);}
        else{input=read_scenario_message(route.file,5040,"@control-point-original-input");battle=get(input,0x20);}
        bool pass=input&&battle&&inspect(f,battle,route.faction,route.recorded,flags,bars);ok&=pass;++candidates;if(pass)++accepted;
        fprintf(f,"{\"phase\":\"original_row\",\"faction\":%d,\"id\":%d,\"pass\":%s}\n",route.faction,route.recorded,pass?"true":"false");}
    bool unchanged=!memcmp(before.data,&progress,sizeof(progress));
    fprintf(f,"{\"summary\":true,\"pass\":%s,\"candidates\":%d,\"accepted\":%d,\"original_flag_commands\":%d,\"original_flag_bars\":%d,\"player_state_unchanged\":%s,\"product_rule_implemented\":false,\"player_save_writes\":0,\"battle_runs\":0}\n",ok&&candidates==55&&unchanged?"true":"false",candidates,accepted,flags,bars,unchanged?"true":"false");fclose(f);
}
}
