/*
 * Heap.h $Id: Heap.h,v 1.14 2004-08-23 13:55:53 mvkorpel Exp $
 */

#ifndef __HEAP_H__
#define __HEAP_H__

#include "../Variable.h"
#include "../Graph.h"
#include "../Clique.h"

#define PARENT(i) ((i-1)/2)
#define LEFT(i) (2*i+1)
#define RIGHT(i) (2*(i+1))

typedef struct {
    Variable* Vs; /* Vs[0] is the variable in the array, rest are neighbours*/
    Sepset s;
    int n; /* size (always at least 1) */

    int primary_key;
    int secondary_key;
} Heap_item;

typedef struct {
    Heap_item* heap_items;
    int heap_size;
    /* MVK: Mikä on orig_size ? */
    /* AR: S'on heapin käyttämän taulukon koko. Ei liene tarpeellinen,
     * ellei sitten heappia tuhottaessa. */
    int orig_size;
    Sepset *useless_sepsets; /* MVK */
} Heap;

Heap* build_heap(Graph* Gm);

Heap* build_sepset_heap(Clique* cliques, int num_of_cliques);

int extract_min(Heap* H, Graph* G, Variable** cluster_vars);

int extract_min_sepset(Heap* H, Sepset* sepset);

void free_heap(Heap* H);

void mark_useful_Sepset(Heap* H, Sepset s);

#endif /* __HEAP_H__ */
