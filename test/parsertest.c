#include "parser.h"
#include <stdio.h>
#include "errorhandler.h"

int main(int argc, char *argv[]){

  int token_length;
  char *token;

  if(argc < 2){
    if(open_infile("infile") != 0)
      return -1;
  }
  else if(open_infile(argv[1]) != 0)
    return -1;

  while(1){
    token = next_token(&token_length);

    if(token_length == 0)
      break; /* no more tokens */

    /* Printataan "token" ruudulle. */
    printf("%s\n", token);
    
  }


  close_infile();

  return 0;
}
