/* Functions for the bison parser.
 * $Id: parser.c,v 1.21 2004-06-07 11:38:01 mvkorpel Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"
#include "fileio.h"
#include "errorhandler.h"

varlink nip_first_var = NULL; /* global stuff, sad but true */
varlink nip_last_var = NULL;
int nip_vars_parsed = 0;

varlink nip_first_temp_var = NULL;
varlink nip_last_temp_var = NULL;
int nip_symbols_parsed = 0;

Graph *nip_graph = NULL;
Clique *nip_cliques = NULL;
int nip_num_of_cliques = 0;

doublelink nip_first_double = NULL;
doublelink nip_last_double = NULL;
int nip_doubles_parsed = 0;

stringlink nip_first_string = NULL;
stringlink nip_last_string = NULL;
int nip_strings_parsed = 0;

initDataLink nip_first_initData = NULL;
initDataLink nip_last_initData = NULL;
int nip_initData_parsed = 0;

/* The current input file */
FILE *nip_parser_infile = NULL;

/* Is there a file open? 0 if no, 1 if yes. */
int nip_file_open = 0;

#define DEBUG_PARSER

int open_infile(const char *file){
  if(!nip_file_open){
    nip_parser_infile = fopen(file,"r");
    if (!nip_parser_infile)
      return ERROR_GENERAL; /* fopen(...) failed */
    else
      nip_file_open = 1;
  }
  return 0;
}

void close_infile(){
  if(nip_file_open){
    fclose(nip_parser_infile);
    nip_file_open = 0;
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

  /* Return if some evil mastermind gives us a NULL pointer */
  if(!token_length)
    return NULL;

  /* Return if input file is not open */
  if(!nip_file_open)
    return NULL;

  /* Read new line if needed and do other magic... */
  while(read_line){
    /* Read the line and check for EOF */
    if(!(fgets(last_line, MAX_LINELENGTH, nip_parser_infile))){
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

  /* NULL terminate the token. */
  token[*token_length] = '\0';

  indexarray += 2;
  
  /* If all the tokens have been handled, read a new line next time */
  if(--tokens_left == 0)
    read_line = 1;

  /* Still some tokens left. Check for COMMENT_CHAR. */
  else if(last_line[indexarray[0]] == COMMENT_CHAR)
    read_line = 1;

#ifdef DEBUG_PARSER
  printf("%s\n", token);
#endif

  return token;
}


/* Adds a variable into a temporary list for creating an array. 
 * The variable is chosen from THE list of variables 
 * according to the given symbol. */
int add_symbol(Variable v){
  varlink new = (varlink) malloc(sizeof(varelement));

  if(v == NULL)
    return ERROR_INVALID_ARGUMENT;

  new->data = v;
  new->fwd = 0;
  new->bwd = nip_last_temp_var;

  if(nip_first_temp_var == NULL)
    nip_first_temp_var = new;
  else
    nip_last_temp_var->fwd = new;
    
  nip_last_temp_var = new;
  nip_symbols_parsed++;

  return 0;
}


/* Gets the parsed variable according to the symbol. */
Variable get_variable(char *symbol){

  varlink pointer = nip_first_var;

#ifdef DEBUG_PARSER
  printf("In get_variable: looking for \"%s\"\n", symbol);
#endif

  if(pointer == NULL)
    return NULL; /* didn't find the variable */
  
  /* search for the variable reference */
  while(strcmp(symbol, pointer->data->symbol) != 0){
    pointer = pointer->fwd;
    if(pointer == NULL){
      return NULL; /* didn't find the variable */
    }
  }
  return pointer->data;
}


/* correctness? */
int add_initData(potential p, Variable child, Variable* parents){
  initDataLink new = (initDataLink) malloc(sizeof(initDataElement));
  new->data = p;
  new->child = child;
  new->parents = parents;
  new->fwd = 0;
  new->bwd = nip_last_initData;
  if(nip_first_initData == NULL)
    nip_first_initData = new;
  else
    nip_last_initData->fwd = new;

  nip_last_initData = new;

  nip_initData_parsed++;
  return 0;
}


/* correctness? */
int add_pvar(Variable var){
  varlink new = (varlink) malloc(sizeof(varelement));
  new->data = var;
  new->fwd = 0;
  new->bwd = nip_last_var;
  if(nip_first_var == NULL)
    nip_first_var = new;
  else
    nip_last_var->fwd = new;

  nip_last_var = new;
  nip_vars_parsed++;

  return 0;
}


/* correctness? */
int add_double(double d){
  doublelink new = (doublelink) malloc(sizeof(doubleelement));
  new->data = d;
  new->fwd = 0;
  new->bwd = nip_last_double;
  if(nip_first_double == NULL)
    nip_first_double = new;
  else
    nip_last_double->fwd = new;

  nip_last_double = new;
  nip_doubles_parsed++;
  return 0;
}


/* correctness? */
int add_string(char* string){
  stringlink new = (stringlink) malloc(sizeof(stringelement));
  new->data = string;
  new->fwd = 0;
  new->bwd = nip_last_string;
  if(nip_first_string == NULL)
    nip_first_string = new;
  else
    nip_last_string->fwd = new;

  nip_last_string = new;
  nip_strings_parsed++;

  return 0;
}


/* Creates an array from the double values in the list. 
 * The size will be doubles_parsed. */
Variable* make_variable_array(){
  int i;
  Variable* vars1 = (Variable*) calloc(nip_symbols_parsed, sizeof(Variable));
  varlink pointer = nip_first_temp_var;
  for(i = 0; i < nip_symbols_parsed; i++){
    vars1[i] = pointer->data;
    pointer = pointer->fwd;
  }
  return vars1;
}


/* Creates an array from the double values in the list. 
 * The size will be doubles_parsed. */
double* make_double_array(){
  double* new = (double*) calloc(nip_doubles_parsed, sizeof(double));
  /* free() is at ? */
  int i;
  doublelink ln = nip_first_double;
  for(i = 0; i < nip_doubles_parsed; i++){
    new[i] = ln->data; /* the data is copied here (=> not lost in reset) */
    ln = ln->fwd;
  }
  return new;
}


/* Creates an array from the strings in the list. 
 * The size will be strings_parsed. */
char** make_string_array(){
  char** new = (char**) calloc(nip_strings_parsed, sizeof(char*));
  /* free() is probably at free_variable() */
  int i;
  stringlink ln = nip_first_string;
  for(i = 0; i < nip_strings_parsed; i++){
    new[i] = ln->data; /* char[] references copied */
    ln = ln->fwd;
  }
  return new;
}


/* Removes everything from the list of doubles. This is likely to be used 
 * after the parser has parsed doubles to the list, created an array out 
 * of it and wants to reset the list for future use. 
 */
int reset_doubles(){
  doublelink ln = nip_last_double;
  nip_last_double = NULL;
  while(ln != NULL){
    free(ln->fwd); /* free(NULL) is O.K. at the beginning */
    ln = ln->bwd;
  }
  nip_first_double = NULL;
  nip_doubles_parsed = 0;
  return 0;
}


/* Removes everything from the list of strings and resets the counter. 
 * The actual memory for the strings is not freed, only the list. */
int reset_strings(){
  stringlink ln = nip_last_string;
  nip_last_string = NULL;
  while(ln != NULL){
    free(ln->fwd); /* free(NULL) is O.K. at the beginning */
    ln = ln->bwd;
  }
  nip_first_string = NULL;
  nip_strings_parsed = 0;  
  return 0;
}


/* Removes everything from the temporary list of variables. */
int reset_symbols(){
  varlink ln = nip_last_temp_var;
  nip_last_temp_var = NULL;
  while(ln != NULL){
    free(ln->fwd); /* free(NULL) is O.K. at the beginning */
    ln = ln->bwd;
  }
  nip_first_temp_var = NULL;
  nip_symbols_parsed = 0;
  return 0;
}


/* Frees some memory after parsing. 
 * JJT: DO NOT TOUCH THE ACTUAL DATA, OR IT WILL BE LOST. */
int reset_initData(){
  initDataLink ln = nip_last_initData;
  nip_last_initData = NULL;
  while(ln != NULL){
    free(ln->fwd); /* free(NULL) is O.K. at the beginning */
    /* NOTE: the potential is probably a part of a variable somewhere */
    /* free_potential(ln->data); */
    free(ln->parents); /* calloc is in make_variable_array(); */
    ln = ln->bwd;
  }
  nip_first_initData = NULL;
  nip_initData_parsed = 0;  
  return 0;
}
