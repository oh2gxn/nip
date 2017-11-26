/**
 * @file
 * @brief Functions for representing and manipulating graphs, and methods for 
 * constructing the join tree.
 *
 * @author Antti Rasinen
 * @author Janne Toivola
 * @copyright &copy; 2007,2012 Janne Toivola <br>
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version. <br>
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details. <br>
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __NIPGRAPH_H__
#define __NIPGRAPH_H__

#include "nipvariable.h"
#include "nipjointree.h"

/**
 * Access the adjacency matrix of graph \p g at element \p i, \p j (0-based) */
#define NIP_ADJM(g, i, j) ( (g)->adj_matrix[(i)*(g)->size + (j)] )

/**
 * Structure for representing graphs of random variables, Bayes nets etc. */
typedef struct {
  unsigned size;           ///< Number of nodes in the graph (allocated size)
  int top;                 ///< Number of variables (nodes) added so far
  int* adj_matrix;         ///< Two dimensional adjacency matrix
  unsigned long* var_ind;  ///< Possible array for [variable id] -> adjm index
  unsigned long min_id;    ///< Minimum ID of variables, an invariant
  unsigned long max_id;    ///< Maximum ID of variables, an invariant
  nip_variable* variables; ///< Array of all the variables (nodes) */
} nip_graph_struct; 

typedef nip_graph_struct* nip_graph; ///< reference to a graph

/**
 * Creates a new graph allocating memory for the whole adjacency matrix.
 * Memory requirements are O(n²)
 * @param n The (maximum) number of variables in the graph
 * @return reference to a new graph, remember to free it
 * @see nip_free_graph()
 */
nip_graph nip_new_graph(unsigned n);

/**
 * Copies a graph. Does not create copies of the nodes (nip_variable),
 * but does copy the references to them.
 * @param g Reference to the graph to be copied
 * @return reference to a new graph, remember to free it
 * @see nip_free_graph()
 */
nip_graph nip_copy_graph(nip_graph g);

/**
 * Frees the memory used by a graph.
 * NOTE: Does not free individual variables in the graph.
 * @param g Reference to the graph */
void nip_free_graph(nip_graph g);

/**
 * Returns the number of nodes (nip_variables) in a graph.
 * @param g Reference to the graph
 * @return number of nodes actually added to the graph (not the max size),
 * or -1 in case of errors */
int nip_graph_size(nip_graph g);

/**
 * Returns an array of references to the nodes (variables) used in a graph.
 * The array is still property of \p g, do not free or modify it.
 * @param g Reference to the graph
 * @return array of references to variables, or NULL */
nip_variable* nip_graph_nodes(nip_graph g);

/**
 * Returns the index of a node in the adjacency matrix.
 * @param g Reference to the graph
 * @param v The node (variable) of interest
 * @return a 0-based index in the adjacency matrix, or -1 if not found
 */
int nip_graph_index(nip_graph g, nip_variable v);

/**
 * Returns the number of neighbours of a node + 1 (i.e. size of the cluster 
 * induced by \p v including \p v itself).
 * @param g Reference to the graph
 * @param v The variable (node) of interest
 * @param neighbours Pointer to a variable array which will be allocated 
 * and contain the variable v (as the first element) 
 * and its neighbouring variables. Free the array when done using it.
 * @return size of the array \p neighbours, or -1 in case of errors
 */
int nip_graph_cluster(nip_graph g, nip_variable v, nip_variable** neighbours);

/**
 * Returns true (non-zero), if there is a link from parent to child in \p g
 * @param g Reference to the graph
 * @param parent The suspected parent
 * @param child The suspected child
 * @return 0 if not linked, or the suspected relation backwards (directed) */
int nip_graph_linked(nip_graph g, nip_variable parent, nip_variable child);

/**
 * Adds a new variable (ie. a node) to the graph. Do this at most
 * nip_graph_size(g) times and preferably during the initialization 
 * of the graph. Add nodes before adding edges between them.
 * @param g Reference to the graph
 * @param v The variable (node) to be added, free variables after freeing g
 * @return an error code, or 0 if successful
 */
int nip_graph_add_node(nip_graph g, nip_variable v);

/**
 * Adds a dependency between a child node and a parent node, i.e. an edge 
 * between the nodes. This assumes the nodes were added in the graph earlier.
 * @param g Reference to the graph
 * @param parent The parent variable
 * @param child The child variable
 * @return an error code, or 0 if successful
 * @see nip_graph_add_node()
 */
int nip_graph_add_child(nip_graph g, nip_variable parent, nip_variable child);

/**
 * Returns an undirected copy of a graph.
 * @param g Reference to the graph
 * @return a new graph with symmetrical adjacency matrix, or NULL
 * @see nip_free_graph() */
nip_graph nip_make_graph_undirected(nip_graph g);

/**
 * Moralises a DAG. (Brit. spelling) Does not modify \p g.
 * @param g An unmoral graph (parents not linked)
 * @return new moralised copy of the graph, or NULL
 * @see nip_free_graph()
 */
nip_graph nip_moralise_graph(nip_graph g);

/**
 * Adds undirected links between interface variables in a DAG.
 * Does not modify \p g. (Author: Janne Toivola)
 * @param g Graph with determined interface variables.
 * @return edited copy of the graph, or NULL
 */
nip_graph nip_add_interface_edges(nip_graph g);

/**
 * Triangulates a graph and finds the cliques. Does not modify \p g.
 * @param g Reference to a graph (no need to be moralised or undirected)
 * @param cliques_p Pointer to a clique array
 * @return the number of cliques, and allocates new array of cliques to 
 * \p *cliques_p, or returns -1 in case of errors */
int nip_graph_to_cliques(nip_graph g, nip_clique** cliques_p);

/**
 * Constructs sepsets and inserts them between the cliques to form a
 * join tree (which is still just the collection of cliques, but connected).
 * @param cliques Array of all cliques
 * @param num_of_cliques Size of the array \p cliques
 * @return an error code, or 0 if successful
 */
int nip_create_sepsets(nip_clique *cliques, int num_of_cliques);

/**
 * Internal helper function for finding cliques from a moral graph.
 * @param gm The moral graph (parents linked)
 * @param clique_p Pointer to a clique array, to be allocated
 * @return Size of the found clique array */
int nip_triangulate_graph(nip_graph gm, nip_clique** clique_p);

#endif /* __GRAPH_H__ */
