#include <string.h>
#include <stdlib.h>
#include "Graph.h"
#include "Variable.h"
#include "Clique.h"

/* XXXX SEURAAVAN REFAKTOROINTIPUUSKAN ISKIESSÄ:
   XXXX Muunnosta Variable -> adj_matrix-indeksi tarvitaan usein
   XXXX Closed taulukko -hash tätä varten.
*/

/* DUMMY-funktio, ehkä nimi pitäisi vaihtaa */
int get_index(Graph* G, Variable v){
  return 0;
}

Graph* new_graph(unsigned n)
{
    int i;
    Graph* newgraph = (Graph*) malloc(sizeof(Graph));
    newgraph->size = n; newgraph->top = 0;
    newgraph->adj_matrix = (int**) calloc(n, sizeof(int*));
    for(i=0; i<n; i++){
      newgraph->adj_matrix[i] = (int*) calloc(n, sizeof(int));
      memset(newgraph->adj_matrix[i], 0, n*sizeof(int));
    }
    newgraph->variables = (Variable*) calloc(n, sizeof(Variable));
    return newgraph;
}

void free_graph(Graph* G){
  int size, i;
  if(G == NULL)
    return;
  size = G->size;
  for(i = 0; i < size; i++)
    free(&(G->adj_matrix[i]));
  free(&(G->adj_matrix));
  free(&(G->variables));
  free(&G);
}

int add_variable(Graph* G, Variable v)
{
    if (G->top == G->size)
	return -1; /* Cannot add more items. */

    G->variables[G->top] = v;
    G->top++;

    return 0; /* Error codes need work */
}

int add_all_variables(Graph* G, Variable vars[])
{
    free(G->variables);
    G->variables = vars; /* Mostly (dangerous) syntactic syrup. */
    /* XX Ei paluuarvoa... */

    return 0;
}

int add_child(Graph* G, Variable parent, Variable child)
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

    return 0;
}

int get_size(Graph* G)
{
    return G->size;
}

Variable* get_variables(Graph* G)
{
    return G->variables;
}

int is_child(Graph* G, Variable parent, Variable child)
{
    int i,j;
    /* MVK korjaus: ei G->get_index(...) vaan get_index(G, ...)
     get_index taitaa olla vähän liian geneerinen nimi */
    i = get_index(G, parent); /* XX kts. refaktorointi ylhäältä */
    j = get_index(G, child);
    return G->adj_matrix[i][j];
}

void add_undirected_edge(Variable v1, Variable v2)
{
}

Graph* make_undirected(Graph* G)
{
    Graph* Gu;   
    int i,j,n;
    int** Gam; /* Gam ie. G adjacency matrix */
    
    if (G == NULL || G->variables == NULL)
        return NULL;

    n = G->size;
    Gam = G->adj_matrix;
    Gu = new_graph(n);
    add_all_variables(Gu, G->variables); /* XX Ongelmia tuhottaessa? */
    
    /* Create the undirected graph */
    for (i = 0; i < n; i++)
        for (j = 0; j < n; j++)
            Gu->adj_matrix[i][j] = Gam[i][j] || Gam[j][i];

    return Gu;
}

void moralise(Graph* Gm, Graph* G)
{
    int i,j,n,v;
        
    if (G == NULL || G->variables == NULL)
        return;
    
    n = G->size;
        
    /* Moralisation */
    for (v = 0; v < n; v++)       /* Iterate variables */
        for (i = 0; i < n; i++) 
            if (G->adj_matrix[i][v])      /* If i parent of v, find those */
                for (j = i+1; j < n; j++) /* parents of v which are > i */
                {
                    Gm->adj_matrix[i][j] |= G->adj_matrix[j][v];
                    Gm->adj_matrix[j][i] |= G->adj_matrix[j][v];
		}
}

/* Not specified Graph.h -- internal helper function */
void triangulate(Graph* Gm)
{
    int i,n;
    Graph* Gm_copy;
    
    /* Step 1: Copy Gm */
    n = Gm->size;
    Gm_copy = new_graph(n);
    for (i = 0; i < n; i++)
      add_variable(Gm_copy, Gm->variables[i]);
        
    memcpy(Gm_copy->adj_matrix[0], Gm->adj_matrix[0], n*n*sizeof(int));
    
    /* Step 2: Triangulate Gm */
}

int find_cliques(Graph* Gm, Clique* cliques_p)
{
    /* XX NULL-tarkastukset uupuu */

    triangulate(Gm);


    return 0;
}
