/*
 * Variable.c $Id: Variable.c,v 1.28 2004-06-24 13:38:18 jatoivol Exp $
 */

#include <string.h>
#include <stdlib.h>
#include "Variable.h"
#include "potential.h"
#include "errorhandler.h"

static varlink nip_first_var = NULL;
static varlink nip_last_var = NULL;
static varlink list_pointer = NULL;
static int nip_vars_parsed = 0;

static int variable_name(Variable v, const char *name);


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
 * NOTE: "the ownership" of the states (array of strings) changes.
 */
Variable new_variable(const char* symbol, const char* name, 
		      char** states, int cardinality){
  static long id = VAR_MIN_ID;
  int i;
  double *dpointer;
  Variable v = (Variable) malloc(sizeof(vtype));
  varlink new = (varlink) malloc(sizeof(varelement));

  if(!(v && new)){
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    return NULL;
  }

  v->cardinality = cardinality;
  v->id = id++;
 
  strncpy(v->symbol, symbol, VAR_SYMBOL_LENGTH);
  v->symbol[VAR_SYMBOL_LENGTH] = '\0';

  if(variable_name(v, name) == ERROR_NULLPOINTER)
    /* DANGER! The name can be omitted and consequently be NULL */
    v->name[0] = '\0';

  /* "the ownership" of the states (array of strings) changes */
  v->statenames = states;

  v->likelihood = (double *) calloc(cardinality, sizeof(double));
  /* initialise likelihoods to 1 */
  for(dpointer=v->likelihood, i=0; i < cardinality; *dpointer++ = 1, i++);

  new->data = v;
  new->fwd = NULL;
  new->bwd = nip_last_var;
  if(nip_first_var == NULL)
    nip_first_var = new;
  else
    nip_last_var->fwd = new;
  nip_last_var = new;
  nip_vars_parsed++;

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
  /* FIXME: remove the Variable from the list ? */
  if(v == NULL)
    return;
  free(v->likelihood);
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


int get_stateindex(Variable v, char *state){
  int i;
  if(!v->statenames)
    return -1;
  for(i = 0; i < v->cardinality; i++)
    if(strcmp(state, v->statenames[i]) == 0)
      return i;
  return -1;
}


int total_num_of_vars(){
  return nip_vars_parsed;
}


void reset_Variable_list(){
  list_pointer = nip_first_var;
}


Variable next_Variable(){
  Variable v;
  if(list_pointer){
    v = list_pointer->data;
    list_pointer = list_pointer->fwd;
  }
  else
    v = NULL;
  return v;
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
  if(v == NULL){
    report_error(__FILE__, __LINE__, ERROR_NULLPOINTER, 1);
    return -1;
  }
  else
    return v->cardinality;
}
