#include "Variable.h"

#define PARENT(i) (i/2)
#define LEFT(i) (2*i)
#define RIGHT(i) (2*i+1)
/* Teepä p = p -1; temppu. Ja sovita synnit. */

typedef struct {
    Variable V;
    int primary;
    int secondary;
} Heap_item;

typedef struct {
    Heap_item* array;
    int heap_size;
    int orig_size;
} Heap;

void heapify(Heap*, int);
