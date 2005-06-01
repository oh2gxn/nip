/*
 * potential.c $Id: potential.c,v 1.59 2005-06-01 12:59:10 jatoivol Exp $
 * Functions for handling potentials. 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "potential.h" 
#include "errorhandler.h"

/*#define DEBUG_POTENTIAL*/

/* TO DO: get rid of the requirement of having potentials in a certain order. 
 * This can be done with some sort of mapping arrays for looking up the 
 * correct index positions. */

static double *get_ppointer(potential p, int indices[]);

static void choose_indices(int source_indices[], int dest_indices[], 
			   int mapping[], int size_of_mapping);

/*
 * Returns a pointer to the potential with given variable values (indices).
 */
static double *get_ppointer(potential p, int indices[]){

  int index = 0;
  int i;
  int card_temp = 1;

  /* THE mapping (JJ: I made this clearer on 22.5.2004)*/
  for(i = 0; i < p->num_of_vars; i++){
    index += indices[i] * card_temp;
    card_temp *= p->cardinality[i];
  }

  return &(p->data[index]);

}


/*
 * Drops the indices that are marginalised or multiplied. 
 * dest_indices[] must have the same size as mapping[] and smaller than 
 * source_indices[].
 * Returns an error code.
 */
static void choose_indices(int source_indices[], int dest_indices[], 
			   int mapping[], int size_of_mapping){
  int i;

  /* Warning: Write Only Code (TM) */
  for(i = 0; i < size_of_mapping; i++)
    dest_indices[i] = source_indices[mapping[i]];

  return;
}


potential make_potential(int cardinality[], int num_of_vars, double data[]){

  /* JJ NOTE: what if num_of_vars = 0 i.e. size_of_data = 1 ???
     (this can happen with sepsets...) */

  int i;
  int size_of_data = 1;
  int *cardinal;
  double *dpointer = NULL;
  potential p = (potential) malloc(sizeof(ptype));

  if(!p){
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    return NULL;
  }  

  if(num_of_vars){
    cardinal = (int *) calloc(num_of_vars, sizeof(int));
    if(!cardinal){
      report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
      free(p);
      return NULL;
    }  
  }
  else
    cardinal = NULL;

  for(i = 0; i < num_of_vars; i++){
    size_of_data *= cardinality[i];
    cardinal[i] = cardinality[i];
  }

  p->cardinality = cardinal;
  p->size_of_data = size_of_data;
  p->num_of_vars = num_of_vars;
  p->data = (double *) calloc(size_of_data, sizeof(double));
  
  if(!(p->data)){
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    free_potential(p);
    return NULL;
  }

  if(data == NULL){
    /* The array has to be initialised. Let's do it right away. */
    for(dpointer=p->data, i=0; i < size_of_data; *dpointer++ = 1, i++);
  }
  else{
    /* Just copy the contents of the array */
    for(i=0; i < size_of_data; i++)
      p->data[i] = data[i];
  }

  return p;
}


void free_potential(potential p){
  if(p){
    free(p->cardinality);
    free(p->data);
    free(p);
  }
  return;
}


double get_pvalue(potential p, int indices[]){
  double *ppointer = get_ppointer(p, indices);
  if(ppointer != NULL)
    return *ppointer;
  else{
    report_error(__FILE__, __LINE__, ERROR_NULLPOINTER, 1);
    return -1;
  }
}


void set_pvalue(potential p, int indices[], double value){
  double *ppointer = get_ppointer(p, indices);
  if(ppointer != NULL)
    *ppointer = value;
  return;
}


void inverse_mapping(potential p, int flat_index, int indices[]){

  int x = p->size_of_data;
  int i;

  /* NOTE: the first variable (a.k.a the 0th variable) is 
     'least significant' in the sense that the value of it 
     has the smallest effect on the memory address */
  for(i = p->num_of_vars - 1; i >= 0; i--){
    x /= p->cardinality[i];
    indices[i] = flat_index / x;    /* integer division */
    flat_index -= indices[i] * x;
  }

  return;
}


int general_marginalise(potential source, potential destination, 
			int mapping[]){

  int i;
  int *source_indices = NULL; 
  int *dest_indices = NULL;
  double *potvalue;

  /* index arrays  (eg. [5][4][3] <-> { 5, 4, 3 }) */

  if(destination->num_of_vars){
    dest_indices = (int *) calloc(destination->num_of_vars, sizeof(int));
    
    if(!dest_indices){
      report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
      return ERROR_OUTOFMEMORY;
    }
  }
  else{ /* the rare event of potential being scalar */
    destination->data[0] = 0;

    for(i = 0; i < source->size_of_data; i++)
      destination->data[0] += source->data[i];
    return NO_ERROR;
  }

  /* source->num_of_vars > 0 always */
  source_indices = (int *) calloc(source->num_of_vars, sizeof(int));
  
  if(!source_indices){
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    free(dest_indices);
    return ERROR_OUTOFMEMORY;
  }

  /* Remove old garbage */
  for(i = 0; i < destination->size_of_data; i++)
    destination->data[i] = 0;

  /* Linear traverse through array for easy access */
  for(i = 0; i < source->size_of_data; i++){

    /* flat index i  ->  index array  */
    inverse_mapping(source, i, source_indices);

    /* remove extra indices, eg. if mapping = { 0, 2, 4 }, then
       source_indices { 2, 6, 7, 5, 3 } becomes dest_indices { 2, 7, 3 }*/
    choose_indices(source_indices, dest_indices, mapping,
		   destination->num_of_vars);

    /* get pointer to the destination potential element where the current
     data should be added */
    potvalue = get_ppointer(destination, dest_indices);
    *potvalue += source->data[i];
  }

  free(source_indices);
  free(dest_indices); /* is allocating and freeing slow??? */

  /* JJ NOTE: WHAT IF EACH POTENTIAL HAD A SMALL ARRAY CALLED temp_indices ??? 
     The space is needed anyway in general_marginalise() and 
     update_potential()... */

  return NO_ERROR;
}


int total_marginalise(potential source, double destination[], int variable){
  int i, j, x, index = 0, flat_index;
  int *source_indices;

  /* index arrays  (eg. [5][4][3] <-> { 5, 4, 3 }) 
                         |  |  |
     variable index:     0  1  2... (or 'significance') */
  if(source->num_of_vars){
    source_indices = (int *) calloc(source->num_of_vars, sizeof(int));
    
    if(!source_indices){
      report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);    
      return ERROR_OUTOFMEMORY;
    }
  }
  else{
    destination[0] = source->data[0];
    return NO_ERROR;
  }

  /* initialization */
  for(i = 0; i < source->cardinality[variable]; i++)
    destination[i] = 0;

  for(i = 0; i < source->size_of_data; i++){
    /* partial inverse mapping to find out the destination index */
    flat_index = i;
    x = source->size_of_data;
    for(j = source->num_of_vars - 1; j >= variable; j--){
      x /= source->cardinality[j];
      index = flat_index / x;    /* integer division */
      flat_index -= index * x;
    }
    /* THE sum */
    destination[index] += source->data[i]; 
  }
  free(source_indices);
  return NO_ERROR;
}


int update_potential(potential numerator, potential denominator, 
		     potential target, int mapping[]){
  int i;
  int *source_indices = NULL; 
  int *target_indices = NULL;
  int nvars = 0;
  double *potvalue;

  if((numerator && denominator && 
      (numerator->num_of_vars != denominator->num_of_vars)) || 
     (numerator == NULL && denominator == NULL)){
    /* I hope the logic behind &&-evaluation is "fail fast" */
    report_error(__FILE__, __LINE__, ERROR_INVALID_ARGUMENT, 1);
    return ERROR_INVALID_ARGUMENT;
  }

  if(numerator)
    nvars = numerator->num_of_vars;
  else
    nvars = denominator->num_of_vars;

  if(nvars){
    source_indices = (int *) calloc(nvars, sizeof(int));
    
    if(!source_indices){
      report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
      return ERROR_OUTOFMEMORY;
    }
  }
  else{ /* when numerator & denominator are scalar */
    for(i = 0; i < target->size_of_data; i++){
      if(numerator)
	target->data[i] *= numerator->data[0];

      if(denominator){
	if(denominator->data[0])
	  target->data[i] /= denominator->data[0];
	else
	  target->data[i] = 0;
      }
    }
    return NO_ERROR;
  }

  target_indices = (int *) calloc(target->num_of_vars, sizeof(int));

  if(!target_indices){
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    free(source_indices);
    return ERROR_OUTOFMEMORY;
  }

  /* The general idea is the same as in marginalise */
  for(i = 0; i < target->size_of_data; i++){
    inverse_mapping(target, i, target_indices);
    choose_indices(target_indices, source_indices, mapping, nvars);

    if(numerator){ /* THE multiplication */
      potvalue = get_ppointer(numerator, source_indices);
      target->data[i] *= *potvalue;
    }

    if(denominator){ /* THE division */
      potvalue = get_ppointer(denominator, source_indices);
      if(*potvalue != 0)
	target->data[i] /= *potvalue;
      else
	target->data[i] = 0;  /* see Procedural Guide p. 20 */
    }
  }

  free(source_indices); /* JJ NOTE: GET RID OF THESE */
  free(target_indices);

  return NO_ERROR;

}

int update_evidence(double numerator[], double denominator[], 
		    potential target, int var){

  int i, source_index;
  int *target_indices;

  /* target->num_of_vars > 0  always */
  target_indices = (int *) calloc(target->num_of_vars, sizeof(int));

  if(!target_indices){
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    return ERROR_OUTOFMEMORY;
  }

  /* The general idea is the same as in marginalise */
  for(i = 0; i < target->size_of_data; i++){
    inverse_mapping(target, i, target_indices); 
    /* some of the work above is useless (only one index is used) */
    source_index = target_indices[var];

    target->data[i] *= numerator[source_index];  /* THE multiplication */

    if(denominator != NULL)
      if(denominator[source_index] != 0){

	target->data[i] /= denominator[source_index];  /* THE division */
      }
    /* ----------------------------------------------------------- */
    /* It is assumed that: denominator[i]==0 => numerator[i]==0 !!!*/
    /* ----------------------------------------------------------- */
  }

  free(target_indices);   /* JJ NOTE: GET RID OF THESE */

  return NO_ERROR;

}

int init_potential(potential probs, potential target, int mapping[]){

  /* probs is assumed to be normalised */

  int i;
  int *probs_indices = NULL; 
  int *target_indices = NULL;
  double *potvalue;

  if(probs->num_of_vars){
    probs_indices = (int *) calloc(probs->num_of_vars, sizeof(int));
    if(!probs_indices){
      report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
      return ERROR_OUTOFMEMORY;
    }
  }
  else /* probs is a scalar & normalised => probs == 1 */
    return NO_ERROR;

  target_indices = (int *) calloc(target->num_of_vars, sizeof(int));

  if(!target_indices){
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    free(probs_indices);
    return ERROR_OUTOFMEMORY;
  }

  /* The general idea is the same as in marginalise */
  if(probs->num_of_vars == target->num_of_vars)
    for(i = 0; i < target->size_of_data; i++) /* similar kind of potentials */
      target->data[i] *= probs->data[i];
  else
    for(i = 0; i < target->size_of_data; i++){
      inverse_mapping(target, i, target_indices);
      choose_indices(target_indices, probs_indices, 
		     mapping, probs->num_of_vars);
      
      potvalue =
	get_ppointer(probs, probs_indices);
      target->data[i] *= *potvalue;  /* THE multiplication */
    }
  
  free(probs_indices); /* JJ NOTE: GET RID OF THESE? */
  free(target_indices);

  return NO_ERROR;

}

void print_potential(potential p){

  int big_index, i;
  int *indices = NULL;

  if(p->num_of_vars){
    indices = (int *) calloc(p->num_of_vars, sizeof(int));
    if(!indices){
      report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
      return;
    }
  }
  else{
    printf("P(0) = %f\n", p->data[0]);
    return;
  }

  if(!indices)
    return;

  for(big_index = 0; big_index < p->size_of_data; big_index++){
    inverse_mapping(p, big_index, indices);
    printf("P(");
    for(i = 0; i < p->num_of_vars; i++){
      printf("%d", indices[i]);
      if(i != p->num_of_vars - 1)
	printf(", ");
    }
    printf(") = %f\n", p->data[big_index]);
    
  }

  free(indices);

}
