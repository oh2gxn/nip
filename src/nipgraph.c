/* nipgraph.c $Id: nipgraph.c,v 1.1 2010-11-26 17:06:02 jatoivol Exp $
 */


#include "nipgraph.h"


static void nip_sort_graph_variables(nip_graph* G);

/*** GRAPH MANAGEMENT ***/

nip_graph* nip_new_graph(unsigned n) {
    nip_graph* newgraph = (nip_graph*) malloc(sizeof(nip_graph));
    if(!newgraph)
      return NULL;

    newgraph->size = n; newgraph->top = 0;

    newgraph->adj_matrix = (int*) calloc(n*n, sizeof(int));
    if(!(newgraph->adj_matrix)){
      free(newgraph);
      return NULL;
    }

    memset(newgraph->adj_matrix, 0, n*n*sizeof(int));

    newgraph->variables = (nip_variable*) calloc(n, sizeof(nip_variable));
    if(!(newgraph->variables)){
      free(newgraph->adj_matrix);
      free(newgraph);
      return NULL;
    }

    newgraph->var_ind = NULL;

    return newgraph;
}

nip_graph* nip_copy_graph(nip_graph* G) {
    int n, var_ind_size;
    nip_graph* G_copy;

    n = G->size;
    G_copy = new_graph(n);

    memcpy(G_copy->adj_matrix, G->adj_matrix, n*n*sizeof(int));
    memcpy(G_copy->variables, G->variables, n*sizeof(nip_variable));
    if (G->var_ind == NULL)
        G_copy->var_ind = NULL;
    else
    {
        var_ind_size = (G->max_id - G->min_id +1)*sizeof(long);

        G_copy->var_ind = (unsigned long*) malloc(var_ind_size);
	if(!(G_copy->var_ind)){
	  free_graph(G_copy);
	  return NULL;
	}

	memcpy(G_copy->var_ind, G->var_ind, var_ind_size);
        G_copy->max_id = G->max_id;
	G_copy->min_id = G->min_id;
    }

    return G_copy;
}

void nip_free_graph(nip_graph* G) { 
  if(G == NULL)
    return;
  free(G->adj_matrix);
  free(G->variables);
  free(G->var_ind); /* JJT: I added this here, but is it correct..? */
  free(G);
}

/*** GETTERS ***/

int nip_graph_size(nip_graph* G) {
    return G->size;
}

nip_variable* nip_graph_variables(nip_graph* G) {
    return G->variables;
}

int nip_graph_index(nip_graph* G, nip_variable v) {
    int i;

    /* NOTE: Relies on variable ids being nearly consecutive
     * If the implementation changes, this becomes a lot less
     * memory efficient and needs to be modified. */

    /* AR: Bugger. adjmatrix does not change. What if children were 
       added only after all variables have been included? */

    if (G->var_ind != NULL)
    {
        i = G->var_ind[nip_variable_id(v) - G->min_id];
        return nip_equal_variables(G->variables[i], v)? i: -1;
    }
    else /* Backup linear search */
        for (i = 0; i < G->size; i++)
            if (nip_equal_variables(G->variables[i], v))
                return i;

    return -1;
} 


int nip_get_neighbours(nip_graph* G, 
		       nip_variable* neighbours, 
		       nip_variable v) {
    int i, j;
    int n = nip_get_graph_size(G);
    int vi = nip_graph_index(G, v);

    j = 0;
    for (i = 0; i < n; i++)
        if (NIP_ADJM(G, vi, i))
            neighbours[j++] = G->variables[i];

    return j; /* # of neighbours */
}


int nip_graph_is_child(nip_graph* G, nip_variable parent, nip_variable child) {
    int i,j;
    i = nip_graph_index(G, parent); /* XX see refactorisation above */
    j = nip_graph_index(G, child);
    return NIP_ADJM(G, i, j);
}

/*** SETTERS ***/

int nip_graph_add_variable(nip_graph* G, nip_variable v){
    if (G->top == G->size)
      return NIP_ERROR_GENERAL; /* Cannot add more items. */

    G->variables[G->top] = v;
    G->top++;

    if (G->top == G->size)
      nip_sort_graph_variables(G);

    return NIP_NO_ERROR;
}

int nip_graph_add_child(nip_graph* G, nip_variable parent, nip_variable child){
    int parent_i, child_i;

    parent_i = nip_graph_index(G, parent);
    child_i = nip_graph_index(G, child);

    if (parent_i == -1 || child_i == -1)
	   return NIP_ERROR_GENERAL;

    NIP_ADJM(G, parent_i, child_i) = 1;

    return NIP_NO_ERROR;
}

/*** OPERATIONS (methods) ***/

/*int varcomp(nip_variable v1, nip_variable v2) {
    return nip_variable_id(v1) - nip_variable_id(v2);
}*/

static void nip_sort_graph_variables(nip_graph* G) {
    int i, id;
	
    G->min_id = nip_variable_id(G->variables[0]); 
    G->max_id = nip_variable_id(G->variables[0]);
    for (i = 1; i < G->size; i++)
    {
        id = nip_variable_id(G->variables[i]);
        G->min_id = (id < G->min_id)?id:G->min_id;
        G->max_id = (id > G->max_id)?id:G->max_id;
    }
    
    G->var_ind = (unsigned long*) calloc(G->max_id - G->min_id +1, 
					 sizeof(long));
    if(!(G->var_ind))
      return;
	
    for (i = 0; i < G->size; i++)
	   G->var_ind[nip_variable_id(G->variables[i]) - G->min_id] = i;

    return;
}

nip_graph* make_graph_undirected(nip_graph* G) {
    nip_graph* Gu;   
    int i,j,n;
    
    if (G == NULL || G->variables == NULL)
        return NULL;

    n = G->size;
    Gu = nip_copy_graph(G);

    /* Create the undirected graph */
    for (i = 0; i < n; i++)
        for (j = 0; j < n; j++)
            NIP_ADJM(Gu, i, j) = NIP_ADJM(G, i, j) || NIP_ADJM(G, j, i);

    return Gu;
}

nip_graph* nip_moralise(nip_graph* G) {
    int i,j,n,v;
    nip_graph* Gm;

    if (G == NULL || G->variables == NULL)
        return NULL;
    
    n = G->size;
    Gm = nip_copy_graph(G);

    /* Moralisation */
    for (v = 0; v < n; v++)       /* Iterate variables */
        for (i = 0; i < n; i++) 
            if (NIP_ADJM(G, i, v))            /* If i parent of v, find those */
                for (j = i+1; j < n; j++) /* parents of v which are > i */
                {
                    NIP_ADJM(Gm, i, j) |= NIP_ADJM(G, j, v);
                    NIP_ADJM(Gm, j, i) |= NIP_ADJM(G, j, v);
                }
                
    return Gm;
}

/* Additional interface edges. By Janne Toivola */
nip_graph* nip_add_interface_edges(nip_graph* G){
  int i,j,n;
  nip_variable v1, v2;
  nip_graph* Gi;
  
  if (G == NULL || G->variables == NULL)
    return NULL;
  
  n = G->size;
  Gi = nip_copy_graph(G);
  
  /* compare variable by variable */
  for (i = 0; i < n; i++)       /* Iterate variables */
    for (j = i+1; j < n; j++) {
      v1 = G->variables[i];
      v2 = G->variables[j];
      /* join interface variables */
      if (((NIP_IF(v1) & NIP_INTERFACE_OLD_OUTGOING) && 
	   (NIP_IF(v2) & NIP_INTERFACE_OLD_OUTGOING)) ||
	  ((NIP_IF(v1) & NIP_INTERFACE_OUTGOING) && 
	   (NIP_IF(v2) & NIP_INTERFACE_OUTGOING))) {
	NIP_ADJM(Gi, i, j) = 1;
	NIP_ADJM(Gi, j, i) = 1;
      }
    }
  return Gi;
}

/* an internal helper function */
int nip_triangulate_graph(nip_graph* Gm, clique** clique_p) {
    int i, j, j_index, k, k_index, n;
    int clique_count = 0;
    int cluster_size;
    nip_variable* min_cluster;
    Heap* H;
    Cluster_list *cl_head = NULL;
    int* variable_set; /* [i] true, if variable[i] is in the cluster */

    n = Gm->size;
    H = build_heap(Gm);

    variable_set = (int*) calloc(n, sizeof(int));
    if(!variable_set)
      return -1;

    for (i = 0; i < n; i++)
    {

      cluster_size = extract_min(H, Gm, &min_cluster);

        /* Clear the variable_set for this cluster */
        memset(variable_set, 0, n*sizeof(int));
           
        for (j = 0; j < cluster_size; j++)
        {
            j_index = nip_graph_index(Gm, min_cluster[j]);
            variable_set[j_index] = 1;
    
            /* Add new edges to Gm. */
            for (k = j+1; k < cluster_size; k++)
            {
                    k_index = nip_graph_index(Gm, min_cluster[k]);
    
                    NIP_ADJM(Gm, j_index, k_index) = 1;
                    NIP_ADJM(Gm, k_index, j_index) = 1;
            }
        }
        
        if (!is_subset(cl_head, variable_set, n))
        {
            cl_head = new_cl_item(n, cl_head, variable_set);
            clique_count++;
        }

	/* MVK: memory leak fix */
	free(min_cluster);
    }
    free(variable_set);

    *clique_p = cl2cliques(Gm->variables, cl_head, clique_count, n);

    free_heap(H);
 
    /* JJT: Free the list cl_head ??? */
    while(cl_head)
      cl_head = remove_cl_item(cl_head);
    
    return clique_count;
}


int nip_find_cliques(nip_graph* G, clique** cliques_p) {
    nip_graph *Gu, *Gm, *Gi;
    int n_cliques = 0;

    Gm = nip_moralise(G);
    Gi = nip_add_interface_edges(Gm); /* added by JJT 6.3.2006 */
    Gu = nip_make_graph_undirected(Gi);

    n_cliques = nip_triangulate_graph(Gu, cliques_p);

    /* Test if triangulate failed */
    if(n_cliques < 0){
      nip_free_graph(Gu);
      nip_free_graph(Gm);
      return -1;
    }

    if(find_sepsets(*cliques_p, n_cliques) != NIP_NO_ERROR)
      nip_report_error(__FILE__, __LINE__, NIP_ERROR_GENERAL, 1);

    /* JJT: I added some free_graph stuff here, because I suspected
     * memory leaks... */
    nip_free_graph(Gu);
    nip_free_graph(Gi);
    nip_free_graph(Gm);

    return n_cliques;
}
