#pragma once
namespace modification_contract_audit {
using namespace local_modifications;
static bool source_contract(int& models){
    Roots roots;void* live=service(),*copy=live?object_new(object_class(live)):nullptr;if(!roots.keep(copy))return false;
    void* ex=nullptr;void* args[]={get(live,0x10),get(live,0x20),get(live,0x18)};invoke_named(copy,".ctor",3,args,&ex);if(ex)return false;
    local_progress::StateBuffer storage;if(!storage.data)return false;auto& state=*storage.data;state=local_progress::seed(0,0,0,360);
    for(const auto& d:modification_content::definitions)if(d.faction==local_faction::active&&!local_progress::grant_stock(state,17,d.id,7))return false;
    void* cfg=configuration(state,local_faction::active,roots);void* a[]={cfg};if(!cfg)return false;invoke_named(copy,"SetModificationConfiguration",1,a,&ex);if(ex||get(copy,0x48)!=cfg)return false;
    for(const auto& d:modification_content::definitions)if(d.faction==local_faction::active){Roots row;int id=d.id;void* p[]={&id};void* found=invoke_named(copy,"GetModificationById",1,p,&ex),*days=invoke_named(copy,"GetDayCountById",1,p,&ex);
        void* set=local_supply::dictionary_list(get(cfg,0x10),d.unit),*list=get(set,0x18),*owned=nullptr;for(int i=0;list&&i<*(int*)((char*)list+0x18);++i){void* item=list_item(list,i);if(get(item,0x10)==local_mod_effects::original(id))owned=item;}
        void* k=find_class("com.geargames.aow.ugui.army.modifications","PrototypeModificationModel"),*model=k?object_new(k):nullptr;if(!row.keep(model)||!owned)return false;ref(model,"m_modification",owned);
        void* permanent=invoke_named(model,"get_IsPermanent",0,nullptr,&ex),*identity=invoke_named(model,"get_Id",0,nullptr,&ex),*unit=invoke_named(model,"get_PrototypeId",0,nullptr,&ex);
        if(ex||found!=local_mod_effects::original(id)||!days||*(int*)((char*)days+16)!=7||!permanent||!*(bool*)((char*)permanent+16)||!identity||*(int*)((char*)identity+16)!=id||!unit||*(int*)((char*)unit+16)!=d.unit)return false;++models;
    }return true;
}
static bool mine_contract(int& migrated){
    preplaced_research_audit::Fixture fixture;if(!fixture.init("fmc1m"))return false;local_research_runtime::Roots roots;
    auto* c=local_mod_effects::compiled(1112);void* type=get(c?c->definition:nullptr,0x38),*side=fixture.side;if(!type)return false;
    void* units=local_mod_effects::clone_dictionary(get(side,0xf0),get(side,0xf0),roots),*mines=local_mod_effects::clone_dictionary(get(side,0x100),get(side,0x100),roots),*buildings=local_mod_effects::clone_dictionary(get(side,0xf8),get(side,0xf8),roots);
    if(!units||!mines||!buildings||!local_mod_effects::add(units,111,type))return false;
    void* definitions=local_career::list("BATTLE","mineTypes");for(int i=0;definitions&&i<*(int*)((char*)definitions+0x18);++i){void* v=list_item(definitions,i);if(!local_mod_effects::add(mines,*(int*)((char*)v+0x10),v))return false;}
    ref(side,"$gg",units);ref(side,"$Gg",buildings);ref(side,"$hg",mines);ref(side,"$Eg",get(type,0x20));
    local_progress::StateBuffer storage;if(!storage.data)return false;auto& state=*storage.data;state=local_progress::seed(0,0,0,360);
    if(!local_progress::grant_stock(state,17,1112,7)||!local_progress::equip_modification(state,111,1112))return false;void* prepared=local_research_runtime::clone(side,roots);
    if(!prepared||!local_mod_effects::prepare_side(prepared,state))return false;
    void* grid_field=class_field_from_name(object_class(fixture.battle),"$hB"),*grid_class=grid_field?class_from_type(field_type(grid_field)):nullptr;
    void* row_class=grid_class?class_element(grid_class):nullptr,*mine_class=find_class("com.geargames.aow.entities","Mine");void* grid=row_class?array_new(row_class,1):nullptr,*row=mine_class?array_new(mine_class,8):nullptr;
    if(!roots.keep(grid)||!roots.keep(row)||!fixture.array_put(grid,0,row))return false;ref(fixture.battle,"$hB",grid);
    void* foreign=local_research_runtime::clone(side,roots);if(!foreign)return false;void* objects[8]={},*old_types[8]={};int count=0;
    for(int i=0;i<c->count;++i){const auto& g=c->groups[i];if(g.key.kind!=4)continue;void* old=local_dictionary_item(mines,g.key.entity);bool known=false;for(int j=0;j<count;++j)if(old_types[j]==old)known=true;if(known)continue;
        for(int owner=0;owner<2;++owner){Roots one;void* mine=object_new(mine_class);if(!one.keep(mine)||count==8)return false;ref(mine,"type",old);ref(mine,"side",owner?foreign:side);*(int16_t*)((char*)mine+0x2a)=13;objects[count]=mine;old_types[count]=old;if(!fixture.array_put(row,count++,mine))return false;}}
    preplaced_research::Migration migration;if(count!=6||!migration.mines(fixture.battle,side,prepared)||migration.mine_links!=3)return false;migration.commit();
    for(int i=0;i<count;++i){void* expected=i%2?old_types[i]:local_dictionary_item(get(prepared,0x100),*(int*)((char*)old_types[i]+0x10));if(get(objects[i],0x18)!=expected||*(int16_t*)((char*)objects[i]+0x2a)!=13)return false;}
    migrated=migration.mine_links;return true;
}
static void run(){
    if(!progress_ready||access(DATA_ROOT "modification-contract-audit.request",F_OK))return;unlink(DATA_ROOT "modification-contract-audit.request");
    local_progress::StateBuffer backup;if(!backup.data)return;*backup.data=progress;int models=0,mines=0;bool source=source_contract(models),mine=mine_contract(mines),isolated=!memcmp(backup.data,&progress,sizeof(progress));
    FILE* f=fopen(DATA_ROOT "modification-contract-audit.jsonl","w");if(!f)return;chmod(DATA_ROOT "modification-contract-audit.jsonl",0644);
    fprintf(f,"{\"summary\":true,\"pass\":%s,\"original_source\":%s,\"permanent_native_models\":%d,\"mine_contract\":%s,\"own_mine_links\":%d,\"player_state_unchanged\":%s,\"player_save_writes\":0,\"player_equips\":0,\"battle_runs\":0}\n",source&&mine&&isolated?"true":"false",source?"true":"false",models,mine?"true":"false",mines,isolated?"true":"false");fclose(f);
}
}
