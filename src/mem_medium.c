/******************************************************
 * Copyright Grégory Mounié 2018                      *
 * This code is distributed under the GLPv3+ licence. *
 * Ce code est distribué sous la licence GPLv3+.      *
 ******************************************************/

#include <stdint.h>
#include <assert.h>
#include "mem.h"
#include "mem_internals.h"

unsigned int puiss2(unsigned long size) {
    unsigned int p=0;
    size = size -1; // allocation start in 0
    while(size) {  // get the largest bit
	p++;
	size >>= 1;
    }
    if (size > (1 << p))
	p++;
    return p;
}

void split(int n){
    printf("split %d\n", n);
    if (n == FIRST_ALLOC_MEDIUM_EXPOSANT + arena.medium_next_exponant){
        mem_realloc_medium();
        return;

    }
    if (arena.TZL[n+1] == NULL){
        split(n+1);
    }

    void * new_head = arena.TZL[n + 1];
    arena.TZL[n + 1] = *((void**) new_head); //Remove new_head from linked list
    void *buddy = (void *)((intptr_t)new_head ^ (1 << (n)));
    assert((char *) buddy - (char*) new_head == (1<< (n)));
    arena.TZL[n] = new_head; //Insert new head and its buddy to the linked list
    *((void **) new_head) = buddy; // ie next_addr de head = buddy
    *((void **) buddy) = NULL;





}

void * emalloc_medium(unsigned long size)
{
    assert(size < LARGEALLOC);
    assert(size > SMALLALLOC);

    int idx = puiss2(size + 32);

    if (arena.TZL[idx] == NULL){ // Split
        split(idx);
    }
    //block found
    // printf("arena.tzl[idx] : %p", arena.TZL[idx]);
    assert(arena.TZL[idx] != NULL);
    void * head = arena.TZL[idx]; // Pointeur vers le premier élément de la liste 
    arena.TZL[idx] = *((void**)head); // Avance au prochain block
    void* user_ptr = mark_memarea_and_get_user_ptr(head, size + 32, MEDIUM_KIND);
    return user_ptr;
}

void * min(void * a, void * b) {
    return (a < b) ? a : b;
}
void merge_blocks(void * ptr_block, int idx){
    Alloc new_a = {.ptr = ptr_block, .size = (1 <<(idx + 1)), .kind = MEDIUM_KIND};
    *((void**) new_a.ptr) = arena.TZL[idx + 1];
    arena.TZL[idx + 1] = new_a.ptr;

    efree_medium(new_a);
}

void efree_medium(Alloc a) {
    int idx = puiss2(a.size);
    void * buddy = (void *)((intptr_t)a.ptr ^ (1 << idx));
    if (arena.TZL[idx] != NULL){
        void * prev = NULL;
        void * current = arena.TZL[idx];

        if (current == buddy){//buddy found in head of the linked list
            arena.TZL[idx] = *((void**)current);
            merge_blocks(min(a.ptr, buddy), idx);
            return;
        } 

        while (current != NULL){ //Search buddy in linked list
            prev = current;
            current = *((void**)current);
            if (current == buddy){ //buddy found
                if(prev == arena.TZL[idx]){
                    arena.TZL[idx] = *((void**)current);
                }
                else {
                *((void **) prev) = *((void**)current);
                }
                merge_blocks(min(a.ptr, buddy), idx);
                return;
            }
        }
    }
    // buddy not found
    *((void**)a.ptr) = arena.TZL[idx];
    arena.TZL[idx] = a.ptr;

}






