/*
 * Variable.c $Id: Variable.c,v 1.24 2004-06-21 06:48:15 mvkorpel Exp $
 */

#include <string.h>
#include <stdlib.h>
#include "Variable.h"
#include "potential.h"
#include "errorhandler.h"

static int variable_name(Variable v, const char *name);

static int variable_statenames(Variable v, char **states);

/*
 * Gives the Variable a verbose name.
 */
static int variable_name(Variable v, const char *name){
  if(!name)
    return ERROR_NULLPOINTER; /* possibly a normal situation */
  strncpy(v->name, name, VAR_NAME_LENGTH);
  v->name[VAR_NAME_LENGTH] = '\0';
  return NO_ERROR;
}


/*
 * Gives the Variable the names of the states if needed. 
 * NOTE: "the ownership" of the states (array of strings) changes.
 */
static int variable_statenames(Variable v, char **states){
  v->statenames = states;
  return 0;
}


Variable new_variable(const char* symbol, const char* name, 
		      char** states, int cardinality){
  static long id = VAR_MIN_ID;
  int i;
  double *dpointer;
  Variable v = (Variable) malloc(sizeof(vtype));

  if(!v){
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    return NULL;
  }

  v->cardinality = cardinality;
  v->id = id++;
  v->probability = NULL; 
 
  strncpy(v->symbol, symbol, VAR_SYMBOL_LENGTH);
  v->symbol[VAR_SYMBOL_LENGTH] = '\0';

  if(variable_name(v, name) == ERROR_NULLPOINTER)
    /* DANGER! The name can be omitted and consequently be NULL */
    v->name[0] = '\0';

  /* "the ownership" of the states (array of strings) changes */
  variable_statenames(v, states); 

  v->likelihood = (double *) calloc(cardinality, sizeof(double));
  /* initialise likelihoods to 1 */
  for(dpointer=v->likelihood, i=0; i < cardinality; *dpointer++ = 1, i++);

  return v;
}


Variable copy_variable(Variable v){
  int i;
  Variable copy = (Variable) malloc(sizeof(vtype));

  if(v == NULL){
    free(copy);
    return NULL;
  }

  if(!copy){
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    return NULL;
  }

  copy->cardinality = v->cardinality;
  copy->id = v->id;

  strncpy(copy->name, v->name, VAR_NAME_LENGTH);
  copy->name[VAR_NAME_LENGTH] = '\0';

  copy->likelihood = (double *) calloc(copy->cardinality, sizeof(double));

  if(!(copy->likelihood)){
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    free(copy);
    return NULL;
  }  

  /* initialise likelihoods to 1 */
  for(i = 0; i < copy->cardinality; i++)
    copy->likelihood[i] = v->likelihood[i];

  return copy;
}


void free_variable(Variable v){
  if(v == NULL)
    return;
  free(v->likelihood);
  free(v->probability);
  free(v);
}


int equal_variables(Variable v1, Variable v2){
  if(v1 && v2)
    return (v1->id == v2->id);
  return 0; /* FALSE if nullpointers */
}


unsigned long get_id(Variable v){
  if(v)
    return v->id;
  return 0;
}


char *get_symbol(Variable v){
  if(v)
    return v->symbol;
  return NULL;
}


int update_likelihood(Variable v, double likelihood[]){

  int i;
  if(v == NULL){
    report_error(__FILE__, __LINE__, ERROR_NULLPOINTER, 1);
    return ERROR_NULLPOINTER;
  }

  for(i = 0; i < v->cardinality; i++)
    (v->likelihood)[i] = likelihood[i];

  return NO_ERROR;
}


int number_of_values(Variable v){
  if(v == NULL)
    return -1;
  else
    return v->cardinality;
}


/* the "ownership" of the potential changes */
int set_probability(Variable v, potential p){
  if(v == NULL){
    report_error(__FILE__, __LINE__, ERROR_NULLPOINTER, 1);
    return ERROR_NULLPOINTER;
  }

  free(v->probability); /* free(NULL) is O.K. */
  v->probability = p; /* cruel, isn't it */
  return NO_ERROR;
}
