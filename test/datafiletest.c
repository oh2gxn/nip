#include "parser.h"
#include <stdio.h>

int main(int argc, char *argv[]){

  datafile *file;

  if(argc < 2){
    printf("Filename must be given!\n");
    return -1;
  }
  else if((file = open_datafile(argv[1], ',', 0, 1)) == NULL){
    printf("Problems opening file %s\n", argv[1]);
    return -1;
  }


  close_datafile(file);

  return 0;
}
