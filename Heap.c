#include "Variable.h"
#include "Heap.h"

/* Voi räkä */
/* Koodi ei tässä muodossaan sovellu lapsille tai raskaana oleville tai 
   imettäville naisille. */
int lessthan(Heap_item h1, Heap_item h2)
{
    /* Ei kovin NULL-varma. */
    /* The heap has two keys */
    if (h1.primary_key < h2.primary_key)
        return 1;
    if (h1.primary_key > h2.primary_key)
        return 0;
    /* If we reach this point, primary_key keys are the same */
    if (h1.secondary_key < h2.secondary_key)
        return 1;
    return 0;
}

void destroy_node()
{
}

void remove_node(Heap_item* node)
{
    int i;
    

    for (i = 0; i < node->n; i++)
    {
        node->adj_list.vars[i]
    }

}

int edges_added(Graph G, Variable* vs, int n)
{
    /* vs is the array of variables in the cluster induced by vs[0] */

    int i,j, sum = 0;

    for (i = 0; i < n; i++)
        for (j = i+1; j < n; j++)
            sum += ~is_child(vs[i], vs[j]);
   
    return sum; /* Number of links to add */
}

int cluster_weight(Variable* vs, int n)
{
    /* vs is the array of variables in the cluster induced by vs[0] */
    int i, int prod = 1;
    
    for (i = 0; i < n; i++)
	prod *= number_of_values(vs[i]);

    return prod;
}

Heap* build_heap(Graph* Gm)
{
    int i,j, n;

    Heap_item* hi;
    Heap* H = (Heap*) malloc(sizeof(Heap));
    Variable* Vs_temp;

    n = get_size(Gm);
    Vs_temp = (Variable*) calloc(n, sizeof(Variable));
     
    H->array = (Heap_item*) calloc(n, sizeof(Heap_item));
    H->heap_size = n;
    H->orig_size = n;
    
    for (i = 0; i < n; i++)
    {
        hi = &(H->array[i]);
        hi->n = get_neighbours(Gm, &Vs_temp) +1;
        hi->Vs = calloc(hi->n, sizeof(int));
        hi->Vs[0] = vars[i];
        
        for (j = 1; j < hi->n; j++)   /* Copy variable-pointers from Vs_temp */
            hi->Vs[j] = Vs_temp[j-1]; /* Note the index-shifting */

        hi->primary_key = edges_added(Gm, hi->Vs, hi->n);
        hi->secondary_key = cluster_weight(Gm, hi->Vs, hi->n);
    }

    delete Vs_temp;

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

Variable* extract_min(Heap* H, Graph* G)
{
    Heap_item min;
    int i, heap_i;

    if (H->heap_size < 1)
        return NULL;
    
    min = H->array[1];

    /* XX Etsi minin naapurit */
    /* ja ne johonki taulukkoon. neighbors */
    
    H->array[1] = H->array[H->heap_size];
    H->heap_size--;
    
    /* Iterate over neighbours of minimum element *
     * and update keys. The loop could be heavy.  */
    for (i = 1; i < min->n; i++)         /* This loop could be heavy. */
    {
	heap_i = get_heap_index(H, min->Vs[i]);
	clean_heap_item(&H->array[heap_i], min.Vs[0], G);
    }

    /* Rebuild the heap. */
    for (i = 1; i < min->n; i++)
        heapify(A, get_heap_index(H, min->Vs[i]));
    heapify(A, 1);
    
    return min.Vs;
}

int get_heap_index(Heap* H, Variable v)
{
    /* Finds the heap element that contains the variable v */
    /* Linear search, but at the moment the only option */
    /* Hopefully this will not be too slow */

    int i;

    for (i = 0; i < H->heap_size; i++)
	if (H->array[i].Vs[0] == v)
	    return i;

    return -1;
}

void clean_heap_item(Heap_item* hi, Variable V_removed, Graph* G)
{
    int i;
    for (i = 1; i < hi->n; i++)
        if (hi->Vs[i] == V_removed)
        {
            hi->Vs[i] = hi->Vs[hi->n];
            --hi->n;
            break;
        }
        
    hi->primary_key = edges_added(G, hi->Vs, hi->n);
    hi->secondary_key = cluster_weight(G, hi->Vs, hi->n);
}
