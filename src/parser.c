/* Functions for the bison parser.
 * $Id: parser.c,v 1.4 2004-03-22 14:16:07 mvkorpel Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"
#include "fileio.h"
#include "errorhandler.h"

int open_infile(const char *file){
  if(!file_open){
    parser_infile = fopen(file,"r");
    if (!parser_infile)
      return 1; /* fopen(...) failed */
    else
      file_open = 1;
  }
  return 0;
}

void close_infile(){
  if(file_open){
    fclose(parser_infile);
    file_open = 0;
  }
}

char *next_token(int *token_length){

  /* The last line read from the file */
  static char last_line[MAX_LINELENGTH];

  /* How many tokens left in the current line of input? */
  static int tokens_left;

  /* Pointer to the index array of token boundaries */
  static int *indexarray;

  /* Should a new line be read in when the next token is requested? */
  static int read_line = 1;

  /* The token we return */
  char *token;

  int tokenlength;

  /* Return if some evil mastermind gives us a NULL pointer */
  if(!token_length)
    return NULL;

  /* Return if input file is not open */
  if(!file_open)
    return NULL;

  /* Read new line if needed and do other magic... */
  while(read_line){
    /* Read the line and check for EOF */
    if(!(fgets(last_line, MAX_LINELENGTH, parser_infile))){
      *token_length = 0;
      return NULL;
    }
    /* How many tokens in the line? */
    tokens_left = count_tokens(last_line, NULL);

    /* Check whether the line has any tokens. If not, read a new line
     * (go to beginning of loop) */
    if(tokens_left > 0){
      /* Adjust pointer to the beginning of token boundary index array */
      indexarray = tokenise(last_line, tokens_left, 1);

      read_line = 0;
    }
  }

  tokenlength = indexarray[1] - indexarray[0];
  token = (char *) calloc(tokenlength, sizeof(char));
  if(!token){
    report_error(ERROR_OUTOFMEMORY, 0);
    *token_length = -1;
    return NULL;
  }

  /* Copy the token */
  strncpy(token, &(last_line[indexarray[0]]), tokenlength);

  indexarray += 2;
  
  /* If all the tokens have been handled, read a new line next time */
  if(--tokens_left == 0)
    read_line = 1;

  return token;
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
