#include "Clique.h"
#include <stdlib.h>
#include <stdio.h>

int main(){
  int i;
  double result[2]; /* note1 */

  /* create the variables 
   * normally this information would be found in a file and parsed */
  Variable variables[5];
  char name1[] = "einari";
  char name2[] = "jalmari";
  char name3[] = "jokke";
  char name4[] = "spede";
  char name5[] = "raimo";
  int cardinality[3];

  /* parameters of the model */
  /*  v0=0,1,2, sum=1    v1=0            v1=1    v1=2    v1=3  */
  double potential0[] = {0.1, 0.3, 0.6,  1,1,1,  1,1,1,  1,1,1,   /* v2=0 */ 
                         1,   1,   1,    1,1,1,  1,1,1,  1,1,1};  /* v2=1 */
  /*  v1=0,1,2,3,        v2=0         v2=1  */
  double potential2[] = {0.4, 1,1,1,  0.6, 1,1,1,  /* v3=0 */
		         1,   1,1,1,  1,   1,1,1,  /* v3=1 */
		         1,   1,1,1,  1,   1,1,1}; /* v3=2 */
  /*                     v3=0    v3=1    v3=2  */
  double potential4[] = {0.6,    0.2,    0.2,    /* v4=0 sum=1 */
                         0.15,   0.25,   0.6};   /* v4=1 sum=1*/

  /* data */
  double prob1[] = {0.25, 0.25, 0.25, 0.25};
  double prob3[] = {0.2, 0.3, 0.5};

  variables[0] = new_variable("A", name1, 3);
  variables[1] = new_variable("B", name2, 4);
  variables[2] = new_variable("C", name3, 2); /* note1 */
  variables[3] = new_variable("D", name4, 3);
  variables[4] = new_variable("E", name5, 2);

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
  Variable parents2[2];
  Variable parents4[1];
  cardinality[0] = 3;
  cardinality[1] = 4;
  cardinality[2] = 2; /* note1 */
  model[0]=make_potential(cardinality, 3);
  cardinality[0] = 4;
  cardinality[1] = 2; /* note1 */
  cardinality[2] = 3;
  model[1]=make_potential(cardinality, 3);
  cardinality[0] = 3;
  cardinality[1] = 2;
  model[2]=make_potential(cardinality, 2);
  /* model[i]->data  i.e. conditional distributions ??? */
  for(i = 0; i < 3; i++)
    free(model[i]->data); /* NEVER do this in your actual code!!! */
  model[0]->data = potential0; 
  model[1]->data = potential2; 
  model[2]->data = potential4;

  parents0[0] = variables[1]; parents0[1] = variables[2];
  parents2[0] = variables[1]; parents2[1] = variables[3];
  parents4[0] = variables[3];
  initialise(clique_pile[0], variables[0], parents0, model[0]);
  initialise(clique_pile[1], variables[2], parents2, model[1]);
  initialise(clique_pile[2], variables[4], parents4, model[2]);

  /* observation entry: This information is from the given data. */
  /* data please? anyone? */
  enter_evidence(clique_pile[1], variables[1], prob1);
  enter_evidence(clique_pile[1], variables[3], prob3);

  /* propagation: some action */
  for(i = 0; i < 3; i++)
    unmark_Clique(clique_pile[i]);
  collect_evidence(NULL, NULL, clique_pile[1]);
  distribute_evidence(clique_pile[1]);

  /* marginalisation */
  marginalise(clique_pile[1], variables[2], result);

  /* normalization */
  normalise(result, 2); /* note1 */
  
  for(i = 0; i < 2; i++) /* note1 */
    printf("result[%d] = %f\n", i, result[i]);
  /* To be continued... */

  return 0;
}
