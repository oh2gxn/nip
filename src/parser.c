/* Functions for the bison parser.
 * $Id: parser.c,v 1.8 2004-05-14 14:13:55 mvkorpel Exp $
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
      return ERROR_GENERAL; /* fopen(...) failed */
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

  int nexttoken_length;

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

      /* Check if tokenise failed. If it failed, we have no other option
       * than to stop: return NULL, *token_length = 0.
       */
      if(!indexarray){
	report_error(ERROR_GENERAL, 0);
	*token_length = 0;
	return NULL;
      }

      /* Ignore lines that have COMMENT_CHAR as first non-whitespace char */
      if(last_line[indexarray[0]] == COMMENT_CHAR)
	read_line = 1;
      else
	read_line = 0;
    }
  }

  *token_length = indexarray[1] - indexarray[0];
  token = (char *) calloc(*token_length + 1, sizeof(char));
  if(!token){
    report_error(ERROR_OUTOFMEMORY, 0);
    *token_length = -1;
    return NULL;
  }

  /* Copy the token */
  strncpy(token, &(last_line[indexarray[0]]), *token_length);

  /* Null terminate the token. */
  token[*token_length] = '\0';

  indexarray += 2;
  
  /* If all the tokens have been handled, read a new line next time */
  if(--tokens_left == 0)
    read_line = 1;

  /* Still some tokens left. Check for COMMENT_CHAR. */
  else if(last_line[indexarray[0]] == COMMENT_CHAR)
    read_line = 1;

  return token;
}

/* correctness? */
int add_pvar(Variable var){
  varlink new = (varlink) malloc(sizeof(varelement));
  new->data = var;
  new->fwd = 0;
  new->bwd = last_double;
  if(first_var = 0)
    first_var = new;
  last_var = new;
  vars_parsed++;
  return 0;
}

/* correctness? */
int add_double(double d){
  doublelink new = (doublelink) malloc(sizeof(doubleelement));
  new->data = d;
  new->fwd = 0;
  new->bwd = last_double;
  if(first_double = 0)
    first_double = new;
  last_double = new;
  doubles_parsed++;
  return 0;
}

/* correctness? */
int add_string(char* string){
  stringlink new = (stringlink) malloc(sizeof(stringelement));
  new->data = string;
  new->fwd = 0;
  new->bwd = last_string;
  if(first_string = 0)
    first_string = new;
  last_string = new;
  doubles_parsed++;
  return 0;
}

/* Creates an array from the double values in the list. 
 * The size will be doubles_parsed. */
double* make_double_array(){
  double* new = (double*) calloc(doubles_parsed, sizeof(double));
  int i;
  doublelink ln = first_double;
  for(i = 0; i < doubles_parsed; i++){
    new[i] = ln->data;
    ln = ln->fwd;
  }
  return new;
}

/* Creates an array from the strings in the list. 
 * The size will be strings_parsed. */
char** make_string_array(){
  char** new = (char**) calloc(strings_parsed, sizeof(char*));
  int i;
  stringlink ln = first_string;
  for(i = 0; i < strings_parsed; i++){
    new[i] = ln->data;
    ln = ln->fwd;
  }
  return new;
}

/* Removes everything from the list of doubles. This is likely to be used 
 * after the parser has parsed doubles to the list, created an array out 
 * of it and wants to reset the list for future use. 
 * JJT: DO NOT TOUCH THE ACTUAL DATA, OR IT WILL BE LOST. */
int reset_doubles(){
  doublelink ln = last_double;
  last_double = 0;
  while(ln != 0){
    ln = ln->bwd;
    free(ln->fwd); /* correct? */
  }
  first_double = 0;
  doubles_parsed = 0;
}

/* Removes everything from the list of strings. */
int reset_strings(){
  stringlink ln = last_string;
  last_string = 0;
  while(ln != 0){
    ln = ln->bwd;
    free(ln->fwd); /* correct? */
  }
  first_string = 0;
  strings_parsed = 0;  
}
