#include <stdlib.h>
#include "cls2clq.h"
#include "../Clique.h"

#define NULL 0
/* URK */

Cluster_list* new_cl_item(int array_size, Cluster_list* next, int* var_set)
{
    Cluster_list* cl_new = (Cluster_list*) malloc(sizeof(Cluster_list));
    cl_new->variable_set = (int*) calloc(array_size, sizeof(int));
    memcpy(cl_new->variable_set, var_set, array_size*sizeof(int));
    cl_new->next = next;

    return cl_new;
}

int is_subset(Cluster_list* cl_head, int* var_set, int size)
{
    Cluster_list* cl_i;
    int i, flag;

    /* Iterate the list of known clusters */
    /* Later additions cannot be supersets of earlier ones */
    /* One of the variables in an earlier var_set is removed */
    for (cl_i = cl_head; cl_i != NULL; cl_i = cl_i->next)
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

Clique* cl2cliques(Variable* vars, Cluster_list* cl_head, int n_cliques, int n)
{
    int n_vars, i;
    Cluster_list* cl_i;
    Clique* cliques = (Clique*) calloc(n_cliques, sizeof(Clique));
    Variable* clique_vars = (Variable*) calloc(n, sizeof(Variable));

    for (cl_i = cl_head; cl_i != NULL; cl_i = cl_i->next)
    {
	n_vars = 0;
	for (i = 0; i < n; i++)
	    if (cl_i->variable_set[i])
		clique_vars[n_vars++] = vars[i];

	cliques[--n_cliques] = make_Clique(clique_vars, n_vars);
	/* This ^^^^^^^^^^^ is sort of dangerous. */
    }
    
    free(clique_vars);
    return cliques;
}
