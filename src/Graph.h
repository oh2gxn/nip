#ifndef __GRAPH_H__
#define __GRAPH_H__

#include "Variable.h"
#include "Clique.h"

typedef struct {
    int** adj_matrix;
    Variable* variables;
    unsigned size;
    int top;
} Graph; /* Esimerkki. Toteutus voi vaihtua */

typedef struct {
    Cluster_list* next;
    int* variable_set;
} Cluster_list;

Graph* new_graph(unsigned n);
/* Creates new graph.
 * Parameter n: number of variables in a graph.
 */

<<<<<<< Graph.h
int add_variable(Graph* G, Variable v);
=======
void free_graph(Graph* G);
/* Frees the memory used by the Graph G.
 */

int add_variable(Graph* G, Variable v);
>>>>>>> 1.8
/* Adds a new variable (ie. a node) to the graph. 
 * Parameter G: the graph
 * Parameter v: the variable to be added
 */

<<<<<<< Graph.h
int add_all_variables(Graph* G, Variable* vars);
=======
int add_all_variables(Graph* G, Variable vars[]);
>>>>>>> 1.8
/* Adds all variables to the graph G.
 * Parameter vars: a Variable-array, size n
 * Returns true, if success. False otherwise.
 * Bugger ye all, if ye use an array other than size n.
 */

int add_child(Graph* G, Variable parent, Variable child);
/* Adds a child to a parent, ie. a dependence.
 * Parameter G: the graph
 * Parameter parent: the parent-variable
 * Parameter child: the child-variable
 */

int get_size(Graph* G);
/* Returns the number of variables in the graph G.
 * Parameter G: the graph
 */

/* MVK: Mitä tämä tekee? Onko prototyyppi puutteellinen? */
int get_neighbours(Graph* G, Variable** neighbours);

Variable* get_variables(Graph* G);
/* Returns the variables used in Graph G;
 * Parameter G: the graph
 */

<<<<<<< Graph.h
<<<<<<< Graph.h
Graph* moralise(Graph* G);
=======
=======
int is_child(Graph* G, Variable parent, Variable child);
/* Returns 1 if "child" is a child of "parent" in Graph "G",
 * 0 if not.
 */

>>>>>>> 1.9
void moralise(Graph* Gm, Graph* G);
>>>>>>> 1.8
/* Moralises a DAG. (Brit. spelling)
 * Parameter G: An unmoral graph.
 * Parameter Gm becomes the moralised Graph.
 * Does not modify G.
 */

<<<<<<< Graph.h
int find_cliques(Graph* Gm, Clique** cliques_p);
=======
int find_cliques(Graph* Gm, Clique* cliques_p);
>>>>>>> 1.8
/* Triangulates G and finds the cliques.
 * Parameter Gm: moralised graph
 * Parameter cliques_p: pointer to a clique array
 * MVK UPDATE: I think there should be only one asterisk in cliques_p, changed
 * Returns the number of cliques.
 * Modifies Gm.
 */


#endif /* __GRAPH_H__ */
