/* Heap.h 
 * Authors: Antti Rasinen, Mikko Korpela, Janne Toivola
 * $Id: Heap.h,v 1.10 2010-11-09 19:06:08 jatoivol Exp $
 */

#ifndef __HEAP_H__
#define __HEAP_H__

#include "nipvariable.h"
#include "Graph.h"
#include "clique.h"

#define PARENT(i) ((i-1)/2)
#define LEFT(i) (2*i+1)
#define RIGHT(i) (2*(i+1))

typedef struct {
  /* Vs[0] is the variable in the array, rest are neighbours*/
  nip_variable* Vs;
  sepset s;
  int n; /* size (always at least 1) */
  int primary_key;
  int secondary_key;
} Heap_item;

typedef struct {
  Heap_item* heap_items;
  int heap_size;
  /* MVK: What is orig_size ? */
  /* AR: Size of the table used by heap. May not be necessary,
   *     unless while freeing the heap. */
  int orig_size;
  sepset *useless_sepsets; /* MVK */
} Heap;

Heap* build_heap(Graph* Gm);

Heap* build_sepset_heap(clique* cliques, int num_of_cliques);

int extract_min(Heap* H, Graph* G, nip_variable** cluster_vars);

int extract_min_sepset(Heap* H, sepset* sepset);

void free_heap(Heap* H);

void mark_useful_sepset(Heap* H, sepset s);

#endif /* __HEAP_H__ */
