#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include "potential.h"
#include "Clique.h"
#include "Graph.h"
/* include some more */
#include "nip.h"

int main(int argc, char *argv[]){
  
  int i, n;
  Nip model;
  double data[] = {
    0.5, 0.3, 0.1, 0.6, 0.9, 31, 235, 3523.24, 7657,
    0.2, 3.7, 9.8, 1.4, 5.5, 17, 918, 4798.28, 1111
  };
  int cardinality[] = {3, 2, 3};
  int num_of_vars = 3;
  Variable vars[3];
  char **symbols;
  char **names;
  char ***states;
  potential p;
  Clique cl1, cl2;
  Sepset s;

  n = atoi(argv[2]);

  /* This works OK, no memory leaks. */
  printf("Allocating and freeing potentials:\n");
  for(i = 0; i < n; i++){
    /* Create a potential */
    p = make_potential(cardinality, num_of_vars, data);
    printf("\rIteration %d of %d                               ", i + 1, n);
    /* Free the potential */
    free_potential(p);
  }

  symbols = (char **) calloc(3, sizeof(char *));
  symbols[0] = (char *) calloc(1, sizeof(char));
  symbols[1] = (char *) calloc(1, sizeof(char));
  symbols[2] = (char *) calloc(1, sizeof(char));
  names = (char **) calloc(3, sizeof(char *));
  names[0] = (char *) calloc(10, sizeof(char));
  names[1] = (char *) calloc(10, sizeof(char));
  names[2] = (char *) calloc(10, sizeof(char));

  printf("Allocating and freeing variables:\n");
  for(i = 0; i < n; i++){
    /* Create Variables */
    vars[0] = new_variable();
    vars[1] = new_variable();
    vars[2] = new_variable();
    printf("\rIteration %d of %d                               ", i + 1, n);
    /* Free Variables */
    free_variable(vars[0]);
    free_variable(vars[1]);
    free_variable(vars[2]);
  }

  printf("\nAllocating and freeing Cliques:\n");
  for(i = 0; i < n; i++){
    /* Create two cliques and a sepset */
    cl1 = make_Clique();
    cl2 = make_Clique();
    printf("\rIteration %d of %d                               ", i + 1, n);
    /* Free the cliques and the sepset */
    free_Clique(cl1);
    free_Clique(cl2);
    free_Sepset(s);
  }

  /* Free some memory */
  for(i = 0; i < 3; i++){
    free(symbols[i]);
    free(names[i]);
  }
  free(symbols);
  free(names);

  /* TODO: Test the free_graph function */


  printf("\nParsing and freeing models:\n");
  for(i = 0; i < n; i++){
    model = parse_model(argv[1]);
    printf("\rIteration %d of %d                               ", i + 1, n);
    free_model(model);
  }
  printf("\n");

  return 0;
}
