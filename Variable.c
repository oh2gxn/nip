#include <string.h>
#include <stdlib.h>
#include "Variable.h"
#include "potential.h"
#include "errorhandler.h"

Variable new_variable(char* name, int cardinality){
  static long id = 0;
  int i;
  double *dpointer;
  Variable v = (Variable) malloc(sizeof(vtype));

  v->cardinality = cardinality;
  v->id = ++id;
  v->probability = NULL;
 
  strncpy(v->name, name, VAR_NAME_LENGTH);
  v->name[VAR_NAME_LENGTH] = '\0';

  v->likelihood = (double *) calloc(cardinality, sizeof(double));
  /* initialise likelihoods to 1 */
  for(dpointer=v->likelihood, i=0; i < cardinality; *dpointer++ = 1, i++);

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
  if(v->probability != NULL)
    free(v->probability);
  free(v);
}

int equal_variables(Variable v1, Variable v2){
  return (v1->id == v2->id);
}

unsigned long get_id(Variable v){
  return v->id;
}

int update_likelihood(Variable v, double likelihood[]){

  int i;
  if(v == NULL)
    return ERROR_NULLPOINTER;

  for(i = 0; i < v->cardinality; i++)
    (v->likelihood)[i] = likelihood[i];

  return NO_ERROR;
}

int number_of_values(Variable v){
  if(v == NULL)
    return ERROR_NULLPOINTER;
  else
    return v->cardinality;
}

int set_probability(Variable v, potential p){
  if(v == NULL)
    return ERROR_NULLPOINTER;

  if(v->probability != NULL)
    free(v->probability);
  v->probability = p;
  return NO_ERROR;
}
