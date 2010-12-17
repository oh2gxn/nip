/* A program for testing the cliques and sepsets 
 * Author: Janne Toivola
 * Version: $Id: cliquetest.c,v 1.39 2010-12-17 18:15:56 jatoivol Exp $
 */

#include <stdlib.h>
#include <stdio.h>
#include "nipjointree.h"


int main(){
  int i;
  double result[3]; /* note1 */

  /* create the variables 
   * normally this information would be found in a file and parsed */
  nip_variable variables[5];
  nip_variable parentsA[2];
  nip_variable parentsC[2];
  nip_variable parentsE[1];
  char nameA[] = "einari";
  char nameB[] = "jalmari";
  char nameC[] = "jokke";
  char nameD[] = "spede";
  char nameE[] = "raimo";
  char *statesA[3];
  char *statesB[4];
  char *statesC[2];
  char *statesD[3];
  char *statesE[2];
  int cardinality[3];

  /* parameters of the model:
   * - potentialA = P(A | B,C)
   * - potentialC = P(C | B,D)
   * - potentialE = P(E | D)  */
  /*   A=0,1,2, sum=1    B=0             B=1             B=2             B=3  */
  double potentialA[] = {0.6, 0.3, 0.1,  0.5, 0.3, 0.2,  0.4, 0.4, 0.2,  0.2, 0.3, 0.5, /* C=0 */ 
                         0.8, 0.1, 0.1,  0.6, 0.3, 0.1,  0.3, 0.4, 0.3,  0.1, 0.2, 0.7}; /* C=1 */
  /*   B=0,1,2,3,        C=0                  C=1 sum=1  */
  double potentialC[] = {0.4, 0.5, 0.5, 0.6,  0.6, 0.5, 0.5, 0.4,  /* D=0 */
		         0.2, 0.4, 0.6, 0.7,  0.8, 0.6, 0.4, 0.3,  /* D=1 */
		         0.1, 0.3, 0.6, 0.9,  0.9, 0.7, 0.4, 0.1}; /* D=2 */

  /* The BCD-potential above, but in different order (CBD) 
   *   C=0,1,             B=0        B=1        B=2        B=3 */
  double potentialC2[] = {0.4, 0.6,  0.5, 0.5,  0.5, 0.5,  0.6, 0.4,  /* D=0 */
			  0.2, 0.8,  0.4, 0.6,  0.6, 0.4,  0.7, 0.3,  /* D=1 */
			  0.1, 0.9,  0.3, 0.7,  0.6, 0.4,  0.9, 0.1}; /* D=2 */

  /*                     D=0     D=1     D=2  */
  double potentialE[] = {0.6,    0.2,    0.4,    /* E=0       */
                         0.4,    0.8,    0.6};   /* E=1 sum=1 */

  /* data */
  double probB[] = {0.25, 0.25, 0.40, 0.10};
  double probD[] = {0.2, 0.3, 0.5};

  /* create some cliques and sepsets (with too many shortcuts...) 
   * This is normally the job of the graph implementation. */
  nip_clique clique_pile[3];
  nip_sepset sepset_pile[2];

  /* initialization: This information is usually parsed from a file. */
  nip_potential model[4]; /* the fourth potential is extra */

  nip_variable set_of_variables[3];

  /* State names for the variables */
  statesA[0] = "a1";
  statesA[1] = "a2";
  statesA[2] = "a3";
  statesB[0] = "b1";
  statesB[1] = "b2";
  statesB[2] = "b3";
  statesB[3] = "b4";
  statesC[0] = "c1";
  statesC[1] = "c2";
  statesD[0] = "d1";
  statesD[1] = "d2";
  statesD[2] = "d3";
  statesE[0] = "e1";
  statesE[1] = "e2";

  /* Create the variables */
  variables[0] = nip_new_variable("A", nameA, statesA, 3);
  variables[1] = nip_new_variable("B", nameB, statesB, 4);
  variables[2] = nip_new_variable("C", nameC, statesC, 2); /* note1 */
  variables[3] = nip_new_variable("D", nameD, statesD, 3);
  variables[4] = nip_new_variable("E", nameE, statesE, 2);

  /* Create cliques */
  clique_pile[0] = nip_new_clique(variables, 3);
  clique_pile[1] = nip_new_clique(&(variables[1]), 3);
  clique_pile[2] = nip_new_clique(&(variables[3]), 2);

  /* Create separator sets */
  sepset_pile[0] = nip_new_sepset(&(variables[1]), 2, 
				  clique_pile[0], clique_pile[1]);
  sepset_pile[1] = nip_new_sepset(&(variables[3]), 1, 
				  clique_pile[1], clique_pile[2]);

  /* Create the join tree */
  nip_add_sepset(clique_pile[0], sepset_pile[0]);
  nip_add_sepset(clique_pile[1], sepset_pile[0]);
  nip_add_sepset(clique_pile[1], sepset_pile[1]);
  nip_add_sepset(clique_pile[2], sepset_pile[1]);

  cardinality[0] = 3;
  cardinality[1] = 4;
  cardinality[2] = 2; /* note1 */
  model[0] = nip_new_potential(cardinality, 3, potentialA);

  cardinality[0] = 4;
  cardinality[1] = 2; /* note1 */
  cardinality[2] = 3;
  model[1] = nip_new_potential(cardinality, 3, potentialC);

  cardinality[0] = 3;
  cardinality[1] = 2;
  model[2] = nip_new_potential(cardinality, 2, potentialE);

  /* The extra potential for testing (should be equal to model[1]) */
  /* variables C, B, D, and the data which is ordered accordingly */
  set_of_variables[0] = variables[2];
  set_of_variables[1] = variables[1];
  set_of_variables[2] = variables[3];
  model[3] = nip_create_potential(set_of_variables, 3, potentialC2);

  /* A test about reordering a potential */
  for(i = 0; i < 24; i++)
    printf("Correct=%f, Reordered=%f\n", model[1]->data[i], model[3]->data[i]);

  /* Set parents of the variables */
  parentsA[0] = variables[1]; parentsA[1] = variables[2];
  parentsC[0] = variables[1]; parentsC[1] = variables[3];
  parentsE[0] = variables[3];
  nip_set_parents(variables[0], parentsA, 2);
  nip_set_parents(variables[2], parentsC, 2);
  nip_set_parents(variables[4], parentsE, 1);

  nip_init_clique(clique_pile[0], variables[0], model[0], 0);
  nip_init_clique(clique_pile[1], variables[2], model[1], 0);
  nip_init_clique(clique_pile[2], variables[4], model[2], 0);

  /* DEBUG */
  /* printf("BCD before evidence:\n"); */
  /* for(i = 0; i < 24; i++) */ /* note1 */
  /*   printf("potential[%d] = %f\n", i, clique_pile[1]->p->data[i]); */

  /* observation entry: This information is from the given data. */
  /* data please? anyone? */
  nip_enter_evidence(variables, 5, clique_pile, 3, variables[1], probB);
  nip_enter_evidence(variables, 5, clique_pile, 3, variables[3], probD);

  /* DEBUG */
  /* printf("BCD after evidence:\n"); */
  /* for(i = 0; i < 24; i++) */ /* note1 */
  /*  printf("potential[%d] = %f\n", i, clique_pile[1]->p->data[i]);*/

  /* propagation: here's some action */
  for(i = 0; i < 3; i++)
    nip_unmark_clique(clique_pile[i]); /* Unmark all cliques */

  /* collect evidence to arbitrary clique */
  nip_collect_evidence(NULL, NULL, clique_pile[1]); 


  for(i = 0; i < 3; i++)
    nip_unmark_clique(clique_pile[i]); /* Unmark all cliques */

  /* distribute evidence from the same clique */
  nip_distribute_evidence(clique_pile[1]);

  /* DEBUG */
  /* printf("BCD after propagation:\n"); */
  /* for(i = 0; i < 24; i++) */ /* note1 */
  /*  printf("potential[%d] = %f\n", i, clique_pile[1]->p->data[i]); */

  /* marginalisation */
  printf("Marginalisation returned %d. (0 is OK)\n", 
	 nip_marginalise_clique(clique_pile[0], variables[0], result));

  /* DEBUG */
  /* printf("Not normalised:\n"); */
  /* for(i = 0; i < 3; i++) */ /* note1 */
  /*  printf("result[%d] = %f\n", i, result[i]); */

  /* normalization */
  nip_normalise_array(result, 3); /* note1 */

  /* DEBUG */
  /* printf("ABC after propagation:\n"); */
  /* for(i = 0; i < 24; i++) */ /* note1 */
  /*  printf("potential[%d] = %f\n", i, clique_pile[0]->p->data[i]); */
  
  printf("Normalised probability of A:\n");
  for(i = 0; i < 3; i++) /* note1 */
    printf("result[%d] = %g\n", i, result[i]);
  /* To be continued... */

  /* free the join tree */
  for(i = 0; i < 3; i++)
    nip_free_clique(clique_pile[i]);

  for(i = 0; i < 4; i++)
    nip_free_potential(model[i]);

  for(i = 0; i < 5; i++)
    nip_free_variable(variables[i]);

  return 0;
}
