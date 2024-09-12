/******************************************************
 * Copyright Grégory Mounié 2018                      *
 * This code is distributed under the GLPv3+ licence. *
 * Ce code est distribué sous la licence GPLv3+.      *
 ******************************************************/

#include <assert.h>
#include "mem.h"
#include "mem_internals.h"

void *
emalloc_small(unsigned long size)
{
    /* ecrire votre code ici */
    void* head = arena.chunkpool;

    if (head == NULL){
        unsigned long size = mem_realloc_small();
        int nb_chunks = size / 8; 
        int counter = 0;
        void ** current = arena.chunkpool;
        while (counter <= nb_chunks){
            *current = &current + CHUNKSIZE;
            current += CHUNKSIZE;
        }
    }
    else{
       arena.chunkpool += 12; 
    }
    
    return mark_memarea_and_get_user_ptr(head, CHUNKSIZE, SMALL_KIND);
}

void efree_small(Alloc a) {
    /* ecrire votre code ici */
}
