/* nipgraph.c $Id: nipgraph.c,v 1.26 2011-03-14 11:09:34 jatoivol Exp $
 */

#include "nipgraph.h"

/*#define NIP_DEBUG_GRAPH*/

/* Internal helper functions */
static nip_error_code nip_build_graph_index(nip_graph g);

static nip_clique* nip_cluster_list_to_clique_array(nip_int_array_list clusters, nip_variable* vars, int n);

/* Function for computing primary keys for cluster heap */
static int nip_cluster_primary_cost(void* variables, int n);

/* Function for computing secondary keys for cluster heap */
static int nip_cluster_secondary_cost(void* variables, int n);

/* Function for computing primary keys for sepset heap */
static int nip_sepset_primary_cost(void* sepset, int n);

/* Function for computing secondary keys for sepset heap */
static int nip_sepset_secondary_cost(void* sepset, int n);

/* Creates a heap of nip_variable arrays (possible cliques) from 
 * an undirected and moralized graph gm */
static nip_heap nip_build_cluster_heap(nip_graph gm);

/* Callback function for searching certain heap items */
static int nip_family_cluster(void* i, int isize, void* r, int rsize);

/* Updates remaining candidate clusters after removing one... */
static nip_error_code nip_update_cluster_heap(nip_heap h, 
					      nip_variable* cluster, 
					      int csize);

/* Creates a heap of possible sepsets from a set of cliques */
static nip_heap nip_build_sepset_heap(nip_clique* cliques, int num_of_cliques);


/*** GRAPH MANAGEMENT ***/

nip_graph nip_new_graph(unsigned n) {
    nip_graph newgraph = (nip_graph) malloc(sizeof(nip_graph_struct));
    if(!newgraph)
      return NULL;

    newgraph->size = n; 
    newgraph->top = 0;

    newgraph->adj_matrix = (int*) calloc(n*n, sizeof(int));
    if(!(newgraph->adj_matrix)){
      free(newgraph);
      return NULL;
    }
    memset(newgraph->adj_matrix, 0, n*n*sizeof(int)); /* calloc does this? */

    newgraph->variables = (nip_variable*) calloc(n, sizeof(nip_variable));
    if(!(newgraph->variables)){
      free(newgraph->adj_matrix);
      free(newgraph);
      return NULL;
    }

    newgraph->min_id = NIP_VAR_INVALID_ID;
    newgraph->max_id = NIP_VAR_INVALID_ID;
    newgraph->var_ind = NULL;
    return newgraph;
}

nip_graph nip_copy_graph(nip_graph g) {
    int n;
    nip_graph g_copy;

    n = g->size;
    g_copy = nip_new_graph(n);

    memcpy(g_copy->adj_matrix, g->adj_matrix, n*n*sizeof(int));
    memcpy(g_copy->variables, g->variables, n*sizeof(nip_variable));
    g_copy->max_id = g->max_id;
    g_copy->min_id = g->min_id;
    g_copy->top = g->top; /* remember this too! 24.1.2011 */

    g_copy->var_ind = NULL; /* this gets sorted during the first search */

    return g_copy;
}

void nip_free_graph(nip_graph g) { 
  if(g == NULL)
    return;
  free(g->adj_matrix);
  free(g->variables);
  free(g->var_ind);
  free(g);
}

/*** GETTERS ***/

int nip_graph_size(nip_graph g) {
  if(g)
    return g->top; /* JJT: changed from g->size */
  return -1;
}

nip_variable* nip_graph_nodes(nip_graph g) {
  if(g)
    return g->variables; /* g still responsible for freeing it */
  return NULL;
}

int nip_graph_index(nip_graph g, nip_variable v) {
    int i;

    /* NOTE: Relies on variable ids being nearly consecutive
     * If the implementation changes, this becomes a lot less
     * memory efficient and needs to be modified. */

    /* AR: Bugger. adjmatrix does not change. What if children were 
       added only after all variables have been included? */

    if(g == NULL || v == NULL)
      return -1;

    if (g->var_ind == NULL) {
      /* no index, try to build one */
      if (nip_build_graph_index(g) != NIP_NO_ERROR) {
	/* backup linear search */
	for (i = 0; i < g->top; i++)
	  if (nip_equal_variables(g->variables[i], v))
	    return i;
	return -1;
      }
    }
    /* we have an index, let's use it */
    i = g->var_ind[nip_variable_id(v) - g->min_id];
    return nip_equal_variables(g->variables[i], v)? i: -1;   
} 


int nip_graph_cluster(nip_graph g, nip_variable v,
		      nip_variable** neighbours) {
  int i, j, n, vi;
  nip_variable* cluster;
  
  if (g == NULL){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_NULLPOINTER, 1);
    return -1;
  }
  
  n = nip_graph_size(g);
  vi = nip_graph_index(g, v);
  if (n < 0 || vi < 0){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_INVALID_ARGUMENT, 1);
    return -1; /* invalid input */
  }
  
  /* Count number of neighbours */
  j = 0;
  for (i = 0; i < n; i++)
    if (NIP_ADJM(g, vi, i)) /* NOTE: assumes ADJM(g, i, i) == 0 !!! */
      j++;

  /* Allocate array */
  cluster = (nip_variable*) calloc(j+1, sizeof(nip_variable));
  if (cluster == NULL){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_OUTOFMEMORY, 1);
    return -1;
  }
  
  /* Populate the array */
  cluster[0] = v;
  j = 1;
  for (i = 0; i < n; i++)
    if (NIP_ADJM(g, vi, i))
      cluster[j++] = g->variables[i];
  
  *neighbours = cluster;
  return j; /* # of neighbours + 1 */
}


int nip_graph_linked(nip_graph g, nip_variable parent, nip_variable child) {
    int i,j;
    i = nip_graph_index(g, parent);
    j = nip_graph_index(g, child);
    if(i<0 || j<0)
      return 0;
    return NIP_ADJM(g, i, j);
}

/*** SETTERS ***/

nip_error_code nip_graph_add_node(nip_graph g, nip_variable v){
  int id;
  if (g->top == g->size)
    return NIP_ERROR_GENERAL; /* Cannot add more items. */
  
  g->variables[g->top] = v;
  g->top++;
  
  id = nip_variable_id(v);
  if (id < g->min_id || g->min_id == NIP_VAR_INVALID_ID){
    g->min_id = id;
    /* TODO: update g->var_ind ? */
  }
  if (id > g->max_id || g->max_id == NIP_VAR_INVALID_ID){
    g->max_id = id;
    /* TODO: update g->var_ind ? */
  }
  if(g->var_ind != NULL){
    free(g->var_ind); /* old var_ind became invalid */
    g->var_ind = NULL;
  }

  if (g->top == g->size)
    nip_build_graph_index(g); /* TODO: continuous update? */
  
  return NIP_NO_ERROR;
}

nip_error_code nip_graph_add_child(nip_graph g, 
				   nip_variable parent, 
				   nip_variable child){
    int parent_i, child_i;

    parent_i = nip_graph_index(g, parent);
    child_i = nip_graph_index(g, child);

    if (parent_i < 0 || child_i < 0)
      return NIP_ERROR_INVALID_ARGUMENT;

    NIP_ADJM(g, parent_i, child_i) = 1;

    return NIP_NO_ERROR;
}

/*** OPERATIONS (methods) ***/

/*int varcomp(nip_variable v1, nip_variable v2) {
    return nip_variable_id(v1) - nip_variable_id(v2);
}*/

static nip_error_code nip_build_graph_index(nip_graph g) {
    int i;

    if(!g)
      return NIP_ERROR_NULLPOINTER;

    /* allocate new index array */
    if (g->var_ind)
      free(g->var_ind);
    g->var_ind = (unsigned long*) calloc(g->max_id - g->min_id + 1, 
					 sizeof(long));
    if(!(g->var_ind))
      return NIP_ERROR_OUTOFMEMORY;

    /* compute the indices */
    for (i = 0; i < g->top; i++)
      g->var_ind[nip_variable_id(g->variables[i]) - g->min_id] = i;

    return NIP_NO_ERROR;
}


nip_graph nip_make_graph_undirected(nip_graph g) {
    nip_graph gu;   
    int i,j,n;
    
    if (g == NULL || g->variables == NULL)
        return NULL;

    n = g->size;
    gu = nip_copy_graph(g);

    /* Create the undirected graph */
    for (i = 0; i < n; i++)
      for (j = 0; j < n; j++)
	NIP_ADJM(gu, i, j) = NIP_ADJM(g, i, j) || NIP_ADJM(g, j, i);

    return gu;
}


nip_graph nip_moralise_graph(nip_graph g) {
    int i,j,n,v;
    nip_graph gm;

    if (g == NULL || g->variables == NULL)
      return NULL;
    
    n = g->size;
    gm = nip_copy_graph(g);

    /* Moralisation */
    for (v = 0; v < n; v++)       /* Iterate variables */
      for (i = 0; i < n; i++) 
	if (NIP_ADJM(g, i, v))       /* If i parent of v, find those */
	  for (j = i+1; j < n; j++){ /* parents of v which are > i */
	    NIP_ADJM(gm, i, j) |= NIP_ADJM(g, j, v);
	    NIP_ADJM(gm, j, i) |= NIP_ADJM(g, j, v);
	  }
    
    return gm;
}

/* Additional interface edges. By Janne Toivola */
nip_graph nip_add_interface_edges(nip_graph g){
  int i,j,n;
  nip_variable v1, v2;
  nip_graph gi;
  
  if (g == NULL || g->variables == NULL)
    return NULL;
  
  n = g->size;
  gi = nip_copy_graph(g);
  
  /* compare variable by variable */
  for (i = 0; i < n; i++)       /* Iterate variables */
    for (j = i+1; j < n; j++) {
      v1 = g->variables[i];
      v2 = g->variables[j];
      /* join interface variables */
      if (((NIP_IF(v1) & NIP_INTERFACE_OLD_OUTGOING) && 
	   (NIP_IF(v2) & NIP_INTERFACE_OLD_OUTGOING)) ||
	  ((NIP_IF(v1) & NIP_INTERFACE_OUTGOING) && 
	   (NIP_IF(v2) & NIP_INTERFACE_OUTGOING))) {
	NIP_ADJM(gi, i, j) = 1;
	NIP_ADJM(gi, j, i) = 1;
      }
    }
  return gi;
}


static nip_clique* nip_cluster_list_to_clique_array(nip_int_array_list clusters, nip_variable* vars, int n) {
    int n_vars, i;
    int ncliques, clique_counter;
    nip_int_array_link lnk;
    nip_clique* cliques;
    nip_variable* clique_vars;

    ncliques = NIP_LIST_LENGTH(clusters);

    cliques = (nip_clique*) calloc(ncliques, sizeof(nip_clique));
    if(!cliques)
      return NULL;

    clique_vars = (nip_variable*) calloc(n, sizeof(nip_variable));
    if(!clique_vars){
      free(cliques);
      return NULL;
    }

    clique_counter = ncliques;
    for (lnk = NIP_LIST_ITERATOR(clusters); 
	 lnk != NULL; 
	 lnk = NIP_LIST_NEXT(lnk)) {

      /* Fill the array of clique variables */
      n_vars = 0;
      for (i = 0; i < n; i++)
	if (lnk->data[i])
	  clique_vars[n_vars++] = vars[i];

      /* Create a new clique */
      cliques[--clique_counter] = nip_new_clique(clique_vars, n_vars);
      /* This ^^^^^^^^^^^^^^^^ is sort of dangerous. */

      /* Clean up in case of errors */
      if(cliques[clique_counter] == NULL){
	for(i = clique_counter + 1; i < ncliques; i++)
	  nip_free_clique(cliques[i]);
	free(clique_vars);
	free(cliques);
	return NULL;
      }
    }
    
    free(clique_vars);
    return cliques;
}


int nip_triangulate_graph(nip_graph gm, nip_clique** clique_p) {
    int i, j, j_index, k, k_index, n;
    int clique_count = 0;
    int cluster_size;
    void* min_item;
    nip_variable* min_cluster;
    nip_heap h;
    nip_int_array_list clusters = NULL;
    int* variable_set; /* [i] true, if variable[i] is in the cluster */

    n = gm->size;

    /* JJT: does the clique listing have anything to do with 
     * the Bron-Kerbosch algorithm or Tsukiyama et al. 1977? */

    /* Create a heap of variable clusters */
    h = nip_build_cluster_heap(gm);

    /* Extract a list of potentially good clusters */
    clusters = nip_new_int_array_list();
    for (i = 0; i < n; i++) {
      min_item = nip_heap_extract_min(h, &cluster_size);
      min_cluster = (nip_variable*) min_item;

      /* New variable_set for this cluster */
      variable_set = (int*) calloc(n, sizeof(int));
      if(!variable_set) {
	nip_report_error(__FILE__, __LINE__, NIP_ERROR_OUTOFMEMORY, 1);
	nip_free_int_array_list(clusters);
	while(NULL != (min_item = nip_heap_extract_min(h, NULL)))
	  free((nip_variable*)min_item);
	nip_free_heap(h);
	return -1;
      }
      /*memset(variable_set, 0, n*sizeof(int)); // calloc did this*/
      
      for (j = 0; j < cluster_size; j++) {
	/* Find out which variables belong to the cluster */
	j_index = nip_graph_index(gm, min_cluster[j]);
	variable_set[j_index] = 1;
    
	/* Add new edges to Gm. */
	for (k = j+1; k < cluster_size; k++) {
	  k_index = nip_graph_index(gm, min_cluster[k]);	  
	  NIP_ADJM(gm, j_index, k_index) = 1;
	  NIP_ADJM(gm, k_index, j_index) = 1;
	}
      }

      /* Update certain clusters, heapify appropriately */
      nip_update_cluster_heap(h, min_cluster, cluster_size);

      /* Add the cluster to a list of cliques if valid */
      if (!nip_int_array_list_contains_subset(clusters, variable_set, n))
	nip_prepend_int_array(clusters, variable_set, n);
      else
	free(variable_set);

      free(min_cluster); /* MVK: memory leak fix */
    }

    /* Create a set of cliques from the found variable sets */
    *clique_p = nip_cluster_list_to_clique_array(clusters, gm->variables, n);
    clique_count = NIP_LIST_LENGTH(clusters);

    /* free the rest of nip_variable arrays stored in the heap */
    while(NULL != (min_item = nip_heap_extract_min(h, NULL)))
      free((nip_variable*) min_item);
    nip_free_heap(h);
    nip_free_int_array_list(clusters); /* JJT: Free the cluster list */
    
    return clique_count;
}


int nip_graph_to_cliques(nip_graph g, nip_clique** cliques_p) {
    nip_graph gu, gm, gi;
    int n_cliques = 0;

    gm = nip_moralise_graph(g);
    gi = nip_add_interface_edges(gm); /* added by JJT 6.3.2006 */
    nip_free_graph(gm);
    gu = nip_make_graph_undirected(gi);
    nip_free_graph(gi);

    /* triangulate and create a set of cliques */
    n_cliques = nip_triangulate_graph(gu, cliques_p);
    if(n_cliques < 0){
      nip_free_graph(gu);
      return -1;
    }

    /* find a set of suitable sepsets to connect the cliques */
    if(nip_create_sepsets(*cliques_p, n_cliques) != NIP_NO_ERROR)
      nip_report_error(__FILE__, __LINE__, NIP_ERROR_GENERAL, 1);

    /* free the modified graph */
    nip_free_graph(gu);

    return n_cliques;
}

/*** Used to be in clique.c for some odd reason ***/
nip_error_code nip_create_sepsets(nip_clique *cliques, int num_of_cliques){

  int inserted = 0;
  int i;
  void* item;
  nip_sepset s;
  nip_clique one, two;

  /* Create a heap of candidate sepsets */
  nip_heap h = nip_build_sepset_heap(cliques, num_of_cliques);
  if(!h){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_GENERAL, 1);
    return NIP_ERROR_GENERAL;
  }

  /* Take the n-1 most promising and useful sepsets */
  while(inserted < num_of_cliques - 1){
    /* Extract the "best" candidate sepset */
    if(NULL != (item = nip_heap_extract_min(h, NULL))){
      nip_free_heap(h);
      nip_report_error(__FILE__, __LINE__, NIP_ERROR_GENERAL, 1);
      return NIP_ERROR_GENERAL;
    }
    s = (nip_sepset) item;

    /* Check who the neighbour cliques would be */
    one = s->first_neighbour;
    two = s->second_neighbour;

    /* Unmark MUST be done before searching (cliques_connected). */
    for(i = 0; i < num_of_cliques; i++)
      nip_unmark_clique(cliques[i]);

    /* Prevent loops in the join tree by checking if the cliques
     * are already in the same tree. */
    if(!nip_cliques_connected(one, two)){

#ifdef NIP_DEBUG_GRAPH
      printf("%s: Trying to add ", __FILE__);
      nip_fprintf_sepset(stdout,s);

      printf(" between ");
      nip_fprintf_clique(stdout,one);

      printf(" and ");
      nip_fprintf_clique(stdout,two);
#endif

      /* Connect s */
      if(nip_confirm_sepset(s) != NIP_NO_ERROR){
	nip_free_heap(h);
	nip_report_error(__FILE__, __LINE__, NIP_ERROR_GENERAL, 1);
	return NIP_ERROR_GENERAL;
      }	

      inserted++;
    }
  }
    
  /* Free the useless sepsets */
  while(NULL != (item = nip_heap_extract_min(h, NULL)))
    nip_free_sepset((nip_sepset)item);
  nip_free_heap(h);

  return NIP_NO_ERROR;
}


/*** Refactored from Heap.c ***/
static int nip_cluster_primary_cost(void* variables, int n) {
    /* vs is the array of variables in the cluster induced by vs[0] */
    int i,j, sum = 0;

    for (i = 0; i < n; i++)
      for (j = i+1; j < n; j++)
	sum += !nip_variable_is_parent(((nip_variable*)variables)[i], 
				       ((nip_variable*)variables)[j]);
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

static int nip_cluster_secondary_cost(void* variables, int n) {
    /* vs is the array of variables in the cluster induced by vs[0] */
    int i, prod = 1;    
    for (i = 0; i < n; i++)
      prod *= NIP_CARDINALITY(((nip_variable*)variables)[i]);
    return prod;
}

static int nip_sepset_primary_cost(void* sepset, int n) {
  if (n>0)
    return -(nip_sepset_size((nip_sepset)sepset));
  else 
    return 0;
}

static int nip_sepset_secondary_cost(void* sepset, int n) {
  int c = 0;
  nip_sepset s = (nip_sepset) sepset;
  nip_clique nc;
  if(!s){
    nc = s->first_neighbour;
    if(nc)
      c += nip_cluster_secondary_cost((void*)(nc->variables), 
				      nip_clique_size(nc));
    nc = s->second_neighbour;
    if(nc)
      c += nip_cluster_secondary_cost((void*)(nc->variables),
				      nip_clique_size(nc));
  }
  return c;
}


static nip_heap nip_build_cluster_heap(nip_graph gm) {
  int i,n;
  int csize;
  void* item;
  nip_variable* cluster;
  nip_heap h;
  
  n = nip_graph_size(gm);

  /* content[0] is the cluster-inducing child variable in the array, 
   * rest are neighbours, when clusters of variables are concerned */
  
  /* Create an empty heap */
  h = nip_new_heap(n, nip_cluster_primary_cost, nip_cluster_secondary_cost);
  if(!h)
    return NULL;
  
  /* Populate the heap */
  for (i = 0; i < n; i++) {
    csize = nip_graph_cluster(gm, gm->variables[i], &cluster);
    if(csize < 0){
      /* Something went wrong, clean up */
      while(NULL != (item = nip_heap_extract_min(h, NULL)))
	free((nip_variable*)item);
      nip_free_heap(h);
    }

    nip_heap_insert(h, (void*) cluster, csize);

#ifdef NIP_DEBUG_GRAPH
    printf("%s: possible clique ", __FILE__);
    while (0 < csize--)
      printf("%s,", nip_variable_symbol(cluster[csize]));
    printf("\n");
#endif
  }

  /* Make it obey the heap property */
  nip_build_min_heap(h);
  
  return h;
}


static int nip_family_cluster(void* i, int isize, void* r, int rsize){
  if (isize < 1 || rsize != 1)
    return 0; /* invalid input */
  if (nip_equal_variables(((nip_variable*)i)[0], ((nip_variable*)r)[0]))
    return 1;
  return 0;
}


static nip_error_code nip_update_cluster_heap(nip_heap h, 
					      nip_variable* cluster, 
					      int csize){
  int i, j, k, index;
  int osize;
  void* old_item;
  nip_variable* old_cluster;
  int nsize;
  nip_variable* new_cluster;
  nip_variable v, v_i;
  nip_variable v_removed = cluster[0];
  
  /* Iterate over potential join tree neighbours where the child node of the 
   * just removed (minimum cost) cluster has its neighbours as the center
   * ("neighbour clusters") and update keys. The loop could be heavy. */
  for (i = 1; i < csize; i++){
    v = cluster[i]; /* search for neighbour clusters */
    index = nip_search_heap_item(h, *nip_family_cluster, &v, 1);
    old_item = nip_get_heap_item(h, index, &osize);
    old_cluster = (nip_variable*) old_item;

    /* Union of old_cluster[1...end] and cluster[1...end] 
     * NOTE: new_cluster[0] == old_cluster[0] */
    new_cluster = nip_variable_union(old_cluster, cluster,
				      osize, csize, &nsize);
    if(!new_cluster){
      nip_report_error(__FILE__, __LINE__, NIP_ERROR_GENERAL, 1);
      return NIP_ERROR_GENERAL;
    }
    assert(new_cluster[0] == old_cluster[0]);

    /* Remove cluster[0] */
    k = 0;
    for (j = 0; j < nsize; j++) {
      v_i = new_cluster[j];
      new_cluster[j] = NULL;
      if (!nip_equal_variables(v_removed, v_i))
	new_cluster[k++] = v_i; /* Note: overwrites itself */
    }
    /* if (k == nsize-1), new_cluster was allocated one element too much */

#ifdef NIP_DEBUG_HEAP
    printf("%s: changing cluster from ", __FILE__);
    for (j=0; j<osize; j++)
      printf("%s ", nip_variable_symbol(old_cluster[j]));
    printf("to ");
    for (j=0; j<k; j++)
      printf("%s ", nip_variable_symbol(new_cluster[j]));
    printf("\n");
#endif

    free(old_cluster); /* FIXME: here or in nip_set_heap_item? */
    nip_set_heap_item(h, index, (void*)new_cluster, k);
  }

  nip_build_min_heap(h); /* Rebuild the heap. */
  return NIP_NO_ERROR;
}


static nip_heap nip_build_sepset_heap(nip_clique* cliques, 
				      int num_of_cliques) {
  int i,j;
  int n = (num_of_cliques * (num_of_cliques - 1)) / 2;
  nip_clique nca, ncb;
  nip_sepset s;
  void* item;
  
  nip_heap h = nip_new_heap(n, nip_sepset_primary_cost, 
			    nip_sepset_secondary_cost);
  if(!h)
    return NULL;
  
  /* Go through each pair of cliques. Create candidate sepsets. */
  for(i = 0; i < num_of_cliques - 1; i++) {
    for(j = i + 1; j < num_of_cliques; j++) {
      nca = cliques[i];
      ncb = cliques[j];

      /* Make a new sepset */
      s = nip_new_sepset(nca, ncb);
      if(!s){
	/* In case of failure, free all sepsets and the heap. */
	nip_report_error(__FILE__, __LINE__, NIP_ERROR_GENERAL, 1);
	while(NULL != (item = nip_heap_extract_min(h, NULL)))
	  nip_free_sepset((nip_sepset)item);
	nip_free_heap(h);
	return NULL;
      }

      /* Put it in the heap */
      nip_heap_insert(h, s, 1);
    }
  }
  
  /* Sort the heap */
  nip_build_min_heap(h);
  
  return h;
}
