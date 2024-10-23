/******************************************************
 * Copyright Grégory Mounié 2018                      *
 * This code is distributed under the GLPv3+ licence. *
 * Ce code est distribué sous la licence GPLv3+.      *
 ******************************************************/

#include <assert.h>
#include <stdint.h>
#include "mem.h"
#include "mem_internals.h"

void *emalloc_small(unsigned long size)
{
	MemArena *arena = get_arena();
	pthread_mutex_lock(&arena->lock);

	if (arena->chunkpool == NULL) {
		int size = mem_realloc_small();
		void **cell = arena->chunkpool;
		for (int i = CHUNKSIZE; i < size; i += CHUNKSIZE) {
			void *next_chunk_addr =
				(void *) ((char *) arena->chunkpool + i);
			*cell = next_chunk_addr;
			cell = next_chunk_addr;
		}
	}
	void **head = arena->chunkpool;
	void *next_addr = *head;
	void *user_ptr =
		mark_memarea_and_get_user_ptr(arena->chunkpool, CHUNKSIZE,
									  SMALL_KIND);
	arena->chunkpool = next_addr;

	pthread_mutex_unlock(&arena->lock);
	return user_ptr;
}

void efree_small(Alloc a)
{
	MemArena *arena = get_arena();
	pthread_mutex_lock(&arena->lock);

	void *old_head_addr = arena->chunkpool;
	arena->chunkpool = a.ptr;
	void **cell = arena->chunkpool;
	*cell = old_head_addr;
    
	pthread_mutex_unlock(&arena->lock);
}
