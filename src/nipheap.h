/* nipheap.h 
 * Heap for storing candidate groups of variables for various algorithms.
 * Authors: Janne Toivola, Antti Rasinen, Mikko Korpela
 * $Id: nipheap.h,v 1.4 2010-12-12 20:08:49 jatoivol Exp $
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

/* TODO: Refactor this as a function pointer supplied by nipgraph. 
 * This computes the primary keys */
int nip_graph_edges_added(nip_variable* vs, int n);

/* TODO: Refactor this as a function pointer supplied by nipgraph. 
 * This computes the secondary keys */
int nip_cluster_weight(nip_variable* vs, int n);

/* TODO: hide this after refactoring */
void nip_heapify(nip_heap h, int i);

/* Returns the least expensive cluster of variables from the heap. */
int nip_extract_min_cluster(nip_heap h, nip_variable** cluster_vars);

/* Returns the least expensive sepset from the heap. */
int nip_extract_min_sepset(nip_heap h, nip_sepset* sepset);


/* Frees the memory allocated to the heap.
 * Does not free the contents, so don't free a heap unless it's empty
 * or you have pointers to the (dynamically allocated) content somewhere. */
void nip_free_heap(nip_heap h);

/* Prevents s from being freed together with the heap. */
void nip_mark_useful_sepset(nip_heap h, nip_sepset s);

#endif /* __HEAP_H__ */
