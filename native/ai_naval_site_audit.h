// One opt-in read-only geometry survey after native base initialization.
// No placement commands, grants, fog edits or synthesized production success.
#pragma once
static bool naval_site_audit(void* battle,int now){
    bool recon_sample=!access(DATA_ROOT "ai-naval-recon-audit.enabled",F_OK);
    if(!naval_test||now<(recon_sample?4500:1000)||access(DATA_ROOT "ai-naval-site-audit.enabled",F_OK))return false;
    void* s=side(battle,2);if(!s||!naval_map(battle))return !fail("naval-site-audit-map");
    void* type=local_dictionary_item(*(void**)((char*)s+0xf8),base_id(s)+11);
    void* yard=local_dictionary_item(*(void**)((char*)s+0xf8),base_id(s)+3);
    int bw=type?*(int8_t*)((char*)type+0x29):0,bh=type?*(int8_t*)((char*)type+0x2a):0;
    if(!type||!yard||bw<1||bh<1||bw>16||bh>16)return !fail("naval-site-audit-prototype");
    Roots roots;void* command=ai_lab::command("AICommBuSet",now,roots,2);ProducerSiteProbe probe;
    if(!command||!probe.init(command,battle,s,type))return !fail("naval-site-audit-probe");
    native_battle_grid::View grid;if(!grid.init(battle))return !fail("naval-site-audit-grid");
    auto tile=[&](int x,int y)->void*{return grid.cell(x,y);};
    void* alliances=*(void**)((char*)battle+0x170);int alliance=*(int8_t*)((char*)s+0x2f8);
    if(!alliances||alliance<0||(size_t)alliance>=array_length(alliances))return !fail("naval-site-audit-alliance");
    void* a=((void**)((char*)alliances+32))[alliance];bool matrix_ok=true;
    auto bit=[&](int offset,int x,int y)->int{
        void* matrix=a?*(void**)((char*)a+offset):nullptr;
        if(!matrix||(size_t)(x>>5)>=array_length(matrix)){matrix_ok=false;return -1;}
        void* column=((void**)((char*)matrix+32))[x>>5];
        if(!column||(size_t)y>=array_length(column)){matrix_ok=false;return -1;}
        return (((uint32_t*)((char*)column+32))[y]>>(x&31))&1;
    };
    auto area=reinterpret_cast<bool(*)(void*,void*,void*,int,int,void*)>(image_base+0x47e7f08);
    auto geometry=reinterpret_cast<bool(*)(void*,void*,int,int,int16_t,void*)>(image_base+0x4922a7c);
    FILE* f=fopen(DATA_ROOT "ai-naval-site-audit.jsonl","w");if(!f)return !fail("naval-site-audit-report");
    chmod(DATA_ROOT "ai-naval-site-audit.jsonl",0644);
    fprintf(f,"{\"record\":\"input\",\"chapter\":%d,\"tick\":%d,\"width\":%d,\"height\":%d,\"side\":2,\"alliance\":%d,\"type\":%d,\"w\":%d,\"h\":%d,\"read_only_probe\":%s,\"read_only_site_calls\":true,\"production_test\":false}\n",naval_fixture_chapter,now,naval_water.width,naval_water.height,alliance,base_id(s)+11,bw,bh,recon_sample?"false":"true");
    const auto& recon=naval_state(s);
    fprintf(f,"{\"record\":\"recon\",\"enabled\":%s,\"unit\":%d,\"ordered\":%s,\"moved\":%s,\"explored\":%s,\"failures\":%d,\"vantage\":[%d,%d],\"terrain_or_fog_edits\":0}\n",recon_sample?"true":"false",recon.scout,recon.scout_ordered?"true":"false",recon.scout_moved?"true":"false",recon.scout_explored?"true":"false",recon.scout_failures,recon.vantage.x,recon.vantage.y);
    for(int y=0;y<naval_water.height;++y){
        fprintf(f,"{\"record\":\"row\",\"y\":%d,\"terrain\":\"",y);
        for(int x=0;x<naval_water.width;++x)fprintf(f,"%d",*(uint8_t*)((char*)tile(x,y)+0x20));
        fputs("\",\"height\":[",f);for(int x=0;x<naval_water.width;++x)fprintf(f,"%s%d",x?",":"",*(int8_t*)((char*)tile(x,y)+0x22));
        fputs("],\"matrix30\":\"",f);for(int x=0;x<naval_water.width;++x)fprintf(f,"%d",bit(0x30,x,y));
        fputs("\",\"matrix38\":\"",f);for(int x=0;x<naval_water.width;++x)fprintf(f,"%d",bit(0x38,x,y));
        fputs("\",\"area_shipyard\":\"",f);for(int x=0;x<naval_water.width;++x)fputc(area(battle,s,type,x,y,nullptr)?'1':'0',f);
        fputs("\",\"area_yard\":\"",f);for(int x=0;x<naval_water.width;++x)fputc(area(battle,s,yard,x,y,nullptr)?'1':'0',f);
        fputs("\"}\n",f);
    }
    auto cell_check=reinterpret_cast<bool(*)(void*,void*,int,int,void*)>(image_base+0x4922500);
    // Exact original kind11/17 branch constant, evaluated by its original
    // generic decoder and metadata. It is terrain20, not height22.
    void** constant_class_slot=*(void***)(image_base+0x9719018);
    void* constant_class=constant_class_slot?*constant_class_slot:nullptr;
    void* constant_statics=constant_class?*(void**)((char*)constant_class+0xb8):nullptr;
    void* constant_blob=constant_statics?*(void**)((char*)constant_statics+0x3830):nullptr;
    void** constant_method_slot=*(void***)(image_base+0x9719028);
    void* constant_method=constant_method_slot?*constant_method_slot:nullptr;
    if(!constant_blob||!constant_method){fclose(f);return !fail("naval-site-audit-terrain-constant");}
    auto decode=reinterpret_cast<int(*)(void*,int,int,int,void*)>(image_base+0x52ab528);
    int expected_terrain=decode(constant_blob,0x6a0,0x3d,0x27428cbf,constant_method);
    fprintf(f,"{\"record\":\"original_terrain_contract\",\"shipyard_terrain20\":%d,\"source_rva\":\"49220bc\",\"Zk_checks_exit_only\":true,\"AK_true_rejects\":true}\n",expected_terrain);
    auto footprint_ready=[&](int x,int y,int w,int h,int terrain)->bool{
        for(int yy=y;yy<y+h;++yy)for(int xx=x;xx<x+w;++xx){void* c=tile(xx,yy);
            if(!*(bool*)((char*)c+0x40)||*(uint8_t*)((char*)c+0x20)!=terrain||!bit(0x38,xx,yy))return false;
        }return true;
    };
    int codes[16]={},counts[16]={},code_count=0,geometric=0,accepted=0,inside=0,wet_inner=0,wet_border=0,samples=0;bool code_ok=true;
    // First count geometry-only candidates. Select eight evenly spaced entries
    // in this same map, rather than issuing full-site calls at every anchor.
    int candidates=0;
    for(int y=1;y+bh<grid.height;++y)for(int x=1;x+bw<grid.width;++x){
        bool exits=false;for(int16_t angle:{0,90,180,270})exits|=geometry(battle,type,x,y,angle,nullptr);
        if(footprint_ready(x,y,bw,bh,expected_terrain)&&exits)++candidates;
    }
    auto guards=[&](void* prototype,int x,int y,int w,int h,const char* scope){
        int flag_false=0,ak_false=0,zero30=0,zero38=0,dynamic10=0,dynamic18=0,dynamic30=0;
        for(int yy=y-1;yy<=y+h;++yy)for(int xx=x-1;xx<=x+w;++xx){
            void* c=tile(xx,yy);bool ak=cell_check(battle,s,xx,yy,nullptr);
            bool flag=*(bool*)((char*)c+0x40);flag_false+=!flag;ak_false+=!ak;
            int b30=bit(0x30,xx,yy),b38=bit(0x38,xx,yy);zero30+=b30==0;zero38+=b38==0;
            bool d10=*(void**)((char*)c+0x10)!=nullptr,d18=*(void**)((char*)c+0x18)!=nullptr,d30=*(void**)((char*)c+0x30)!=nullptr;
            dynamic10+=d10;dynamic18+=d18;dynamic30+=d30;
            fprintf(f,"{\"record\":\"guard_cell\",\"scope\":\"%s\",\"anchor_x\":%d,\"anchor_y\":%d,\"x\":%d,\"y\":%d,\"inner\":%s,\"terrain20\":%d,\"byte21\":%d,\"height22\":%d,\"flag40\":%s,\"native_AK\":%s,\"matrix30\":%d,\"matrix38\":%d,\"dynamic10\":%s,\"dynamic18\":%s,\"dynamic30\":%s}\n",scope,x,y,xx,yy,xx>=x&&xx<x+w&&yy>=y&&yy<y+h?"true":"false",*(uint8_t*)((char*)c+0x20),*(int8_t*)((char*)c+0x21),*(int8_t*)((char*)c+0x22),flag?"true":"false",ak?"true":"false",b30,b38,d10?"true":"false",d18?"true":"false",d30?"true":"false");
        }
        fprintf(f,"{\"record\":\"guard_totals\",\"scope\":\"%s\",\"x\":%d,\"y\":%d,\"flag40_false\":%d,\"native_AK_false\":%d,\"matrix30_zero\":%d,\"matrix38_zero\":%d,\"dynamic10\":%d,\"dynamic18\":%d,\"dynamic30\":%d}\n",scope,x,y,flag_false,ak_false,zero30,zero38,dynamic10,dynamic18,dynamic30);
    };
    int candidate_index=0,next_sample=0;
    // Match the native positive-anchor rectangle plus its original extra=1;
    // border-excluded anchors are recorded as outside this diagnostic scope.
    for(int y=1;y+bh<naval_water.height&&!probe.refused&&code_ok;++y)for(int x=1;x+bw<naval_water.width&&!probe.refused&&code_ok;++x){
        bool water=true,border=true;int unseen=0;
        for(int yy=y-1;yy<=y+bh;++yy)for(int xx=x-1;xx<=x+bw;++xx){bool inner=xx>=x&&xx<x+bw&&yy>=y&&yy<y+bh;
            if(*(uint8_t*)((char*)tile(xx,yy)+0x20)!=2){border=false;if(inner)water=false;}
            if(inner&&!bit(0x38,xx,yy))++unseen;
        }
        wet_inner+=water;wet_border+=border;
        if(!footprint_ready(x,y,bw,bh,expected_terrain))continue;
        bool exits=false;for(int16_t angle:{0,90,180,270})exits|=geometry(battle,type,x,y,angle,nullptr);
        if(!exits)continue;
        int selected_index=candidates?next_sample*candidates/8:0;
        bool selected=next_sample<8&&candidate_index++>=selected_index;
        if(!selected)continue;
        ++next_sample;guards(type,x,y,bw,bh,"shipyard_anchor");
        for(int16_t angle:{0,90,180,270}){
            if(!geometry(battle,type,x,y,angle,nullptr))continue;++geometric;*(int16_t*)((char*)command+0x44)=angle;
            bool legal=probe.permits(x,y);int code=probe.last,k=0;while(k<code_count&&codes[k]!=code)++k;
            if(k==code_count){if(k==16){code_ok=false;break;}codes[code_count++]=code;}++counts[k];
            bool covered=area(battle,s,type,x,y,nullptr);accepted+=legal;inside+=legal&&covered;
            ++samples;fprintf(f,"{\"record\":\"site\",\"x\":%d,\"y\":%d,\"angle\":%d,\"code\":%d,\"area\":%s,\"inner_water\":%s,\"border_water\":%s,\"zero_matrix38_inner\":%d,\"own_unit_count\":%d,\"other_unit_count\":%d}\n",x,y,angle,code,covered?"true":"false",water?"true":"false",border?"true":"false",unseen,*(int*)((char*)probe.own+0x18),*(int*)((char*)probe.other+0x18));
            if(probe.refused)break;
        }
        if(probe.refused)break;
    }
    ProducerSiteProbe ground;
    if(!ground.init(command,battle,s,yard))code_ok=false;
    else{
        int w=*(int8_t*)((char*)yard+0x29),h=*(int8_t*)((char*)yard+0x2a);bool found=false;
        for(int y=1;w>0&&h>0&&y+h<grid.height&&!found;++y)for(int x=1;x+w<grid.width&&!found;++x){
            if(!footprint_ready(x,y,w,h,0)||!area(battle,s,yard,x,y,nullptr)||!geometry(battle,yard,x,y,0,nullptr))continue;
            found=true;guards(yard,x,y,w,h,"ground_control");*(int16_t*)((char*)command+0x44)=0;
            bool legal=ground.permits(x,y);fprintf(f,"{\"record\":\"ground_control\",\"x\":%d,\"y\":%d,\"code\":%d,\"accepted\":%s,\"own_unit_count\":%d,\"other_unit_count\":%d}\n",x,y,ground.last,legal?"true":"false",*(int*)((char*)ground.own+0x18),*(int*)((char*)ground.other+0x18));
        }
        code_ok&=found&&!ground.refused;
    }
    bool unchanged=!memcmp(&tech_before,&progress,sizeof(progress));
    bool recon_pass=recon.scout_ordered&&recon.scout_moved&&recon.scout_explored;
    fprintf(f,"{\"summary\":true,\"pass\":%s,\"read_only_probe\":%s,\"read_only_site_calls\":true,\"scouting_passed\":%s,\"production_passed\":false,\"tick\":%d,\"geometry\":%d,\"full_accepted\":%d,\"accepted_in_area\":%d,\"inner_water_rectangles\":%d,\"border_water_rectangles\":%d,\"codes\":[",matrix_ok&&code_ok&&!probe.refused&&unchanged&&(!recon_sample||recon_pass)?"true":"false",recon_sample?"false":"true",recon_pass?"true":"false",now,geometric,accepted,inside,wet_inner,wet_border);
    for(int j=0;j<code_count;++j)fprintf(f,"%s{\"code\":%d,\"count\":%d}",j?",":"",codes[j],counts[j]);
    fprintf(f,"],\"player_state_unchanged\":%s,\"player_match\":false,\"career_writes\":false}\n",unchanged?"true":"false");fclose(f);
    log("END naval-site-audit tick=%d read_only_site_calls=1 scouting_passed=%d full_accepted=%d inside_area=%d production_passed=0",now,recon_pass,accepted,inside);stopped=true;return true;
}
