#ifndef __VARIABLE_H__
#define __VARIABLE_H__

#include "Potential.h" /* XX Onkohan tämä viisasta? */

typedef struct {
  Potential p; /*XX Siis tämä */
  char* name;
} Variable;

Variable new_variable(/*XX auki*/);
/* Creates a new Variable */

#endif __VARIABLE_H__
