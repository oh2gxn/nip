#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "potential.h"
#include "Clique.h"
#include "Graph.h"
#include "Variable.h"
#include "nip.h"

#define FOO

int main(int argc, char *argv[]){
  
  int i, n = atoi(argv[2]);
  Nip model;

#ifdef FOO
  double data[] = {
    0.5, 0.3, 0.1, 0.6, 0.9, 31, 235, 3523.24, 7657,
    0.2, 3.7, 9.8, 1.4, 5.5, 17, 918, 4798.28, 1111
  };
  int cardinality[] = {3, 2, 3};
  int num_of_vars = 3;
  Variable vars[3];
  varlink first, last;
  char **symbols;
  char **names;
  char ***states;
  potential p;
  Clique cl[2];
  Sepset s;

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
  symbols[0] = (char *) calloc(2, sizeof(char));
  symbols[1] = (char *) calloc(2, sizeof(char));
  symbols[2] = (char *) calloc(2, sizeof(char));
  strncpy(symbols[0], "A", 2);
  strncpy(symbols[1], "B", 2);
  strncpy(symbols[2], "C", 2);

  names = (char **) calloc(3, sizeof(char *));
  names[0] = (char *) calloc(10, sizeof(char));
  names[1] = (char *) calloc(10, sizeof(char));
  names[2] = (char *) calloc(10, sizeof(char));
  strncpy(names[0], "Nimi1", 10);  
  strncpy(names[1], "Nimi2", 10);  
  strncpy(names[2], "Nimi3", 10);  

  states = (char ***) calloc(3, sizeof(char **));
  states[0] = (char **) calloc(3, sizeof(char *));
  states[1] = (char **) calloc(2, sizeof(char *));
  states[2] = (char **) calloc(3, sizeof(char *));
  states[0][0] = (char *) calloc(3, sizeof(char));
  states[0][1] = (char *) calloc(3, sizeof(char));
  states[0][2] = (char *) calloc(3, sizeof(char));
  states[1][0] = (char *) calloc(3, sizeof(char));
  states[1][1] = (char *) calloc(3, sizeof(char));
  states[2][0] = (char *) calloc(3, sizeof(char));
  states[2][1] = (char *) calloc(3, sizeof(char));
  states[2][2] = (char *) calloc(3, sizeof(char));
  strncpy(states[0][0], "a1", 3);
  strncpy(states[0][1], "a2", 3);
  strncpy(states[0][2], "a3", 3);
  strncpy(states[1][0], "b1", 3);
  strncpy(states[1][1], "b2", 3);
  strncpy(states[2][0], "c1", 3);
  strncpy(states[2][1], "c2", 3);
  strncpy(states[2][2], "c3", 3);

  printf("\nAllocating and freeing variables:\n");
  for(i = 0; i < n; i++){
    /* Create Variables */
    new_variable(symbols[0], names[0], states[0], cardinality[0]);
    new_variable(symbols[1], names[1], states[1], cardinality[1]);
    new_variable(symbols[2], names[2], states[2], cardinality[2]);
    printf("\rIteration %d of %d                               ", i + 1, n);
    /* Free Variables */
    first = get_first_variable();
    last = get_last_variable();
    reset_Variable_list();
    while(first != NULL){
      free_variable(first->data);
      first = first->fwd;
      if(first)
	free(first->bwd);
    }
    free(last);
  }

  vars[0] = new_variable(symbols[0], names[0], states[0], cardinality[0]);
  vars[1] = new_variable(symbols[1], names[1], states[1], cardinality[1]);
  vars[2] = new_variable(symbols[2], names[2], states[2], cardinality[2]);

  printf("\nAllocating and freeing Cliques:\n");
  for(i = 0; i < n; i++){
    /* Create two cliques and a sepset */
    cl[0] = make_Clique(vars, 2);
    cl[1] = make_Clique(vars+1, 2);
    s = make_Sepset(vars+1, 1, cl);
    add_Sepset(cl[0], s);
    add_Sepset(cl[1], s);
    printf("\rIteration %d of %d                               ", i + 1, n);
    /* Free the cliques (and the sepset, automatically) */
    free_Clique(cl[0]);
    free_Clique(cl[1]);
  }

  /* Free some memory */
  for(i = 0; i < 3; i++){
    free(states[i][0]);
    free(states[i][1]);
    free(symbols[i]);
    free(names[i]);
  }
  free(states[0][2]);
  free(states[2][2]);
  for(i = 0; i < 3; i++)
    free(states[i]);
  free(states);
  free(symbols);
  free(names);

  /* TODO: Test the free_graph function */


#endif

  printf("\nParsing and freeing models:\n");
  for(i = 0; i < n; i++){
    model = parse_model(argv[1]);
    printf("\rIteration %d of %d                               ", i + 1, n);
    free_model(model);
  }
  printf("\n");

  return 0;
}
