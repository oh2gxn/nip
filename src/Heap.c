#include "Variable.h"
#include "Heap.h"

/* Voi räkä */
/* Koodi ei tässä muodossaan sovellu lapsille tai raskaana oleville tai 
   imettäville naisille. */
int lessthan(Heap_item h1, Heap_item h2)
{
    /* Ei kovin NULL-varma. */
    if (h1.primary < h2.primary)
        return 1;
    if (h1.primary > h2.primary)
        return 0;
    /* If we reach this point, primary keys are the same */
    if (h1.secondary < h2.secondary)
        return 1;
    return 0;
}

Heap* build_heap(Variable* vars, int n)
{
    int i;
    Heap_item* hi;
    Heap* H = (Heap*) malloc(sizeof(Heap));
     
    H->array = (Heap_item*) calloc(n, sizeof(Heap_item));
    H->heap_size = n;
    H->orig_size = n;
    
    for (i = 0; i < n; i++)
    {
        hi = &(H->array[i]);
        hi->V = vars[i];
        hi->primary = XX; /* Eli parametriksi Gm */
        hi->secondary = XX;
    }

    H->array = H->array-1; /* Tästä ne murheet alkaa. */

    for (i = n/2; i > 0; i--)
        heapify(H, i);
}

void heapify(Heap* H, int i)
{
    int l,r;
    int min, flag;
    Heap_item temp;
    
    do {
        flag = 0;   
        l = LEFT(i); r = RIGHT(i);
    
        /* Note the different between l (ell) and i (eye) */
        if (l <= H->heap_size && lessthan(H->array[l], H->array[i]))
            min = l;
        else
            min = i;
            
        if (r <= H->heap_size && lessthan(H->array[r], H->array[min]))
            min = r;
            
        if (min != i)
        {
            /* Exchange array[min] and array[i] */
            temp = H->array[min];
            H->array[min] = H->array[i];
            H->array[i] = temp;
            
            i = min; flag = 1;
        }
    } while (flag);
}

Variable extract_min(Heap* H)
{
    Heap_item min;
    int i, n_neighbors;

    if (H->heap_size < 1)
        return NULL;
    
    min = H->array[1];
    /* XX Etsi minin naapurit */
    /* ja ne johonki taulukkoon. neighbors */
    
    H->array[1] = H->array[H->heap_size];
    H->heap_size--;
    
    /* RATKAISU! Aja heapify jokaiselle muuttujan V (pienimmän painon omaava)
     * naapurille! Sä oot nero! Sä oot myös aika väsynyt ja tuhnuinen.
     * Ja muista myös ajaa sit koko nipulle se heapify. */
    
    for (i = 0; i < n_neighbors; i++)
        heapify(A, neighbors[i]);
    heapify(A, 1);
    
    return min.V;
}

