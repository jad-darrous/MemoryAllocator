#include "mem_alloc.h"

#include <stdio.h>
#include <assert.h>
#include <string.h>


/* memory - statically allocated */
char memory[MEMORY_SIZE];


/* Structure declaration for a free block */
typedef struct free_block {
	unsigned int size;
	struct free_block *next;
} free_block_s, *free_block_t;

/* Structure declaration for an occupied block */
typedef struct {
	unsigned int size;
} busy_block_s, *busy_block_t;


/* These functions select the free block that should be used
	to reserve the requested memory size according to the
	selected strategy: first fit/best fit/worst fit */
void get_block_first_fit(int, free_block_t*, free_block_t*);
void get_block_best_fit (int, free_block_t*, free_block_t*);
void get_block_worst_fit(int, free_block_t*, free_block_t*);

/* initialize a function pointer according to a preprocessor directive */
#ifdef BEST_FIT /* best fit strategy */
void (*get_block)(int, free_block_t*, free_block_t*) = get_block_best_fit;
#elif WORST_FIT /* worst fit strategy */
void (*get_block)(int, free_block_t*, free_block_t*) = get_block_worst_fit;
#else  /* first fit strategy */
void (*get_block)(int, free_block_t*, free_block_t*) = get_block_first_fit;
#endif

#define FREE_BLOCK_SIZE sizeof(free_block_s)
#define BUSY_BLOCK_SIZE sizeof(busy_block_s)

/* Pointer to the first free block in the memory */
free_block_t first_free;


#define ULONG(x)((long unsigned int)(x))
/* #define max(x,y) (x>y?x:y) */

/**
 * Checks if the supplied address points to a busy block
 * This methods supposes that the memory is not corrupted
 */
int is_valid_busy_block(char *pp) {
	free_block_t p;
	busy_block_t r = (busy_block_t) heap_base();
	while ((char*) r < (char*) first_free) {
		if ((char*) (r + 1) == pp)
			return 1;
		r = (busy_block_t) ((char*) (r + 1) + *((int*) r));
	}
	for (p = first_free; p != NULL; p = p->next) {
		r = (busy_block_t) ((char*) p + p->size);
		while ((char*) r < (char*) p->next) {
			if ((char*) (r + 1) == pp)
				return 1;
			r = (busy_block_t) ((char*) (r + 1) + *((int*) r));
		}
	}
	return 0;
}

float calculate_fragmentation(void) {
	int total = 0, max_size = 0;
	free_block_t current;
	for (current = first_free; current != NULL; current = current->next) {
		total += current->size;
		if (current->size > max_size)
			max_size = current->size;
	}
	return 1 - (1.0 * max_size / total);
}

void print_fragmentation_info(void) {
#ifdef FRAG
	fprintf(stderr, "Fragmentation at this point is %.3f\n", calculate_fragmentation());
#endif
}

free_block_t get_prev_free_block(char *pointer) {

	free_block_t prev = first_free;
	free_block_t p = prev->next;
	while (p && ((char *) p < pointer)) {
		prev = p;
		p = p->next;
	}
	return prev;
}

void get_block_best_worst_fit(int size, int best, free_block_t* prev, free_block_t* block) {
	int waste = best ? 1 << 30 : -1;
	free_block_t pr = NULL;
	free_block_t p = first_free;
	while (p) {
		int diff = p->size - size - BUSY_BLOCK_SIZE;
		if (diff >= 0) {
			int cond = best ? diff < waste : diff > waste;
			if (cond) {
				waste = diff;
				*block = p;
				*prev = pr;
			}
		}
		pr = p;
		p = p->next;
	}
}

void get_block_worst_fit(int size, free_block_t* prev, free_block_t* block) {
	return get_block_best_worst_fit(size, 0, prev, block);
}

void get_block_best_fit(int size, free_block_t* prev, free_block_t* block) {
	return get_block_best_worst_fit(size, 1, prev, block);
}

void get_block_first_fit(int size, free_block_t* prev, free_block_t* p) {
	*prev = NULL;
	*p = first_free;
	while (*p) {
		int diff = (*p)->size - size - BUSY_BLOCK_SIZE;
		if (diff >= 0) {
			return;
		}
		*prev = (*p);
		(*p) = (*p)->next;
	}
}

void memory_init(void) {

	first_free = (free_block_t) memory;
	first_free->size = MEMORY_SIZE;
	first_free->next = NULL;
}

char *memory_alloc(int size) {

	// the allocated busy_block should be at least
	// the size of a free_block
	if (size < FREE_BLOCK_SIZE - BUSY_BLOCK_SIZE)
		size = FREE_BLOCK_SIZE - BUSY_BLOCK_SIZE;

	free_block_t prev = NULL, block = NULL;

	// get a free block according to the selected strategy
	get_block(size, &prev, &block);

	// can't find a free_block with enough size;
	if (!block) return NULL;

	/**
	 * We have to split this block into two parts:
	 * - the first is the busy block to be returned to the user
	 * - the second is the new free block
	 */
	free_block_t next = block->next;

	int rem_size = block->size - (size + BUSY_BLOCK_SIZE);

	if (rem_size < FREE_BLOCK_SIZE) {
		// Allocate the remaining bytes
		size = block->size - BUSY_BLOCK_SIZE;
	} else {
		// Split the free block
		free_block_t node = (free_block_t) ((char*) block + BUSY_BLOCK_SIZE + size);
		node->size = block->size - (size + BUSY_BLOCK_SIZE);
		node->next = next;
		next = node;
	}
	// updates the head of the list
	if (!prev) {
		first_free = next;
	} else {
		prev->next = next;
	}
	// Allocate the busy block
	((busy_block_t) block)->size = size;
	char* ret = (char *) (((busy_block_t) block) + 1);
	print_alloc_info(ret, size);
	return ret;
}

void memory_free(char *p) {
	print_free_info(p);

#ifdef CHECK
	if (!is_valid_busy_block(p)) {
		fprintf(stderr, "WRONG memory address at : %lu\n", ULONG(p - memory));
		return ;
	}
#endif

	busy_block_t to_del = (busy_block_t) p - 1;
	unsigned int size = to_del->size;

	unsigned int to_del_index = (char *) to_del - heap_base();
	unsigned int first_free_index = (char *) first_free - heap_base();

	// is before first_free -- updates the first_free
	if (to_del_index < first_free_index) {
		free_block_t prev_head = first_free;
		first_free = (free_block_t) to_del;
		// the about to delete block is directly before the current head
		if (to_del_index + size + BUSY_BLOCK_SIZE == first_free_index) {
			free_block_t next = prev_head;
			first_free->size = size + BUSY_BLOCK_SIZE + next->size;
			first_free->next = next->next;
		} else {
			first_free->size = size + BUSY_BLOCK_SIZE;
			first_free->next = prev_head;
		}
		return;
	}

	free_block_t prev = get_prev_free_block((char *) to_del);
	free_block_t next = prev->next;

	// chech if adjacent to free blocks
	int adj_prev = (char *) prev + prev->size == (char *) to_del;
	int adj_next = (char *) (next) == (p + size);

	if (adj_prev && adj_next) {
		prev->size += BUSY_BLOCK_SIZE + size + next->size;
		prev->next = next->next;
	} else if (adj_next) {
		free_block_t curr = (free_block_t) to_del;
		curr->size = size + BUSY_BLOCK_SIZE + next->size;
		curr->next = next->next;
		prev->next = curr;
	} else if (adj_prev) {
		prev->size += size + BUSY_BLOCK_SIZE;
	} else {
		free_block_t curr = (free_block_t) to_del;
		curr->size = size + BUSY_BLOCK_SIZE;
		curr->next = next;
		prev->next = curr;
	}
}


void print_info(void) {
#ifdef INFO
	fprintf(stderr, "Memory : [%lu %lu] (%lu bytes)\n",
		(long unsigned int) 0, (long unsigned int) (memory + MEMORY_SIZE),
		(long unsigned int) (MEMORY_SIZE));
	fprintf(stderr, "Free block : %lu bytes; busy block : %lu bytes.\n",
		ULONG(sizeof (free_block_s)), ULONG(sizeof (busy_block_s)));
#endif
}

void print_free_info(char *addr) {
#ifdef INFO
	if (addr)
		fprintf(stderr, "FREE  at : %lu \n", ULONG(addr - memory));
	else
		fprintf(stderr, "FREE  at : %lu \n", ULONG(0));
#endif
}

void print_alloc_info(char *addr, int size) {
#ifdef INFO
	if (addr) {
		fprintf(stderr, "ALLOC at : %lu (%d byte(s))\n",
							ULONG(addr - memory), size);
	} else {
		fprintf(stderr, "Warning, system is out of memory\n");
	}
#endif
}

void print_free_blocks(void) {
#ifdef INFO
	free_block_t current;
	fprintf(stderr, "Begin of free block list :\n");
	for (current = first_free; current != NULL; current = current->next)
		fprintf(stderr, "Free block at address %lu, size %u\n",
			ULONG((char*) current - memory), current->size);
#endif
}

void print_busy_blocks(void) {
#ifdef INFO
  	free_block_t p;
  	busy_block_t r = (busy_block_t) memory;
	free_block_t p_next = first_free;
  	fprintf(stderr, "Begin of busy block list :\n");
	do {
	  	while ((char*) r < (char*)p_next) {
			fprintf(stderr, "Memoty leak at %ld of %d bytes\n",
				(char*) (r + 1) - heap_base(), *((int*) r));
			r = (busy_block_t) ((char*) (r + 1) + *((int*) r));
	  	}
		p = p_next;
		p_next = p->next;
	  	r = (busy_block_t) ((char*) p + p->size);
	} while(p_next);
#endif
}

int is_memory_leak_exist(void) {
	return first_free->size != MEMORY_SIZE;
}

inline char *heap_base(void) {
	return memory;
}


void *malloc(size_t size) {
	static int init_flag = 0;
	if (!init_flag) {
		init_flag = 1;
		memory_init();
	}
	return (void*) memory_alloc((size_t) size);
}

void free(void *p) {
	if (p == NULL) return;
	memory_free((char*) p);
	print_free_blocks();
}

void *realloc(void *ptr, size_t size) {
	if (ptr == NULL)
		return memory_alloc(size);

	busy_block_t bb = ((busy_block_t) ptr) - 1;
#ifdef INFO
	fprintf(stderr, "Reallocating %d bytes to %d\n",
		bb->size - (int) sizeof (busy_block_s), (int) size);
#endif
	if (size <= bb->size - sizeof (busy_block_s))
		return ptr;

	char *new = memory_alloc(size);
	memcpy(new, (void*) (bb + 1), bb->size - sizeof (busy_block_s));
	memory_free((char*) (bb + 1));
	return (void*) (new);
}
