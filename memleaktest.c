#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
/* include some more */
#include "nip.h"

int main(int argc, char *argv[]){
  
  int i, n;
  Nip model;
  n = atoi(argv[2]);

  printf("Allocating and freeing potentials:\n");
  for(i = 0; i < n; i++){
    /* Create a potential */
    printf("\rIteration %d of %d                               ", i + 1, n);
    /* Free the potential */
  }


  printf("\nAllocating and freeing Cliques:\n");
  for(i = 0; i < n; i++){
    /* Create two cliques and a sepset */
    printf("\rIteration %d of %d                               ", i + 1, n);
    /* Free the cliques and the sepset */
  }


  /* TODO: Test the free_graph function */


  printf("\nParsing and freeing models:\n");
  for(i = 0; i < n; i++){
    model = parse_model(argv[1]);
    printf("\rIteration %d of %d                               ", i + 1, n);
    free_model(model);
  }
  printf("\n");

  return 0;
}
