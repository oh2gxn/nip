#include "Graph.h"

Graph new_graph(unsigned n)
{
    newgraph = (Graph) malloc(sizeof(Graph));
    newgraph->size = n; newgraph->top = 0;
    newgraph->adj_matrix = (int*) calloc(n*n, sizeof(int));
    newgraph->variables = (Variable*) calloc(n, sizeof(Variable));
    return newgraph;
}

int add_variable(Graph G, Variable v)
{
    if (G->top == G->size)
	return -1; /* Cannot add more items. */

    G->variables[G->top] = v;
    G->top++;

    return 0; /* Error codes need work */
}

int add_all_variables(Graph G, Variable* vars)
{
    G->variables = vars; /* Mostly (dangerous) syntactic syrup. */
}

int add_child(Graph G, Variable parent, Variable child)
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
}
