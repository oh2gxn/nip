#include "Clique.h"
#include <stdlib.h>
#include <stdio.h>

#define YKS_KARDINALITEETTI_VAAN 2

int main(){
  int i;
  double result[YKS_KARDINALITEETTI_VAAN];

  /* create the variables 
   * normally this information would be found in a file and parsed */
  Variable variables[5];
  char name1[] = "einari";
  char name2[] = "jalmari";
  char name3[] = "jokke";
  char name4[] = "spede";
  char name5[] = "raimo";
  variables[0] = new_variable(name1, 3);
  variables[1] = new_variable(name2, 4);
  variables[2] = new_variable(name3, YKS_KARDINALITEETTI_VAAN);
  variables[3] = new_variable(name4, 3);
  variables[4] = new_variable(name5, 2);

  /* create some cliques and sepsets (with too many shortcuts...) 
   * This is normally the job of the graph implementation. */
  Clique clique_pile[3];
  Sepset sepset_pile[2];
  clique_pile[0] = make_Clique(variables, 3);
  clique_pile[1] = make_Clique(&(variables[1]), 3);
  clique_pile[2] = make_Clique(&(variables[3]), 2);
  sepset_pile[0] = make_Sepset(&(variables[1]), 2, &(clique_pile[0]));
  sepset_pile[1] = make_Sepset(&(variables[3]), 1, &(clique_pile[1]));
  add_Sepset(clique_pile[0], sepset_pile[0]);
  add_Sepset(clique_pile[1], sepset_pile[0]);
  add_Sepset(clique_pile[1], sepset_pile[1]);
  add_Sepset(clique_pile[2], sepset_pile[1]);

  /* initialization: This information is usually parsed from a file. */
  potential model[3];
  Variable parents0[2];
  Variable parents2[2];  /* JJ: HOW TO MAKE ARRAYS EASILY? */
  Variable parents4[1];
  model[0]=make_potential({3,4,YKS_KARDINALITEETTI_VAAN}, 3);
  model[1]=make_potential({4,YKS_KARDINALITEETTI_VAAN,3}, 3);
  model[2]=make_potential({3,2}, 2);
  /* model[i]->data  i.e. conditional distributions ??? */
  parents0 = {variables[1], variables[2]};
  parents2 = {variables[1], variables[3]};
  parents4 = {variables[3]};
  initialise(clique_pile[0], variables[0], parents0, model[0]);
  initialise(clique_pile[1], variables[2], parents2, model[1]);
  initialise(clique_pile[2], variables[4], parents4, model[2]);

  /* observation entry: This information is from the given data. */
  /* data please? anyone? */


  /* propagation: some action */
  for(i = 0; i < 3; i++)
    unmark_Clique(clique_pile[i]);
  collect_evidence(NULL, NULL, clique_pile[1]);
  distribute_evidence(clique_pile[1]);

  /* marginalization */
  marginalise(clique_pile[1], variables[2], result);

  /* normalization */
  normalise(result, YKS_KARDINALITEETTI_VAAN);
  
  for(i = 0; i < YKS_KARDINALITEETTI_VAAN; i++)
    printf("result[%d] = %f\n", i, result[i]);
  /* To be continued... */

  return 0;
}
