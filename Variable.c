#include <string.h>
#include <stdlib.h>
#include "Variable.h"

/* Method for creating new variables */
Variable new_variable(char* name, int cardinality){
  int i;
  static long id = 0;
  Variable v = (Variable) malloc(sizeof(vtype));
  v->cardinality = cardinality;
  v->id = ++id;
 
  strncpy(v->name, name, VAR_NAME_LENGTH);
  v->name[VAR_NAME_LENGTH] = '\0';

  v->likelihood = (double *) calloc(cardinality, sizeof(double));
  /* initialise likelihoods to 1 */
  for(i = 0; i < cardinality; i++)
    v->likelihood[i] = 1;

  return v;
}

/* Method for testing variable equality. 
   This may be needed to keep potentials in order. INEQUALITIES ??? */
int equal_variables(Variable v1, Variable v2){
  return (v1->id == v2->id);
}

/* An alternative interface for keeping variables 
   and thus the potentials in order. */
unsigned long get_id(Variable v){
  return v->id;
}

/* Gives v a new likelihood (array of doubles). The size of the array
   must match v->cardinality */
int update_likelihood(Variable v, double* likelihood){
  v->likelihood = likelihood;
  return 0;
}
