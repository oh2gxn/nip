#include <string.h>
#include "Variable.h"

/* Method for creating new variables */
Variable new_variable(char* name, int cardinality) {
  int i = 0;
  static long id = 0;
  Variable v;
  v.cardinality = cardinality;
  v.id = ++id;
 
  strncpy(v.name, name, VAR_NAME_LENGTH);
  v.name[VAR_NAME_LENGTH] = '\0'; 
 }

/* Method for testing variable equality. 
   This may be needed to keep potentials in order. INEQUALITIES ??? */
int equal_variables(Variable v1, Variable v2){
  return (v1.id == v2.id);
}

/* An alternative interface for keeping variables 
   and thus the potentials in order. */
unsigned long get_id(Variable v){
  return v.id;
}
