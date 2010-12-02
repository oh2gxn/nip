/* nipheap.h 
 * Heap for storing candidate groups of variables for various algorithms.
 * Authors: Janne Toivola, Antti Rasinen, Mikko Korpela
 * $Id: nipheap.h,v 1.2 2010-12-02 16:38:29 jatoivol Exp $
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
  /* variables[0] is the key variable in the array, rest are neighbours */
  nip_variable* variables;
  nip_sepset s;
  int nvariables; /* size of variables (always at least 1) */
  int primary_key;
  int secondary_key;
} nip_heap_item_struct;
typedef nip_heap_item_struct* nip_heap_item;

typedef struct {
  nip_heap_item* heap_items;
  int heap_size;
  /* MVK: What is orig_size ? */
  /* AR: Size of the table used by heap. May not be necessary,
   *     unless while freeing the heap. */
  int orig_size;
  nip_sepset* useless_sepsets; /* MVK */
} nip_heap_struct;
typedef nip_heap_struct* nip_heap;

/* TODO: Refactor this as a function pointer supplied by nipgraph. 
 * This computes the primary keys */
int nip_graph_edges_added(nip_variable* vs, int n);

/* TODO: Refactor this as a function pointer supplied by nipgraph. 
 * This computes the secondary keys */
int nip_cluster_weight(nip_variable* vs, int n);

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
