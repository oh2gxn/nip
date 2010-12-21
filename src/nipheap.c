/* nipheap.c 
 * Authors: Antti Rasinen, Janne Toivola
 * Version: $Id: nipheap.c,v 1.13 2010-12-21 16:34:06 jatoivol Exp $
 */

#include "nipheap.h"

#define NIP_DEBUG_HEAP

/* Defines the heap order between two heap items */
static int nip_heap_less_than(nip_heap_item h1, nip_heap_item h2);

/* FIXME: this assumes we have stored nip_variable[] as content! 
 * Just make it "void* v" ??? */
static int nip_heap_index(nip_heap h, nip_variable v);

/* An undocumented function by Antti Rasinen:
 * seems to update heap item hi after removing heap item min from h.
 * After this, hi contains the union of hi->content and min->content,
 * except min->content[0]. */
static void nip_clean_heap_item(nip_heap h, nip_heap_item hi, 
				nip_heap_item min);


nip_heap nip_new_heap(int initial_size, 
		      int (*primary)(void* item, int size),
		      int (*secondary)(void* item, int size)) {
  nip_heap h;

  if (initial_size <= 0){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_INVALID_ARGUMENT, 1);
    return NULL;
  }

  h = (nip_heap) malloc(sizeof(nip_heap_struct));
  if(!h){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_OUTOFMEMORY, 1);
    return NULL;
  }
  
  h->heap_items = (nip_heap_item*) calloc(initial_size, sizeof(nip_heap_item));
  if(!h->heap_items){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_OUTOFMEMORY, 1);
    free(h);
    return NULL;
  }

  h->heap_size = 0;
  h->allocated_size = initial_size;
  h->primary_key = primary;
  h->secondary_key = secondary;
  h->heapified = 0;
  
  return h;
}


int nip_heap_insert(nip_heap h, void* content, int size) {
  int i;
  nip_heap_item hi;
  nip_heap_item* bigger;

  /* Create a new heap element */
  hi = (nip_heap_item) malloc(sizeof(nip_heap_item_struct));
  if(!hi){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_OUTOFMEMORY, 1);
    return NIP_ERROR_OUTOFMEMORY;
  }
  hi->content = content;
  hi->content_size = size;
  hi->primary_key = (*h->primary_key)(hi->content, hi->content_size);
  hi->secondary_key = (*h->secondary_key)(hi->content, hi->content_size);

  /* Assign it to the heap */
  if(h->heap_size == h->allocated_size){
    /* time to expand */
    i = 2 * h->allocated_size;
    bigger = (nip_heap_item*) realloc(h->heap_items, i); /* realloc */
    if(bigger != NULL){
      h->heap_items = bigger;
      h->allocated_size = i;
      for(i = h->allocated_size-1; i >= h->heap_size; i--)
	h->heap_items[i] = NULL; /* Empty the newly allocated area */
    }
    else {
      nip_report_error(__FILE__, __LINE__, NIP_ERROR_OUTOFMEMORY, 1);
      return NIP_ERROR_OUTOFMEMORY;
    }
  }
  h->heap_items[h->heap_size] = hi;
  h->heap_size++;
  h->heapified = 0;

  return NIP_NO_ERROR;
}


static void nip_clean_heap_item(nip_heap h,
				nip_heap_item hi, 
				nip_heap_item min) {
    int i, j, n_vars, n_total;
    nip_variable v_i;
    nip_variable V_removed = ((nip_variable*)min->content)[0];
    nip_variable* cluster_vars;

    /* Copy all variables in hi and min together.
     * Copy hi first, because hi->content[0] must be the generating node 
     * also in the future */
    n_total = hi->content_size + min->content_size;
    cluster_vars = (nip_variable*) calloc(n_total, sizeof(nip_variable));
    if(!cluster_vars)
      return; /* FIXME: report error? */

    /*for (i = 0; i < hi->content_size; i++)
      cluster_vars[i] = hi->content[i];
      for (i = 0; i < min->content_size; i++)
      cluster_vars[hi->n +i] = min->content[i];*/

    memcpy(cluster_vars, hi->content, 
	   hi->content_size*sizeof(nip_variable));
    memcpy(cluster_vars+hi->content_size, min->content, 
	   min->content_size*sizeof(nip_variable));
    
    /* Remove duplicates and min_vs[0] */
    n_vars = 0;
    for (i = 0; i < n_total; i++) {
      v_i = cluster_vars[i];
      if (v_i == NULL) 
	continue;
      for (j = i+1; j < n_total; j++) {
	if (cluster_vars[j] == NULL) 
	  continue;
	if (nip_equal_variables(v_i, cluster_vars[j]) ||
	    nip_equal_variables(V_removed, cluster_vars[j]))
	  cluster_vars[j] = NULL;
      }
      cluster_vars[n_vars++] = v_i; /* Note: overwrites itself */
    }


#ifdef NIP_DEBUG_HEAP
    if(n_vars > hi->content_size){
      printf("Heap: expanding cluster from ");
      for (i=0; i<hi->content_size; i++)
	printf("%s ", ((nip_variable*)(hi->content))[i]->symbol);
      printf("to ");
      for (i=0; i<n_vars; i++)
	printf("%s ", cluster_vars[i]->symbol);
      printf("\n");
    }
#endif

    /* Replace the old content with updated one */
    hi->content_size = n_vars;
    free(hi->content);
    hi->content = calloc(n_vars, sizeof(nip_variable));
    if(!(hi->content)){
      free(cluster_vars);
      return;
    }
    memcpy(hi->content, cluster_vars, n_vars*sizeof(nip_variable));
    free(cluster_vars);

    hi->primary_key = (*h->primary_key)(hi->content, hi->content_size);
    hi->secondary_key = (*h->secondary_key)(hi->content, hi->content_size);

    return;
}


/* Heap management */

static int nip_heap_less_than(nip_heap_item h1, nip_heap_item h2) {
  if(h1!=NULL){
    if(h2!=NULL)
      return ((h1->primary_key < h2->primary_key) || 
	      (h1->primary_key == h2->primary_key && 
	       h1->secondary_key < h2->secondary_key));
    else
      return 1; /* h2 == NULL => belongs to the bottom of the heap */
  }
  return 0; /* h1 == NULL => belongs to the bottom */
}


void nip_build_min_heap(nip_heap h) {
  int i;
  if(!h)
    return;
  for (i = NIP_HEAP_PARENT(h->heap_size-1); i >= 0; i--)
    nip_min_heapify(h, i);
  h->heapified = 1;
}


void nip_min_heapify(nip_heap h, int i) {
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
	   nip_variable_symbol(((nip_variable*)min->content)[0]), 
	   min->content_size);
#endif
    
    /* Move the last one to the top */
    h->heap_items[0] = h->heap_items[h->heap_size-1]; 

    /* Mark this as NULL, so that free() won't be called twice */
    h->heap_items[h->heap_size-1] = NULL;

    h->heap_size--;

    /*--- begin weird stuff ---*/
    /* Iterate over potential join tree neighbours where the child node of the 
     * just removed (minimum cost) cluster has its parent as the child
     * ("grandparent clusters") and update keys. The loop could be heavy. */
    for (i = 1; i < min->content_size; i++) {
      heap_i = nip_heap_index(h, ((nip_variable*)min->content)[i]);
      nip_clean_heap_item(h, h->heap_items[heap_i], min);
    }

    /* Rebuild the heap. */
    for (i = 1; i < min->content_size; i++) {
      heap_i = nip_heap_index(h, ((nip_variable*)min->content)[i]);
      nip_min_heapify(h, heap_i);
    }
    nip_min_heapify(h, 0);
    /*--- end weird stuff --- */
    
    *cluster_vars = min->content;
    i = min->content_size;
    free(min);
    return i; /* Cluster size*/
}


int nip_extract_min_sepset(nip_heap h, nip_sepset* sepset) {
  nip_heap_item min; /* Sepset with smallest weight */
  
  /* Empty heap, nothing to extract. */
  if (h->heap_size < 1) {
    *sepset = NULL;
    return 0; /* no sepsets found */
  }
  
  min = h->heap_items[0];

  /* Return the min */
  *sepset = (nip_sepset)(min->content);
  free(min);
  
  /* Replace the root with the last item */
  h->heap_items[0] = h->heap_items[h->heap_size - 1];
  h->heap_items[h->heap_size - 1] = NULL;
  h->heap_size--;
    
  /* Rebuild the heap. Is this enough? */
  nip_min_heapify(h, 0);
  
  return 1; /* one sepset found */
}


static int nip_heap_index(nip_heap h, nip_variable v){
    /* Finds the heap element that contains the variable v */
    /* Linear search, but at the moment the only option */
    /* Hopefully this will not be too slow */
    int i;
    nip_variable child;
    nip_heap_item hi;
    /* DEBUG */
    /*printf("Heap looking for variable %s\n", v->symbol);*/

    for (i = 0; i < h->heap_size; i++) {
      hi = h->heap_items[i];
      /*printf("  heap item %d should have %d variables", i, hi->nvariables);*/
      child = ((nip_variable*)hi->content)[0]; /* FIXME! */
      /*printf("  comparing to child %s\n", child->symbol);*/
      if (nip_equal_variables(child, v))
	return i;
    }
    return -1;
}


void nip_free_heap(nip_heap h) {
  int i;
  nip_heap_item hi;

  if(!h)
    return;

  /* Go through every heap item*/
  for(i = 0; i < h->heap_size; i++){
    hi = h->heap_items[i];
    if(hi){
      free(hi);
    }
  }
  free(h->heap_items);
  free(h);
  return;
}

int nip_heap_size(nip_heap h) {
  if(h)
    return h->heap_size;
  return 0;
}
