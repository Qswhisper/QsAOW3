#include "../tests/test_paths.h"
#include "local_progress.h"
#include <pthread.h>
#include <cassert>
#include <cstdlib>
struct Input {const char* fixture;const char* root;bool passed=false;};
static void* work(void* pointer){
    auto& input=*(Input*)pointer;local_progress::StateBuffer state,restored;
    if(!state.data||!restored.data||!local_progress::read(input.fixture,*state.data))return nullptr;
    if(!local_progress::write(input.root,*state.data)||local_progress::load(input.root,*restored.data)!=1)return nullptr;
    if(memcmp(state.data,restored.data,sizeof(*state.data)))return nullptr;
    if(!local_progress::write(input.root,*restored.data)||local_progress::load(input.root,*state.data)!=1)return nullptr;
    input.passed=!memcmp(state.data,restored.data,sizeof(*state.data));return nullptr;
}
int main(int argc,char** argv){
    assert(argc==2);char root[]=QSAOW3_TEST_TMP "art3-stack-test-XXXXXX";assert(mkdtemp(root));Input input{argv[1],root};
    pthread_attr_t attr;assert(!pthread_attr_init(&attr)&&!pthread_attr_setstacksize(&attr,64*1024));pthread_t thread;
    assert(!pthread_create(&thread,&attr,work,&input));pthread_attr_destroy(&attr);assert(!pthread_join(thread,nullptr)&&input.passed);
    for(int slot=0;slot<2;++slot){char path[640];snprintf(path,sizeof(path),"%s/local-progress.%d",root,slot);unlink(path);}assert(!rmdir(root));
    puts("PASS: growth migration/current two-slot read-write-load on a 64 KiB worker stack; large save scratch buffers stay on heap");
}
