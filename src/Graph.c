#include <string.h>
#include <stdlib.h>
#include "Graph.h"
#include "Variable.h"
#include "Clique.h"
#include "Heap.h"

/* XXXX SEURAAVAN REFAKTOROINTIPUUSKAN ISKIESSƒ:
   XXXX Muunnosta Variable -> adj_matrix-indeksi tarvitaan usein
   XXXX Closed taulukko -hash t‰t‰ varten.
*/

/* DUMMY-funktio, ehk‰ nimi pit‰isi vaihtaa */
int get_graph_index(Graph* G, Variable v){
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
    free(G->adj_matrix[i]);
  free(G->adj_matrix);

  /* Dangerous if we have add_all_variables(...) as it is now. */
  free(G->variables);

  free(G);
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

    /* MVK NOTE: Too dangerous ( see free_graph(...) ) */
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
    /* Eik‰ paluuarvoa */

    return 0;
}

int get_size(Graph* G)
{
    return G->size;
}

int get_neighbours(Graph* G, Variable** neighbours)
{
  int i, j = 0;

    /* XX ihan vaiheessa. Mietip‰ joskus miten k‰y, jos refleksiivinen */
  /* MVK: Mik‰ on n? Ei m‰‰ritelty. */
    for (i = 0; i < n; i++)
        if (G->adj_matrix)
            *neighbours[j++] = 1;

    return j;/* # of neighbours */
}

Variable* get_variables(Graph* G)
{
    return G->variables;
}

int is_child(Graph* G, Variable parent, Variable child)
{
    int i,j;
    i = get_graph_index(G, parent); /* XX kts. refaktorointi ylh‰‰lt‰ */
    j = get_graph_index(G, child);
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
/* MVK: Pit‰‰kˆ t‰m‰n olla void? Nyt siin‰ on ei-void palautusarvo. */
void triangulate(Graph* Gm)
{
    /*    Graph* Gm_copy; */

    int i, j, j_index, k, k_index, n;
    int clique_count = 0;
    Variable* min_cluster;
    Heap* H;
    Cluster_list *cl_head, *cl_end;
    int* variable_set;

    n = Gm->size;
 
    /* Step 1: Copy Gm */
/*Copy of Gm probably unnecessary!
    Gm_copy = new_graph(n);
    for (i = 0; i < n; i++)
	add_variable(Gm_copy, Gm->variables[i]);
        
    memcpy(Gm_copy->adj_matrix[0], Gm->adj_matrix[0], n*n*sizeof(int));
*/    
    /* Step 2: Triangulate Gm */
 
    H = build_heap(Gm);

    variable_set = (int*) calloc(n, sizeof(int));

    cl_head = new_cl_item(n);
    cl_end = cl_head;

    for (i = 0; i < n; i++)
    {
	min_cluster = extract_min(H, Gm);

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

		Gm->adj_matrix[j_index][k_index] = 1;
		Gm->adj_matrix[k_index][j_index] = 1;
	    }
	}
	
	/* MVK: is_subset ei m‰‰ritelty */
	if (!is_subset(cl_head, variable_set, n))
	{
	    cl_end->next = new_cl_item(n);
	    cl_end = cl_end->next;
	    clique_count++;
	}
    }
    
    return cl2cliques(cl_head, clique_count, n);
}

/* MVK: T‰st‰kin funktiosta tarvitaan esittely. */
Clique* cl2cliques(Cluster_list* cl_head, int n_cliques, int n)
{
    int n_vars, i;
    Cluster_list* cl_i;
    Clique* cliques = calloc(n_cliques, sizeof(Clique));
    Variable* clique_vars = calloc(n, sizeof(Variable));

    /* MVK: Mik‰ on vars? Ei m‰‰ritelty. */
    for (cl_i = cl_head; cl_i->next != NULL; cl_i = cl_i->next)
    {
	n_vars = 0;
	for (i = 0; i < n; i++)
	    if (cl_i->variable_set[i])
		clique_vars[n_vars++] = vars[i];

	cliques[--n_cliques] = make_Clique(clique_vars, n_vars);
	/* This ^^^^^^^^^^^ is sort of dangerous. */
    }
    
    return cliques;
}

/* MVK: T‰st‰ t‰ytyy saada esittely joko Graph.h:hon, tai t‰m‰n tiedoston
 * alkuun, koska t‰t‰ k‰ytet‰‰n jo aiemmin. */
Cluster_list* new_cl_item(int array_size)
{
    Cluster_list* cl_new = (Cluster_list*) malloc(sizeof(Cluster_list));
    cl_new->variable_set = (int*) calloc(array_size, sizeof(int));
    cl_new->next = NULL;

    /* MVK: Palautetaan jotain. Onko oikein? */
    return cl_new;
}

int is_subset(Cluster_list* cl_head, int* var_set, int size)
{
    Cluster_list* cl_i;
    int i, flag;

    /* Iterate the list of known clusters */
    /* Later additions cannot be supersets of earlier ones */
    /* One of the variables in an earlier var_set is removed */
    for (cl_i = cl_head; cl_i->next != NULL; cl_i = cl_i->next)
    {
	flag = 0;
	for (i = 0; i < size; i++)
	{
	    if (var_set[i] & !cl_i->variable_set[i])
	    {
		flag = 1; /* var_set not a subset of cl_i */
		break;
	    }
	}
	if (!flag) /* We have a subset */
	    return 1;
    }

    return 0;
}

int find_cliques(Graph* Gm, Clique* cliques_p)
{
    /* XX NULL-tarkastukset uupuu */

    triangulate(Gm);


    return 0;
}
