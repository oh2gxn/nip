/* Tässä on jotain säätöä. Ei ole ihan yhteensopiva potential.h:n kanssa. */

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
void update(potential, potential, potential);
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

/* Drops the indices that are marginalised. source_vars must be in ascending
   order (see marginalise(...). dest_indices[] must be of right size */
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
             (ascending order!) */
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
  return;
}

/* Method for updating target potential by multiplying with enumerator 
   potentials and dividing with denominator potentials. Useful in message 
   passing from sepset to clique. 
-target: the potential whose values are updated
-enumerator: multiplier, usually the newer sepset potential
-denominator: divider, usually the older sepset potential */
void update(potential enumerator, potential denominator, potential target){

  /* a VERY intriguing task: which target variables correspond to the 
     multiplier variables? */

}

/* Main function for testing */
int main(){

  /* Use big numbers here and see that 640K isn't enough for everyone... */
  int cardinality[] = { 2, 3, 4, 5, 6};
  int card2[] = { 2, 3, 4, 5};
  int num_of_vars = 5;
  int margin_var = 4; /* usually an array */
  int indices[5], i, j, k, l, m, x = 0;
  double value;
  potential p, q;
  p = make_potential(cardinality, num_of_vars);
  q = make_potential(card2, num_of_vars - 1);

  /* Set values of p */
  for(i = 0; i < cardinality[0]; i++){
    indices[0] = i;
    for(j = 0; j < cardinality[1]; j++){
      indices[1] = j;
      for(k = 0; k < cardinality[2]; k++){
	indices[2] = k;
	for(l = 0; l < cardinality[3]; l++){
	  indices[3] = l;
	  for(m = 0; m < cardinality[4]; m++){
	    indices[4] = m;
	    x++;
	    set_pvalue(p, indices, num_of_vars, x);
	  }
	}
      }
    }
  }

  /* Print values of p */
  for(i = 0; i < cardinality[0]; i++){
    indices[0] = i;
    for(j = 0; j < cardinality[1]; j++){
      indices[1] = j;
      for(k = 0; k < cardinality[2]; k++){
	indices[2] = k;
	for(l = 0; l < cardinality[3]; l++){
	  indices[3] = l;
	  for(m = 0; m < cardinality[4]; m++){
	    indices[4] = m;
	    value = get_pvalue(p, indices, num_of_vars);
	    printf("Potential (%d, %d, %d, %d, %d) is: %f\n", indices[0],
		   indices[1], indices[2], indices[3], indices[4], value);
	  }
	}
      }
    }
  }

  /* testing inverse mapping */
  for(i = 0; i < p->size_of_data; i++){
    inverse_mapping(p, i, indices);
    printf("Inverse mapping: %d --> (%d, %d, %d, %d, %d)\n", i, indices[0],
	   indices[1], indices[2], indices[3], indices[4]);
  }

  /* marginalise over variable 4 (fifth variable of p) */
  marginalise(p, q, &margin_var);

  /* Print values of q */
  for(i = 0; i < card2[0]; i++){
    indices[0] = i;
    for(j = 0; j < card2[1]; j++){
      indices[1] = j;
      for(k = 0; k < card2[2]; k++){
	indices[2] = k;
	for(l = 0; l < card2[3]; l++){
	  indices[3] = l;
	  value = get_pvalue(q, indices, num_of_vars - 1);
	  printf("Potential (%d, %d, %d, %d) is: %f\n", indices[0],
		 indices[1], indices[2], indices[3], value);
	}
      }
    }
  }

  return 0;
}
