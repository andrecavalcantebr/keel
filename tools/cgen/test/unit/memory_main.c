#include <assert.h>
#include <stdint.h>
#include "tool/memory.h"
int main(void) {
    unsigned char bytes[257]; memset(bytes,0xa5,sizeof bytes);
    keel_arena a; assert(keel_arena_from_memory(&a,bytes+1,256));
    char original[]="abc";
    string view=str_str(original), copy=str_dup(&a,view);
    assert(view.len==3 && view.cap==4 && str_cstr(view)==original);
    assert(copy.len==3 && copy.cap==4 && copy.ptr!=original);
    original[0]='x'; assert(strcmp(copy.ptr,"abc")==0);
    char span[]={'a','\0','b'};
    string binary=str_dup(&a,(string){.ptr=span,.len=3});
    assert(binary.len==3 && memcmp(binary.ptr,span,3)==0 && binary.ptr[3]==0);
    string empty=str_dup(&a,(string){0});
    assert(empty.ptr && empty.len==0 && empty.cap==1 && empty.ptr[0]==0);
    uint64_t *numbers=CGEN_NEW(&a,uint64_t,3);
    assert(numbers && (uintptr_t)numbers%_Alignof(uint64_t)==0);
    assert(numbers[0]==0 && numbers[1]==0 && numbers[2]==0);
    size_t mark=a.top;
    assert(!str_dup(&a,(string){.ptr=original,.len=SIZE_MAX}).ptr);
    assert(!keel_arena_alloc_(&a,8,8,SIZE_MAX)); assert(a.top==mark);
    assert(!cgen_alloc(&a,512,1,1,false,"test exhaustion"));
    assert(a.top==mark && strcmp(copy.ptr,"abc")==0);
    puts("ok");
}
