#include "Clique.h"

int main(){
  Variable variables[5];
  char names[5][10];
  int cardinality[] = { 2, 3, 4, 5, 6};
  int i;
  names[0] = "einari";
  names[1] = "jalmari";
  names[2] = "spede";
  names[3] = "jokke";
  names[4] = "raimo";
  for(i = 0; i < 5; i++)
    variables[i] = new_variable(names[i], cardinality[i]);

  /* To be continued */
  return 0;
}
