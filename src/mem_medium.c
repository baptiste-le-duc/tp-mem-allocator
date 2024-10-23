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

// Split a block of size 2^(n+1) into two blocks of size 2^n
void split(int n) {
    MemArena *arena = get_arena();

    // If at the largest possible block size, allocate more memory
    if (n == FIRST_ALLOC_MEDIUM_EXPOSANT + arena->medium_next_exponant) {
        mem_realloc_medium();
        return;
    }

    // If no free blocks in the larger size, recursively split a larger block
    if (arena->TZL[n+1] == NULL) {
        split(n+1);
    }

    // Get the first block from the larger size list and remove it
    void * new_head = arena->TZL[n + 1];
    arena->TZL[n + 1] = *((void**) new_head);

    // Calculate buddy address
    void *buddy = (void *)((intptr_t)new_head ^ (1 << (n)));
    assert((char *) buddy - (char*) new_head == (1 << n));

    // Insert new head and its buddy into the free list for size 2^n
    arena->TZL[n] = new_head;
    *((void **) new_head) = buddy;
    *((void **) buddy) = NULL;
}

// Allocate a medium-sized block
void * emalloc_medium(unsigned long size) {
    MemArena *arena = get_arena();
    pthread_mutex_lock(&arena->lock);

    assert(size < LARGEALLOC && size > SMALLALLOC);

    // Get index for the appropriate block size
    int idx = puiss2(size + MARK_SIZE);

    // If no blocks available, split larger ones
    if (arena->TZL[idx] == NULL) {
        split(idx);
    }

    // Allocate block from the free list
    void * head = arena->TZL[idx];
    arena->TZL[idx] = *((void**)head); // Move to next block
    pthread_mutex_unlock(&arena->lock);
    return mark_memarea_and_get_user_ptr(head, (1<<idx), MEDIUM_KIND);
}

// Return the minimum of two pointers (address-wise)
void * min(void * a, void * b) {
    return (a < b) ? a : b;
}

// Merge two adjacent blocks back into a larger one
void merge_blocks(void * ptr_block, int idx) {
    Alloc new_a = {.ptr = ptr_block, .size = (1 << (idx + 1)), .kind = MEDIUM_KIND};
    
    efree_medium(new_a);  // Free the merged block
}

// Free a medium-sized block, and try to merge it with its buddy
void efree_medium(Alloc a) {
    MemArena *arena = get_arena();
    pthread_mutex_lock(&arena->lock);

    int idx = puiss2(a.size);
    void * buddy = (void *)((intptr_t)a.ptr ^ (1 << idx));

    // If at the maximum block size, add back to the free list
    if (idx == FIRST_ALLOC_MEDIUM_EXPOSANT + arena->medium_next_exponant) {
        *((void**) a.ptr) = arena->TZL[idx];
        arena->TZL[idx] = a.ptr;
        pthread_mutex_unlock(&arena->lock);
        return;
    }

    // Try to find the buddy in the free list to merge
    if (arena->TZL[idx] != NULL) {
        void * prev = NULL;
        void * current = arena->TZL[idx];

        // If buddy is at the head of the list
        if (current == buddy) {
            arena->TZL[idx] = *((void**)current);
            pthread_mutex_unlock(&arena->lock);
            merge_blocks(min(a.ptr, buddy), idx);
            return;
        }

        // Search for buddy in the list
        while (current != NULL) {
            prev = current;
            current = *((void**)current);
            if (current == buddy) {
                *((void **) prev) = *((void**)current);  // Remove buddy
                pthread_mutex_unlock(&arena->lock);
                merge_blocks(min(a.ptr, buddy), idx);     // Merge blocks
                
                return;
            }
        }
    }

    // Buddy not found, add the block to the free list
    *((void**)a.ptr) = arena->TZL[idx];
    arena->TZL[idx] = a.ptr;
    pthread_mutex_unlock(&arena->lock);
}







