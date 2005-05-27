/*
 * Just testing the Graph.
 */

#include <stdio.h>
#include <stdlib.h>
#include "Graph.h"
#include "variable.h"

int main(){
  
  int i, j;
  int GRAPHS = 100000;
  Graph *gr = new_graph(5);
  Graph **graphs = (Graph **) calloc(GRAPHS, sizeof(Graph *));
  char *symbol_a = "einari";
  char *symbol_b = "jalmari";
  char *symbol_c = "jokke";
  char *symbol_d = "spede";
  char *symbol_e = "tauno";
  char *name_a = "elostelija";
  char *name_b = "jaloviina";
  char *name_c = "gggggrrrrrrrrrrrrrrr";
  char *name_d = "pasanen";
  char *name_e = "tappi";
  char *states_a[] = {"a", "b", "c"};
  char *states_b[] = {"d", "e", "f", "g"};
  char *states_c[] = {"h", "i"};
  char *states_d[] = {"j", "k", "l"};
  char *states_e[] = {"m", "n"};
  variable *variables = (variable *) calloc(5, sizeof(variable));

  /* Nyt on meill‰ verkko... Koko? */
  printf("Size of graph is %d.\n", get_size(gr));

  variables[0] = new_variable(symbol_a, name_a, states_a, 3);
  variables[1] = new_variable(symbol_b, name_b, states_b, 4);
  variables[2] = new_variable(symbol_c, name_c, states_c, 2);
  variables[3] = new_variable(symbol_d, name_d, states_d, 3);
  variables[4] = new_variable(symbol_e, name_e, states_e, 2);

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

  /* Tarkistetaan, ett‰ muuttujat ovat viel‰ hengiss‰. */
  for(i = 0; i < 5; i++)
    printf("%s\n", variables[i]->name);

  /* Luodaan monta verkkoa, joilla on yhteiset muuttujat. */
  for(i = 0; i < GRAPHS; i++){
    graphs[i] = new_graph(5);
    for(j = 0; j < 5; j++)
      add_variable(graphs[i], variables[j]);
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
