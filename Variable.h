#ifndef __VARIABLE_H__
#define __VARIABLE_H__

#include "Potential.h" /* XX Onkohan tämä viisasta? */

typedef struct {
  Potential p; /*XX Siis tämä */
  char* name;
  int id;
  int cardinality;
} Variable;

Variable new_variable(/*XX auki*/);
/* Creates a new Variable */

int equal(Variable v1, Variable v2);
/* Checks if two variables are the same */

#endif __VARIABLE_H__
