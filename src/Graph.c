/*
 * Graph.c $Id: Graph.c,v 1.43 2004-08-25 11:14:41 mvkorpel Exp $
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "Graph.h"
#include "Variable.h"
#include "Clique.h"
#include "grphmnp/Heap.h"
#include "grphmnp/cls2clq.h"
#include "errorhandler.h"

static void sort_variables(Graph* G);

/*** GRAPH MANAGEMENT ***/

Graph* new_graph(unsigned n)
{
    Graph* newgraph = (Graph*) malloc(sizeof(Graph));
    if(!newgraph)
      return NULL;

    newgraph->size = n; newgraph->top = 0;

    newgraph->adj_matrix = (int*) calloc(n*n, sizeof(int));
    if(!(newgraph->adj_matrix)){
      free(newgraph);
      return NULL;
    }

    memset(newgraph->adj_matrix, 0, n*n*sizeof(int));

    newgraph->variables = (Variable*) calloc(n, sizeof(Variable));
    if(!(newgraph->variables)){
      free(newgraph->adj_matrix);
      free(newgraph);
      return NULL;
    }

    newgraph->var_ind = NULL;

    return newgraph;
}

Graph* copy_graph(Graph* G)
{
    int n, var_ind_size;
    Graph* G_copy;

    n = G->size;
    G_copy = new_graph(n);

    memcpy(G_copy->adj_matrix, G->adj_matrix, n*n*sizeof(int));
    memcpy(G_copy->variables, G->variables, n*sizeof(Variable));
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

void free_graph(Graph* G) /* XX Onko täysin valmis? */
{
    if(G == NULL)
        return;
        
    free(G->adj_matrix);
    free(G->variables);
    free(G->var_ind); /* JJT: I added this here, but is it correct... */
    free(G);
}

/*** GETTERS ***/

int get_size(Graph* G)
{
    return G->size;
}

Variable* get_variables(Graph* G)
{
    return G->variables;
}

int get_graph_index(Graph* G, Variable v)
{
    int i;

    /* NOTE: Relies on variable ids being nearly consecutive
     * If the implementation changes, this becomes a lot less
     * memory efficient and needs to be modified. */

    /* Voi räkä. adjmatrix ei muutu. Jospa lapsia vois lisätä vasta kun
       kaikki muuttujat on lisätty?*/

    if (G->var_ind != NULL)
    {
        i = G->var_ind[get_id(v) - G->min_id];
        return equal_variables(G->variables[i], v)? i: -1;
    }
    else /* Backup linear search */
        for (i = 0; i < G->size; i++)
            if (equal_variables(G->variables[i], v))
                return i;

    return -1;
} 

int get_neighbours(Graph* G, Variable* neighbours, Variable V)
{
    int i, j;
    int n = get_size(G);
    int vi = get_graph_index(G, V);

	j = 0;
    for (i = 0; i < n; i++)
        if (ADJM(G, vi, i))
            neighbours[j++] = G->variables[i];

    return j; /* # of neighbours */
}

int is_child(Graph* G, Variable parent, Variable child)
{
    int i,j;
    i = get_graph_index(G, parent); /* XX kts. refaktorointi ylhäältä */
    j = get_graph_index(G, child);
    return ADJM(G, i, j);
}

/*** SETTERS ***/

int add_variable(Graph* G, Variable v)
{
    if (G->top == G->size)
	   return ERROR_GENERAL; /* Cannot add more items. */

    G->variables[G->top] = v;
    G->top++;

    if (G->top == G->size)
		sort_variables(G);

    return NO_ERROR;
}

int add_child(Graph* G, Variable parent, Variable child)
{
    int parent_i, child_i;

    parent_i = get_graph_index(G, parent);
    child_i = get_graph_index(G, child);

    if (parent_i == -1 || child_i == -1)
	   return ERROR_GENERAL;

    ADJM(G, parent_i, child_i) = 1;

    return NO_ERROR;
}

/*** OPERATIONS (methods) ***/

/*int varcomp(Variable v1, Variable v2) {
    return get_id(v1) - get_id(v2);
}*/

static void sort_variables(Graph* G) 
{
    int i, id;
	
    G->min_id = get_id(G->variables[0]); G->max_id = get_id(G->variables[0]);
    for (i = 1; i < G->size; i++)
    {
        id = get_id(G->variables[i]);
        G->min_id = (id < G->min_id)?id:G->min_id;
        G->max_id = (id > G->max_id)?id:G->max_id;
    }
    
    G->var_ind = (unsigned long*) calloc(G->max_id - G->min_id +1, 
					 sizeof(long));
    if(!(G->var_ind))
      return;
	
    for (i = 0; i < G->size; i++)
	   G->var_ind[get_id(G->variables[i]) - G->min_id] = i;

    return;
}

Graph* make_undirected(Graph* G)
{
    Graph* Gu;   
    int i,j,n;
    
    if (G == NULL || G->variables == NULL)
        return NULL;

    n = G->size;
    Gu = copy_graph(G);

    /* Create the undirected graph */
    for (i = 0; i < n; i++)
        for (j = 0; j < n; j++)
            ADJM(Gu, i, j) = ADJM(G, i, j) || ADJM(G, j, i);

    return Gu;
}

Graph* moralise(Graph* G)
{
    int i,j,n,v;
    Graph* Gm;

    if (G == NULL || G->variables == NULL)
        return NULL;
    
    n = G->size;
    Gm = copy_graph(G);

    /* Moralisation */
    for (v = 0; v < n; v++)       /* Iterate variables */
        for (i = 0; i < n; i++) 
            if (ADJM(G, i, v))            /* If i parent of v, find those */
                for (j = i+1; j < n; j++) /* parents of v which are > i */
                {
                    ADJM(Gm, i, j) |= ADJM(G, j, v);
                    ADJM(Gm, j, i) |= ADJM(G, j, v);
                }
                
    return Gm;
}

/* Not specified in Graph.h -- internal helper function */
int triangulate(Graph* Gm, Clique** clique_p)
{
    int i, j, j_index, k, k_index, n;
    int clique_count = 0;
    int cluster_size;
    Variable* min_cluster;
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
            j_index = get_graph_index(Gm, min_cluster[j]);
            variable_set[j_index] = 1;
    
            /* Add new edges to Gm. */
            for (k = j+1; k < cluster_size; k++)
            {
                    k_index = get_graph_index(Gm, min_cluster[k]);
    
                    ADJM(Gm, j_index, k_index) = 1;
                    ADJM(Gm, k_index, j_index) = 1;
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


int find_cliques(Graph* G, Clique** cliques_p)
{
    Graph *Gu, *Gm;
    int n_cliques = 0;

    Gm = moralise(G);
    Gu = make_undirected(Gm);

    n_cliques = triangulate(Gu, cliques_p);

    /* Test if triangulate failed */
    if(n_cliques < 0){
      free_graph(Gu);
      free_graph(Gm);
      return -1;
    }

    if(find_sepsets(*cliques_p, n_cliques) != NO_ERROR)
      report_error(__FILE__, __LINE__, ERROR_GENERAL, 1);

    /* JJT: I added some free_graph stuff here, because I suspected
     * memory leaks... */
    free_graph(Gu);
    free_graph(Gm);

    return n_cliques;
}
