/* Tässä on jotain säätöä. Ei ole ihan yhteensopiva potential.h:n kanssa. */

#include <stdio.h>
#include <stdlib.h>

struct array {
  int size;
  int has_data;
  struct array *subarrays;
  double *data;
};

typedef struct array ptype;
typedef ptype *potential;

potential make_potential(int[], int);
double get_pvalue(potential, int[], int);
void set_pvalue(potential, int[], int, double);
double *get_ppointer(potential, int[], int);
potential mk_pot(int, int);
int main();

/* Make a num_of_vars -dimension potential array. */
potential make_potential(int cardinality[], int num_of_vars){

  if(num_of_vars < 1)
    return NULL;

  potential p;
  int i;
  if(num_of_vars == 1)
    p = mk_pot(cardinality[0], 1);
  else{
    p = mk_pot(cardinality[0], 0);
    for(i = 0; i < cardinality[0]; i++)
      p->subarrays[i] = *make_potential(cardinality + 1,
					num_of_vars - 1);
  }
  return p;
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

  int i;

  if(num_of_vars < 1)
    return NULL;

  for(i = 0; i < num_of_vars - 1; i++){
    if(p->has_data)
      return NULL;
    if(indices[i] >= p->size ||
       indices[i] < 0)
      return NULL;
    p = &(p->subarrays[indices[i]]);
  }
  
  if(!(p->has_data))
    return NULL;
  if(indices[num_of_vars - 1] >= p->size ||
     indices[num_of_vars - 1] < 0)
    return NULL;

  return &(p->data[indices[num_of_vars - 1]]);
}

/* This is an internal function for make_potential(...) */
potential mk_pot(int cardinality, int has_data){
  potential p;
  p = (potential) malloc(sizeof(ptype));
  p->size = cardinality;
  if(has_data){
    p->has_data = 1;
    p->data = (double *) calloc(cardinality, sizeof(double));
  }
  else{
    p->has_data = 0;
    p->subarrays = (struct array *) calloc(cardinality, sizeof(struct array));
  }
  return p;
}

/* Main function for testing */
int main(){

  /* Use big numbers here and see that 640K isn't enough for everyone... */
  int cardinality[] = { 3, 4, 5};
  int num_of_vars = 3;
  int indices[3], i, j, k;
  double value;
  potential p;
  p = make_potential(cardinality, num_of_vars);

  /* Set values */
  for(i = 0; i < cardinality[0]; i++){
    indices[0] = i;
    for(j = 0; j < cardinality[1]; j++){
      indices[1] = j;
      for(k = 0; k < cardinality[2]; k++){
	indices[2] = k;
	set_pvalue(p, indices, num_of_vars,i + 500 * j + 2500 * k);
      }
    }
  }

  /* Print values */
  for(i = 0; i < cardinality[0]; i++){
    indices[0] = i;
    for(j = 0; j < cardinality[1]; j++){
      indices[1] = j;
      for(k = 0; k < cardinality[2]; k++){
	indices[2] = k;
	value = get_pvalue(p, indices, num_of_vars);
	printf("Potential (%d, %d, %d) is: %f\n", indices[0],
	       indices[1], indices[2], value);
      }
    }
  }

  return 0;
}
