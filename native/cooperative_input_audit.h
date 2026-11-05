// One opt-in decoded-input audit before implementing original cooperative rules.
// Never initializes a battle, submits results, or touches progression/AI ledgers.
#pragma once
namespace cooperative_input_audit {
static void run(){
    if(access(DATA_ROOT "cooperative-input-audit.enabled",F_OK))return;
    FILE* file=fopen(DATA_ROOT "art3offline-cooperative-input.txt","w");
    if(!file)return;
    chmod(DATA_ROOT "art3offline-cooperative-input.txt",0644);
    void* message=read_scenario_message(DATA_ROOT "cooperative-input.ggs",5040,"@cooperative-input-audit");
    void* battle=message?*(void**)((char*)message+0x20):nullptr;
    if(!battle){fputs("DECODE_FAILED\n",file);fclose(file);return;}
    fprintf(file,"DECODE_OK type=%s mission=%d tick=%d progression_writes=0\n",class_name(object_class(message)),*(int*)((char*)battle+0x28),*(int*)((char*)message+0x30));
    visited_count=0;dump_object(file,battle,0,2);
    void* alliances=*(void**)((char*)battle+0x170);
    for(size_t i=0;alliances&&i<array_length(alliances)&&i<8;++i){
        void* alliance=((void**)((char*)alliances+32))[i];
        fprintf(file,"\nALLIANCE %zu\n",i);visited_count=0;dump_object(file,alliance,0,3);
    }
    void* sides=*(void**)((char*)battle+0x178);
    for(int id=0;id<=8;++id){
        void* side=local_dictionary_item(sides,id);if(!side)continue;
        fprintf(file,"\nSIDE %d\n",id);visited_count=0;dump_object(file,side,0,1);
        fprintf(file,"\nSIDE %d FRACTION\n",id);visited_count=0;dump_object(file,*(void**)((char*)side+0xd8),0,1);
    }
    void* gai=*(void**)((char*)battle+0x140);
    fprintf(file,"\nGAI\n");visited_count=0;dump_object(file,gai,0,2);
    void* commands=gai?*(void**)((char*)gai+0x30):nullptr;
    int count=commands?*(int*)((char*)commands+0x18):0;
    fprintf(file,"\nCOMMAND_COUNT %d\n",count);
    if(count>=0&&count<=10000)for(int i=0;i<count;++i){
        fprintf(file,"\nCOMMAND %d\n",i);visited_count=0;dump_object(file,list_item(commands,i),0,4);
    }
    fprintf(file,"\nWORKSPACE\n");visited_count=0;dump_object(file,*(void**)((char*)battle+0x138),0,4);
    fclose(file);LOG("Original cooperative input decoded for bounded audit; no battle or ledger mutation");
}
}
