#include "Variable.h"
#include "Heap.h"

/* Voi räkä */
/* Koodi ei tässä muodossaan sovellu lapsille tai raskaana oleville tai 
   imettäville naisille. */
int lessthan(Heap_item h1, Heap_item h2)
{
    /* Ei kovin NULL-varma. */
    if (h1.edges_added < h2.edges_added)
        return 1;
    if (h1.edges_added > h2.edges_added)
        return 0;
    /* If we reach this point, edges_added keys are the same */
    if (h1.new_weight < h2.new_weight)
        return 1;
    return 0;
}

adj_list build_adjacency_list(Variable* neighbours, int n)
{
    adj_list nb;
    
    nb.vars = neighbours;
    nb.size = n;
    return nb;
}

void destroy_node()
{
}

void remove_node(Heap_item* node)
{
    int i;
    

    for (i = 0; i < node->adj_list.size; i++)
    {
        node->adj_list.vars[i]
    }

}

int primary_weight(Graph G, Variable* vs, int n)
{
    int i,j,sum;

    sum = 0;
    for (i = 0; i < n; i++)
	for (j = i+1; j < n; j++)
	    sum += ~is_child(vs[i], vs[j]);
   
    return sum; /* Number of links to be added */
}

int secondary_weight(Variable* vs, int n)
{
    int i, int prod = 1;
    
    for (i = 0; i < n; i++)
	prod *= number_of_values(vs[i]);

    return prod;
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
        hi->neighbours = build_adjacency_list(get_neighbours(Gm));
        hi->edges_added = primary_weight(Gm, V u neighbours, n_neigh);
        hi->new_weight = secondary_weight(Gm, V u neighbours, n_neigh);
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

