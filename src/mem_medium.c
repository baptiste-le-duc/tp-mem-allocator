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
    if (n == FIRST_ALLOC_MEDIUM_EXPOSANT + arena.medium_next_exponant){
        mem_realloc_medium(n);
    }

    if (arena.TZL[n] != NULL){
        int child_block_size = n / 2;
        void * head = arena.TZL[n];
        void * buddy = (void *)(((intptr_t) head) ^ child_block_size) ; // Get buddy address
        arena.TZL[n-1] = head; //Replace block in n-1 list
        *((void **) head) = buddy; // ie next_addr de head = buddy
        *(void **)buddy = NULL;
    }
    else {
        split(n+1);
    }
}


void * emalloc_medium(unsigned long size)
{
    assert(size < LARGEALLOC);
    assert(size > SMALLALLOC);

    int idx = puiss2(size);

    if (arena.TZL[idx]){ // Block found
        void * head = arena.TZL[idx]; // Pointeur vers le premier élément de la liste 
        void * next_addr = *((void**)head);
        
        void* user_ptr = mark_memarea_and_get_user_ptr(head, CHUNKSIZE, MEDIUM_KIND);
        arena.TZL[idx] = next_addr;
        return user_ptr;
    }

    split(idx + 1);
    
    return (void *) 0;
}


void efree_medium(Alloc a) {
    /* ecrire votre code ici */
}


