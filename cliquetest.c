#include "Clique.h"

int main(){
  /* create the variables */
  Variable variables[5];
  char name1[] = "einari";
  char name2[] = "jalmari";
  char name3[] = "jokke";
  char name4[] = "spede";
  char name5[] = "raimo";
  variables[0] = new_variable(name1, 2);
  variables[1] = new_variable(name2, 3);
  variables[2] = new_variable(name3, 4);
  variables[3] = new_variable(name4, 5);
  variables[4] = new_variable(name5, 6);
  /* create some cliques and sepsets (with too many shortcuts...) */
  Clique clique_pile[2];
  Sepset sepset_pile[1];
  clique_pile[0] = make_Clique(variables, 3);
  clique_pile[1] = make_Clique(&(variables[2]), 3); /* odd, isn't it? */
  sepset_pile[0] = make_Sepset(&(variables[2]), 1, clique_pile);
  add_Sepset(clique_pile[0], sepset_pile[0]);
  add_Sepset(clique_pile[1], sepset_pile[0]);

  /* To be continued */

  return 0;
}
