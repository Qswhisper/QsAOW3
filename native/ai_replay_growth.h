// Private preset from two real, supported first HQ research levels. No save edit.
#pragma once
namespace ai_replay_growth {
static bool prepare(void* battle){
    if(!offline_world_runtime::batch_fixture||!offline_world_runtime::batch_job.growth)return true;
    for(int id=1;id<=offline_world_runtime::batch_job.count;++id){
        void* side=local_duel::side(battle,id),*fraction=side?*(void**)((char*)side+0xd8):nullptr;int faction=fraction?*(int*)((char*)fraction+0x10):0;
        void* config=saved(faction==1?"CONFEDERATION":"RESISTANCE");void* field=config?class_field_from_name(object_class(config),"upgradeLevels"):nullptr;
        void* levels=field?*(void**)((char*)config+field_offset(field)):nullptr,*owner=nullptr,*hp_level=nullptr,*income_level=nullptr;
        research_effects::Delta hp,income;
        for(int i=0;levels&&i<*(int*)((char*)levels+0x18);++i){void* level=list_item(levels,i),*up=level?*(void**)((char*)level+0x48):nullptr;
            if(!up||strcmp(class_name(object_class(up)),"UpgradeBuilding"))continue;
            void* candidate=*(void**)((char*)up+0x48);if(!candidate||*(int8_t*)((char*)candidate+0x30)!=1)continue;
            char desc[4096];research_effects::Delta d;
            if(!local_research_runtime::ascii(*(void**)((char*)level+0x18),desc,sizeof(desc))||!research_effects::parse(desc,d))continue;
            if(d.hp[0]>0&&(!hp_level||*(int*)((char*)level+0x10)<*(int*)((char*)hp_level+0x10))){hp_level=level;hp=d;owner=candidate;}
            if(d.income_base>0&&(!income_level||*(int*)((char*)level+0x10)<*(int*)((char*)income_level+0x10))){income_level=level;income=d;}
        }
        research_effects::Delta delta;
        if(!owner||!hp_level||!income_level||!research_effects::add(hp,income,delta)||!local_research_runtime::apply(battle,id,delta,owner))return false;
        auto m=local_research_runtime::metrics(side);if(!m.valid)return false;
        LOG("AI batch growth side=%d faction=%d hp_level=%d income_level=%d hp0=%d income=%d/%d original_supported_effects=1 career_writes=0",id,faction,*(int*)((char*)hp_level+0x10),*(int*)((char*)income_level+0x10),m.hp[0],m.base,m.increment);
    }return true;
}
}
