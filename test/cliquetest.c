#include "Clique.h"

int main(){
  /* create the variables */
  Variable variables[5];
  char names[5][10];
  int cardinality[] = { 2, 3, 4, 5, 6};
  int i;
  names[0] = "einari";
  names[1] = "jalmari";
  names[2] = "spede";
  names[3] = "jokke";
  names[4] = "raimo";
  for(i = 0; i < 5; i++)
    variables[i] = new_variable(names[i], cardinality[i]);

  /* create some cliques and sepsets (with too many shortcuts...) */
  Clique clique_pile[2];
  Sepset sepset_pile[1];
  clique_pile[0] = make_Clique(&variables, 3);
  clique_pile[1] = make_Clique(&&(variables[2]), 3); /* odd, isn't it? */
  sepset_pile[0] = make_Sepset(&&(variables[2]), 1, &clique_pile);
  add_sepset(&(clique_pile[0]), &(sepset_pile[0]));
  add_sepset(&(clique_pile[1]), &(sepset_pile[0]));

  /* To be continued */

  return 0;
}
