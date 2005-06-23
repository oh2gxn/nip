#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "clique.h"
#include "Graph.h"
#include "variable.h"
#include "potential.h"
#include "parser.h"
#include "nip.h"

#define TESTATUT
#define THRESHOLD 0.001

int main(int argc, char *argv[]){
  
  int i, j, m, n;

#ifdef TESTATUT
  int cardinality[] = {3, 2, 3};
  int num_of_vars = 3;
  int num_of_cliques;
  double data[] = {
    0.5, 0.3, 0.1, 0.6, 0.9, 31, 235, 3523.24, 7657,
    0.2, 3.7, 9.8, 1.4, 5.5, 17, 918, 4798.28, 1111
  };
  variable vars[3];
  varlink first, last;
  char **symbols;
  char **names;
  char ***states;
  potential p;
  clique cl[2];
  clique *cl2 = NULL;
  sepset s;
  Graph *g;
#endif

  nip model = NULL;
  time_series *ts_set = NULL;

  if(argc > 3)
    n = atoi(argv[3]);
  else if(argc > 1)
    n = atoi(argv[argc - 1]);
  else{
    printf("Usage: ./memleaktest <net> <data> <integer>\n");
    printf("Where <net> is the name of a Hugin .net file,\n");
    printf("<data> is the name of a data file, and\n");
    printf("<integer> is the number of iterations for the test.\n");
    return 0;
  }

#ifdef TESTATUT
  /* This works well, no memory leaks. */
  printf("Allocating and freeing potentials:\n");
  for(i = 0; i < n; i++){
    /* Create a potential */
    p = make_potential(cardinality, num_of_vars, data);
    printf("\rIteration %d of %d                               ", i + 1, n);
    /* Free the potential */
    free_potential(p);
  }
  printf("\rDone.                                             \n");

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
    /* Create variables */
    new_variable(symbols[0], names[0], states[0], cardinality[0]);
    new_variable(symbols[1], names[1], states[1], cardinality[1]);
    new_variable(symbols[2], names[2], states[2], cardinality[2]);
    printf("\rIteration %d of %d                               ", i + 1, n);
    /* Free variables */
    first = get_first_variable();
    last = get_last_variable();
    reset_variable_list();
    while(first != NULL){
      free_variable(first->data);
      first = first->fwd;
      if(first)
	free(first->bwd);
    }
    free(last);
  }
  printf("\rDone.                                             \n");

  vars[0] = new_variable(symbols[0], names[0], states[0], cardinality[0]);
  vars[1] = new_variable(symbols[1], names[1], states[1], cardinality[1]);
  vars[2] = new_variable(symbols[2], names[2], states[2], cardinality[2]);

  printf("\nAllocating and freeing cliques:\n");
  for(i = 0; i < n; i++){
    /* Create two cliques and a sepset */
    cl[0] = make_clique(vars, 2);
    cl[1] = make_clique(vars+1, 2);
    s = make_sepset(vars+1, 1, cl);
    add_sepset(cl[0], s);
    add_sepset(cl[1], s);
    printf("\rIteration %d of %d                               ", i + 1, n);
    /* Free the cliques (and the sepset, automatically) */
    free_clique(cl[0]);
    free_clique(cl[1]);
  }
  printf("\rDone.                                             \n");

  printf("\nAllocating and freeing Graphs:\n");
  for(i = 0; i < n; i++){
    g = new_graph(3);
    add_variable(g, vars[0]);
    add_variable(g, vars[1]);
    add_variable(g, vars[2]);
    add_child(g, vars[1], vars[0]);
    add_child(g, vars[1], vars[2]);
    num_of_cliques = 0;
    num_of_cliques = find_cliques(g, &cl2);
    printf("\rIteration %d of %d                               ", i + 1, n);
    free_graph(g);
    for(j = 0; j < num_of_cliques; j++)
      free_clique(cl2[j]);
    free(cl2);
  }
  printf("\rDone.                                             \n");

  /* Free some memory */
  first = get_first_variable();
  last = get_last_variable();
  reset_variable_list();
  while(first != NULL){
    free_variable(first->data);
    first = first->fwd;
    if(first)
      free(first->bwd);
  }
  free(last);
  
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
#endif /* TESTATUT */


  if(argc > 2){
    printf("\nParsing and freeing models:\n");
    for(i = 0; i < n/200; i++){
      /* FIXME: These operations leak memory */
      model = parse_model(argv[1]);
      printf("\rIteration %d of %d                               ", i + 1, 
	     n/200);
      free_model(model);
    }
    printf("\rDone.                                             \n");
    model = parse_model(argv[1]);
  }

  if(argc > 3){
    printf("\nReading and freeing data:\n");
    for(i = 0; i < n/500; i++){
      /* ...guess this is OK... */
      m = read_timeseries(model, argv[2], &ts_set);
      printf("\rIteration %d of %d                               ", i + 1, 
	     n/500); 
      for(j = 0; j < m; j++)
	free_timeseries(ts_set[0]);
      free(ts_set);
    }
    printf("\rDone.                                             \n");
    m = read_timeseries(model, argv[2], &ts_set);

    printf("\nRunning EM-algorithm %d times:\n",n);
    for(i = 0; i < n; i++){
      total_reset(model);
      em_learn(ts_set, m, THRESHOLD);
      printf("\rIteration %d of %d                               ", i + 1, n);
    }
    printf("\rDone.                                             \n");

    printf("\nWriting models into memwaste.net:\n");
    for(i = 0; i < n; i++){
      write_model(model, "memwaste");
      printf("\rIteration %d of %d                               ", i + 1, n);
    }
    printf("\rDone.                                             \n");
    for(i = 0; i < m; i++)
      free_timeseries(ts_set[i]);
    free(ts_set);
  }
  free_model(model);

  return 0;
}
