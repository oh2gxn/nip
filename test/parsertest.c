#include "parser.h"
#include "huginnet.tab.h"
#include <stdio.h>

extern FILE *open_net_file(const char *filename);
extern void close_net_file();

/* Tries out the Huginnet parser stuff. */
int main(int argc, char *argv[]){

  int token_length;
  int ok = 1;
  char *token;
  FILE *f = NULL;

  if(argc < 2){
    printf("Filename must be given!\n");
    return -1;
  }
  else if((f = open_net_file(argv[1])) == NULL)
    return -1;

  while(ok){
    token = next_token(&token_length, f);

    if(token_length == 0)
      ok = 0; /* no more tokens */

    /* Printataan "token" ruudulle. */
    if(ok)
      printf("%s\n", token);
    
  }


  close_net_file();

  return 0;
}
