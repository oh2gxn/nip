#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include "nip.h"

int main(int argc, char *argv[]){
  
  int i, n;
  Nip model;
  n = atoi(argv[2]);

  for(i = 0; i < n; i++){
    model = parse_model(argv[1]);
    printf("\rIteration %d of %d                               ", i + 1, n);
    free_model(model);
  }
  printf("\n");

  return 0;
}
