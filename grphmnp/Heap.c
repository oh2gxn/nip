#include <stdlib.h>
#include "../Variable.h"
#include "../Graph.h"
#include "Heap.h"
#include <assert.h>

/* Voi r‰k‰ */
/* Koodi ei t‰ss‰ muodossaan sovellu lapsille tai raskaana oleville tai 
   imett‰ville naisille. */

int edges_added(Graph* G, Variable* vs, int n)
{
    /* vs is the array of variables in the cluster induced by vs[0] */

    int i,j, sum = 0;

    for (i = 0; i < n; i++)
        for (j = i+1; j < n; j++)
            sum += !is_child(G, vs[i], vs[j]);

      /* AR: Joo, po. looginen not. Tarkoitus on laskea, kuinka monta
         kaarta on lis‰tt‰v‰ graafiin, eli kaikki ne tapaukset, 
         kun A ei ole B:n lapsi. T‰ss‰ k‰yd‰‰n taulukko vain yhteen suuntaan,
         eroaa alkuper‰isest‰ ideasta kertoimella 2.*/
   
    return sum; /* Number of links to add */
}

int cluster_weight(Variable* vs, int n)
{
    /* vs is the array of variables in the cluster induced by vs[0] */
    int i, prod = 1;
    
    for (i = 0; i < n; i++)
	prod *= number_of_values(vs[i]);

    return prod;
}

/* Heap management */

Heap* build_heap(Graph* Gm)
{
    int i,j, n;

    Heap_item* hi;
    Heap* H = (Heap*) malloc(sizeof(Heap));
    Variable* Vs_temp;

    n = get_size(Gm);
    Vs_temp = (Variable*) calloc(n, sizeof(Variable));
     
    H->heap_items = (Heap_item*) calloc(n, sizeof(Heap_item));
    H->heap_size = n;
    H->orig_size = n;
    
    for (i = 0; i < n; i++)
    {
        hi = &(H->heap_items[i]);
        hi->n = get_neighbours(Gm, Vs_temp, Gm->variables[i]) +1;
        /* get_neighbours could be modified to use the array Vs directly;
           the cost associated with it would be having all Vs take
           get_size(G) units of memory.
        */

		hi->Vs = (Variable *) calloc(hi->n, sizeof(Variable));

		hi->Vs[0] = Gm->variables[i];
        
        for (j = 1; j < hi->n; j++)   /* Copy variable-pointers from Vs_temp */
            hi->Vs[j] = Vs_temp[j-1]; /* Note the index-shifting */

        hi->primary_key = edges_added(Gm, hi->Vs, hi->n);
        hi->secondary_key = cluster_weight(hi->Vs, hi->n);
    }

    free(Vs_temp);

    H->heap_items = H->heap_items-1; /* T‰st‰ ne murheet alkaa. */

    for (i = n/2; i > 0; i--)
        heapify(H, i);

    return H;
}

/* MVK: Kusee. Ei k‰‰nny. */
/* AR: Eip‰ joo. Ei sit‰ mist‰‰n kyll‰ kutsutakaan :)
       Muistinhallinnalliset asiat viel‰ hiomatta. */
void remove_node(Heap_item* node)
{
    int i;
   /* 

    for (i = 0; i < node->n; i++)
    {
        node->adj_list.vars[i] = 0;
    }
    */
}

int lessthan(Heap_item h1, Heap_item h2)
{
    return (h1.primary_key < h2.primary_key) || 
	   (h1.primary_key == h2.primary_key && 
	    h1.secondary_key < h2.secondary_key);
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
    
        min = (l <= H->heap_size && lessthan(H->heap_items[l], H->heap_items[i]))?
			   l:i;
            
        if (r <= H->heap_size && lessthan(H->heap_items[r], H->heap_items[min]))
            min = r;
            
        if (min != i)
        {
            /* Exchange array[min] and array[i] */
            temp = H->heap_items[min];
            H->heap_items[min] = H->heap_items[i];
            H->heap_items[i] = temp;
            
            i = min; flag = 1;
        }
    } while (flag);
}

int extract_min(Heap* H, Graph* G, Variable** cluster_vars)
{
    Heap_item min;	/* Cluster with smallest weight */
    int i, heap_i;

    if (H->heap_size < 1)
        return 0;
    
    min = H->heap_items[1];
    
    H->heap_items[1] = H->heap_items[H->heap_size];
    H->heap_size--;
    
    /* Iterate over neighbours of minimum element *
     * and update keys. The loop could be heavy.  */
	for (i = 1; i < min.n; i++)         
    {
		heap_i = get_heap_index(H, min.Vs[i]);
		clean_heap_item(&H->heap_items[heap_i], min.Vs[0], G);
    }

    /* Rebuild the heap. */
    for (i = 1; i < min.n; i++)
		heapify(H, get_heap_index(H, min.Vs[i]));
    heapify(H, 1);
    
    *cluster_vars = min.Vs; 
    return min.n; /* Cluster size*/
}

int get_heap_index(Heap* H, Variable v)
{
    /* Finds the heap element that contains the variable v */
    /* Linear search, but at the moment the only option */
    /* Hopefully this will not be too slow */

    int i;

    for (i = 1; i <= H->heap_size; i++)
		if (equal_variables(H->heap_items[i].Vs[0], v))
			return i;

    return -1;
}

void clean_heap_item(Heap_item* hi, Variable V_removed, Graph* G)
{
    int i;
    for (i = 1; i < hi->n; i++)
		if (equal_variables(hi->Vs[i], V_removed))
        {
			--hi->n;
			hi->Vs[i] = hi->Vs[hi->n];
			break;
        }

    hi->primary_key = edges_added(G, hi->Vs, hi->n);
    hi->secondary_key = cluster_weight(hi->Vs, hi->n);
}
