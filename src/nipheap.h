/* nipheap.h 
 * Heap for storing candidate groups of variables for various algorithms.
 * Authors: Janne Toivola, Antti Rasinen, Mikko Korpela
 * $Id: nipheap.h,v 1.1 2010-11-30 18:12:04 jatoivol Exp $
 */

#ifndef __NIPHEAP_H__
#define __NIPHEAP_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "nipgraph.h"
#include "nipjointree.h"
#include "nipvariable.h"
#include "niperrorhandler.h"

#define NIP_HEAP_PARENT(i) ((i-1)/2)
#define NIP_HEAP_LEFT(i) (2*i+1)
#define NIP_HEAP_RIGHT(i) (2*(i+1))

typedef struct {
  /* variables[0] is the key variable in the array, rest are neighbours */
  nip_variable* variables;
  sepset s;
  int nvariables; /* size of variables (always at least 1) */
  int primary_key;
  int secondary_key;
} nip_heap_item_struct;
typedef nip_heap_item_struct* nip_heap_item

typedef struct {
  nip_heap_item* heap_items;
  int heap_size;
  /* MVK: What is orig_size ? */
  /* AR: Size of the table used by heap. May not be necessary,
   *     unless while freeing the heap. */
  int orig_size;
  sepset* useless_sepsets; /* MVK */
} nip_heap_struct;
typedef nip_heap_struct* nip_heap

nip_heap nip_build_cluster_heap(nip_graph gm);

nip_heap nip_build_sepset_heap(nip_clique* cliques, int num_of_cliques);

int nip_extract_min_cluster(nip_heap h, nip_graph g, 
			    nip_variable** cluster_vars);

int nip_extract_min_sepset(nip_heap h, nip_sepset* sepset);

void nip_free_heap(nip_heap H);

void nip_mark_useful_sepset(nip_heap h, sepset s);

#endif /* __HEAP_H__ */
