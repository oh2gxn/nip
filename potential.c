/* T‰h‰n saattaa tulla viel‰ lis‰‰ hienouksia. */

#include <stdio.h>
#include <stdlib.h>
#include "potential.h" 

potential make_potential(int[], int);
int free_potential(potential);
int copy_potential(potential, potential);
double get_pvalue(potential, int[], int);
int set_pvalue(potential, int[], int, double);
double *get_ppointer(potential, int[]);
int general_marginalise(potential, potential, int[]);
int update_potential(potential, potential, potential, int[]);
int main();

/* Make a num_of_vars -dimension potential array. */
potential make_potential(int cardinality[], int num_of_vars){

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

  return p;
}

/* Free the memory used by potential p. Returned value is an error code. */
int free_potential(potential p){

  free(p->cardinality);
  free(p->data);
  free(p);
  return 0;
}

/* Make a copy of a potential. 
 Source and destination must be potentials of same cardinality! 
 Returns an error code.
*/
/* TARVITAANKO? */
int copy_potential(potential source, potential destination){

  int i;
  for(i = 0; i < source->size_of_data; i++)
    destination->data[i] = source->data[i];
  return 0;
}

/* Syntactic sugar */
double get_pvalue(potential p, int indices[]){
  double *ppointer = get_ppointer(p, indices);
  if(ppointer != NULL)
    return *ppointer;
  else
    return -1;
}

/* Syntactic sugar and returns an error code */
int set_pvalue(potential p, int indices[], double value){
  double *ppointer = get_ppointer(p, indices);
  if(ppointer != NULL)
    *ppointer = value;
  return 0;
}

/* Returns a pointer to the potential with given variable values (indices). */
double *get_ppointer(potential p, int indices[]){

  /* JJ NOTE: num_of_vars can be found in potential! 
     MVK: fixed this in get_ppointer, get_pvalue, set_pvalue and in
     calls to these functions */

  int index = indices[0];
  int i;
  int card_temp = 1;

  for(i = 1; i < num_of_vars; i++){
    card_temp *= p->cardinality[i-1];
    index += indices[i] * card_temp;
  }

  return &(p->data[index]);

}

/* Mapping from flat index to n-dimensional index, where n is the number of
   variables in potential p. Returns an error code.
*/
int inverse_mapping(potential p, int big_index, int indices[]){

  int x = p->size_of_data;
  int i;

  for(i = p->num_of_vars - 1; i >= 0; i--){
    x /= p->cardinality[i];
    indices[i] = big_index / x;    /* integer division */
    big_index -= indices[i] * x;
  }

  return 0;
}

/* Drops the indices that are marginalised or multiplied. 
   source_vars must be in ascending order (see marginalise(...)). 
   dest_indices[] must be of the right size. Returns an error code.
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

/* Method for marginalising over certain variables. Useful in message passing
   from clique to sepset. It is best that sepsets have two static potentials 
   which take turns as the old and the new potential.
   TAKE CARE OF THE ORDER OF VARIABLES! 
-source: the potential to be marginalised
-destination: the potential to put the answer into, variables will be 
              in the same order
-source_vars: indices of the marginalised variables in source potential
             (ascending order and between 0...num_of_vars-1 inclusive!) 
EXAMPLE: If sepset variables are the second (i.e. 1) and third (i.e. 2) 
variable in a five variable clique, the call is 
marginalise(cliquePotential, newSepsetPotential, {0, 3, 4}) 
-Returns an error code.
*/
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
     The space is needed anyway in general_marginalise() and update()... */

  return 0;
}


/* Method for finding out the probability distribution of a single variable 
   according to a clique potential. This one is marginalisation too, but 
   this one is not generic. The outcome is not normalized. 
-source: the potential to be marginalised
-destination: the double array for the answer
              SIZE OF THE ARRAY MUST BE CORRECT (check it from the variable)
-variable: the index of the variable of interest 
*/
int total_marginalise(potential source, double[] destination, int variable){
  int i, j, x, index;
  int *source_indices;
  double *potvalue;

  /* index arrays  (eg. [5][4][3] <-> { 5, 4, 3 }) 
                         |  |  |
     variable index:     0  1  2 */
  source_indices = (int *) calloc(source->num_of_vars, sizeof(int));

  /* initialization */
  for(i = 0; i < source->cardinality[variable]; i++)
    destination[i] = 0;

  for(i = 0; i < source->size_of_data; i++){
    /* partial inverse mapping to find out the destination index 
       NOT SURE IF THIS WORKS */
    flat_index = i;
    for(j = source->num_of_vars - 1; j >= variable; j--){
      x /= source->cardinality[j];
      index = flat_index / x;    /* integer division */
      flat_index -= index * x;
    }
    destination[index] += source->data[i];
  }
  free(source_indices);
  return 0;
}


/* Method for updating target potential by multiplying with enumerator 
   potential and dividing with denominator potential. Useful in message 
   passing from sepset to clique. 
-target: the potential whose values are updated
-enumerator: multiplier, usually the newer sepset potential (source)
-denominator: divider, usually the older sepset potential. This MUST have 
 similar geometry to enumerator.
-extra_vars: an integer array which holds the target variable indices 
 (0...num_of_vars - 1 inclusive) that are NOT in source potentials and in 
 ascending order. Length of the array must be at least the number of 
 variables in source potentials. 
EXAMPLE: If two sepset variables are the third and fifth variables in 
a five variable clique, the call is 
update_potential(newSepsetPotential, oldSepsetPotential,
                 cliquePotential, {0, 1, 3})
-Returns an error code.
*/
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






