#include <stdio.h>
#include <stdlib.h>
#include "potential.h" 
#include "errorhandler.h"

int choose_indices(potential source, int source_indices[],
		   int dest_indices[], int source_vars[]);

potential make_potential(int cardinality[], int num_of_vars){

  /* JJ NOTE: what if num_of_vars = 0 i.e. size_of_data = 1 ???
     (this can happen in sepsets...) */

  int i;
  int size_of_data = 1;
  int *cardinal;
  potential p;
  p = (potential) malloc(sizeof(ptype));
  cardinal = (int *) calloc(num_of_vars, sizeof(int));

  for(i = 0; i < num_of_vars; i++){
    size_of_data *= cardinality[i];
    cardinal[i] = cardinality[i];
  }

  p->cardinality = cardinal;
  p->size_of_data = size_of_data;
  p->num_of_vars = num_of_vars;
  p->data = (double *) calloc(size_of_data, sizeof(double));
  /* The array has to be initialised. This is probably done somewhere else. */

  return p;
}

int free_potential(potential p){

  free(p->cardinality);
  free(p->data);
  free(p);
  return 0;
}

/* TARVITAANKO? */
int copy_potential(potential source, potential destination){

  int i;
  for(i = 0; i < source->size_of_data; i++)
    destination->data[i] = source->data[i];
  return 0;
}

double get_pvalue(potential p, int indices[]){
  double *ppointer = get_ppointer(p, indices);
  if(ppointer != NULL)
    return *ppointer;
  else
    return -1;
}

int set_pvalue(potential p, int indices[], double value){
  double *ppointer = get_ppointer(p, indices);
  if(ppointer != NULL)
    *ppointer = value;
  return 0;
}

double *get_ppointer(potential p, int indices[]){

  int index = indices[0];
  int i;
  int card_temp = 1;

  for(i = 1; i < p->num_of_vars; i++){
    card_temp *= p->cardinality[i-1];
    index += indices[i] * card_temp;
  }

  return &(p->data[index]);

}

int inverse_mapping(potential p, int big_index, int indices[]){

  int x = p->size_of_data;
  int i;

  /* NOTE: the first variable (a.k.a the 0th variable) is 
     'least significant' in the sense that the value of it 
     has the smallest effect on the memory address */
  for(i = p->num_of_vars - 1; i >= 0; i--){
    x /= p->cardinality[i];
    indices[i] = big_index / x;    /* integer division */
    big_index -= indices[i] * x;
  }

  return 0;
}

/* Drops the indices that are marginalised or multiplied. 
 * source_vars must be in ascending order (see marginalise(...)). 
 * dest_indices[] must be of the right size. Returns an error code.
 */
int choose_indices(potential source, int source_indices[],
		    int dest_indices[], int source_vars[]){

  /* JJ NOTE: What if this is done only once to form some sort of 
   *          mask and then the mask could be more efficient for 
   *          the rest of the calls..? */
  int i, j = 0, k = 0;
  for(i = 0; i < source->num_of_vars; i++){    
    if(i == source_vars[j])
      j++;
    else{
      dest_indices[k] = source_indices[i];
      k++;
    }
  } 
  return 0;
}

int general_marginalise(potential source, potential destination, 
			int source_vars[]){

  int i;
  int *source_indices, *dest_indices;
  double *potvalue;

  /* index arrays  (eg. [5][4][3] <-> { 5, 4, 3 }) */
  source_indices = (int *) calloc(source->num_of_vars, sizeof(int));
  dest_indices = (int *) calloc(destination->num_of_vars, sizeof(int));

  /* Remove old garbage */
  for(i = 0; i < destination->size_of_data; i++)
    destination->data[i] = 0;

  /* Linear traverse through array for easy access */
  for(i = 0; i < source->size_of_data; i++){

    /* flat index i  ->  index array  */
    inverse_mapping(source, i, source_indices);

    /* remove extra indices, eg. if source_vars = { 1, 3 }, then
     source_indices { 2, 6, 7, 5, 3 } becomes dest_indices { 2, 7, 3 }*/
    choose_indices(source, source_indices, dest_indices, source_vars);

    /* get pointer to the destination potential element where the current
     data should be added */
    potvalue =
      get_ppointer(destination, dest_indices);
    *potvalue += source->data[i];
  }

  free(source_indices);
  free(dest_indices); /* is allocating and freeing slow??? */

  /* JJ NOTE: WHAT IF EACH POTENTIAL HAD A SMALL ARRAY CALLED temp_indices ??? 
     The space is needed anyway in general_marginalise() and 
     update_potential()... */

  return 0;
}

int total_marginalise(potential source, double destination[], int variable){
  int i, j, x, index, flat_index;
  int *source_indices;

  /* index arrays  (eg. [5][4][3] <-> { 5, 4, 3 }) 
                         |  |  |
     variable index:     0  1  2 (or 'significance') */
  source_indices = (int *) calloc(source->num_of_vars, sizeof(int));

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
  return 0;
}

int update_potential(potential enumerator, potential denominator, 
		     potential target, int extra_vars[]){

  int i;
  int *source_indices, *target_indices;
  double *potvalue;

  source_indices = (int *) calloc(enumerator->num_of_vars, sizeof(int));
  target_indices = (int *) calloc(target->num_of_vars, sizeof(int));

  /* The general idea is the same as in marginalise */
  for(i = 0; i < target->size_of_data; i++){
    inverse_mapping(target, i, target_indices);
    choose_indices(target, target_indices, source_indices, extra_vars);

    potvalue =
      get_ppointer(enumerator, source_indices);
    target->data[i] *= *potvalue;  /* THE multiplication */

    potvalue = 
      get_ppointer(denominator, source_indices);
    if(*potvalue == 0)
      target->data[i] = 0;  /* see Procedural Guide p. 20 */
    else
      target->data[i] /= *potvalue;  /* THE division */
  }

  free(source_indices); /* JJ NOTE: GET RID OF THESE */
  free(target_indices);

  return 0;

}

int update_evidence(double numerator[], double denominator[], 
		    potential target, int var){

  int i, source_index;
  int *target_indices;

  target_indices = (int *) calloc(target->num_of_vars, sizeof(int));

  /* The general idea is the same as in marginalise */
  for(i = 0; i < target->size_of_data; i++){
    inverse_mapping(target, i, target_indices); 
    /* some of the work above is useless (only one index is used) */
    source_index = target_indices[var];

    target->data[i] *= numerator[source_index];  /* THE multiplication */

    if(denominator[source_index] != 0)
      target->data[i] /= denominator[source_index];  /* THE division */
  }

  free(target_indices);   /* JJ NOTE: GET RID OF THESE */

  return GLOBAL_UPDATE;

}
