#include "parser.h"
#include <stdio.h>
#include "errorhandler.h"

int main(int argc, char *argv[]){

  int token_length;
  int ok = 1;
  char *token;

  if(argc < 2){
    if(open_parser_infile("infile") != 0)
      return -1;
  }
  else if(open_parser_infile(argv[1]) != 0)
    return -1;

  while(ok){
    token = next_token(&token_length);

    if(token_length == 0)
      ok = 0; /* no more tokens */

    /* Printataan "token" ruudulle. */
    if(ok)
      printf("%s\n", token);
    
  }


  close_parser_infile();

  return 0;
}
