#include "parser.h"

int yyparse();

int main(int argc, char *argv[]){

  int retval;

  if(argc < 2){
    if(open_infile("infile") != 0)
      return -1;
  }
  else if(open_infile(argv[1]) != 0)
    return -1;

  retval = yyparse();

  close_infile();
  return retval;
}
