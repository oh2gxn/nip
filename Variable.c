/*
 * Variable.c $Id: Variable.c,v 1.37 2004-08-12 09:47:21 jatoivol Exp $
 */

#include <string.h>
#include <stdlib.h>
#include "Variable.h"
#include "potential.h"
#include "errorhandler.h"

static varlink nip_first_var = NULL;
static varlink nip_last_var = NULL;
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
  v->previous = NULL;
  v->next = NULL;
 
  strncpy(v->symbol, symbol, VAR_SYMBOL_LENGTH);
  v->symbol[VAR_SYMBOL_LENGTH] = '\0';

  if(variable_name(v, name) == ERROR_NULLPOINTER)
    /* DANGER! The name can be omitted and consequently be NULL */
    v->name[0] = '\0';

  if(states){
    v->statenames = (char **) calloc(cardinality, sizeof(char *));
    if(!(v->statenames)){
      report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
      free_variable(v);
      return NULL;
    }else
      for(i = 0; i < cardinality; i++){
	v->statenames[i] = (char *) calloc(strlen(states[i]) + 1, 
					   sizeof(char));
	strcpy(v->statenames[i], states[i]);
      }
  }
  else
    report_error(__FILE__, __LINE__, ERROR_INVALID_ARGUMENT, 1);

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
  int i;
  if(v == NULL)
    return;
  if(v->statenames)
    for(i = 0; i < v->cardinality; i++)
      free(v->statenames[i]);
  free(v->statenames);
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


varlink get_first_variable(){
  return nip_first_var;
}


varlink get_last_variable(){
  return nip_last_var;
}


void reset_Variable_list(){
  nip_first_var = NULL;
  nip_last_var = NULL;
  nip_vars_parsed = 0;
}


Variable next_Variable(Variable_iterator* it){
  Variable v;
  if(*it){
    v = (*it)->data;
    *it = (*it)->fwd;
  }
  else
    v = NULL;
  return v;
}


Variable get_variable(char *symbol){

  Variable v; 
  Variable_iterator it = nip_first_var;
  v = next_Variable(&it);

#ifdef DEBUG_PARSER
  printf("In get_variable: looking for \"%s\"\n", symbol);
#endif

  if(v == NULL)
    return NULL; /* didn't find the variable (possibly normal) */
  
  /* search for the variable reference */
  while(strcmp(symbol, v->symbol) != 0){
    v = next_Variable(&it);
    if(v == NULL){
      return NULL; /* didn't find the variable (a normal situation) */
    }
  }
#ifdef DEBUG_PARSER
  printf("In get_variable: Found \"%s\"\n", v->symbol);
#endif
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


void reset_likelihood(Variable v){
  int i;
  if(v == NULL){
    report_error(__FILE__, __LINE__, ERROR_NULLPOINTER, 1);
    return;
  }
  
  for(i = 0; i < v->cardinality; i++)
    (v->likelihood)[i] = 1;
}


int number_of_values(Variable v){
  if(v == NULL){
    report_error(__FILE__, __LINE__, ERROR_NULLPOINTER, 1);
    return -1;
  }
  else
    return v->cardinality;
}
