#include "parser.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]){

  int i, j;
  int num_of_tokens;
  datafile *file;
  char **tokens;

  if(argc < 2){
    printf("Filename must be given!\n");
    return -1;
  }
  else if((file = open_datafile(argv[1], ',', 0, 1)) == NULL){
    printf("Problems opening file %s\n", argv[1]);
    return -1;
  }

  printf("Information about the datafile:\n");
  printf("\tName: %s\n", file->name);
  printf("\tSeparator: %c\n", file->separator);
  printf("\tis_open: %d\n", file->is_open);
  printf("\tNumber of data rows: %d\n", file->datarows);
  printf("\tNumber of nodes: %d\n", file->num_of_nodes);
  printf("Nodes:\n");
  for(i = 0; i < file->num_of_nodes; i++){
    printf("\tNode %s, states:\n\t", file->node_symbols[i]);
    for(j = 0; j < file->num_of_states[i]; j++)
      printf("%s ", file->node_states[i][j]);
    printf("\n");
  }

  printf("\nReading tokens one line at a time.\n");

  while((num_of_tokens = nextline_tokens(file, ',', &tokens)) >= 0){

    for(i = 0; i < num_of_tokens; i++){
      printf("%s ", tokens[i]);
      free(tokens[i]);
    }
    printf("\n");
    free(tokens);
  }

  close_datafile(file);

  return 0;
}
