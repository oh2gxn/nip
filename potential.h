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

/* method for marginalising over certain variables */
void marginalise(potential source, potential destination, int source_vars[]);

/* Method for updating target potential by multiplying with enumerator 
   potentials and dividing with denominator potentials. Useful in message 
   passing from sepset to clique. 
-target: the potential whose values are updated
-enumerator: multiplier, usually the newer sepset potential
-denominator: divider, usually the older sepset potential */
void update(potential enumerator, potential denominator, potential target);

#endif
