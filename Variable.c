#include <string.h>
#include "Variable.h"

Variable new_variable(char* name, int cardinality) {
  int i = 0;
  static long id = 0;
  Variable v;
  v->cardinality = cardinality;
  v->id = ++id;
 
  strncpy(v->name, name, VAR_NAME_LENGTH);
  v->name[VAR_NAME_LENGTH] = '\0'; 
 }

int equal(Variable v1, Variable v2){
  return (v1.id == v2.id);
}

unsigned long get_id(Variable v){
  return v.id;
}
