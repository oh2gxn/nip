#ifndef __GRAPH_H__
#define __GRAPH_H__

#include "Variable.h"
#include "Clique.h"

typedef struct {
    int[][] adj_matrix;
    Variable* variables;
    unsigned size;
    int top;
} Graph; /* Esimerkki. Toteutus voi vaihtua */

Graph new_graph(unsigned n);
/* Creates new graph.
 * Parameter n: number of variables in a graph.
 */

int add_variable(Graph G, Variable v);
/* Adds a new variable (ie. a node) to the graph. 
 * Parameter G: the graph
 * Parameter v: the variable to be added
 * Returns a graph.
 */

int add_all_variables(Graph G, Variable* vars);
/* Adds all variables to the graph G.
 * Parameter vars: a Variable-array, size n
 * Returns true, if success. False otherwise.
 * Bugger ye all, if ye use an array other than size n.
 */

int add_child(Graph G, Variable parent, Variable child);
/* Adds a child to a parent, ie. a dependence.
 * Parameter G: the graph
 * Parameter parent: the parent-variable
 * Parameter child: the child-variable
 */

Graph moralise(G);
/* Moralises a DAG. (Brit. spelling)
 * Parameter G: An unmoral graph.
 * Returns the moralised graph Gm.
 */

int find_cliques(Graph Gm, Clique** cliques_p)
/* Triangulates G and finds the cliques.
 * Parameter Gm: moralised graph
 * Parameter cliques_p: pointer to a clique array
 * Returns the number of cliques.
 */


#endif /* __GRAPH_H__ */
