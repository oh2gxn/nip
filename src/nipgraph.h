/* nipgraph.h 
 * Functions for representing and manipulating graphs, and methods for 
 * finding the cliques.
 * Authors: Antti Rasinen, Janne Toivola
 * Version: $Id: nipgraph.h,v 1.1 2010-11-26 17:06:02 jatoivol Exp $
 */

#ifndef __NIPGRAPH_H__
#define __NIPGRAPH_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nipvariable.h"
#include "clique.h"
#include "Heap.h"
#include "cls2clq.h"
#include "niperrorhandler.h"

#define NIP_ADJM(G, i, j) ( (G)->adj_matrix[(i)*(G)->size + (j)] )

typedef struct {
    int* adj_matrix; /* Two dimensional */
    unsigned long* var_ind;
    unsigned long min_id;
    unsigned long max_id;
    nip_variable* variables;
    unsigned size;
    int top;
} nip_graph; 

nip_graph* nip_new_graph(unsigned n);
/* Creates new graph.
 * Parameter n: number of variables in a graph.
 */

nip_graph* nip_copy_graph(nip_graph* G);
/* Copies a graph. Does not create copies of variables,
 * but does copy the array of variables.
 * Parameter G: The graph to be copied
 */

void nip_free_graph(nip_graph* G);
/* Frees the memory used by the Graph G.
 */

int nip_graph_size(nip_graph* G);
/* Returns the number of variables in the graph G.
 * Parameter G: the graph
 */

nip_variable* nip_graph_variables(nip_graph* G);
/* Returns the variables used in Graph G;
 * Parameter G: the graph
 */

int nip_graph_index(nip_graph* G, nip_variable v);
/* Returns the index of v in the variable-array 
 * Parameter G: the graph
 * Parameter v: the variable
 */

int nip_get_neighbours(nip_graph* G, nip_variable* neighbours, nip_variable v);
/* Returns the number of neighbours of v.
 * Parameter G: the graph
 * Parameter neighbours: a pointer to a variable array
                         will contain the neighbouring variables after call
 * Parameter v: the variable 
 */

int nip_graph_is_child(nip_graph* G, nip_variable parent, nip_variable child);
/* Returns true, if there is a link from parent to child
 * Parameter G: the graph
 * Parameter parent: the suspected parent
 * Parameter child: the suspected child
 */

int nip_graph_add_variable(nip_graph* G, nip_variable v);
/* Adds a new variable (ie. a node) to the graph. 
 * Parameter G: the graph
 * Parameter v: the variable to be added
 */

int nip_graph_add_child(nip_graph* G, nip_variable parent, nip_variable child);
/* Adds a child to a parent, ie. a dependence.
 * Parameter G: the graph
 * Parameter parent: the parent-variable
 * Parameter child: the child-variable
 */

nip_graph* nip_moralise(nip_graph* G);
/* Moralises a DAG. (Brit. spelling)
 * Parameter G: An unmoral graph.
 * Returns moralised Graph.
 * Does not modify G.
 */

nip_graph* nip_add_interface_edges(nip_graph* G);
/* Adds undirected links between interface variables in a DAG.
 * Parameter G: A graph with determined interface variables.
 * Returns edited Graph.
 * Does not modify G. (Author: Janne Toivola)
 */

int nip_find_cliques(nip_graph* G, clique** cliques_p);
/* Triangulates G and finds the cliques.
 * Parameter G: moralised undirected graph
 * Parameter cliques_p: pointer to a clique array
 * Returns the number of cliques.
 * Modifies G.
 */

/* Internal helper */
nip_graph* nip_make_graph_undirected(nip_graph* G);
int nip_triangulate_graph(nip_graph* Gm, clique** clique_p);

#endif /* __GRAPH_H__ */
