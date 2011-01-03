/* Program for testing the graph implementation.
 * Authors: Antti Rasinen, Janne Toivola
 * Version: $Id: graphtest.c,v 1.7 2011-01-03 18:04:55 jatoivol Exp $
 */

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "nipgraph.h"
#include "nipjointree.h"
#include "nipvariable.h"

/* TODO: you could print an input file for graphviz (the dot program) */
void print_adjm(nip_graph g) {
	nip_variable* v;
	int i,j,n;

	v = nip_graph_variables(g);
	n = nip_graph_size(g);

	/*printf("\t\tAdjacency matrix:\n");*/
	for (i = 0; i < n; i++) {
	  printf("\t\t");
	  for (j=0; j < n; j++)
	    printf("%d ", nip_graph_linked(g, v[i], v[j]));
	  printf("\n");
	}
}

nip_graph test1() {
  nip_graph g;
  nip_variable v[4];
  nip_variable *w;
  /* char *symbol, *name; */
  char symbol[NIP_VAR_TEXT_LENGTH]; 
  char name[NIP_VAR_TEXT_LENGTH];
  char* states[] = {"0", "1"};
  /* MVK: Must reserve some memory (?)*/
  /* AR: True. IRIX has a black magic sprintf.*/
  
  int i;
  
  printf("\tTest 1... new_graph and add_variable\n");
  
  g = nip_new_graph(5);
  printf("\t\tNew graph created.\n");
  
  for (i = 0; i < 5; i++) {
    sprintf(symbol, "var%d", i);
    sprintf(name, "variable %d", i);
    v[i] = nip_new_variable(symbol, name, states, 2);
    assert(nip_graph_add_variable(g, v[i])==0);
  }

  w = nip_graph_variables(g);
  printf("\t\tget_variables OK.\n");
  
  assert(nip_graph_size(g) == 5);
  printf("\t\tget_size OK.\n");

  for (i = 0; i<5; i++) {
    assert(nip_equal_variables(v[i], w[i]));
  }
  
  printf("\tTest 1 done.\n");
  return g;
}

void test2(nip_graph g) {
    nip_variable* v = nip_graph_variables(g);
    
    printf("\tTest 2... add_child(), linked()\n");

    /* v0 -> v2 -> v4; v1 -> v3 -> v4 */
    
    assert(nip_graph_add_child(g, v[0], v[2]) == 0);
    assert(nip_graph_add_child(g, v[2], v[4]) == 0);
    assert(nip_graph_add_child(g, v[1], v[3]) == 0);
    assert(nip_graph_add_child(g, v[3], v[4]) == 0);

    assert(nip_graph_linked(g, v[0], v[2]));
    assert(nip_graph_linked(g, v[1], v[3]));        
    assert(nip_graph_linked(g, v[2], v[4]));        
    assert(nip_graph_linked(g, v[3], v[4]));        
    
    assert(!nip_graph_linked(g, v[0], v[0]));
    assert(!nip_graph_linked(g, v[4], v[2]));
    assert(!nip_graph_linked(g, v[3], v[1]));

    printf("\tTest 2 done.\n");    
}


void test3(nip_graph g) {
    nip_graph gc;
    nip_variable *v, *w;
    int i,j,n;
    
    printf("\tTest 3... copy_graph\n");
    gc = nip_copy_graph(g);
    v = nip_graph_variables(g);
    w = nip_graph_variables(gc);
    n = nip_graph_size(g);
    assert(n == nip_graph_size(gc));
    for (i = 0; i < n; i++) {
        assert(nip_equal_variables(v[i],w[i]));
    }
    for (i = 0; i < n; i++)
        for (j = 0; j < n; j++) {
	  assert(nip_graph_linked(g, v[i], v[j]) == 
		 nip_graph_linked(g, w[i], w[j]));
        }
    nip_free_graph(gc);
    printf("\tTest 3 done.\n");
}

void test4(nip_graph g) {
    nip_graph gu;
    nip_variable* v;
    int i,j,n;
    
    printf("\tTest 4... make_undirected\n");
    gu = nip_make_graph_undirected(g);
    
    n = nip_graph_size(g);
    v = nip_graph_variables(g);
    for (i = 0; i < n; i++) {
        for (j=0;j < n;j++) {
            if (nip_graph_linked(g, v[i], v[j])) {
                assert(nip_graph_linked(gu, v[i], v[j]));
                assert(nip_graph_linked(gu, v[j], v[i]));
            }
            if (nip_graph_linked(gu, v[i], v[j])) {
                assert(nip_graph_linked(g,v[i],v[j]) || 
		       nip_graph_linked(g,v[j],v[i]));
            }
        }
    }
    nip_free_graph(gu);
    printf("\tTest 4 done.\n");
}

void test5(nip_graph g) {
    nip_graph gm;
    nip_variable* v;
    int i,j,n;
    printf("\tTest 5... moralise\n");
    gm = nip_moralise_graph(g);
    v = nip_graph_variables(g);
    n = nip_graph_size(g);
    for (i=0; i<n; i++)
        for (j=0; j<n; j++)
	    if (nip_graph_linked(g,v[i],v[j])) {
	        assert(nip_graph_linked(gm, v[i], v[j]));
	    }    
    assert(nip_graph_linked(gm, v[2], v[3]) && 
	   nip_graph_linked(gm, v[3], v[2]));

    nip_free_graph(gm);
    printf("\tTest 5 done.\n");
}

nip_graph test6() {
	nip_graph g, gu, gm;
	int i,n = 8;
	char* states[] = {"0", "1"};
	nip_clique* cliques;
	nip_variable v[8];
	
	printf("\tTest 6... triangulation\n");
	g = nip_new_graph(n);
	
	v[0] =  nip_new_variable("A", "", states, 2);
	v[1] =  nip_new_variable("B", "", states, 2);
	v[2] =  nip_new_variable("C", "", states, 2);
	v[3] =  nip_new_variable("D", "", states, 2);
	v[4] =  nip_new_variable("E", "", states, 2);
	v[5] =  nip_new_variable("F", "", states, 2);
	v[6] =  nip_new_variable("G", "", states, 2);
	v[7] =  nip_new_variable("H", "", states, 2);
	/* these variables get freed before g is freed, I hope */

	for (i=0; i<n; i++)
	    nip_graph_add_variable(g, v[i]);
	
	nip_graph_add_child(g, v[0],v[1]);
	nip_graph_add_child(g, v[0],v[2]);
	nip_graph_add_child(g, v[1],v[3]);	
	nip_graph_add_child(g, v[2],v[4]);
	nip_graph_add_child(g, v[2],v[6]);	
	nip_graph_add_child(g, v[3],v[5]);
	nip_graph_add_child(g, v[4],v[5]);
	nip_graph_add_child(g, v[4],v[7]);
	nip_graph_add_child(g, v[6],v[7]);	

	printf("\t\tOriginal graph:\n");
	print_adjm(g);
	gm = nip_moralise_graph(g);
	gu = nip_make_graph_undirected(gm);
	nip_free_graph(gm);
	printf("\t\tMoral and undirected graph:\n");
	print_adjm(gu);

	/* construct cliques */
	n = nip_triangulate_graph(gu, &cliques);
	printf("\t\tTriangulated graph:\n");
	print_adjm(gu);
	/* free cliques */
	for(i = 0; i<n; i++)
	  nip_free_clique(cliques[i]);
	free(cliques);
	
	printf("\tTest 6 done.\n");
	nip_free_graph(gu);
	return g;
}

void test7(nip_graph g) {
	nip_clique* cliques;
	nip_clique ci;
	int i, j, n_cliques, n_vars;
	
	printf("\tTest 7... graph_to_cliques\n");

	/* construct cliques */
	n_cliques = nip_graph_to_cliques(g, &cliques);
	for (i = 0; i < n_cliques; i++) {
		printf("\t\tclique %i: ", i);
		ci = cliques[i];
		n_vars = nip_clique_size(ci);
		for (j = 0; j < n_vars; j++)
			printf("%s ", ci->variables[j]->symbol);
		printf("\n");	
	}
	
	/* free the cliques */
	for (i = 0; i < n_cliques; i++)
	  nip_free_clique(cliques[i]);
	free(cliques);

	printf("\tTest 7 done.\n");	
}

int main(void) {
  int i,n;
  nip_variable* v;
  nip_graph g, g2;
  printf("-------------------------------\n");
  printf("Testing graphs:\n");
  
  g = test1();
  test2(g);
  test3(g);
  test4(g);
  test5(g);
  /* test1 created variables, but the only pointers to them are in g */
  n = nip_graph_size(g);
  v = nip_graph_variables(g);
  for (i=0; i<n; i++)
    nip_free_variable(v[i]);
  nip_free_graph(g);

  g2=test6();
  test7(g2);
  n = nip_graph_size(g2);
  v = nip_graph_variables(g2);
  for (i=0; i<n; i++)
    nip_free_variable(v[i]);
  nip_free_graph(g2);
  
  return 0;
}

