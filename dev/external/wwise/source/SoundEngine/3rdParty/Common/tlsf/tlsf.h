/*
** Two Level Segregated Fit memory allocator, version 1.9w (wwise-ified, with more heap integrity checking).
** Written by Matthew Conte, and placed in the Public Domain.
**	http://tlsf.baisoku.org
**
** Based on the original documentation by Miguel Masmano:
**	http://rtportal.upv.es/rtmalloc/allocators/tlsf/index.shtml
**
** Please see the accompanying Readme.txt for implementation
** notes and caveats.
**
** This implementation was written to the specification
** of the document, therefore no GPL restrictions apply.
*/

#pragma once

#include <stddef.h>

/* Create/destroy a memory pool. */
typedef void* tlsf_pool;
tlsf_pool tlsf_create(void* mem, size_t bytes);
void tlsf_destroy(tlsf_pool pool);

/* malloc/memalign/realloc/free replacements. */
void* tlsf_malloc(tlsf_pool pool, size_t bytes);
void* tlsf_memalign(tlsf_pool pool, size_t align, size_t bytes);
void* tlsf_realloc(tlsf_pool pool, void* ptr, size_t size);
void tlsf_free(tlsf_pool pool, void* ptr);

/* Debugging. */
typedef void (*tlsf_walker)(void* ptr, size_t size, int used, void* user);
void tlsf_walk_heap(tlsf_pool pool, tlsf_walker walker, void* user);
/* Returns nonzero if heap check fails. */
int tlsf_check_heap(tlsf_pool tlsf);
/* Returns nonzero if heap check near ptr fails. Meant to be used when speed of check is important. */
int tlsf_check_ptr(tlsf_pool tlsf, const void * ptr);

/* Returns internal block size, not original request size */
size_t tlsf_block_size(void* ptr);

/* Overhead of per-pool internal structures. */
size_t tlsf_overhead();

/* Allocate a block in multiple tries.  Useful for big alignments only.*/
void* tlsf_AllocBigAlignment(tlsf_pool in_pPool, size_t in_ulAlign, size_t in_ulSize);

/*
** Data structures and associated constants.
*/

/*
** Block header structure.
**
** There are several implementation subtleties involved:
** - The prev_phys_block field is only valid if the previous block is free.
** - The prev_phys_block field is actually stored at the end of the
**   previous block. It appears at the beginning of this structure only to
**   simplify the implementation.
** - The next_free / prev_free fields are only valid if the block is free.
*/
typedef struct block_header_t
{
	/* Points to the previous physical block. */
	struct block_header_t* prev_phys_block;

	/* The size of this block, excluding the block header. */
	size_t size;

	/* Next and previous free blocks. */
	struct block_header_t* next_free;
	struct block_header_t* prev_free;
} block_header_t;
