/* nipheap.h 
 * Heap for storing candidate groups of variables for various algorithms.
 * Authors: Janne Toivola, Antti Rasinen, Mikko Korpela
 * $Id: nipheap.h,v 1.7 2010-12-16 16:59:46 jatoivol Exp $
 */

#ifndef __NIPHEAP_H__
#define __NIPHEAP_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "nipjointree.h"
#include "nipvariable.h"
#include "niperrorhandler.h"

#define NIP_HEAP_PARENT(i) ((i-1)/2)
#define NIP_HEAP_LEFT(i) (2*i+1)
#define NIP_HEAP_RIGHT(i) (2*(i+1))

typedef struct {
  /* content[0] is the key variable in the array, rest are neighbours,
   * when a cluster of variables are concerned 
   * (TODO: move this comment to nipgraph.c) */
  void* content;
  int content_size; /* in case content is an array (always at least 1) */
  int primary_key;
  int secondary_key;
} nip_heap_item_struct;
typedef nip_heap_item_struct* nip_heap_item;

typedef struct {
  nip_heap_item* heap_items;
  int heap_size;      /* Number of elements inserted to the heap */
  int allocated_size; /* Size of the currently allocated table */
  int (*primary_key)(void* item, int size);
  int (*secondary_key)(void* item, int size);  
} nip_heap_struct;
typedef nip_heap_struct* nip_heap;

/* Creates a new heap */
nip_heap nip_new_heap(int initial_size,
		      int (*primary)(void* item, int size),
		      int (*secondary)(void* item, int size));

/* Inserts a new element into the heap h. 
 * The heap property is not valid after this, so remember to heapify... */
int nip_heap_insert(nip_heap h, void* content, int size);

/* TODO: hide this after refactoring */
void nip_heapify(nip_heap h, int i);

/* Returns the least expensive cluster of variables from the heap. */
int nip_extract_min_cluster(nip_heap h, nip_variable** cluster_vars);

/* Returns the least expensive sepset from the heap. */
int nip_extract_min_sepset(nip_heap h, nip_sepset* sepset);


/* Frees the memory allocated to the heap.
 * Does not free the contents, so don't free a heap unless it's empty
 * or you have pointers to the (dynamically allocated) content somewhere. */
void nip_empty_heap(nip_heap h);

/* Tells how many elements the heap contains */
int nip_heap_size(nip_heap h);

#endif /* __HEAP_H__ */
