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

/* method for copying a potential */
void copy_potential(potential source, potential destination);

/* method for marginalising over certain variables */
void marginalise(potential source, potential destination, int source_vars[], 
		 int num_of_vars);
/* WILL CHANGE */

/* method for multiplying potentials */
void mul_potential(potential factor, potential target);

#endif __POTENTIAL_H__
