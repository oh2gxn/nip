#ifndef __VARIABLE_H__
#define __VARIABLE_H__

#define VAR_NAME_LENGTH 40

typedef struct {
  char name[VAR_NAME_LENGTH + 1];
  int cardinality;
  unsigned long id; /* unique id for every variable */
  double *likelihood; /* likelihood of each value */
} vtype;

/* MVK NOTE: Variable changed to pointer type */

typedef vtype *Variable;

/* Creates a new Variable */
Variable new_variable(char* name, int cardinality);

/* Checks if two variables are the same */
int equal_variables(Variable v1, Variable v2);

/* The order of variables is essential for working with potentials. */
unsigned long get_id(Variable v);

/* Gives v a new likelihood array. The size of the array
   must match v->cardinality */
int update_likelihood(Variable v, double* likelihood);

#endif /* __VARIABLE_H__ */
