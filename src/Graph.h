/* Graph.h $Id: Graph.h,v 1.24 2010-11-09 19:06:08 jatoivol Exp $
 */

#ifndef __GRAPH_H__
#define __GRAPH_H__

#include "nipvariable.h"
#include "clique.h"

#define ADJM(G, i, j) ( (G)->adj_matrix[(i)*(G)->size + (j)] )

typedef struct {
    int* adj_matrix; /* Two dimensional */
    unsigned long* var_ind;
    unsigned long min_id;
    unsigned long max_id;
    nip_variable* variables;
    unsigned size;
    int top;
} Graph; 

Graph* new_graph(unsigned n);
/* Creates new graph.
 * Parameter n: number of variables in a graph.
 */

Graph* copy_graph(Graph* G);
/* Copies a graph. Does not create copies of variables,
 * but does copy the array of variables.
 * Parameter G: The graph to be copied
 */

void free_graph(Graph* G);
/* Frees the memory used by the Graph G.
 */

int get_size(Graph* G);
/* Returns the number of variables in the graph G.
 * Parameter G: the graph
 */

nip_variable* get_variables(Graph* G);
/* Returns the variables used in Graph G;
 * Parameter G: the graph
 */

int get_graph_index(Graph* G, nip_variable v);
/* Returns the index of v in the variable-array 
 * Parameter G: the graph
 * Parameter v: the variable
 */

int get_neighbours(Graph* G, nip_variable* neighbours, nip_variable v);
/* Returns the number of neighbours of v.
 * Parameter G: the graph
 * Parameter neighbours: a pointer to a variable array
                         will contain the neighbouring variables after call
 * Parameter v: the variable 
 */

int is_child(Graph* G, nip_variable parent, nip_variable child);
/* Returns true, if there is a link from parent to child
 * Parameter G: the graph
 * Parameter parent: the suspected parent
 * Parameter child: the suspected child
 */

int add_variable(Graph* G, nip_variable v);
/* Adds a new variable (ie. a node) to the graph. 
 * Parameter G: the graph
 * Parameter v: the variable to be added
 */

int add_child(Graph* G, nip_variable parent, nip_variable child);
/* Adds a child to a parent, ie. a dependence.
 * Parameter G: the graph
 * Parameter parent: the parent-variable
 * Parameter child: the child-variable
 */

Graph* moralise(Graph* G);
/* Moralises a DAG. (Brit. spelling)
 * Parameter G: An unmoral graph.
 * Returns moralised Graph.
 * Does not modify G.
 */

Graph* add_interface_edges(Graph* G);
/* Adds undirected links between interface variables in a DAG.
 * Parameter G: A graph with determined interface variables.
 * Returns edited Graph.
 * Does not modify G. (Author: Janne Toivola)
 */

int find_cliques(Graph* G, clique** cliques_p);
/* Triangulates G and finds the cliques.
 * Parameter G: moralised undirected graph
 * Parameter cliques_p: pointer to a clique array
 * Returns the number of cliques.
 * Modifies G.
 */

/* Internal helper */
Graph* make_undirected(Graph* G);
int triangulate(Graph* Gm, clique** clique_p);
#endif /* __GRAPH_H__ */
