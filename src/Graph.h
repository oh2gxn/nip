#ifndef __GRAPH_H__
#define __GRAPH_H__

#include "Variable.h"
#include "Clique.h"

#define ADJM(G, i, j) ((G)->adj_matrix[(i)*(G)->size + (j)])

typedef struct {
    int* adj_matrix; /* Two dimensional */
    unsigned long* var_ind;
    unsigned long min_id;
    unsigned long max_id;
    Variable* variables;
    unsigned size;
    int top;
} Graph; /* Esimerkki. Toteutus voi vaihtua */

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

Variable* get_variables(Graph* G);
/* Returns the variables used in Graph G;
 * Parameter G: the graph
 */

int get_graph_index(Graph* G, Variable v);
/* Returns the index of v in the variable-array 
 * Parameter G: the graph
 * Parameter v: the variable
 */

int get_neighbours(Graph* G, Variable* neighbours, Variable v);
/* Returns the number of neighbours of v.
 * Parameter G: the graph
 * Parameter neighbours: a pointer to a variable array
                         will contain the neighbouring variables after call
 * Parameter v: the variable 
 */

int is_child(Graph* G, Variable parent, Variable child);
/* Returns true, if there is a link from parent to child
 * Parameter G: the graph
 * Parameter parent: the suspected parent
 * Parameter child: the suspected child
 */

int add_variable(Graph* G, Variable v);
/* Adds a new variable (ie. a node) to the graph. 
 * Parameter G: the graph
 * Parameter v: the variable to be added
 */

int add_child(Graph* G, Variable parent, Variable child);
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

int find_cliques(Graph* Gu, Clique** cliques_p);
/* Triangulates G and finds the cliques.
 * Parameter Gm: moralised undirected graph
 * Parameter cliques_p: pointer to a clique array
 * MVK UPDATE: I think there should be only one asterisk in cliques_p, changed
 * Returns the number of cliques.
 * Modifies Gm.
 */

void sort_variables(Graph* G); 
/* Internal helper */
Graph* make_undirected(Graph* G);
int triangulate(Graph* Gm, Clique** clique_p);
#endif /* __GRAPH_H__ */
