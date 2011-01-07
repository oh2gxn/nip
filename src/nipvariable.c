/* nipvariable.c 
 * Authors: Janne Toivola, Mikko Korpela
 * Version: $Id: nipvariable.c,v 1.8 2011-01-07 01:52:05 jatoivol Exp $
 */


#include "nipvariable.h"


static int nip_set_variable_text(char** record, const char *name);

/*
 * Gives the variable a verbose name, symbol etc.
 */
static int nip_set_variable_text(char** record, const char *name){
  int len;

  if(!record || !name)
    return NIP_ERROR_NULLPOINTER; /* possibly a normal situation? */

  len = strlen(name);
  if(len > NIP_VAR_TEXT_LENGTH)
    len = NIP_VAR_TEXT_LENGTH; /* FIXME: UTF-8 incompatible limit! */

  if(*record)
    free(*record); /* Although free(NULL) would be fine? */

  *record = (char*) calloc(len+1, sizeof(char));
  if(!(*record))
    return NIP_ERROR_OUTOFMEMORY;

  strncpy(*record, name, len);
  (*record)[len] = '\0'; /* null termination */
  return NIP_NO_ERROR;
}


nip_variable nip_new_variable(const char* symbol, const char* name, 
			      char** states, int cardinality){
  /* NOTE: This id-stuff may overflow if variables are created and 
   * freed over and over again. */
  static long id = NIP_VAR_MIN_ID;
  int i, j;
  double *dpointer;
  nip_variable v;

  v = (nip_variable) malloc(sizeof(nip_variable_struct));
  if(!v){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_OUTOFMEMORY, 1);
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
  v->interface_status = NIP_INTERFACE_NONE; /* does not belong to interfaces */
  v->pos_x = 100;
  v->pos_y = 100;
  v->mark = NIP_MARK_OFF; /* If this becomes 0x00, there will be a bug (?) */

  v->symbol = NULL;
  nip_set_variable_text(&(v->symbol), symbol);

  v->name = NULL;
  if(name)
    nip_set_variable_text(&(v->name), name);
  /* DANGER! The name can be omitted and consequently be NULL */

  if(states){
    v->state_names = (char **) calloc(cardinality, sizeof(char *));
    if(!(v->state_names)){
      nip_report_error(__FILE__, __LINE__, NIP_ERROR_OUTOFMEMORY, 1);
      free(v);
      return NULL;
    }
    for(i = 0; i < cardinality; i++){
      if(nip_set_variable_text(&(v->state_names[i]), states[i]) == 
	 NIP_ERROR_OUTOFMEMORY){
	nip_report_error(__FILE__, __LINE__, NIP_ERROR_OUTOFMEMORY, 1);
	for(j = 0; j < i; j++)
	  free(v->state_names[j]);
	free(v->state_names);
	free(v);
      }
    }
  }
  else
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_INVALID_ARGUMENT, 1);

  v->likelihood = (double *) calloc(cardinality, sizeof(double));
  if(!(v->likelihood)){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_OUTOFMEMORY, 1);
    for(i = 0; i < v->cardinality; i++)
      free(v->state_names[i]);
    free(v->state_names);
    free(v);
  }

  /* initialise likelihoods to 1 */
  for(dpointer=v->likelihood, i=0; i < cardinality; *dpointer++ = 1, i++);

  return v;
}


/* Useful? Get rid of this function? */
nip_variable nip_copy_variable(nip_variable v){
  int i;
  int len;
  nip_variable copy;

  if(v == NULL)
    return NULL;

  copy = (nip_variable) malloc(sizeof(nip_variable_struct));
  if(!copy){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_OUTOFMEMORY, 1);
    return NULL;
  }

  copy->cardinality = v->cardinality;
  copy->id = v->id;

  len = strlen(v->name);
  nip_set_variable_text(&(copy->name), v->name);
  /* symbol etc? */

  copy->num_of_parents = v->num_of_parents;
  if(v->parents){
    copy->parents = (nip_variable*) calloc(v->num_of_parents, 
					   sizeof(nip_variable));
    if(!(copy->parents)){
      nip_report_error(__FILE__, __LINE__, NIP_ERROR_OUTOFMEMORY, 1);
      free(copy);
      return NULL;
    }
    for(i = 0; i < v->num_of_parents; i++)
      copy->parents[i] = v->parents[i];
  }
  else
    copy->parents = NULL;
  copy->family_clique = v->family_clique;

  copy->likelihood = (double*) calloc(copy->cardinality, sizeof(double));
  if(!(copy->likelihood)){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_OUTOFMEMORY, 1);
    free(copy->parents);
    free(copy);
    return NULL;
  }
  for(i = 0; i < copy->cardinality; i++)
    copy->likelihood[i] = v->likelihood[i];

  /* FIXME: also other fields, like copy->prior, copy->previous, etc... */

  return copy;
}


void nip_free_variable(nip_variable v){
  int i;
  if(v == NULL)
    return;
  free(v->symbol);
  free(v->name);
  if(v->state_names)
    for(i = 0; i < v->cardinality; i++)
      free(v->state_names[i]);
  free(v->state_names);
  free(v->parents);
  free(v->family_mapping);
  free(v->likelihood);
  free(v->prior);
  free(v);
}


int nip_equal_variables(nip_variable v1, nip_variable v2){
  if(v1 && v2)
    return (v1->id == v2->id);
  return 0; /* FALSE if nullpointers */
}


unsigned long nip_variable_id(nip_variable v){
  if(v)
    return v->id;
  return 0;
}


void nip_mark_variable(nip_variable v){
  if(v) 
    v->mark = NIP_MARK_ON;
}

void nip_unmark_variable(nip_variable v){
  if(v)
    v->mark = NIP_MARK_OFF;
}

int nip_variable_marked(nip_variable v){
  if(v)
    return (v->mark != NIP_MARK_OFF);
  return 0;
}


char* nip_variable_symbol(nip_variable v){
  if(v)
    return v->symbol;
  return NULL;
}


int nip_variable_state_index(nip_variable v, char *state){
  int i;
  if(!v->state_names)
    return -1;
  for(i = 0; i < v->cardinality; i++)
    if(strcmp(state, v->state_names[i]) == 0)
      return i;
  return -1;
}


char* nip_variable_state_name(nip_variable v, int index){
  if(!v->state_names)
    return NULL;
  return v->state_names[index];
}


nip_variable nip_search_variable_array(nip_variable* vars, int nvars, 
				       char *symbol){
  int i;
  nip_variable v; 
  
  /* search for the variable reference */
  for(i = 0; i < nvars; i++){
    v = vars[i];
    if(strcmp(symbol, v->symbol) == 0)
      return v;
  }

  return NULL;
}


nip_error_code nip_update_likelihood(nip_variable v, double likelihood[]){

  int i;
  if(v == NULL || likelihood == NULL){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_NULLPOINTER, 1);
    return NIP_ERROR_NULLPOINTER;
  }

  for(i = 0; i < v->cardinality; i++)
    (v->likelihood)[i] = likelihood[i];

  return NIP_NO_ERROR;
}


void nip_reset_likelihood(nip_variable v){
  int i;
  if(v == NULL){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_NULLPOINTER, 1);
    return;
  }
  
  for(i = 0; i < v->cardinality; i++)
    (v->likelihood)[i] = 1;
}


/* OLD STUFF
int nip_number_of_values(nip_variable v){
  if(v == NULL){
    report_error(__FILE__, __LINE__, ERROR_NULLPOINTER, 1);
    return -1;
  }
  return v->cardinality;
}
*/


int nip_number_of_parents(nip_variable v){
  if(v == NULL){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_NULLPOINTER, 1);
    return -1;
  }
  return v->num_of_parents;
}


void nip_set_variable_position(nip_variable v, int x, int y){
  if(v){
    v->pos_x = x;
    v->pos_y = y;
  }
  else
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_NULLPOINTER, 1);
}


void nip_get_variable_position(nip_variable v, int* x, int* y){
  if(v && x && y){
    *x = v->pos_x;
    *y = v->pos_y;
  }
  else
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_NULLPOINTER, 1);
}


nip_error_code nip_set_parents(nip_variable v, 
			       nip_variable* parents, 
			       int nparents){
  int i;
  if(v == NULL || (nparents > 0 && parents == NULL)){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_NULLPOINTER, 1);
    return NIP_ERROR_NULLPOINTER;
  }

  free(v->parents); /* in case it previously had another array */
  
  if(nparents > 0){
    v->parents = (nip_variable *) calloc(nparents, sizeof(nip_variable));
    if(!(v->parents)){
      nip_report_error(__FILE__, __LINE__, NIP_ERROR_OUTOFMEMORY, 1);
      return NIP_ERROR_OUTOFMEMORY;
    }
    
    for(i = 0; i < nparents; i++)
      v->parents[i] = parents[i]; /* makes a copy of the array */
  }
  
  v->num_of_parents = nparents;  
  return NIP_NO_ERROR;
}


nip_variable* nip_get_parents(nip_variable v){
  if(v == NULL){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_NULLPOINTER, 1);
    return NULL;
  }
  return v->parents;
}


int nip_variable_is_parent(nip_variable parent, nip_variable child) {
  int p;
  if (!parent || !child)
    return 0;

  for (p = 0; p < child->num_of_parents; p++)
    if (nip_equal_variables(parent, child->parents[p]))
      return 1;

  return 0;
}


nip_error_code nip_set_prior(nip_variable v, double* prior){
  int i;
  if(v == NULL){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_NULLPOINTER, 1);
    return NIP_ERROR_NULLPOINTER;
  }
  if(!prior)
    return NIP_NO_ERROR;

  free(v->prior); /* reuse the allocated space??? */
  v->prior = (double*) calloc(v->cardinality, sizeof(double));
  for(i = 0; i < v->cardinality; i++)
    v->prior[i] = prior[i]; /* makes a copy of the array */

  return NIP_NO_ERROR;
}


double* nip_get_prior(nip_variable v){
  if(v == NULL){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_NULLPOINTER, 1);
    return NULL;
  }
  return v->prior;
}


nip_variable* nip_sort_variables(nip_variable* vars, int nvars){
  int i, j;
  nip_variable *sorted;
  nip_variable temp;

  if(nvars < 1){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_INVALID_ARGUMENT, 1);
    return NULL;
  }

  sorted = (nip_variable *) calloc(nvars, sizeof(nip_variable));

  if(!sorted){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_OUTOFMEMORY, 1);
    return NULL;
  }

  /* Selection sort (simple, fast enough) */

  /* Fill the sorted array naively (unsorted) */
  for(i = 0; i < nvars; i++)
    sorted[i] = vars[i];

  /* Sort in place (lots of swaps, I know...) */
  for(i = 0; i < nvars - 1; i++)
    for(j = 1; j < nvars; j++)
      if(sorted[j]->id < sorted[i]->id){
	temp = sorted[j];
	sorted[j] = sorted[i];
	sorted[i] = temp;
      }

  return sorted;
}


nip_variable* nip_variable_union(nip_variable* a, nip_variable* b, 
				 int na, int nb, int* nc){
  int i, j, n, found;
  nip_variable* c = NULL;

  /* Check stuff */
  if((nc == NULL) || (na > 0 && a == NULL) || (nb > 0 && b == NULL)){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_INVALID_ARGUMENT, 1);
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
      if(nip_equal_variables(a[i], b[j])){
	n--;
	break;
      }
    }
  }
  *nc = n; /* save */
  if(!n)
    return NULL; /* empty set... shouldn't happen at this phase though */

  /* The union operation */
  c = (nip_variable*) calloc(*nc, sizeof(nip_variable));
  if(!c){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_OUTOFMEMORY, 1);
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
      if(nip_equal_variables(b[i], c[j])){
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


nip_variable* nip_variable_isect(nip_variable *a, nip_variable *b, 
				 int na, int nb, int* nc){
  int i, j, n;
  nip_variable *c = NULL;
  
  /* Check stuff */
  if(!(a && b && na > 0 && nb > 0)){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_INVALID_ARGUMENT, 1);
    if(nc)
      *nc = -1;
    return NULL;
  }
  
  /* Size of the intersection */
  n = 0;
  for(i = 0; i < na; i++){
    for(j = 0; j < nb; j++){
      if(nip_equal_variables(a[i], b[j])){
	n++;
	break;
      }
    }
  }
  *nc = n; /* save */
  if(n == 0)
    return NULL; /* empty set */

  /* The intersection operation */
  c = (nip_variable*) calloc(*nc, sizeof(nip_variable));
  if(!c){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_OUTOFMEMORY, 1);
    if(nc)
      *nc = -1;
    return NULL;
  }
  n = 0;
  for(i = 0; i < na; i++){
    for(j = 0; j < nb; j++){
      if(nip_equal_variables(a[i], b[j])){
	c[n++] = a[i];
	break;
      }
    }
  }
  /* assert(*nc==n); */
  return c;
}


int* nip_mapper(nip_variable *set, nip_variable *subset, 
		int nset, int nsubset){
  int i, j;
  int* mapping = NULL;
  
  /* Check stuff */
  if(!(set && subset && nset > 0 && nsubset > 0 && nset >= nsubset)){
    if(nsubset != 0)
      nip_report_error(__FILE__, __LINE__, NIP_ERROR_INVALID_ARGUMENT, 1);
    return NULL;
  }

  mapping = (int*) calloc(nsubset, sizeof(int));
  if(!mapping){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_OUTOFMEMORY, 1);
    return NULL;
  }

  for(i = 0; i < nsubset; i++){
    for(j = 0; j < nset; j++){ /* linear search */
      if(nip_equal_variables(subset[i], set[j])){
	mapping[i] = j;
	break;
      }
    }
  }

  return mapping;
}


nip_variable_list nip_new_variable_list(){
  nip_variable_list vl = (nip_variable_list) 
    malloc(sizeof(nip_variable_list_struct));
  /* Q: What if NULL was returned? */
  vl->length = 0;
  vl->first  = NULL;
  vl->last   = NULL;
  return vl;
}


nip_interface_list nip_new_interface_list(){
  nip_interface_list l = (nip_interface_list) 
    malloc(sizeof(nip_iflist_struct));
  /* Q: What if NULL was returned? */
  l->length = 0;
  l->first  = NULL;
  l->last   = NULL;
  return l;
}


nip_error_code nip_append_variable(nip_variable_list l, nip_variable v){
  nip_variable_link new = (nip_variable_link) 
    malloc(sizeof(nip_variable_link_struct));

  if(!l || !v){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_INVALID_ARGUMENT, 1);
    return NIP_ERROR_INVALID_ARGUMENT;
  }

  if(!new){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_OUTOFMEMORY, 1);
    return NIP_ERROR_OUTOFMEMORY;
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
  return NIP_NO_ERROR;
}


nip_error_code nip_append_interface(nip_interface_list l, 
				    nip_variable var, 
				    char* next){
  nip_interface_link new = (nip_interface_link) 
    malloc(sizeof(nip_iflink_struct));

  if(!l || !var){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_INVALID_ARGUMENT, 1);
    return NIP_ERROR_INVALID_ARGUMENT;
  }

  if(!new){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_OUTOFMEMORY, 1);
    return NIP_ERROR_OUTOFMEMORY;
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
  return NIP_NO_ERROR;
}


nip_error_code nip_prepend_variable(nip_variable_list l, nip_variable v){
  nip_variable_link new = (nip_variable_link) 
    malloc(sizeof(nip_variable_link_struct));

  if(!l || !v){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_INVALID_ARGUMENT, 1);
    return NIP_ERROR_INVALID_ARGUMENT;
  }

  if(!new){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_OUTOFMEMORY, 1);
    return NIP_ERROR_OUTOFMEMORY;
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
  return NIP_NO_ERROR;
}

nip_error_code nip_prepend_interface(nip_interface_list l, 
				     nip_variable var, 
				     char* next){
  nip_interface_link new = (nip_interface_link) 
    malloc(sizeof(nip_iflink_struct));

  if(!l || !var){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_INVALID_ARGUMENT, 1);
    return NIP_ERROR_INVALID_ARGUMENT;
  }

  if(!new){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_OUTOFMEMORY, 1);
    return NIP_ERROR_OUTOFMEMORY;
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
  return NIP_NO_ERROR;
}


nip_variable* nip_variable_list_to_array(nip_variable_list l){
  int i;
  nip_variable_link ln;
  nip_variable* new;
  
  if(!l){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_INVALID_ARGUMENT, 1);
    return NULL;
  }

  if(l->length == 0)
    return NULL;

  new = (nip_variable*) calloc(l->length, sizeof(nip_variable));
  if(!new){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_OUTOFMEMORY, 1);
    return NULL;
  }

  ln = l->first;
  for(i = 0; i < l->length; i++){
    new[i] = ln->data; /* the pointer is copied here */
    ln = ln->fwd;
  }
  return new;
}


void nip_empty_variable_list(nip_variable_list l){
  nip_variable_link ln;

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


void nip_free_interface_list(nip_interface_list l){
  nip_interface_link ln;

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
nip_variable nip_next_variable(nip_variable_iterator* it){
  nip_variable v;
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
nip_variable nip_search_variable_list(nip_variable_list l, char *symbol){
  nip_variable v; 
  nip_variable_iterator it = l->first; /* a private copy */
  v = nip_next_variable(&it);

  if(v == NULL)
    return NULL; /* didn't find the variable (possibly normal) */
  
  /* search for the variable reference */
  while(strcmp(symbol, nip_variable_symbol(v)) != 0){
    v = nip_next_variable(&it);
    if(v == NULL){
      return NULL; /* didn't find the variable (a normal situation) */
    }
  }
  return v;
}
