#ifndef __POTENTIAL_H__
#define __POTENTIAL_H__

struct pot_array {
  int size_of_data;
  int *cardinality;
  int num_of_vars;
  double *data;
};

typedef struct pot_array ptype;
typedef ptype *potential;

/* method for initializing a potential */
potential make_potential(int cardinality[], int num_of_vars);

/* method for freeing the memory taken by potential */
void free_potential(potential p);

/* method for copying a potential */
void copy_potential(potential source, potential destination);

/* gets a value from the potential p */
double get_pvalue(potential p, int indices[], int num_of_vars);

/* sets a value in the potential p */
void set_pvalue(potential p, int indices[], int num_of_vars, double value);

/* Returns a pointer to the potential with given variable values (indices).
   num_of_vars must be equal to the size of indices[] */
double *get_ppointer(potential p, int indices[], int num_of_vars);

/* Mapping from flat index to n-dimensional index, where n is the number of
   variables in potential p. USUALLY NOT NEEDED outside of potential.c */
void inverse_mapping(potential p, int big_index, int indices[]);

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
void marginalise(potential source, potential destination, int source_vars[]);

/* Method for updating target potential by multiplying with enumerator 
   potentials and dividing with denominator potentials. Useful in message 
   passing from sepset to clique. 
-target: the potential whose values are updated
-enumerator: multiplier, usually the newer sepset potential
-denominator: divider, usually the older sepset potential 
-extra_vars: an integer array which holds the target variable ID's 
 that are NOT in source potentials and in ascending order. Length of the 
 array must be at least the number of variables in source potentials */
void update_potential(potential enumerator, potential denominator, 
		      potential target, int extra_vars[]);

#endif