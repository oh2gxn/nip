/*
 * Variable.h $Id: Variable.h,v 1.18 2004-03-19 15:25:58 jatoivol Exp $
 */

#ifndef __VARIABLE_H__
#define __VARIABLE_H__

#include "potential.h"
#define VAR_SYMBOL_LENGTH 20
#define VAR_NAME_LENGTH 40
#define VAR_STATENAME_LENGTH 20

typedef struct {
  char symbol[VAR_SYMBOL_LENGTH + 1]; /* short symbol for the node */
  char name[VAR_NAME_LENGTH + 1]; /* label in the Net language*/
  char **statenames; /* a string array with <cardinality> strings */
  int cardinality;
  unsigned long id; /* unique id for every variable */
  double *likelihood; /* likelihood of each value */
  potential probability; /* probability of the variable, given parents */
  /* JJT: potential is mainly for making global retraction easier! */
} vtype;

typedef vtype *Variable;

/* Creates a new Variable:
 * - symbol is a short name e.g. A (= array [A, \0])
 * - name is a more verbose name e.g. "rain" or NULL 
 * - cardinality is the number of states/values the variable has */
Variable new_variable(const char* symbol, const char* name, int cardinality);

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
