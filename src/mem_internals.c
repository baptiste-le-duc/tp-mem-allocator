/******************************************************
 * Copyright Grégory Mounié 2018-2022                 *
 * This code is distributed under the GLPv3+ licence. *
 * Ce code est distribué sous la licence GPLv3+.      *
 ******************************************************/

#include <sys/mman.h>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>


#include "mem.h"
#include "mem_internals.h"

unsigned long knuth_mmix_one_round(unsigned long in)
{
	return in * 6364136223846793005UL % 1442695040888963407UL;
}

void *mark_memarea_and_get_user_ptr(void *ptr, unsigned long size,
									MemKind k)
{
	if (size < MARK_SIZE || ptr == NULL) {
		return NULL;
	}
	unsigned long magic = knuth_mmix_one_round((unsigned long) ptr);
	magic = (magic & ~(0b11UL)) | k;
	unsigned long *ptr_ulong = (unsigned long *) ptr;
	ptr_ulong[0] = size;
	ptr_ulong[1] = magic;
	ptr_ulong[size / sizeof(long) - 1] = size;
	ptr_ulong[size / sizeof(long) - 2] = magic;

	return (void *) (ptr_ulong + 2);
}

Alloc mark_check_and_get_alloc(void *ptr)
{
	/* ecrire votre code ici */
	unsigned long *ptr_ulong = (unsigned long *) (ptr);
	ptr_ulong -= 2;
	MemKind k = ptr_ulong[1] & 0b11UL;
	Alloc a = {.size = ptr_ulong[0],.kind = k,.ptr = ptr_ulong };
	assert(a.size == ptr_ulong[a.size / sizeof(long) - 1]);
	assert(ptr_ulong[1] == ptr_ulong[a.size / sizeof(long) - 2]);

	return a;
}

static int get_nb_cores(void)
{
	long nb_cores = sysconf(_SC_NPROCESSORS_ONLN);
	if (nb_cores == -1) {
		perror("sysconf");
	}
	return nb_cores;
}

void init_arenas()
{
	long nb_cores = get_nb_cores();
	long nb_arenas = nb_cores * nb_cores;	// We take the square of nb_cores
	arenas.nb_arenas = nb_arenas;
	assert(nb_arenas < MAX_ARENAS);
	for (int i = 0; i < nb_arenas; i++) {
		MemArena arena = { };
		pthread_mutex_init(&arena.lock, NULL);
		arenas.arena_pool[i] = arena;
	}
}

MemArena *get_arena()
{
	pthread_t thread_id = pthread_self();
	long index = (long) thread_id % arenas.nb_arenas;
	return &arenas.arena_pool[index];
}

unsigned long mem_realloc_small()
{
	MemArena *arena = get_arena();

	assert(arena->chunkpool == 0);
	unsigned long size = (FIRST_ALLOC_SMALL << arena->small_next_exponant);
	arena->chunkpool = mmap(0,
							size,
							PROT_READ | PROT_WRITE | PROT_EXEC,
							MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (arena->chunkpool == MAP_FAILED)
		handle_fatalError("small realloc");
	arena->small_next_exponant++;

	return size;
}

unsigned long mem_realloc_medium()
{
	MemArena *arena = get_arena();

	uint32_t indice =
		FIRST_ALLOC_MEDIUM_EXPOSANT + arena->medium_next_exponant;
	assert(arena->TZL[indice] == 0);
	unsigned long size =
		(FIRST_ALLOC_MEDIUM << arena->medium_next_exponant);
	assert(size == (1UL << indice));
	arena->TZL[indice] = mmap(0, size * 2,	// twice the size to allign
							  PROT_READ | PROT_WRITE | PROT_EXEC,
							  MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (arena->TZL[indice] == MAP_FAILED)
		handle_fatalError("medium realloc");
	// align allocation to a multiple of the size for buddy algo
	arena->TZL[indice] +=
		(size - (((intptr_t) arena->TZL[indice]) % size));
	arena->medium_next_exponant++;
	return size;				// lie on allocation size, but never free
}


// used for test in buddy algo
unsigned int nb_TZL_entries()
{
	MemArena *arena = get_arena();
	pthread_mutex_lock(&arena->lock);

	int nb = 0;

	for (int i = 0; i < TZL_SIZE; i++)
		if (arena->TZL[i])
			nb++;

	pthread_mutex_unlock(&arena->lock);
	return nb;
}
