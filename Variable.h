#ifndef __VARIABLE_H__
#define __VARIABLE_H__

#define VAR_NAME_LENGTH 40

typedef struct {
  char name[VAR_NAME_LENGTH + 1];
  int cardinality;
  unsigned long id; /* unique id for every variable */
  double *likelihood; /* likelihood of each value */
} vtype;

typedef vtype *Variable;

/* Creates a new Variable */
Variable new_variable(char* name, int cardinality);

/* Function for copying a Variable (if needed).
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
int update_likelihood(Variable v, double* likelihood);

#endif /* __VARIABLE_H__ */
