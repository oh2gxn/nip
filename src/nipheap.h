/* nipheap.h 
 * Heap for storing candidate groups of variables for various algorithms.
 * Authors: Janne Toivola, Antti Rasinen, Mikko Korpela
 * $Id: nipheap.h,v 1.14 2011-03-14 11:09:35 jatoivol Exp $
 */

#ifndef __NIPHEAP_H__
#define __NIPHEAP_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "niplists.h"
#include "nipjointree.h"
#include "nipvariable.h"
#include "niperrorhandler.h"

#define NIP_HEAP_PARENT(i) ((i-1)/2)
#define NIP_HEAP_LEFT(i) (2*i+1)
#define NIP_HEAP_RIGHT(i) (2*(i+1))

typedef struct {
  void* content;     /* the pointer to the content (possibly array) */
  int content_size;  /* in case content is an array (always at least 1) */
  int primary_key;   /* key determining the order in the heap */
  int secondary_key; /* secondary key determining the order in case of ties */
} nip_heap_item_struct;
typedef nip_heap_item_struct* nip_heap_item;

typedef struct {
  nip_heap_item* heap_items; /* The array of elements */
  int heap_size;      /* Number of elements inserted to the heap */
  int allocated_size; /* Size of the currently allocated table */
  int (*primary_key)(void* item, int size); /* for computing key values */
  int (*secondary_key)(void* item, int size);
  int heapified; /* Flag if someone has seen the trouble of heapifying */
  nip_int_list updated_items; /* List of indices of updated elements 
			       * which potentially violate heap property */
} nip_heap_struct;
typedef nip_heap_struct* nip_heap;


/* Creates a new heap */
nip_heap nip_new_heap(int initial_size,
		      int (*primary)(void* item, int size),
		      int (*secondary)(void* item, int size));


/* Inserts a new element into the heap h. 
 * The heap property is not valid after this, so remember to heapify... */
nip_error_code nip_heap_insert(nip_heap h, void* content, int size);


/* Makes a linear search through all items in the heap by using 
 * the supplied comparison operation and reference content. 
 * Returns a heap index to be used for reference in the 
 * nip_update_heap_item function, or -1 if not found. */
int nip_search_heap_item(nip_heap h, 
			 int (*comparison)(void* i, int isize, 
					   void* r, int rsize), 
			 void* ref, int refsize);


/* Retrieves the content stored to given index (random access to heap).
 * Returns the void pointer contained by the heap and size of the content 
 * (or 0 in case of error) via given pointer. */
void* nip_get_heap_item(nip_heap h, int index, int* size);


/* Replaces the heap item referenced by index (given by nip_search_heap_item)
 * with the given content. */
nip_error_code nip_set_heap_item(nip_heap h, int index, 
				 void* content, int size);


/* Makes the heap obey the heap property after modifications to the root */
void nip_build_min_heap(nip_heap h);


/* Returns the least expensive item from the heap. The content pointer is 
 * returned and size of the content returned via int pointer. If the heap 
 * is empty, NULL pointer and 0 is returned. */
void* nip_heap_extract_min(nip_heap h, int* size);


/* Frees the memory allocated to the heap.
 * Does not free the contents, so don't free a heap unless it's empty
 * or you have pointers to the (dynamically allocated) content somewhere. */
void nip_free_heap(nip_heap h);


/* Tells how many elements the heap contains */
int nip_heap_size(nip_heap h);

#endif /* __HEAP_H__ */
