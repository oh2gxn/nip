/* nipgraph.h 
 * Functions for representing and manipulating graphs, and methods for 
 * constructing the join tree.
 * Authors: Antti Rasinen, Janne Toivola
 * Version: $Id: nipgraph.h,v 1.3 2010-11-30 18:12:04 jatoivol Exp $
 */

#ifndef __NIPGRAPH_H__
#define __NIPGRAPH_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nipheap.h"
#include "niplists.h"
#include "nipjointree.h"
#include "nipvariable.h"
#include "niperrorhandler.h"

#define NIP_ADJM(g, i, j) ( (g)->adj_matrix[(i)*(g)->size + (j)] )

typedef struct {
    int* adj_matrix; /* Two dimensional */
    unsigned long* var_ind;
    unsigned long min_id;
    unsigned long max_id;
    nip_variable* variables;
    unsigned size;
    int top;
} nip_graph_struct; 
typedef nip_graph_struct* nip_graph

nip_graph nip_new_graph(unsigned n);
/* Creates new graph.
 * Parameter n: number of variables in a graph.
 */

nip_graph nip_copy_graph(nip_graph g);
/* Copies a graph. Does not create copies of variables,
 * but does copy the array of variables.
 * Parameter g: The graph to be copied
 */

void nip_free_graph(nip_graph g);
/* Frees the memory used by the graph g.
 */

int nip_graph_size(nip_graph g);
/* Returns the number of variables in the graph g.
 * Parameter g: the graph
 */

nip_variable* nip_graph_variables(nip_graph g);
/* Returns the variables used in graph g;
 * Parameter g: the graph
 */

int nip_graph_index(nip_graph g, nip_variable v);
/* Returns the index of v in the variable-array 
 * Parameter g: the graph
 * Parameter v: the variable
 */

int nip_graph_neighbours(nip_graph g, nip_variable v, 
			 nip_variable* neighbours);
/* Returns the number of neighbours of v.
 * Parameter g: the graph
 * Parameter neighbours: a pointer to a variable array
                         will contain the neighbouring variables after call
 * Parameter v: the variable 
 */

int nip_graph_is_child(nip_graph g, nip_variable parent, nip_variable child);
/* Returns true, if there is a link from parent to child
 * Parameter g: the graph
 * Parameter parent: the suspected parent
 * Parameter child: the suspected child
 */

int nip_graph_add_variable(nip_graph g, nip_variable v);
/* Adds a new variable (ie. a node) to the graph. 
 * Parameter g: the graph
 * Parameter v: the variable to be added
 */

int nip_graph_add_child(nip_graph g, nip_variable parent, nip_variable child);
/* Adds a child to a parent, ie. a dependence.
 * Parameter g: the graph
 * Parameter parent: the parent-variable
 * Parameter child: the child-variable
 */

nip_graph nip_moralise(nip_graph g);
/* Moralises a DAG. (Brit. spelling)
 * Parameter g: An unmoral graph.
 * Returns moralised graph.
 * Does not modify g.
 */

nip_graph nip_add_interface_edges(nip_graph g);
/* Adds undirected links between interface variables in a DAG.
 * Parameter g: A graph with determined interface variables.
 * Returns edited graph.
 * Does not modify g. (Author: Janne Toivola)
 */

int nip_find_cliques(nip_graph g, clique** cliques_p);
/* Triangulates g and finds the cliques.
 * Parameter g: moralised undirected graph
 * Parameter cliques_p: pointer to a clique array
 * Returns the number of cliques.
 * Modifies g.
 */

int nip_find_sepsets(clique *cliques, int num_of_cliques);
/* Constructs sepsets and inserts them between the cliques to form a
 * join tree.
 * Returns an error code.
 * Parameters:
 *  - cliques : an array of cliques
 *  - num_of_cliques : the number of cliques in the given array
 */

/* Internal helper */
nip_graph nip_make_graph_undirected(nip_graph g);
int nip_triangulate_graph(nip_graph gm, clique** clique_p);

#endif /* __GRAPH_H__ */
