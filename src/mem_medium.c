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
    if (n >= FIRST_ALLOC_MEDIUM_EXPOSANT + arena.medium_next_exponant){
        mem_realloc_medium();
    }

    if (arena.TZL[n] != NULL){
        void * new_head = arena.TZL[n];
        arena.TZL[n] = *((void**) new_head); //avance dans la liste
        void *buddy = (void *)((intptr_t)new_head ^ (1 << (n-1)));
        arena.TZL[n-1] = new_head; //Insert new head and its buddy to the linked list
        *((void **) new_head) = buddy; // ie next_addr de head = buddy
        *((void **) buddy) = NULL;

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

    while (1) { 
        if (arena.TZL[idx]){ // Block found
            void * head = arena.TZL[idx]; // Pointeur vers le premier élément de la liste 
            void * next_addr = *((void**)head);
            arena.TZL[idx] = next_addr;
            void* user_ptr = mark_memarea_and_get_user_ptr(head, (1<< idx), MEDIUM_KIND);
            return user_ptr;
        }
        split(idx + 1);
    }
}


void efree_medium(Alloc a) {
    /* ecrire votre code ici */
}


