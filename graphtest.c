/*
 * Just testing the Graph.
 */

#include <stdio.h>
#include <stdlib.h>
#include "Graph.h"
#include "Variable.h"

int main(){
  
  int i, GRAPHS = 100000;
  Graph *gr = new_graph(5);
  Graph **graphs = (Graph **) calloc(GRAPHS, sizeof(Graph *));
  char *name_a = "einari";
  char *name_b = "jalmari";
  char *name_c = "jokke";
  char *name_d = "spede";
  char *name_e = "tauno";
  Variable *variables = (Variable *) calloc(5, sizeof(Variable));

  /* Nyt on meill‰ verkko... Koko? */
  printf("Size of graph is %d.\n", get_size(gr));

  variables[0] = new_variable(name_a, 3);
  variables[1] = new_variable(name_b, 4);
  variables[2] = new_variable(name_c, 2);
  variables[3] = new_variable(name_d, 3);
  variables[4] = new_variable(name_e, 2);

  /* Lis‰t‰‰n muuttujat verkkoon. */
  for(i = 0; i < 7; i++)
    if(add_variable(gr, variables[i]) != 0)
      printf("No can do... (variable #%d).\n", i + 1);
    else
      printf("Variable #%d.\n", i + 1);

  /* Lis‰t‰‰n muuttujien v‰liset riippuvuudet. */
  if(add_child(gr, variables[1], variables[0]) != 0 ||
     add_child(gr, variables[1], variables[2]) != 0 ||
     add_child(gr, variables[2], variables[0]) != 0 ||
     add_child(gr, variables[3], variables[2]) != 0 ||
     add_child(gr, variables[3], variables[4]) != 0)
    printf("Pointterivertailu kusee (kai)!\n");

  free_graph(gr);

  /* Luodaan monta verkkoa, joilla on yhteiset muuttujat. */
  for(i = 0; i < GRAPHS; i++){
    graphs[i] = new_graph(5);
    add_all_variables(graphs[i], variables);
    /* Lis‰t‰‰n muuttujien v‰liset riippuvuudet. */
    if(add_child(graphs[i], variables[1], variables[0]) != 0 ||
       add_child(graphs[i], variables[1], variables[2]) != 0 ||
       add_child(graphs[i], variables[2], variables[0]) != 0 ||
       add_child(graphs[i], variables[3], variables[2]) != 0 ||
       add_child(graphs[i], variables[3], variables[4]) != 0)
      printf("Pointterivertailu kusee (kai)!\n");
  }

  /* Vapautetaan muistia. */
  for(i = 0; i < GRAPHS; i++)
    free_graph(graphs[i]);

  /* Tarkistetaan, ett‰ muuttujat ovat viel‰ hengiss‰. */
  for(i = 0; i < 5; i++)
    printf("%s\n", variables[i]->name);

  return 0;

}
