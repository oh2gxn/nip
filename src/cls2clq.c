/* cls2clq.c $Id: cls2clq.c,v 1.4 2010-11-09 19:06:08 jatoivol Exp $
 */

#include <stdlib.h>
#include <string.h>
#include "cls2clq.h"
#include "clique.h"

Cluster_list* new_cl_item(int array_size, Cluster_list* next, int* var_set)
{
    Cluster_list* cl_new = (Cluster_list*) malloc(sizeof(Cluster_list));
    if(!cl_new)
      return NULL;

    cl_new->variable_set = (int*) calloc(array_size, sizeof(int));
    if(!(cl_new->variable_set)){
      free(cl_new);
      return NULL;
    }

    memcpy(cl_new->variable_set, var_set, array_size*sizeof(int));
    cl_new->next = next;

    return cl_new;
}

Cluster_list* remove_cl_item(Cluster_list* head){
    Cluster_list* cl_new;
    if(!head)
        return NULL;
    
    cl_new = head->next;
    free(head->variable_set);
    free(head);

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
            if (var_set[i] && !cl_i->variable_set[i])
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

clique* cl2cliques(nip_variable* vars, Cluster_list* cl_head, 
		   int n_cliques, int n)
{
    int n_vars, i, j;
    int orig_n_cliques = n_cliques;
    Cluster_list* cl_i;
    clique* cliques;
    nip_variable* clique_vars;

    cliques = (clique*) calloc(n_cliques, sizeof(clique));
    if(!cliques)
      return NULL;

    clique_vars = (nip_variable*) calloc(n, sizeof(nip_variable));
    if(!clique_vars){
      free(cliques);
      return NULL;
    }

    for (cl_i = cl_head; cl_i != NULL; cl_i = cl_i->next)
    {
        n_vars = 0;
        for (i = 0; i < n; i++)
            if (cl_i->variable_set[i])
                clique_vars[n_vars++] = vars[i];

	   cliques[--n_cliques] = new_clique(clique_vars, n_vars);
	   /* This ^^^^^^^^^^^ is sort of dangerous. */

	   if(cliques[n_cliques] == NULL){
	     for(j = n_cliques + 1; j < orig_n_cliques; j++)
	       free_clique(cliques[j]);
	     free(clique_vars);
	     free(cliques);
	     return NULL;
	   }
    }
    
    free(clique_vars);
    return cliques;
}
