/* Program for testing parser utility functions
 * Author: Janne Toivola
 * Version: $Id: parsertest.c,v 1.10 2010-12-07 17:23:19 jatoivol Exp $
 */

#include <stdio.h>
#include "nipparsers.h"
#include "huginnet.tab.h"

FILE *open_net_file(const char *filename);
void close_net_file();

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
    token = nip_next_hugin_token(&token_length, f);

    if(token_length == 0)
      ok = 0; /* no more tokens */

    /* Print each "token" on a new line */
    if(ok)
      printf("%s\n", token);
    
  }


  close_net_file();

  return 0;
}
