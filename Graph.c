#include <string.h>
#include "Graph.h"

Graph new_graph(unsigned n)
{
    newgraph = (Graph) malloc(sizeof(Graph));
    newgraph->size = n; newgraph->top = 0;
    newgraph->adj_matrix = (int*) calloc(n*n, sizeof(int));
    newgraph->variables = (Variable*) calloc(n, sizeof(Variable));
    return newgraph;
}

int add_variable(Graph G, Variable v)
{
    if (G->top == G->size)
	return -1; /* Cannot add more items. */

    G->variables[G->top] = v;
    G->top++;

    return 0; /* Error codes need work */
}

int add_all_variables(Graph G, Variable* vars)
{
    free(G->variables);
    G->variables = vars; /* Mostly (dangerous) syntactic syrup. */
    /* XX Ei paluuarvoa... */
}

int add_child(Graph G, Variable parent, Variable child)
{
    int parent_i = -1, child_i = -1, i;

    for (i = 0; i < G->size; i++)
    {
	if (G->variables[i] == parent)
	    parent_i = i;
	if (G->variables[i] == child)
	    child_i = i;
    }

    if (parent_i == -1 || child_i == -1)
	return -1;

    G->adj_matrix[parent_i][child_i] = 1;
    /* Eikä paluuarvoa */
}

int get_size(Graph G)
{
    return G->size;
}

Variable[] get_variables(Graph G)
{
    return G->variables;
}

Graph moralise(Graph G)
{
    Graph Gm;
    int i,j,n,v;
    Variable[] vars;
    int* parents;
    int n_parents;
        
    if (G == NULL || G->vars == NULL)
        return NULL;
    
    n = G->size;
    vars = G->variables; /* XX aiheuttaako ongelmia tuhottaessa */
    
    Gm = new_graph(n);
    add_all_variables(Gm, vars);
    
    /* Create the undirected graph */
    for (i = 0; i < n; i++) {
        for (j = i+1; j < n; j++) {
            if (Gm->adj_matrix[i][j] || Gm->adj_matrix[j][i])
            {
                Gm->adj_matrix[i][j] = 1;
                Gm->adj_matrix[j][i] = 1;
            }
        }
    }

    /* Moralisation */
    parents = (int*) calloc(n, sizeof(int));
    for (v = 0; v < n; v++)       /* Iterate variables */
    { 
        /* Step 1: Find parents (note use of G) */

        n_parents = 0;
        for (i = 0; i < n; i++) { /* Iterate possible parents */
            if G->adj_matrix[i][v]
            {
                parents[n_parents] = i;
                n_parents++;
            }
        }
        
        /* Step 2: Marry parents */
        for (i = 0; i < n_parents; i++) {
            for (j = i+1; j < n_parents; j++) {
                Gm->adj_matrix[i][j] = 1;
                Gm->adj_matrix[i][j] = 1;
            }
        }
    }
    
    return Gm;
}

/* Not specified Graph.h -- internal helper function */
void triangulate(Graph Gm)
{
    int i,j,n;
    Graph Gm_copy;
    
    /* Step 1: Copy Gm */
    n = Gm->size;
    Gm_copy = new_graph(n);
    for (i = 0; i < n; i++)
        add_variable(Gm_copy, Gm->variables[i]);
        
    memcpy(Gm_copy->adj_matrix[0], Gm->adj_matrix[0], n*n*sizeof(int));
    
    /* Step 2: Triangulate Gm */
}

int find_cliques(Graph Gm, Clique** cliques_p)
{
    /* XX NULL-tarkastukset uupuu */

    triangulate(Gm);


}
