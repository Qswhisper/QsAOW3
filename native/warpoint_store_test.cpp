#include "../tests/test_paths.h"
#include "warpoint_store.h"
#include <cassert>
#include <cstring>
#include <cstdlib>
int main(){
    char root[]=QSAOW3_TEST_TMP "art3-warpoint-test-XXXXXX";assert(mkdtemp(root));
    auto progress=local_progress::seed(1000,30,100,9);assert(local_progress::write(root,progress));
    local_match_store::State game{};game.next=game.settled=25;game.outcome=local_match_store::Victory;game.reason=1;game.checksum=local_match_store::hash(game);
    bool changed=true;assert(warpoint_store::reconcile(progress,game,changed)&&!changed&&!progress.quest_warpoints); // Legacy has no retrospective rewards.
    assert(local_match_store::begin_pvp(game,true)&&local_match_store::write(root,game));
    assert(local_match_store::settle(game,26,local_match_store::Victory,1234,1,1,nullptr,nullptr));game.reserved=100;
    assert(local_match_store::write(root,game));
    assert(warpoint_store::reconcile(progress,game,changed)&&changed&&progress.quest_warpoints==100&&progress.warpoint_pvp_cursor==26);
    auto uncommitted=progress;
    // A failed wallet write leaves the previous durable cursor/balance, so a
    // cold retry reapplies this same immutable award once.
    char missing[640];snprintf(missing,sizeof(missing),"%s/missing",root);assert(!local_progress::write(missing,progress)&&!memcmp(&progress,&uncommitted,sizeof(progress)));
    assert(local_progress::load(root,progress)==1&&progress.quest_warpoints==0);
    assert(warpoint_store::reconcile(progress,game,changed)&&changed&&local_progress::write(root,progress));
    assert(local_progress::load(root,progress)==1&&warpoint_store::reconcile(progress,game,changed)&&!changed&&progress.quest_warpoints==100);
    assert(local_match_store::begin_pvp(game,false)&&local_match_store::write(root,game));
    assert(local_match_store::settle(game,27,local_match_store::Victory,1234,1,1,nullptr,nullptr)&&local_match_store::write(root,game));
    assert(warpoint_store::reconcile(progress,game,changed)&&!changed&&progress.quest_warpoints==100); // Training stays zero.
    auto invalid=game;invalid.reserved=100;invalid.checksum=local_match_store::hash(invalid);assert(!local_match_store::valid(invalid));
    assert(local_match_store::begin_pvp(game,true)&&local_match_store::write(root,game));
    assert(local_match_store::settle(game,28,local_match_store::Defeat,5000,1,2,nullptr,nullptr)&&local_match_store::write(root,game));
    assert(warpoint_store::reconcile(progress,game,changed)&&changed&&progress.quest_warpoints==100&&progress.warpoint_pvp_cursor==28&&local_progress::write(root,progress));
    invalid=game;invalid.settled=invalid.next=26;invalid.checksum=local_match_store::hash(invalid);auto before=progress;
    assert(!warpoint_store::reconcile(progress,invalid,changed)&&!memcmp(&progress,&before,sizeof(progress))); // Regressed ledger cannot reuse run IDs.
    assert(local_match_store::begin_pvp(game,true)&&local_match_store::write(root,game));
    assert(local_match_store::settle(game,29,local_match_store::Victory,9000,1,1,nullptr,nullptr));game.reserved=100;assert(local_match_store::write(root,game));
    progress.quest_warpoints=INT_MAX;before=progress;assert(!warpoint_store::reconcile(progress,game,changed)&&!memcmp(&progress,&before,sizeof(progress)));
    progress=local_progress::seed(17,3,4,5);progress.generation=UINT64_MAX;before=progress;
    assert(!local_progress::write(root,progress)&&!memcmp(&progress,&before,sizeof(progress)));
    // Exact v7/v8 prefixes migrate without rewriting the source or inventing a cursor.
    char path[640];snprintf(path,sizeof(path),"%s/legacy",root);
    for(int version=7;version<=8;++version){auto old=local_progress::seed(1690,30,197,9);old.generation=53;old.version=version;
        old.size=version==7?local_progress::kV7Size:local_progress::kV8Size;old.checksum=local_progress::checksum_bytes(old,old.size);
        FILE* f=fopen(path,"wb");assert(f&&fwrite(&old,1,old.size,f)==old.size);fclose(f);
        local_progress::State migrated{};assert(local_progress::read(path,migrated)&&migrated.version==local_progress::kVersion&&migrated.size==sizeof(migrated)&&migrated.generation==53&&migrated.credits==1690&&migrated.quest_warpoints==0&&migrated.warpoint_pvp_cursor==0);
        f=fopen(path,"rb");assert(f);uint32_t h[4];assert(fread(h,1,16,f)==16&&h[1]==unsigned(version)&&h[2]==old.size);fclose(f);
    }
    auto coop=game;coop.mode=3;coop.reserved2=1;coop.reserved=0;coop.checksum=local_match_store::hash(coop);assert(!local_match_store::valid(coop));
    unlink(path);for(int slot=0;slot<2;++slot){snprintf(path,sizeof(path),"%s/local-ai.%d",root,slot);unlink(path);snprintf(path,sizeof(path),"%s/local-progress.%d",root,slot);unlink(path);}
    assert(rmdir(root)==0);
    puts("PASS: war-point immutable result/source, no retrospective/training award, failed-write cold retry, duplicate/zero/overflow/regressed-run guards, v7/v8 read-only migration, coop isolation");
}
