/* Functions for the bison parser. $Id: parser.c,v 1.3 2004-03-19 15:09:28 mvkorpel Exp $
 */

#include <stdio.h>
#include "parser.h"

int open_infile(const char *file){
  if(!file_opened){
    parser_infile = fopen(file,"r");
    if (!parser_infile)
      return 1; /* fopen(...) failed */
    else
      file_opened = 1;
  }
  return 0;
}

void close_infile(){
  if(file_opened){
    fclose(parser_infile);
    file_opened = 0;
  }
}

char *next_token(int *token_length){
  /* TO DO... */
  return null;
}

/* correctness? */
void add_pvar(Variable var){
  varlink new = (varlink) malloc(sizeof(varelement));
  new->data = var;
  new->fwd = 0;
  new->bwd = last;
  if(first = 0)
    first = new;
  last = new;
  vars_parsed++;
}
