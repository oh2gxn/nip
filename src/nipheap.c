/* nipheap.c 
 * Authors: Antti Rasinen, Janne Toivola
 * Version: $Id: nipheap.c,v 1.4 2010-12-03 17:21:28 jatoivol Exp $
 */

#include "nipheap.h"


/* Internal helper functions */
static void nip_free_useless_sepsets(nip_heap h);
static int nip_heap_less_than(nip_heap_item h1, nip_heap_item h2);
static int nip_heap_index(nip_heap h, nip_variable v);
static void nip_clean_heap_item(nip_heap_item hi, nip_heap_item min_cluster);

int nip_graph_edges_added(nip_variable* vs, int n) {
    /* vs is the array of variables in the cluster induced by vs[0] */
    int i,j, sum = 0;

    for (i = 0; i < n; i++)
        for (j = i+1; j < n; j++)
	    sum += !nip_variable_is_parent(vs[i], vs[j]);
	  /*sum += !nip_graph_linked(g, vs[i], vs[j]);*/
    /* JJT: Not sure if this is supposed to be true "childrenship" in the
       original Bayes network, or just neigbourhood in the subsequent
       moralised and undirected graph. */

    /* AR: Joo, po. logical not. The purpose is to count how many
       edges must be added to the graph, i.e. all the cases, 
       when A is not child of B. Here the table is traversed only 
       to one direction, which differs from the original idea by 
       factor of 2.*/
   
    return sum; /* Number of links to add */
}

int nip_cluster_weight(nip_variable* vs, int n) {
    /* vs is the array of variables in the cluster induced by vs[0] */
    int i, prod = 1;
    
    for (i = 0; i < n; i++)
	prod *= NIP_CARDINALITY(vs[i]);

    return prod;
}


/* WTF does this function do? */
static void nip_clean_heap_item(nip_heap_item hi, 
				nip_heap_item min_cluster) {
    int i, j, n_vars = 0, n_total;
    nip_variable v_i, V_removed = min_cluster->variables[0];
    nip_variable* cluster_vars;

    /* FIXME: is this a duplicate of nip_variable_union() ??? */

    /* Copy all variables in hi and min_cluster together.
       Copy hi first, because Vs[0] must be the generating node */
    n_total = hi->nvariables + min_cluster->nvariables;
    cluster_vars = (nip_variable*) calloc(n_total, sizeof(nip_variable));
    if(!cluster_vars)
      return; /* FIXME: report error? */

    /*for (i = 0; i < hi->nvariables; i++)
      cluster_vars[i] = hi->variables[i];
      for (i = 0; i < min_cluster->nvariables; i++)
      cluster_vars[hi->n +i] = min_cluster->variables[i];*/

    memcpy(cluster_vars, hi->variables, 
	   hi->nvariables*sizeof(nip_variable));
    memcpy(cluster_vars+hi->nvariables, min_cluster->variables, 
	   min_cluster->nvariables*sizeof(nip_variable));
		
    /* Remove duplicates and min_vs[0] */
    for (i = 0; i < n_total; i++) {
      v_i = cluster_vars[i];
      if (v_i == NULL) continue;
      for (j = i+1; j < n_total; j++) {
	if (cluster_vars[j] == NULL) continue;
	if (nip_equal_variables(v_i, cluster_vars[j]) ||
	    nip_equal_variables(V_removed, cluster_vars[j]))
	  cluster_vars[j] = NULL;
      }
      cluster_vars[n_vars++] = v_i; /* Note: overwrites itself */
    }
    
    hi->nvariables = n_vars;

    free(hi->variables);

    hi->variables = (nip_variable*) calloc(n_vars, sizeof(nip_variable));
    if(!(hi->variables)){
      free(cluster_vars);
      return;
    }

    memcpy(hi->variables, cluster_vars, n_vars*sizeof(nip_variable));
    free(cluster_vars);

    hi->primary_key = nip_graph_edges_added(hi->variables, hi->nvariables);
    hi->secondary_key = nip_cluster_weight(hi->variables, hi->nvariables);

    return;
}


/* Heap management */

static int nip_heap_less_than(nip_heap_item h1, nip_heap_item h2) {
    return (h1->primary_key < h2->primary_key) || 
	   (h1->primary_key == h2->primary_key && 
	    h1->secondary_key < h2->secondary_key);
}

void nip_heapify(nip_heap h, int i) {
    int l,r;
    int min, flag;
    nip_heap_item temp;
    
    do {
        flag = 0;   
        l = NIP_HEAP_LEFT(i); r = NIP_HEAP_RIGHT(i);
    
        /* Note the difference between l (ell) and i (eye) */
	min = i;
	if (l < h->heap_size && 
	    nip_heap_less_than(h->heap_items[l], h->heap_items[i]))
	  min = l;
        if (r < h->heap_size && 
	    nip_heap_less_than(h->heap_items[r], h->heap_items[min]))
	  min = r;
            
        if (min != i) {
            /* Exchange array[min] and array[i] */
            temp = h->heap_items[min];
            h->heap_items[min] = h->heap_items[i];
            h->heap_items[i] = temp;
            i = min; flag = 1;
        }
    } while (flag);
}


int nip_extract_min_cluster(nip_heap h, nip_variable** cluster_vars) {
    nip_heap_item min;	/* Cluster with smallest weight */
    int i, heap_i;

    if (h->heap_size < 1)
      return 0;
    
    min = h->heap_items[0];

#ifdef NIP_DEBUG_HEAP
    printf("Eliminated node: %s (%i)\n", 
	   nip_variable_symbol(min->variables[0]), min->nvariables);
#endif
    
    /* Move the last one to the top */
    h->heap_items[0] = h->heap_items[h->heap_size-1]; 

    /* Mark this as NULL, so that free() won't be called twice */
    h->heap_items[h->heap_size-1]->variables = NULL;

    h->heap_size--;
    
    /* Iterate over neighbours of minimum element *
     * and update keys. The loop could be heavy.  */
    for (i = 1; i < min->nvariables; i++) {
      heap_i = nip_heap_index(h, min->variables[i]);
      nip_clean_heap_item(h->heap_items[heap_i], min);
    }

    /* Rebuild the heap. */
    for (i = 1; i < min->nvariables; i++)
      nip_heapify(h, nip_heap_index(h, min->variables[i]));
    nip_heapify(h, 0);
    
    *cluster_vars = min->variables;
    return min->nvariables; /* Cluster size*/
}

int nip_extract_min_sepset(nip_heap h, nip_sepset* sepset) {
  nip_heap_item min; /* Cluster with smallest weight */
  
  /* Empty heap, nothing to extract. */
  if (h->heap_size < 1)
    return NIP_ERROR_GENERAL;
  
  min = h->heap_items[0];

  /* Not a sepset heap */
  if(min->s == NULL)
    return NIP_ERROR_GENERAL;
  
  /* What exactly happens here? */
  h->heap_items[0] = h->heap_items[h->heap_size -1];
  h->heap_size--;
    
  /* Rebuild the heap. Is this enough? */
  nip_heapify(h, 0);
    
  *sepset = min->s;
  
  return NIP_NO_ERROR;
}

/* Called by find_sepsets when a sepset is accepted to the join tree. */
void nip_mark_useful_sepset(nip_heap h, nip_sepset s){

  int i, n;

  if(!h || !s)
    return;

  n = h->orig_size;

  for(i = 0; i < n; i++)
    if(h->useless_sepsets[i] == s){
      /* prevents the useful sepset from being freed later */
      h->useless_sepsets[i] = NULL; 
      break;
    }

  return;
}

static void nip_free_useless_sepsets(nip_heap h){
  int i, n;
  nip_sepset s;

  if(!h || !(h->useless_sepsets))
    return;

  n = h->orig_size;
  for(i = 0; i < n; i++){
    s = h->useless_sepsets[i];
    if(s != NULL)
      nip_free_sepset(s);
  }
}

static int nip_heap_index(nip_heap h, nip_variable v){
    /* Finds the heap element that contains the variable v */
    /* Linear search, but at the moment the only option */
    /* Hopefully this will not be too slow */

    int i;
    for (i = 0; i < h->heap_size; i++)
      if (nip_equal_variables(h->heap_items[i]->variables[0], v))
	return i;
    return -1;
}


void nip_free_heap(nip_heap h) {
  int i;
  nip_heap_item hi;

  if(!h)
    return;

  /* Go through every heap item*/
  for(i = 0; i < h->orig_size; i++){
    hi = h->heap_items[i];
    free(hi->variables); /* Free variable array in the heap item */
    free(hi);
  }

  nip_free_useless_sepsets(h);
  free(h->heap_items);
  free(h->useless_sepsets);
  free(h);
  return;
}

