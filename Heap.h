#ifndef __HEAP_H__
#define __HEAP_H__

#include "Variable.h"
#include "Graph.h"

#define PARENT(i) (i/2)
#define LEFT(i) (2*i)
#define RIGHT(i) (2*i+1)
/* Teepä p = p -1; temppu. Ja sovita synnit. */

typedef struct {
    Variable* Vs; /* Vs[0] is the variable in the array, rest are neighbours*/
    int n; /* size (always at least 1) */

    int primary_key;
    int secondary_key;
} Heap_item;

typedef struct {
    Heap_item* array;
    int heap_size;
    /* MVK: Mikä on orig_size ? */
    /* AR: S'on heapin käyttämän taulukon koko. Ei liene tarpeellinen,
     * ellei sitten heappia tuhottaessa. */
    int orig_size;
} Heap;


Heap* build_heap(Graph* Gm);

void heapify(Heap*, int);

Variable* extract_min(Heap* H, Graph* G);

int get_heap_index(Heap* H, Variable v);

void clean_heap_item(Heap_item* hi, Variable V_removed, Graph* G);

#endif /* __HEAP_H__ */
