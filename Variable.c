#include <string.h>
#include <stdlib.h>
#include "Variable.h"

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

Variable copy_variable(Variable v){
  int i;
  if(v == NULL)
    return NULL;
  Variable copy = (Variable) malloc(sizeof(vtype));
  copy->cardinality = v->cardinality;
  copy->id = v->id;

  strncpy(copy->name, v->name, VAR_NAME_LENGTH);
  copy->name[VAR_NAME_LENGTH] = '\0';

  copy->likelihood = (double *) calloc(copy->cardinality, sizeof(double));
  /* initialise likelihoods to 1 */
  for(i = 0; i < copy->cardinality; i++)
    copy->likelihood[i] = v->likelihood[i];

  return v;
}

void free_variable(Variable v){
  if(v == NULL)
    return;
  free(v->likelihood);
  free(v);
}

int equal_variables(Variable v1, Variable v2){
  return (v1->id == v2->id);
}

unsigned long get_id(Variable v){
  return v->id;
}

int update_likelihood(Variable v, double* likelihood){
  v->likelihood = likelihood;
  return 0;
}
