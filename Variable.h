/*
 * Variable.h $Id: Variable.h,v 1.16 2004-02-13 14:12:20 mvkorpel Exp $
 */

#ifndef __VARIABLE_H__
#define __VARIABLE_H__

#include "potential.h"
#define VAR_SYMBOL_LENGTH 20
#define VAR_NAME_LENGTH 40

typedef struct {
  char symbol[VAR_SYMBOL_LENGTH + 1];
  char name[VAR_NAME_LENGTH + 1];
  int cardinality;
  unsigned long id; /* unique id for every variable */
  double *likelihood; /* likelihood of each value */
  potential probability; /* probability of variable, given parents */
  /* JJT: potential is mainly for making global retraction easier! */
} vtype;

typedef vtype *Variable;

/* Creates a new Variable */
Variable new_variable(const char* symbol, int cardinality);

/* Gives the Variable a verbose name */
int variable_name(Variable v, const char *name);

/* Function for copying a Variable (if needed). Handle with care.
 * v: the Variable to be copied
 * Returns the copy.
 */
Variable copy_variable(Variable v);

/* Frees the memory used by the Variable v. */
void free_variable(Variable v);

/* Method for testing variable equality. 
 * This may be needed to keep potentials in order. INEQUALITIES ???
 */
int equal_variables(Variable v1, Variable v2);

/* An alternative interface for keeping variables 
 * and thus the potentials in order.
 */
unsigned long get_id(Variable v);

/* Gives v a new likelihood array. The size of the array
 * must match v->cardinality
 */
int update_likelihood(Variable v, double likelihood[]);

/* Returns the number of possible values in the Variable v.
 */
int number_of_values(Variable v);

/* Sets the conditional probability of variable v.
 * If probability is already set, the previous values are freed from memory.
 * This means that the "ownership" of the potential will change.
 * DO NOT USE THE SAME POTENTIAL FOR ANY OTHER VARIABLE!
 */
int set_probability(Variable v, potential p);

#endif /* __VARIABLE_H__ */
