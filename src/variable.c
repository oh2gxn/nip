/*
 * variable.c $Id: variable.c,v 1.8 2006-10-10 13:34:16 jatoivol Exp $
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "variable.h"
#include "potential.h"
#include "errorhandler.h"

static varlink nip_first_var = NULL;
static varlink nip_last_var = NULL;
static int nip_vars_parsed = 0;

static int variable_name(variable v, const char *name);


/*
 * Gives the variable a verbose name.
 */
static int variable_name(variable v, const char *name){
  if(!name)
    return ERROR_NULLPOINTER; /* possibly a normal situation */
  strncpy(v->name, name, VAR_NAME_LENGTH);
  v->name[VAR_NAME_LENGTH] = '\0';
  return NO_ERROR;
}


variable new_variable(const char* symbol, const char* name, 
		      char** states, int cardinality){
  /* NOTE: This id-stuff may overflow if variables are created and 
   * freed over and over again. */
  static long id = VAR_MIN_ID;
  int i, j;
  double *dpointer;
  variable v;
  varlink new;

  v = (variable) malloc(sizeof(vtype));
  if(!v){
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    return NULL;
  }

  new = (varlink) malloc(sizeof(varelement));
  if(!new){
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    free(v);
    return NULL;
  }

  v->cardinality = cardinality;
  v->id = id++;
  v->previous = NULL;
  v->next = NULL;

  v->parents = NULL;
  v->prior = NULL; /* Usually prior == NULL  =>  num_of_parents > 0 */
  v->num_of_parents = 0;
  v->family_clique = NULL;
  v->family_mapping = NULL;
  v->if_status = INTERFACE_NONE; /* does not belong to interfaces at first */
  v->pos_x = 100;
  v->pos_y = 100;
  v->mark = 0;
 
  strncpy(v->symbol, symbol, VAR_SYMBOL_LENGTH);
  v->symbol[VAR_SYMBOL_LENGTH] = '\0';

  if(variable_name(v, name) == ERROR_NULLPOINTER)
    /* DANGER! The name can be omitted and consequently be NULL */
    v->name[0] = '\0';

  if(states){
    v->statenames = (char **) calloc(cardinality, sizeof(char *));
    if(!(v->statenames)){
      report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
      free(v);
      free(new);
      return NULL;
    }
    for(i = 0; i < cardinality; i++){
      v->statenames[i] = (char *) calloc(strlen(states[i]) + 1, 
					   sizeof(char));
      if(!(v->statenames[i])){
	report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
	for(j = 0; j < i; j++)
	  free(v->statenames[j]);
	free(v->statenames);
	free(v);
	free(new);
      }
      strcpy(v->statenames[i], states[i]);
    }
  }
  else
    report_error(__FILE__, __LINE__, ERROR_INVALID_ARGUMENT, 1);

  v->likelihood = (double *) calloc(cardinality, sizeof(double));
  if(!(v->likelihood)){
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    for(i = 0; i < v->cardinality; i++)
      free(v->statenames[i]);
    free(v->statenames);
    free(v);
    free(new);
  }

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


variable copy_variable(variable v){
  int i;
  variable copy;

  if(v == NULL)
    return NULL;

  copy = (variable) malloc(sizeof(vtype));
  if(!copy){
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    return NULL;
  }

  copy->cardinality = v->cardinality;
  copy->id = v->id;

  strncpy(copy->name, v->name, VAR_NAME_LENGTH);
  copy->name[VAR_NAME_LENGTH] = '\0';

  copy->num_of_parents = v->num_of_parents;
  if(v->parents){
    copy->parents = (variable*) calloc(v->num_of_parents, sizeof(variable));
    if(!(copy->parents)){
      report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
      free(copy);
      return NULL;
    }
  }
  else
    copy->parents = NULL;
  copy->family_clique = v->family_clique;

  copy->likelihood = (double *) calloc(copy->cardinality, sizeof(double));

  if(!(copy->likelihood)){
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    free(copy->parents);
    free(copy);
    return NULL;
  }  

  /* initialise likelihoods to 1 */
  for(i = 0; i < copy->cardinality; i++)
    copy->likelihood[i] = v->likelihood[i];

  return copy;
}


void free_variable(variable v){
  int i;
  if(v == NULL)
    return;
  if(v->statenames)
    for(i = 0; i < v->cardinality; i++)
      free(v->statenames[i]);
  free(v->statenames);
  free(v->parents);
  free(v->family_mapping);
  free(v->likelihood);
  free(v->prior);
  free(v);
}


int equal_variables(variable v1, variable v2){
  if(v1 && v2)
    return (v1->id == v2->id);
  return 0; /* FALSE if nullpointers */
}


unsigned long get_id(variable v){
  if(v)
    return v->id;
  return 0;
}


void mark_variable(variable v){
  if(v) 
    v->mark = 1;
}

void unmark_variable(variable v){
  if(v)
    v->mark = 0;
}

int variable_marked(variable v){
  if(v)
    return (v->mark != 0);
  return 0;
}


char *get_symbol(variable v){
  if(v)
    return v->symbol;
  return NULL;
}


int get_stateindex(variable v, char *state){
  int i;
  if(!v->statenames)
    return -1;
  for(i = 0; i < v->cardinality; i++)
    if(strcmp(state, v->statenames[i]) == 0)
      return i;
  return -1;
}


char* get_statename(variable v, int index){
  if(!v->statenames)
    return NULL;
  return v->statenames[index];
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


void reset_variable_list(){
  nip_first_var = NULL;
  nip_last_var = NULL;
  nip_vars_parsed = 0;
}


variable next_variable(variable_iterator* it){
  variable v;
  if(*it){
    v = (*it)->data;
    *it = (*it)->fwd;
  }
  else
    v = NULL;
  return v;
}


variable get_parser_variable(char *symbol){

  variable v; 
  variable_iterator it = nip_first_var; /* a private copy */
  v = next_variable(&it);

  if(v == NULL)
    return NULL; /* didn't find the variable (possibly normal) */
  
  /* search for the variable reference */
  while(strcmp(symbol, v->symbol) != 0){
    v = next_variable(&it);
    if(v == NULL){
      return NULL; /* didn't find the variable (a normal situation) */
    }
  }

  return v;

}

variable get_variable(variable* vars, int nvars, char *symbol){

  int i;
  variable v; 
  
  /* search for the variable reference */
  for(i = 0; i < nvars; i++){
    v = vars[i];
    if(strcmp(symbol, v->symbol) == 0)
      return v;
  }

  return NULL;
}


int update_likelihood(variable v, double likelihood[]){

  int i;
  if(v == NULL){
    report_error(__FILE__, __LINE__, ERROR_NULLPOINTER, 1);
    return ERROR_NULLPOINTER;
  }

  for(i = 0; i < v->cardinality; i++)
    (v->likelihood)[i] = likelihood[i];

  return NO_ERROR;
}


void reset_likelihood(variable v){
  int i;
  if(v == NULL){
    report_error(__FILE__, __LINE__, ERROR_NULLPOINTER, 1);
    return;
  }
  
  for(i = 0; i < v->cardinality; i++)
    (v->likelihood)[i] = 1;
}


int number_of_values(variable v){
  if(v == NULL){
    report_error(__FILE__, __LINE__, ERROR_NULLPOINTER, 1);
    return -1;
  }
  return v->cardinality;
}


int number_of_parents(variable v){
  if(v == NULL){
    report_error(__FILE__, __LINE__, ERROR_NULLPOINTER, 1);
    return -1;
  }
  return v->num_of_parents;
}


void set_position(variable v, int x, int y){
  if(v){
    v->pos_x = x;
    v->pos_y = y;
  }
  else
    report_error(__FILE__, __LINE__, ERROR_NULLPOINTER, 1);
}


void get_position(variable v, int* x, int* y){
  if(v && x && y){
    *x = v->pos_x;
    *y = v->pos_y;
  }
  else
    report_error(__FILE__, __LINE__, ERROR_NULLPOINTER, 1);
}


int set_parents(variable v, variable *parents, int nparents){
  int i;
  if(v == NULL || (nparents > 0 && parents == NULL)){
    report_error(__FILE__, __LINE__, ERROR_NULLPOINTER, 1);
    return ERROR_NULLPOINTER;
  }

  free(v->parents); /* in case it previously had parents */
  
  if(nparents > 0){
    v->parents = (variable *) calloc(nparents, sizeof(variable));
    if(!(v->parents)){
      report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
      return ERROR_OUTOFMEMORY;
    }
    
    for(i = 0; i < nparents; i++)
      v->parents[i] = parents[i]; /* makes a copy of the array */
  }
  
  v->num_of_parents = nparents;  
  return NO_ERROR;
}


variable* get_parents(variable v){
  if(v == NULL){
    report_error(__FILE__, __LINE__, ERROR_NULLPOINTER, 1);
    return NULL;
  }
  return v->parents;
}


int set_prior(variable v, double* prior){
  int i;
  if(v == NULL){
    report_error(__FILE__, __LINE__, ERROR_NULLPOINTER, 1);
    return ERROR_NULLPOINTER;
  }
  if(!prior)
    return NO_ERROR;

  free(v->prior);
  v->prior = (double*) calloc(v->cardinality, sizeof(double));
  for(i = 0; i < v->cardinality; i++)
    v->prior[i] = prior[i]; /* makes a copy of the array */

  return NO_ERROR;
}


double* get_prior(variable v){
  if(v == NULL){
    report_error(__FILE__, __LINE__, ERROR_NULLPOINTER, 1);
    return NULL;
  }
  return v->prior;
}


variable *sort_variables(variable *vars, int num_of_vars){

  /* Selection sort (simple, fast enough) */

  int i, j;
  variable *sorted;
  variable temp;

  if(num_of_vars < 1){
    report_error(__FILE__, __LINE__, ERROR_INVALID_ARGUMENT, 1);
    return NULL;
  }

  sorted = (variable *) calloc(num_of_vars, sizeof(variable));

  if(!sorted){
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    return NULL;
  }

  /* Fill the sorted array naively (unsorted) */
  for(i = 0; i < num_of_vars; i++)
    sorted[i] = vars[i];

  /* Sort in place (lots of swaps, I know...) */
  for(i = 0; i < num_of_vars - 1; i++)
    for(j = 1; j < num_of_vars; j++)
      if(sorted[j]->id < sorted[i]->id){
	temp = sorted[j];
	sorted[j] = sorted[i];
	sorted[i] = temp;
      }

  return sorted;

}


variable *variable_union(variable *a, variable *b, int na, int nb, int* nc){
  int i, j, n, found;
  variable *c = NULL;

  /* Check stuff */
  if(!(a && na > 0 && ((b == NULL && nb == 0) || (b != NULL && nb > 0)))){
    report_error(__FILE__, __LINE__, ERROR_INVALID_ARGUMENT, 1);
    if(nc)
      *nc = -1;
    return NULL;
  }
  
  /* Size of the union */
  n = na + nb;
  for(i = 0; i < na; i++){
    for(j = 0; j < nb; j++){
      if(equal_variables(a[i], b[j])){
	n--;
	break;
      }
    }
  }
  *nc = n; /* save */
  if(!n)
    return NULL; /* empty set */

  /* The union operation */
  c = (variable*) calloc(*nc, sizeof(variable));
  if(!c){
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    if(nc)
      *nc = -1;
    return NULL;
  }
  for(i = 0; i < na; i++){ /* first the set a */
    c[i] = a[i];
  }
  n = na;
  for(i = 0; i < nb; i++){ /* then the missing ones from b */
    found = 0;
    for(j = 0; j < n; j++){
      if(equal_variables(b[i], c[j])){
	found = 1;
	break;
      }
    }
    if(!found)
      c[n++] = b[i];
  }
  /* assert(*nc==n); */
  return c;
}


variable *variable_isect(variable *a, variable *b, int na, int nb, int* nc){
  int i, j, n;
  variable *c = NULL;
  
  /* Check stuff */
  if(!(a && b && na > 0 && nb > 0)){
    report_error(__FILE__, __LINE__, ERROR_INVALID_ARGUMENT, 1);
    if(nc)
      *nc = -1;
    return NULL;
  }
  
  /* Size of the intersection */
  n = 0;
  for(i = 0; i < na; i++){
    for(j = 0; j < nb; j++){
      if(equal_variables(a[i], b[j])){
	n++;
	break;
      }
    }
  }
  *nc = n; /* save */
  if(!n)
    return NULL; /* empty set */

  /* The intersection operation */
  c = (variable*) calloc(*nc, sizeof(variable));
  if(!c){
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    if(nc)
      *nc = -1;
    return NULL;
  }
  n = 0;
  for(i = 0; i < na; i++){
    for(j = 0; j < nb; j++){
      if(equal_variables(a[i], b[j])){
	c[n++] = a[i];
	break;
      }
    }
  }
  /* assert(*nc==n); */
  return c;
}


int *mapper(variable *set, variable *subset, int nset, int nsubset){
  int i, j;
  int *mapping = NULL;
  
  /* Check stuff */
  if(!(set && subset && nset > 0 && nsubset > 0 && nset >= nsubset)){
    if(nsubset != 0)
      report_error(__FILE__, __LINE__, ERROR_INVALID_ARGUMENT, 1);
    return NULL;
  }

  mapping = (int*) calloc(nsubset, sizeof(int));
  if(!mapping){
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    return NULL;
  }

  for(i = 0; i < nsubset; i++){
    for(j = 0; j < nset; j++){ /* linear search */
      if(equal_variables(subset[i], set[j])){
	mapping[i] = j;
	break;
      }
    }
  }

  return mapping;
}
