/* Tässä on vielä jotain säätöä. */

#include <stdio.h>
#include <stdlib.h>
#include "potential.h" 

potential make_potential(int[], int);
void free_potential(potential);
void copy_potential(potential, potential);
double get_pvalue(potential, int[], int);
void set_pvalue(potential, int[], int, double);
double *get_ppointer(potential, int[], int);
void marginalise(potential, potential, int[]);
void update(potential, potential, potential, int[]);
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

/* Free the memory used by potential p */
void free_potential(potential p){

  free(p->cardinality);
  free(p->data);
  free(p);

}

/* Make a copy of a potential. 
 source and destination must be potentials of same cardinality! */
/* TARVITAANKO? */
void copy_potential(potential source, potential destination){

  int i;
  for(i = 0; i < source->size_of_data; i++)
    destination->data[i] = source->data[i];

}

/* Syntactic sugar */
double get_pvalue(potential p, int indices[], int num_of_vars){
  double *ppointer = get_ppointer(p, indices, num_of_vars);
  if(ppointer != NULL)
    return *ppointer;
  else
    return -1;
}

/* Syntactic sugar */
void set_pvalue(potential p, int indices[], int num_of_vars, double value){
  double *ppointer = get_ppointer(p, indices, num_of_vars);
  if(ppointer != NULL)
    *ppointer = value;
}

/* Returns a pointer to the potential with given variable values (indices).
   num_of_vars must be equal to the size of indices[] */
double *get_ppointer(potential p, int indices[], int num_of_vars){

  /* JJ NOTE: num_of_vars can be found in potential! */

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
   variables in potential p. */
void inverse_mapping(potential p, int big_index, int indices[]){

  int x = p->size_of_data;
  int i;

  for(i = p->num_of_vars - 1; i >= 0; i--){
    x /= p->cardinality[i];
    indices[i] = big_index / x;    /* integer division */
    big_index -= indices[i] * x;
  }

  return;
}

/* Drops the indices that are marginalised or multiplied. 
   source_vars must be in ascending order (see marginalise(...). 
   dest_indices[] must be of right size */
void choose_indices(potential source, int source_indices[],
		    int dest_indices[], int source_vars[]){

  int i, j = 0, k = 0;
  for(i = 0; i < source->num_of_vars; i++){    
    if(i == source_vars[j])
      j++;
    else{
      dest_indices[k] = source_indices[i];
      k++;
    }
  } 
  return;
}

/* Method for marginalising over certain variables. Useful in message passing
   from clique to sepset. It is best that sepsets have two static potentials 
   which take turns as the old and the new potential.
   TAKE CARE OF THE ORDER OF VARIABLES! 
-source: potential to be marginalised
-destination: potential to put the answer, variables will be in the same order
-source_vars: indices of the marginalised variables in source potential
             (ascending order and between 0...num_of_vars-1 inclusive!) 
EXAMPLE: If sepset variables are the second (i.e. 1) and third (i.e. 2) 
variable in a five variable clique, the call is 
marginalise(cliquePotential, newSepsetPotential, {0, 3, 4}) */
void marginalise(potential source, potential destination, int source_vars[]){

  int i;
  int *source_indices, *dest_indices;
  double *potvalue;

  source_indices = (int *) calloc(source->num_of_vars, sizeof(int));
  dest_indices = (int *) calloc(destination->num_of_vars, sizeof(int));

  for(i = 0; i < destination->size_of_data; i++)
    destination->data[i] = 0;

  for(i = 0; i < source->size_of_data; i++){
    inverse_mapping(source, i, source_indices);
    choose_indices(source, source_indices, dest_indices, source_vars);
    potvalue =
      get_ppointer(destination, dest_indices, destination->num_of_vars);
    *potvalue += source->data[i];
  }

  free(source_indices);
  free(dest_indices); /* is allocating and freeing slow??? */

  /* JJ NOTE: WHAT IF EACH POTENTIAL HAD A SMALL ARRAY CALLED temp_indices ??? 
     The space is needed anyway in marginalise() and update()... */

  return;
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
update(newSepsetPotential, oldSepsetPotential, cliquePotential, {0, 1, 3}) 
*/
void update(potential enumerator, potential denominator, potential target,
	    int extra_vars[]){

  int i;
  int *source_indices, *target_indices;
  double *potvalue;

  source_indices = (int *) calloc(enumerator->num_of_vars, sizeof(int));
  target_indices = (int *) calloc(target->num_of_vars, sizeof(int));

  for(i = 0; i < target->size_of_data; i++){
    inverse_mapping(target, i, target_indices);
    choose_indices(target, target_indices, source_indices, extra_vars);

    potvalue =
      get_ppointer(enumerator, source_indices, enumerator->num_of_vars);
    target->data[i] *= *potvalue;  /* THE multiplication */

    potvalue = 
      get_ppointer(denominator, source_indices, denominator->num_of_vars);
    if(*potvalue == 0)
      target->data[i] = 0;  /* see Procedural Guide p. 20 */
    else
      target->data[i] /= *potvalue;  /* THE division */
  }

  free(source_indices); /* JJ NOTE: GET RID OF THESE */
  free(target_indices);

  return;  

}






