#include "Variable.h"
#include "Heap.h"

/* Voi räkä */
/* Koodi ei tässä muodossaan sovellu lapsille tai raskaana oleville tai 
   imettäville naisille. */
int lessthan(Heap_item h1, Heap_item h2)
{
    /* Ei kovin NULL-varma. */
    /* The heap has two keys: edges_added is the primary */
    if (h1.edges_added < h2.edges_added)
        return 1;
    if (h1.edges_added > h2.edges_added)
        return 0;
    /* If we reach this point, edges_added keys are the same */
    if (h1.new_weight < h2.new_weight)
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

int primary_weight(Graph G, Variable* vs, int n)
{
    /* vs is the array of variables in the cluster induced by vs[0] */

    int i,j, sum = 0;

    for (i = 0; i < n; i++)
        for (j = i+1; j < n; j++)
            sum += ~is_child(vs[i], vs[j]);
   
    return sum; /* Number of links to add */
}

int secondary_weight(Variable* vs, int n)
{
    /* vs is the array of variables in the cluster induced by vs[0] */
    int i, int prod = 1;
    
    for (i = 0; i < n; i++)
	prod *= number_of_values(vs[i]);

    return prod;
}

Heap* build_heap(Variable* vars, int n)
{
    int i,j;
    Heap_item* hi;
    Heap* H = (Heap*) malloc(sizeof(Heap));
    Variable* Vs_temp = (Variable*) calloc(n, sizeof(Variable));
     
    H->array = (Heap_item*) calloc(n, sizeof(Heap_item));
    H->heap_size = n;
    H->orig_size = n;
    
    for (i = 0; i < n; i++)
    {
        hi = &(H->array[i]);
        hi->n = get_neighbours(Gm, &Vs_temp) +1;
        hi->Vs = calloc(hi->n, sizeof(int));
        hi->Vs[0] = vars[i];
        
        for (j = 1; j < hi->n; j++)      /* Copy variable-pointers from Vs_temp */
            hi->Vs[j] = Vs_temp[j-1];    /* Note the index-shifting */

        hi->edges_added = primary_weight(Gm, hi->Vs, hi->n);
        hi->new_weight = secondary_weight(Gm, hi->Vs, hi->n);
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

Variable extract_min(Heap* H)
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
    
    /* RATKAISU! Aja heapify jokaiselle muuttujan V (pienimmän painon omaava)
     * naapurille! Sä oot nero! Sä oot myös aika väsynyt ja tuhnuinen.
     * Ja muista myös ajaa sit koko nipulle se heapify. */
    
    /* Iterate over neighbours of minimum element */
    for (i = 1; i < min->n; i++)         /* This loop could be heavy. */
        clean_heap_item(&H->array[get_heap_index(min->Vs[i])], min.Vs[0]); 
 
    /* Rebuild the heap */
    for (i = 1; i < min->n; i++)
        heapify(A, get_heap_index(min->Vs[i]));
    heapify(A, 1);
    
    return min.Vs[0];
}

int get_heap_index(Variable v)
{
    /* Maksamme velkaa, olemme vaiheessa */
}

void clean_heap_item(Heap_item* hi, Variable V_removed)
{
    int i;
    for (i = 1; i < hi->n; i++)
        if (hi->Vs[i] == V_removed)
        {
            hi->Vs[i] = hi->Vs[hi->n];
            --hi->n;
            break;
        }
        
    hi->edges_added = primary_weight(G, hi->Vs, hi->n);
    hi->new_weight = secondary_weight(G, hi->Vs, hi->n    
}