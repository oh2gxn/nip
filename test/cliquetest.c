#include "Clique.h"

int main(){
  /* create the variables */
  Variable variables[5];
  char name1[] = "einari";
  char name2[] = "jalmari";
  char name3[] = "jokke";
  char name4[] = "spede";
  char name5[] = "raimo";
  variables[0] = new_variable(name1, 3);
  variables[1] = new_variable(name2, 4);
  variables[2] = new_variable(name3, 2);
  variables[3] = new_variable(name4, 3);
  variables[4] = new_variable(name5, 2);
  /* create some cliques and sepsets (with too many shortcuts...) */
  Clique clique_pile[3];
  Sepset sepset_pile[2];
  clique_pile[0] = make_Clique(variables, 3);
  clique_pile[1] = make_Clique(&(variables[1]), 3); /* odd, isn't it? */
  clique_pile[2] = make_Clique(&(variables[3]), 2);
  sepset_pile[0] = make_Sepset(&(variables[1]), 2, &(clique_pile[0]));
  sepset_pile[1] = make_Sepset(&(variables[3]), 1, &(clique_pile[1]));
  add_Sepset(clique_pile[0], sepset_pile[0]);
  add_Sepset(clique_pile[1], sepset_pile[0]);
  add_Sepset(clique_pile[1], sepset_pile[1]);
  add_Sepset(clique_pile[2], sepset_pile[1]);

  /* To be continued... */

  return 0;
}
