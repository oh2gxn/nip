/*
 * variable.c $Id: variable.c,v 1.15 2007-09-30 19:20:59 jatoivol Exp $
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "variable.h"
#include "potential.h"
#include "errorhandler.h"

static int set_variable_text(char** record, const char *name);

/*
 * Gives the variable a verbose name, symbol etc.
 */
static int set_variable_text(char** record, const char *name){
  int len;

  if(!record || !name)
    return ERROR_NULLPOINTER; /* possibly a normal situation? */

  len = strlen(name);
  if(len > VAR_TEXT_LENGTH)
    len = VAR_TEXT_LENGTH;

  if(*record)
    free(*record); /* Although free(NULL) would be fine? */

  *record = (char*) calloc(len+1, sizeof(char));
  if(!(*record))
    return ERROR_OUTOFMEMORY;

  strncpy(*record, name, len);
  (*record)[len] = '\0'; /* null termination */
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

  v = (variable) malloc(sizeof(vtype));
  if(!v){
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    return NULL;
  }

  v->cardinality = cardinality;
  v->id = id++;
  v->previous = NULL;
  v->next = NULL;

  v->parents = NULL;
  v->prior = NULL; /* Usually prior == NULL  =>  num_of_parents > 0 */
  v->prior_entered = 0;
  v->num_of_parents = 0;
  v->family_clique = NULL;
  v->family_mapping = NULL;
  v->if_status = INTERFACE_NONE; /* does not belong to interfaces at first */
  v->pos_x = 100;
  v->pos_y = 100;
  v->mark = MARK_OFF; /* If this becomes 0x00, there will be a bug (?) */

  v->symbol = NULL;
  set_variable_text(&(v->symbol), symbol);

  v->name = NULL;
  if(name)
    set_variable_text(&(v->name), name);
  /* DANGER! The name can be omitted and consequently be NULL */

  if(states){
    v->statenames = (char **) calloc(cardinality, sizeof(char *));
    if(!(v->statenames)){
      report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
      free(v);
      return NULL;
    }
    for(i = 0; i < cardinality; i++){
      if(set_variable_text(&(v->statenames[i]), states[i]) == 
	 ERROR_OUTOFMEMORY){
	report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
	for(j = 0; j < i; j++)
	  free(v->statenames[j]);
	free(v->statenames);
	free(v);
      }
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
  }

  /* initialise likelihoods to 1 */
  for(dpointer=v->likelihood, i=0; i < cardinality; *dpointer++ = 1, i++);

  return v;
}


/* Useful? Get rid of this function? */
variable copy_variable(variable v){
  int i;
  int len;
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

  len = strlen(v->name);
  set_variable_text(&(copy->name), v->name);
  /* symbol etc? */

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
  free(v->symbol);
  free(v->name);
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
    v->mark = MARK_ON;
}

void unmark_variable(variable v){
  if(v)
    v->mark = MARK_OFF;
}

int variable_marked(variable v){
  if(v)
    return (v->mark != MARK_OFF);
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


/* OLD STUFF
int number_of_values(variable v){
  if(v == NULL){
    report_error(__FILE__, __LINE__, ERROR_NULLPOINTER, 1);
    return -1;
  }
  return v->cardinality;
}
*/


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

  free(v->parents); /* in case it previously had another array */
  
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
  if((nc == NULL) || (na > 0 && a == NULL) || (nb > 0 && b == NULL)){
    report_error(__FILE__, __LINE__, ERROR_INVALID_ARGUMENT, 1);
    if(nc)
      *nc = -1;
    return NULL;
  }

  if(na <= 0 && nb <= 0){
    *nc = 0;
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
    return NULL; /* empty set... shouldn't happen at this phase though */

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


variablelist make_variablelist(){
  variablelist vl = (variablelist) malloc(sizeof(variableliststruct));
  /* Q: What if NULL was returned? */
  vl->length = 0;
  vl->first  = NULL;
  vl->last   = NULL;
  return vl;
}


interfaceList make_interfaceList(){
  interfaceList l = (interfaceList) malloc(sizeof(interfaceListStruct));
  /* Q: What if NULL was returned? */
  l->length = 0;
  l->first  = NULL;
  l->last   = NULL;
  return l;
}


int append_variable(variablelist l, variable v){
  variablelink new = (variablelink) malloc(sizeof(variablelinkstruct));

  if(!l || !v){
    report_error(__FILE__, __LINE__, ERROR_INVALID_ARGUMENT, 1);
    return ERROR_INVALID_ARGUMENT;
  }

  if(!new){
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    return ERROR_OUTOFMEMORY;
  }

  new->data = v;
  new->fwd = NULL;
  new->bwd = l->last;
  if(l->first == NULL)
    l->first = new;
  else
    l->last->fwd = new;

  l->last = new;
  l->length++;
  return NO_ERROR;
}


int append_interface(interfaceList l, variable var, char* next){
  interfaceLink new = (interfaceLink) malloc(sizeof(interfaceLinkStruct));

  if(!l || !var){
    report_error(__FILE__, __LINE__, ERROR_INVALID_ARGUMENT, 1);
    return ERROR_INVALID_ARGUMENT;
  }

  if(!new){
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    return ERROR_OUTOFMEMORY;
  }

  new->var = var;
  new->next = next;
  /* The ownership of the "next" string changes! */
  new->fwd = NULL;
  new->bwd = l->last;
  if(l->first == NULL)
    l->first = new;
  else
    l->last->fwd = new;

  l->last = new;
  l->length++;
  return NO_ERROR;
}


int prepend_variable(variablelist l, variable v){
  variablelink new = (variablelink) malloc(sizeof(variablelinkstruct));

  if(!l || !v){
    report_error(__FILE__, __LINE__, ERROR_INVALID_ARGUMENT, 1);
    return ERROR_INVALID_ARGUMENT;
  }

  if(!new){
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    return ERROR_OUTOFMEMORY;
  }

  new->data = v;
  new->bwd = NULL;
  new->fwd = l->first;
  if(l->last == NULL)
    l->last = new;
  else
    l->first->bwd = new;

  l->first = new;
  l->length++;
  return NO_ERROR;
}

int prepend_interface(interfaceList l, variable var, char* next){
  interfaceLink new = (interfaceLink) malloc(sizeof(interfaceLinkStruct));

  if(!l || !var){
    report_error(__FILE__, __LINE__, ERROR_INVALID_ARGUMENT, 1);
    return ERROR_INVALID_ARGUMENT;
  }

  if(!new){
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    return ERROR_OUTOFMEMORY;
  }

  new->var = var;
  new->next = next;
  new->bwd = NULL;
  new->fwd = l->first;
  if(l->last == NULL)
    l->last = new;
  else
    l->first->bwd = new;

  l->first = new;
  l->length++;
  return NO_ERROR;
}


variable* list_to_variable_array(variablelist l){
  int i;
  variablelink ln;
  variable* new;
  
  if(!l){
    report_error(__FILE__, __LINE__, ERROR_INVALID_ARGUMENT, 1);
    return NULL;
  }

  if(l->length == 0)
    return NULL;

  new = (variable*) calloc(l->length, sizeof(variable));
  if(!new){
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    return NULL;
  }

  ln = l->first;
  for(i = 0; i < l->length; i++){
    new[i] = ln->data; /* the pointer is copied here */
    ln = ln->fwd;
  }
  return new;
}


void empty_variablelist(variablelist l){
  variablelink ln;

  if(!l)
    return;
  
  ln = l->last;

  l->last = NULL;
  while(ln != NULL){
    free(ln->fwd); /* free(NULL) is O.K. at the beginning */
    ln = ln->bwd;
  }
  free(l->first);
  l->first = NULL;
  l->length = 0;
  return;
}


void free_interfaceList(interfaceList l){
  interfaceLink ln;

  if(!l)
    return;
  
  ln = l->last;

  l->last = NULL;
  while(ln != NULL){
    if(ln->fwd != NULL){
      free(ln->fwd->next);
      free(ln->fwd);
    }
    ln = ln->bwd;
  }
  if(l->first != NULL){
    free(l->first->next);
    free(l->first);
    l->first = NULL;
  }
  l->length = 0;

  free(l);
  l = NULL;
  return;
}


/*
 * Iterator
 */
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


/*
 * Search method
 */
variable get_parser_variable(variablelist l, char *symbol){
  variable v; 
  variable_iterator it = l->first; /* a private copy */
  v = next_variable(&it);

  if(v == NULL)
    return NULL; /* didn't find the variable (possibly normal) */
  
  /* search for the variable reference */
  while(strcmp(symbol, get_symbol(v)) != 0){
    v = next_variable(&it);
    if(v == NULL){
      return NULL; /* didn't find the variable (a normal situation) */
    }
  }
  return v;
}
