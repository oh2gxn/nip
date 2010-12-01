/* nipgraph.c $Id: nipgraph.c,v 1.4 2010-12-01 13:34:11 jatoivol Exp $
 */


#include "nipgraph.h"


static void nip_sort_graph_variables(nip_graph g);

static nip_clique* nip_cluster_list_to_clique_array(nip_int_array_list clusters, nip_variable* vars, int n,);

/*** GRAPH MANAGEMENT ***/

nip_graph nip_new_graph(unsigned n) {
    nip_graph newgraph = (nip_graph) malloc(sizeof(nip_graph_struct));
    if(!newgraph)
      return NULL;

    newgraph->size = n; newgraph->top = 0;

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

    newgraph->var_ind = NULL;
    return newgraph;
}

nip_graph nip_copy_graph(nip_graph g) {
    int n, var_ind_size;
    nip_graph g_copy;

    n = g->size;
    g_copy = new_graph(n);

    memcpy(g_copy->adj_matrix, g->adj_matrix, n*n*sizeof(int));
    memcpy(g_copy->variables, g->variables, n*sizeof(nip_variable));
    if (g->var_ind == NULL)
      g_copy->var_ind = NULL;
    else {
      var_ind_size = (g->max_id - g->min_id +1)*sizeof(long);
      
      g_copy->var_ind = (unsigned long*) malloc(var_ind_size);
      if(!(g_copy->var_ind)){
	free_graph(g_copy);
	return NULL;
      }

      memcpy(g_copy->var_ind, g->var_ind, var_ind_size);
      g_copy->max_id = g->max_id;
      g_copy->min_id = g->min_id;
    }

    return g_copy;
}

void nip_free_graph(nip_graph g) { 
  if(g == NULL)
    return;
  free(g->adj_matrix);
  free(g->variables);
  free(g->var_ind); /* JJT: I added this here, but is it correct..? */
  free(g);
}

/*** GETTERS ***/

int nip_graph_size(nip_graph g) {
  if(g)
    return g->size;
  return -1;
}

nip_variable* nip_graph_variables(nip_graph g) {
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

    if (g->var_ind != NULL) {
      i = g->var_ind[nip_variable_id(v) - g->min_id];
      return nip_equal_variables(g->variables[i], v)? i: -1;
    }
    else /* Backup linear search */
      for (i = 0; i < g->size; i++)
	if (nip_equal_variables(g->variables[i], v))
	  return i;

    return -1;
} 


int nip_graph_neighbours(nip_graph g, nip_variable v,
			 nip_variable* neighbours) {
    int i, j;
    int n = nip_graph_size(g);
    int vi = nip_graph_index(g, v);

    if (n < 0 || vi < 0)
      return -1; /* invalid input */

    j = 0;
    for (i = 0; i < n; i++)
      if (NIP_ADJM(g, vi, i))
	neighbours[j++] = g->variables[i]; /* neighbours array big enough? */

    return j; /* # of neighbours */
}


int nip_graph_is_child(nip_graph g, nip_variable parent, nip_variable child) {
    int i,j;
    i = nip_graph_index(g, parent); /* XX see refactorisation above */
    j = nip_graph_index(g, child);
    if(i<0 || j<0)
      return 0; /* invalid input */
    return NIP_ADJM(g, i, j);
}

/*** SETTERS ***/

int nip_graph_add_variable(nip_graph g, nip_variable v){
    if (g->top == g->size)
      return NIP_ERROR_GENERAL; /* Cannot add more items. */

    g->variables[g->top] = v;
    g->top++;

    if (g->top == g->size)
      nip_sort_graph_variables(g);

    return NIP_NO_ERROR;
}

int nip_graph_add_child(nip_graph g, nip_variable parent, nip_variable child){
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

static void nip_sort_graph_variables(nip_graph g) {
    int i, id;

    if(!g)
      return;

    /* find min and max ids */
    g->min_id = nip_variable_id(g->variables[0]); 
    g->max_id = nip_variable_id(g->variables[0]);
    for (i = 1; i < g->size; i++) {
        id = nip_variable_id(g->variables[i]);
        g->min_id = (id < g->min_id)?id:g->min_id;
        g->max_id = (id > g->max_id)?id:g->max_id;
    }

    /* allocate new index array */
    if (g->var_ind)
      free(g->var_ind);
    g->var_ind = (unsigned long*) calloc(g->max_id - g->min_id + 1, 
					 sizeof(long));
    if(!(g->var_ind))
      return;
	
    for (i = 0; i < g->size; i++)
      g->var_ind[nip_variable_id(g->variables[i]) - g->min_id] = i;

    return;
}


nip_graph make_graph_undirected(nip_graph g) {
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


nip_graph nip_moralise(nip_graph g) {
    int i,j,n,v;
    nip_graph gm;

    if (g == NULL || g->variables == NULL)
      return NULL;
    
    n = g->size;
    gm = nip_copy_graph(g);

    /* Moralisation */
    for (v = 0; v < n; v++)       /* Iterate variables */
      for (i = 0; i < n; i++) 
	if (NIP_ADJM(g, i, v))            /* If i parent of v, find those */
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
    int clique_counter = n_cliques;
    nip_int_array_link lnk;
    nip_clique* cliques;
    nip_variable* clique_vars;

    cliques = (nip_clique*) calloc(n_cliques, sizeof(nip_clique));
    if(!cliques)
      return NULL;

    clique_vars = (nip_variable*) calloc(n, sizeof(nip_variable));
    if(!clique_vars){
      free(cliques);
      return NULL;
    }

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
	for(i = clique_counter + 1; i < n_cliques; i++)
	  free_clique(cliques[i]);
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
    nip_variable* min_cluster;
    nip_heap h;
    nip_int_array_list clusters = NULL;
    int* variable_set; /* [i] true, if variable[i] is in the cluster */

    n = gm->size;
    h = nip_build_cluster_heap(gm); /* TODO: refactor this stuff */
    clusters = nip_new_int_array_list();

    for (i = 0; i < n; i++) {

      cluster_size = nip_extract_min_cluster(h, gm, &min_cluster);
      
      /* New variable_set for this cluster */
      variable_set = (int*) calloc(n, sizeof(int));
      if(!variable_set) {
	/* FIXME: better error handling */
	nip_free_int_array_list(clusters);
	return -1;
      }
      /*memset(variable_set, 0, n*sizeof(int));/*calloc did this*/
      
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

      /* Add the cluster to a list of cliques if valid */
      if (!nip_int_array_list_contains_subset(clusters, variable_set, n))
	nip_prepend_int_array(clusters, variable_set, n);
      else
	free(variable_set);

      /* MVK: memory leak fix */
      free(min_cluster);
    }

    /* Create a set of cliques from the found variable sets */
    *clique_p = nip_cluster_list_to_clique_array(clusters, gm->variables, n);
    clique_count = NIP_LIST_LENGTH(clusters);

    nip_free_heap(h);
    nip_free_int_array_list(clusters); /* JJT: Free the cluster list */
    
    return clique_count;
}


int nip_find_cliques(nip_graph g, nip_clique** cliques_p) {
    nip_graph gu, gm, gi;
    int n_cliques = 0;

    gm = nip_moralise(g);
    gi = nip_add_interface_edges(gm); /* added by JJT 6.3.2006 */
    nip_free_graph(gm);
    gu = nip_make_graph_undirected(gi);
    nip_free_graph(gi);

    /* triangulate and create a set of cliques */
    n_cliques = nip_triangulate_graph(gu, cliques_p);

    /* test if triangulate failed */
    if(n_cliques < 0){
      nip_free_graph(gu);
      return -1;
    }

    /* find a set of suitable sepsets to connect the cliques */
    if(nip_find_sepsets(*cliques_p, n_cliques) != NIP_NO_ERROR)
      nip_report_error(__FILE__, __LINE__, NIP_ERROR_GENERAL, 1);

    /* JJT: I added some free_graph stuff here, because I suspected
     * memory leaks... */
    nip_free_graph(gu);

    return n_cliques;
}

/*** Used to be in clique.c for some odd reason ***/
int nip_find_sepsets(nip_clique *cliques, int num_of_cliques){

  int inserted = 0;
  int i;
  nip_sepset s;
  nip_clique one, two;

#ifdef NIP_DEBUG_CLIQUE
  int j, k;
  int ok = 1;
#endif

  Heap *H = nip_build_sepset_heap(cliques, num_of_cliques);

  if(!H){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_GENERAL, 1);
    return NIP_ERROR_GENERAL;
  }

  while(inserted < num_of_cliques - 1){

    /* Extract the "best" candidate sepset from the heap. */
    if(extract_min_sepset(H, &s)){

      /* In case of error */
      free_heap(H);
      nip_report_error(__FILE__, __LINE__, NIP_ERROR_GENERAL, 1);
      return NIP_ERROR_GENERAL;
    }

    one = s->cliques[0];
    two = s->cliques[1];

    /* Unmark MUST be done before searching (clique_search). */
    for(i = 0; i < num_of_cliques; i++)
      unmark_clique(cliques[i]);

    /* Prevent loops by checking if the cliques
     * are already in the same tree. */
    if(!clique_search(one, two)){

      mark_useful_sepset(H, s); /* MVK */

#ifdef NIP_DEBUG_CLIQUE
      printf("In nipgraph.c: Trying to add ");
      print_sepset(s);

      printf(" to ");
      print_clique(one);

      printf(" and ");
      print_clique(two);
#endif

      if(add_sepset(one, s) != NIP_NO_ERROR){
	free_heap(H);
	nip_report_error(__FILE__, __LINE__, NIP_ERROR_GENERAL, 1);
	return NIP_ERROR_GENERAL;
      }	
      if(add_sepset(two, s) != NIP_NO_ERROR){
	free_heap(H);
	nip_report_error(__FILE__, __LINE__, NIP_ERROR_GENERAL, 1);
	return NIP_ERROR_GENERAL;
      }

      inserted++;
    }
  }

  free_heap(H);

  return NIP_NO_ERROR;
}


/*** old cls2clq.c ***/
