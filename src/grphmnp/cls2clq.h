#ifndef __CLS2CLQ_H__
#define __GRAPH_H__

#include "../Clique.h"

struct clustlist {
    struct clustlist* next;
    int* variable_set;
};

typedef struct clustlist Cluster_list;


Cluster_list* new_cl_item(int array_size, Cluster_list* next, int* var_set);
/*
 */

int is_subset(Cluster_list* cl_head, int* var_set, int size);
/*
 */

Clique* cl2cliques(Variable* vars, Cluster_list* cl_head, int n_cliques,int n);
/*
 */



#endif __CLS2CLQ_H__
