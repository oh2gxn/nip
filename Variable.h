#ifndef __VARIABLE_H__
#define __VARIABLE_H__

#define VAR_NAME_LENGTH = 40;

typedef struct {
  char name[VAR_NAME_LENGTH + 1];
  int cardinality;
  unsigned long id; /* unique id for every variable */
} Variable;

/* Creates a new Variable */
Variable new_variable(/*XX auki*/);

/* Checks if two variables are the same (by address)*/
int equal(Variable v1, Variable v2);

/* The order of variables is essential for working with potentials. */
unsigned long get_id(Variable v);

#endif __VARIABLE_H__
