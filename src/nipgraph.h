/*  NIP - Dynamic Bayesian Network library
    Copyright (C) 2012  Janne Toivola

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, see <http://www.gnu.org/licenses/>.
*/

/* nipgraph.h 
 * Functions for representing and manipulating graphs, and methods for 
 * constructing the join tree.
 * Authors: Antti Rasinen, Janne Toivola
 * Version: $Id: nipgraph.h,v 1.11 2011-01-23 23:01:47 jatoivol Exp $
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
  int* adj_matrix;        /* Two dimensional adjacency matrix */
  unsigned long* var_ind; /* Possible array for [variable id] -> adjm index */
  unsigned long min_id;   /* Minimum ID of variables, an invariant */
  unsigned long max_id;   /* Maximum ID of variables, an invariant */
  nip_variable* variables;/* Array of all the variables (nodes) */
  unsigned size;          /* Number of nodes in the graph (allocated size) */
  int top;                /* Number of variables (nodes) added so far */
} nip_graph_struct; 
typedef nip_graph_struct* nip_graph;


/* Creates a new graph allocating memory for the whole adjacency matrix.
 * Parameter n: (maximum) number of variables in the graph.
 * Memory requirements are O(n²)
 */
nip_graph nip_new_graph(unsigned n);


/* Copies a graph. Does not create copies of the nodes (nip_variable),
 * but does copy the references to them.
 * Parameter g: The graph to be copied
 */
nip_graph nip_copy_graph(nip_graph g);


/* Frees the memory used by the graph g. */
void nip_free_graph(nip_graph g);


/* Returns the number of nodes (nip_variables) in the graph g. */
int nip_graph_size(nip_graph g);


/* Returns an array of references to the nodes (variables) used in graph g.
 * The array is still property of g, do not free or modify it.
 */
nip_variable* nip_graph_nodes(nip_graph g);


/* Returns the index of node v in the adjacency matrix.
 * Parameter g: the graph
 * Parameter v: the node (variable)
 */
int nip_graph_index(nip_graph g, nip_variable v);


/* Returns the number of neighbours of v + 1 (i.e. size of the cluster 
 * induced by v including v itself).
 * Parameter g: the graph
 * Parameter v: the variable (node) 
 * Parameter neighbours: a pointer to a variable array which
 * will be allocated and contain the variable v (as the first element) 
 * and its neighbouring variables. Free the array when done using it.
 */
int nip_graph_cluster(nip_graph g, nip_variable v, 
		      nip_variable** neighbours);


/* Returns true (non-zero), if there is a link from parent to child
 * Parameter g: the graph
 * Parameter parent: the suspected parent
 * Parameter child: the suspected child
 */
int nip_graph_linked(nip_graph g, nip_variable parent, nip_variable child);


/* Adds a new variable (ie. a node) to the graph. Do this at most
 * nip_graph_size(g) times and preferably during the initialization 
 * of the graph.
 * Parameter g: the graph
 * Parameter v: the variable (node) to be added
 * Returns an error code.
 */
nip_error_code nip_graph_add_node(nip_graph g, nip_variable v);


/* Adds a dependency between a child node and a parent node, 
 * ie. an edge between the nodes.
 * Parameter g: the graph
 * Parameter parent: the parent variable
 * Parameter child: the child variable
 */
nip_error_code nip_graph_add_child(nip_graph g, 
				   nip_variable parent, 
				   nip_variable child);


/* Returns an undirected copy of the graph g. */
nip_graph nip_make_graph_undirected(nip_graph g);


/* Moralises a DAG. (Brit. spelling)
 * Parameter g: An unmoral graph.
 * Returns a new moralised copy of the graph.
 * Does not modify g.
 */
nip_graph nip_moralise_graph(nip_graph g);


/* Adds undirected links between interface variables in a DAG.
 * Parameter g: A graph with determined interface variables.
 * Returns edited copy of the graph.
 * Does not modify g. (Author: Janne Toivola)
 */
nip_graph nip_add_interface_edges(nip_graph g);


/* Triangulates g and finds the cliques.
 * Parameter g: a graph (no need to be moralised or undirected)
 * Parameter cliques_p: pointer to a clique array
 * Returns the number of cliques and allocates new array of cliques to 
 * *cliques_p. Does not modify g.
 */
int nip_graph_to_cliques(nip_graph g, nip_clique** cliques_p);


/* Constructs sepsets and inserts them between the cliques to form a
 * join tree. Returns an error code.
 * Parameters:
 *  - cliques : an array of cliques
 *  - num_of_cliques : the number of cliques in the given array
 */
nip_error_code nip_create_sepsets(nip_clique *cliques, int num_of_cliques);


/* Internal helper function for finding cliques from a moral graph. */
int nip_triangulate_graph(nip_graph gm, nip_clique** clique_p);

#endif /* __GRAPH_H__ */
