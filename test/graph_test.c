#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "Graph.h"
#include "Variable.h"


Graph* test1(void)
{
    Graph *G;
    Variable v[4];
    Variable *w;
    char *symbol, *name;
    
    int i;

    printf("\tTest 1... new_graph and add_variable\n");

    G = new_graph(5);
    printf("\t\tNew graph created.\n");
    
    for (i = 0; i < 5; i++)
    {
        sprintf(symbol, "var%d", i);
        sprintf(name, "Variable %d", i);
        v[i] = new_variable(symbol, name, NULL, 2);
        assert(add_variable(G, v[i])==0);
    }

    w = get_variables(G);
	printf("\t\tget_variables OK.\n");

    assert(get_size(G) == 5);
    printf("\t\tget_size OK.\n");

    for (i = 0; i<5; i++) {
        assert(equal_variables(v[i], w[i]));
    }
    
    printf("\tTest 1 done.\n");
    return G;
}

void test2(Graph* G)
{
    Variable* v = get_variables(G);
    
    printf("\tTest 2... add_child, is_child\n");

    /* v0 -> v2 -> v4; v1 -> v3 -> v4 */
    
    assert(add_child(G, v[0], v[2]) == 0);
    assert(add_child(G, v[2], v[4]) == 0);
    assert(add_child(G, v[1], v[3]) == 0);
    assert(add_child(G, v[3], v[4]) == 0);

    assert(is_child(G, v[0], v[2]));
    assert(is_child(G, v[1], v[3]));        
    assert(is_child(G, v[2], v[4]));        
    assert(is_child(G, v[3], v[4]));        
    
    assert(!is_child(G, v[0], v[0]));
    assert(!is_child(G, v[4], v[2]));
    assert(!is_child(G, v[3], v[1]));

    printf("\tTest 2 done.\n");    
}


void test3(Graph* G)
{
    Graph* Gc;
    Variable *v, *w;
    int i,j;
    
    printf("\tTest 3... copy_graph\n");
    Gc = copy_graph(G);
    v = get_variables(G);
    w = get_variables(Gc);
    assert(get_size(G) == get_size(Gc));
    for (i = 0; i < get_size(G); i++) {
        assert(equal_variables(v[i],w[i]));
    }
    for (i = 0; i < get_size(G); i++)
        for (j = 0; j < get_size(G); j++) {
            assert(is_child(G, v[i], v[j]) == is_child(G,w[i], w[j]));
        }
    printf("\tTest 3 done.\n");
}

void test4(Graph* G)
{
    Graph *Gu;
    Variable *v;
    int i,j,n;
    
    printf("\tTest 4... make_undirected\n");
    Gu = make_undirected(G);
    
    n = get_size(G); v = get_variables(G);
    for (i = 0; i < n; i++)
    {
        for (j=0;j < n;j++)
        {
            if (is_child(G, v[i], v[j]))
            {
                assert(is_child(Gu, v[i], v[j]));
                assert(is_child(Gu, v[j], v[i]));
            }
            if (is_child(Gu, v[i], v[j]))
            {
                assert(is_child(G,v[i],v[j]) || is_child(G,v[j],v[i]));
            }
        }
    }
    printf("\tTest 4 done.\n");
}

void test5(Graph* G)
{
    Graph *Gm;
    Variable *v;
    int i,j,n;
    printf("\tTest 5... moralise\n");
    Gm = moralise(G);
    v = get_variables(G);
    n = get_size(G);
    for (i=0; i<n;i++)
        for (j=0;j<n;j++)
			if (is_child(G,v[i],v[j]))
			{
				assert(is_child(Gm, v[i], v[j]));
			}    
	assert(is_child(Gm, v[2], v[3]) && is_child(Gm, v[3],v[2]));
	printf("\tTest 5 done.\n");
}

void test6(Graph* G)
{
	Graph *Gm, *Gu;
	int i,j,n,n_cliques;
	Variable *v;
	Clique* fooga;
	
	printf("\tTest 6... triangulate\n");
	Gm = moralise(G);
	Gu = make_undirected(Gm);
	n_cliques = triangulate(Gu, &fooga);

	n = get_size(Gu);
	v = get_variables(Gu);

	printf("\tAdjacency matrix:\n");
	for (i=0;i<n;i++)
	{
		for (j=0; j<n; j++)
			printf("%d ",is_child(Gu, v[i], v[j]));
		printf("\n");
	}

	printf("\tTest6 done.\n");
}

void main(void)
{
    Graph* G;
    printf("-------------------------------\n");
    printf("Testing graphs:\n");
    
    G = test1();
    test2(G);
    test3(G);
    test4(G);
    test5(G);
    test6(G);
}

