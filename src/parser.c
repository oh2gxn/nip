/*
 * Functions for the bison parser.
 * Also contains other functions for handling different files.
 * $Id: parser.c,v 1.70 2004-08-16 14:06:49 jatoivol Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Graph.h"
#include "Clique.h"
#include "Variable.h"
#include "parser.h"
#include "fileio.h"
#include "errorhandler.h"

/*
#define DEBUG_PARSER
*/

/*
#define PRINT_TOKENS
*/

/*
#define DEBUG_DATAFILE
*/

static varlink nip_first_temp_var = NULL;
static varlink nip_last_temp_var = NULL;
static int nip_symbols_parsed = 0;

static Graph *nip_graph = NULL;

static doublelink nip_first_double = NULL;
static doublelink nip_last_double = NULL;
static int nip_doubles_parsed = 0;

static stringlink nip_first_string = NULL;
static stringlink nip_last_string = NULL;
static int nip_strings_parsed = 0;

static initDataLink nip_first_initData = NULL;
static initDataLink nip_last_initData = NULL;
static int nip_initData_parsed = 0;

static time_init_link nip_first_timeinit = NULL;

static char** nip_statenames;
static char* nip_label;
static char* nip_next;

/* The current input file */
static FILE *nip_yyparse_infile = NULL;

/* Is there a hugin net file open? 0 if no, 1 if yes. */
static int nip_yyparse_infile_open = 0;

static int add_to_stringlink(stringlink *s, char* string);

static int search_stringlinks(stringlink s, char* string);

static int nullobservation(char *token);

static void free_datafile(datafile *f);

int open_yyparse_infile(const char *filename){
  if(!nip_yyparse_infile_open){
    nip_yyparse_infile = fopen(filename,"r");
    if (!nip_yyparse_infile){
      report_error(__FILE__, __LINE__, ERROR_IO, 1);
      return ERROR_IO; /* fopen(...) failed */
    }
    else
      nip_yyparse_infile_open = 1;
  }
  return NO_ERROR;
}


void close_yyparse_infile(){
  if(nip_yyparse_infile_open){
    fclose(nip_yyparse_infile);
    nip_yyparse_infile_open = 0;
  }
}


datafile *open_datafile(char *filename, char separator,
			int write, int nodenames){

  char last_line[MAX_LINELENGTH];
  char *token;
  int length_of_name = 0;
  int num_of_tokens = 0;
  int *token_bounds;
  int linecounter = 0;
  int dividend;
  int i, j;
  stringlink *statenames = NULL;
  stringlink temp, temp2;
  datafile *f = (datafile *) malloc(sizeof(datafile));

  if(!f){
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    return NULL;
  }


  if(write)
    f->file = fopen(filename,"w");
  else
    f->file = fopen(filename,"r");

  if (!f->file){
    report_error(__FILE__, __LINE__, ERROR_IO, 1);
    free(f);
    return NULL; /* fopen(...) failed */
  }
  else
    f->is_open = 1;

  length_of_name = strlen(filename);

  f->name = (char *) calloc(length_of_name + 1, sizeof(char));
  if(!f->name){
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    free(f);
    return NULL;
  }

  strcpy(f->name, filename);

  f->separator = separator;
  f->datarows = 0;

  /*
   * If file is opened in read mode, check the contents.
   * This includes names of nodes and their states.
   */
  if(!write){

    while(fgets(last_line, MAX_LINELENGTH, f->file)){
      num_of_tokens = count_tokens(last_line, NULL, 0, &separator, 1, 0, 0);
      token_bounds =
	tokenise(last_line, num_of_tokens, 0, &separator, 1, 0, 0);

      /* Read node names or make them up. */
      if(linecounter == 0){
	f->num_of_nodes = num_of_tokens;
	f->node_symbols = (char **) calloc(num_of_tokens, sizeof(char *));

	if(!f->node_symbols){
	  report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
	  free_datafile(f);
	  free(token_bounds);
	  return NULL;
	}

	statenames = (stringlink *) calloc(num_of_tokens, sizeof(stringlink));

	if(!statenames){
	  report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
	  free_datafile(f);
	  free(token_bounds);
	  return NULL;
	}

	f->num_of_states = (int *) calloc(num_of_tokens, sizeof(int));

	if(!f->num_of_states){
	  report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
	  free_datafile(f);
	  free(statenames);
	  free(token_bounds);
	  return NULL;
	}

	f->node_states = (char ***) calloc(num_of_tokens, sizeof(char **));

	if(!f->node_states){
	  report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
	  free_datafile(f);
	  free(statenames);
	  free(token_bounds);
	  return NULL;
	}

	if(nodenames){
	  f->firstline_labels = 1;

	  for(i = 0; i < num_of_tokens; i++){

	    f->node_symbols[i] =
	      (char *) calloc(token_bounds[2*i+1] - token_bounds[2*i] + 1,
			      sizeof(char));
	    if(!f->node_symbols[i]){
	      report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
	      free_datafile(f);
	      free(statenames);
	      free(token_bounds);
	      return NULL; /* error horror */
	    }

	    strncpy(f->node_symbols[i], &(last_line[token_bounds[2*i]]),
		    token_bounds[2*i+1] - token_bounds[2*i]);
	    f->node_symbols[i][token_bounds[2*i+1] - token_bounds[2*i]] = '\0';
	  }

	}
	else{
	  f->firstline_labels = 0;

	  for(i = 0; i < num_of_tokens; i++){

	    dividend = i + 1;
	    length_of_name = 5;

	    while((dividend /= 10) > 1)
	      length_of_name++;

	    f->node_symbols[i] = (char *) calloc(length_of_name, sizeof(char));
	    if(!f->node_symbols[i]){
	      report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
	      free_datafile(f);
	      free(statenames);
	      free(token_bounds);
	      return NULL; /* error horror */
	    }

	    sprintf(f->node_symbols[i], "node%d", i + 1);
	  }
	}
      }

      /* Read observations (just in order to see all the different
	 kinds of observations for each node). */
      if(linecounter != 0 || (linecounter == 0 && !nodenames)){

	if(num_of_tokens > 0)
	  f->datarows++;

	for(i = 0;
	    i < (f->num_of_nodes<num_of_tokens?f->num_of_nodes:num_of_tokens);
	    i++){

	  token = (char *) calloc(token_bounds[2*i+1] - token_bounds[2*i] + 1,
				  sizeof(char));

	  if(!token){
	    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
	    free_datafile(f);
	    free(statenames);
	    free(token_bounds);
	    return NULL;
	  }

	  strncpy(token, &(last_line[token_bounds[2*i]]),
		  token_bounds[2*i+1] - token_bounds[2*i]);

	  token[token_bounds[2*i+1] - token_bounds[2*i]] = '\0';

	  if(!(search_stringlinks(statenames[i], token) ||
	       nullobservation(token))){
	    add_to_stringlink(&(statenames[i]), token);
	  }
	  free(token);
	}
      }

      free(token_bounds);
      linecounter++;
    }

    /* Count number of states in each variable. */
    for(i = 0; i < f->num_of_nodes; i++){
      j = 0;
      temp = statenames[i];
      while(temp != NULL){
	j++;
	temp = temp->fwd;
      }
      f->num_of_states[i] = j;
    }

    for(i = 0; i < f->num_of_nodes; i++){

      f->node_states[i] =
	(char **) calloc(f->num_of_states[i], sizeof(char *));

      if(!f->node_states[i]){
	report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
	free_datafile(f);
	free(statenames);
	return NULL;
      }

      /* Copy statenames from the list */
      temp = statenames[i];
      for(j = 0; j < f->num_of_states[i]; j++){
	strcpy(f->node_states[i][j], temp->data);
	temp = temp->fwd;
      }
    }

  }

  /* JJT: Added 13.8. because of possible memory leaks. */
  for(i = 0; i < num_of_tokens; i++){
    temp = statenames[i];
    while(temp != NULL){
      temp2 = temp->fwd;
      free(temp->data);
      free(temp);
      temp = temp2;
    }
  }
  free(statenames);

  rewind(f->file);
  f->line_now = 0;

  return f;
}


/*
 * Tells if the given token indicates a missing value, a "null observation".
 * The token must be nul terminated.
 */
static int nullobservation(char *token){

#ifdef DEBUG_DATAFILE
  printf("nullobservation called\n");
#endif

  if(token == NULL){
    report_error(__FILE__, __LINE__, ERROR_NULLPOINTER, 1);
    return 0;
  }

  else if((strcmp("N/A", token) == 0) ||
	  (strcmp("null", token) == 0) ||
	  (strcmp("<null>", token) == 0)){
    return 1;
  }
  else{

    return 0;
  }
}


void close_datafile(datafile *file){

  if(!file){
    report_error(__FILE__, __LINE__, ERROR_NULLPOINTER, 1);
    return;
  }

  if(file->is_open){
    fclose(file->file);
    file->is_open = 0;
  }

  /* Release the memory of file struct. */
  free_datafile(file);
}


/* Frees the memory used by (possibly partially allocated) datafile f. */
static void free_datafile(datafile *f){
  
  int j;
  if(!f)
    return;
  free(f->name);
  if(f->node_symbols){
    for(j = 0; j < f->num_of_nodes; j++)
      free(f->node_symbols[j]);
    free(f->node_symbols);
  }
  free(f->num_of_states);
  if(f->node_states){
    for(j = 0; j < f->num_of_nodes; j++)
      free(f->node_states[j]);
    free(f->node_states);
  }
  free(f);
}


int nextline_tokens(datafile *f, char separator, char ***tokens){

  char line[MAX_LINELENGTH];
  char *token;
  int num_of_tokens = 0;
  int *token_bounds;
  int i, j;

  if(!f || !tokens){
    report_error(__FILE__, __LINE__, ERROR_NULLPOINTER, 1);
    return -1;
  }

  if(!(f->is_open)){
    report_error(__FILE__, __LINE__, ERROR_GENERAL, 1);
    return -1;
  }

  /* Skip the first line if it contains node labels. */
  if(f->firstline_labels && f->line_now == 0){
    if(fgets(line, MAX_LINELENGTH, f->file) == NULL)
      return -1;
    else
      f->line_now++;
  }

  if(fgets(line, MAX_LINELENGTH, f->file) == NULL)
    return -1;
  else
    f->line_now++;

  num_of_tokens = count_tokens(line, NULL, 0, &separator, 1, 0, 0);
  token_bounds = tokenise(line, num_of_tokens, 0, &separator, 1, 0, 0);
  
  if(num_of_tokens == 0){
    free(token_bounds);
    return 0;
  }

  *tokens = (char **)
    calloc(f->num_of_nodes<num_of_tokens?f->num_of_nodes:num_of_tokens,
	   sizeof(char *));

  if(!tokens){
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    free(token_bounds);
    return -1;
  }


  for(i = 0;
      i < (f->num_of_nodes<num_of_tokens?f->num_of_nodes:num_of_tokens);
      i++){

    token = (char *) calloc(token_bounds[2*i+1] - token_bounds[2*i] + 1,
			    sizeof(char));

    if(!token){
      report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
      free_datafile(f);
      for(j = 0; j < i; j++)
	free(*tokens[j]);
      free(token_bounds);
      return -1;
    }

    strncpy(token, &(line[token_bounds[2*i]]),
	    token_bounds[2*i+1] - token_bounds[2*i]);

    token[token_bounds[2*i+1] - token_bounds[2*i]] = '\0';

    /* Children, remember what papa said: always use parentheses... */
    (*tokens)[i] = token;
  }

  free(token_bounds);

  /* Return the number of acquired tokens. */
  return f->num_of_nodes<num_of_tokens?f->num_of_nodes:num_of_tokens;

}


char *next_token(int *token_length){

  /* The last line read from the file */
  static char last_line[MAX_LINELENGTH];

  /* How many tokens left in the current line of input? */
  static int tokens_left;

  /* Pointer to the index array of token boundaries */
  static int *indexarray = NULL;

  /* Should a new line be read in when the next token is requested? */
  static int read_line = 1;

  /* The token we return */
  char *token;

  /* Return if some evil mastermind gives us a NULL pointer */
  if(!token_length){
    if(indexarray)
      free(indexarray);
    return NULL;
  }

  /* Return if input file is not open */
  if(!nip_yyparse_infile_open){
    if(indexarray)
      free(indexarray);
    return NULL;
  }

  /* Read new line if needed and do other magic... */
  while(read_line){
    /* Read the line and check for EOF */
    if(!(fgets(last_line, MAX_LINELENGTH, nip_yyparse_infile))){
      if(indexarray != NULL)
	free(indexarray);
      *token_length = 0;
      return NULL;
    }
    /* How many tokens in the line? */
    tokens_left = count_tokens(last_line, NULL, 1, "(){}=,;", 7, 1, 1);

    /* Check whether the line has any tokens. If not, read a new line
     * (go to beginning of loop) */
    if(tokens_left > 0){
      /* Adjust pointer to the beginning of token boundary index array */
      if(indexarray != NULL)
	free(indexarray);
      indexarray = tokenise(last_line, tokens_left, 1, "(){}=,;", 7, 1, 1);

      /* Check if tokenise failed. If it failed, we have no other option
       * than to stop: return NULL, *token_length = 0.
       */
      if(!indexarray){
	report_error(__FILE__, __LINE__, ERROR_GENERAL, 0);
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
    if(indexarray)
      free(indexarray);
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
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

#ifdef PRINT_TOKENS
  printf("%s\n", token);
#endif

  return token;
}


/* Adds a variable into a temporary list for creating an array. 
 * The variable is chosen from THE list of variables 
 * according to the given symbol. */
int add_symbol(Variable v){
  varlink new = (varlink) malloc(sizeof(varelement));

  if(v == NULL){
    report_error(__FILE__, __LINE__, ERROR_NULLPOINTER, 1);
    free(new);
    return ERROR_NULLPOINTER;
  }

  if(!new){
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    return ERROR_OUTOFMEMORY;
  }

  new->data = v;
  new->fwd = NULL;
  new->bwd = nip_last_temp_var;

  if(nip_first_temp_var == NULL)
    nip_first_temp_var = new;
  else
    nip_last_temp_var->fwd = new;
    
  nip_last_temp_var = new;
  nip_symbols_parsed++;

  return NO_ERROR;
}


/* correctness? */
int add_initData(potential p, Variable child, Variable* parents){
  initDataLink new = (initDataLink) malloc(sizeof(initDataElement));

  if(!new){
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    return ERROR_OUTOFMEMORY;
  }

  new->data = p;
  new->child = child;
  new->parents = parents;
  new->fwd = NULL;
  new->bwd = nip_last_initData;
  if(nip_first_initData == NULL)
    nip_first_initData = new;
  else
    nip_last_initData->fwd = new;

  nip_last_initData = new;

  nip_initData_parsed++;
  return NO_ERROR;
}


int add_time_init(Variable var, char* next){
  time_init_link new = (time_init_link) malloc(sizeof(time_init_element));

  if(!new){
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    return ERROR_OUTOFMEMORY;
  }

  new->var = var;
  new->next = next;
  new->fwd = nip_first_timeinit;

  nip_first_timeinit = new;

  return NO_ERROR;
}

/* correctness? */
int add_double(double d){
  doublelink new = (doublelink) malloc(sizeof(doubleelement));

  if(!new){
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    return ERROR_OUTOFMEMORY;
  }

  new->data = d;
  new->fwd = NULL;
  new->bwd = nip_last_double;
  if(nip_first_double == NULL)
    nip_first_double = new;
  else
    nip_last_double->fwd = new;

  nip_last_double = new;
  nip_doubles_parsed++;
  return NO_ERROR;
}


int add_string(char* string){
  stringlink new = (stringlink) malloc(sizeof(stringelement));

  if(!new){
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    return ERROR_OUTOFMEMORY;
  }

  new->data = string;
  new->fwd = NULL;
  new->bwd = nip_last_string;
  if(nip_first_string == NULL)
    nip_first_string = new;
  else
    nip_last_string->fwd = new;

  nip_last_string = new;
  nip_strings_parsed++;

  return NO_ERROR;
}


/*
 * Adds a string to the beginning of the list s.
 * Pointer s is altered so that it points to the new beginning of the list.
 */
static int add_to_stringlink(stringlink *s, char* string){
  stringlink new = (stringlink) malloc(sizeof(stringelement));

#ifdef DEBUG_DATAFILE
  printf("add_to_stringlink called\n");
#endif

  if(!new){
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    return ERROR_OUTOFMEMORY;
  }

  if(s == NULL){
    free(new);
    report_error(__FILE__, __LINE__, ERROR_NULLPOINTER, 1);
    return ERROR_NULLPOINTER;
  }

  /*  new->data = string; */ /* Let's copy the string instead */
  if(string){
    new->data = (char *) calloc(strlen(string) + 1, sizeof(char));
    if(!(new->data)){
      free(new);
      report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
      return ERROR_OUTOFMEMORY;
    }
    strcpy(new->data, string);
  }

  new->fwd = *s;
  new->bwd = NULL;

  if(*s != NULL)
    (*s)->bwd = new;

  *s = new;

  return NO_ERROR;
}


/*
 * Checks if the given string is in the list s (search forward).
 */
static int search_stringlinks(stringlink s, char* string){

  while(s != NULL){
    if(strcmp(string, s->data) == 0){
      return 1;
    }
    s = s->fwd;
  }

  return 0;
}

/* Creates an array from the variable in the list. 
 * NOTE: because of misunderstanding, the list is backwards. 
 * (Can't understand Hugin fellows...) */
Variable* make_variable_array(){
  int i;
  Variable* vars1 = (Variable*) calloc(nip_symbols_parsed, sizeof(Variable));
  varlink pointer = nip_last_temp_var;

  if(!vars1){
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    return NULL;
  }

  for(i = 0; i < nip_symbols_parsed; i++){
    vars1[i] = pointer->data;
    pointer = pointer->bwd;
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

  if(!new){
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    return NULL;
  }

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

  if(!new){
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    return NULL;
  }

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
  free(nip_first_double);
  nip_first_double = NULL;
  nip_doubles_parsed = 0;
  return NO_ERROR;
}


/* Removes everything from the list of strings and resets the counter. 
 * The actual memory for the strings is not freed, only the list. */
int reset_strings(){
  int i;
  stringlink ln = nip_last_string;
  nip_last_string = NULL;
  while(ln != NULL){
    free(ln->fwd); /* free(NULL) is O.K. at the beginning */
    ln = ln->bwd;
  }
  free(nip_first_string);
  nip_first_string = NULL;

  for(i = 0; i < nip_strings_parsed; i++)
    free(nip_statenames[i]);
  free(nip_statenames);
  nip_statenames = NULL;

  nip_strings_parsed = 0;  
  return NO_ERROR;
}


/* Removes everything from the temporary list of variables. */
int reset_symbols(){
  varlink ln = nip_last_temp_var;
  nip_last_temp_var = NULL;
  while(ln != NULL){
    free(ln->fwd); /* free(NULL) is O.K. at the beginning */
    ln = ln->bwd;
  }
  free(nip_first_temp_var);
  nip_first_temp_var = NULL;
  nip_symbols_parsed = 0;
  return NO_ERROR;
}


/* Frees some memory after parsing. 
 * JJT: DO NOT TOUCH THE ACTUAL DATA, OR IT WILL BE LOST. */
int reset_initData(){
  initDataLink ln = nip_last_initData;
  initDataLink temp;
  nip_last_initData = NULL;
  while(ln != NULL){
    free_potential(ln->data);
    free(ln->parents); /* calloc is in make_variable_array(); */
    temp = ln;
    ln = ln->bwd;
    free(temp);
  }
  nip_first_initData = NULL;
  nip_initData_parsed = 0;  
  return NO_ERROR;
}


/* Frees some memory after parsing. 
 * JJT: DO NOT TOUCH THE ACTUAL DATA, OR IT WILL BE LOST. */
int reset_timeinit(){
  time_init_link ln = nip_first_timeinit;
  while(ln != NULL){
    nip_first_timeinit = ln->fwd;
    free(ln->next); /* calloc is somewhere in the lexer */
    free(ln);
    ln = nip_first_timeinit;    
  }

  return NO_ERROR;
}


void init_new_Graph(){
  nip_graph = new_graph(total_num_of_vars());
}


int parsedVars2Graph(){
  int i;
  Variable v;
  Variable_iterator it;
  initDataLink initlist = nip_first_initData;

  it = get_first_variable();
  v = next_Variable(&it);
  
  /* Add parsed variables to the graph. */
  while(v != NULL){
    
    add_variable(nip_graph, v);
    v = next_Variable(&it);
  }

  /* Add child - parent relations to the graph. */
  while(initlist != NULL){
    
    for(i = 0; i < initlist->data->num_of_vars - 1; i++)
      add_child(nip_graph, initlist->parents[i], initlist->child);
    
    initlist = initlist->fwd;
  }

  return NO_ERROR;
}


int time2Vars(){
  Variable v1, v2;
  time_init_link initlist = nip_first_timeinit;

  /* Add time relations to Variables. */
  while(initlist != NULL){
    
    v1 = initlist->var;
    v2 = get_parser_variable(initlist->next);
    if(v1->cardinality == v2->cardinality){
      v1->next = v2;
      v2->previous = v1;
    }
    else{
      report_error(__FILE__, __LINE__, ERROR_GENERAL, 1);
      return ERROR_GENERAL;
    }
    
    initlist = initlist->fwd;
  }

  return NO_ERROR;
}


int Graph2JTree(){

  Clique **cliques = get_cliques_pointer();

  /* Construct the Cliques. */
  set_num_of_cliques(find_cliques(nip_graph, cliques));

  free_graph(nip_graph); /* Get rid of the graph (?) */
  nip_graph = NULL;

  return 0; 
}


int parsedPots2JTree(){
  int i; 
  int nip_num_of_cliques = get_num_of_cliques();
  initDataLink initlist = nip_first_initData;
  Clique *nip_cliques = *(get_cliques_pointer());

  while(initlist != NULL){
    
    Variable *family = (Variable *) calloc(initlist->data->num_of_vars, 
					   sizeof(Variable));
    Clique fam_clique;

    if(!family){
      report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
      return ERROR_OUTOFMEMORY;
    }

    family[0] = initlist->child;
    for(i = 0; i < initlist->data->num_of_vars - 1; i++)
      family[i + 1] = initlist->parents[i];

    fam_clique = find_family(nip_cliques, nip_num_of_cliques,
			     family, initlist->data->num_of_vars);
    free(family);

    if(fam_clique != NULL){
      if(initlist->data->num_of_vars > 1)
	initialise(fam_clique, initlist->child, initlist->parents, 
		   initlist->data); /* THE job */
      else
	enter_evidence(get_first_variable(), *get_cliques_pointer(), 
		       get_num_of_cliques(), initlist->child, 
		       initlist->data->data);
    }
    else
      fprintf(stderr, "In parser.c (%d): find_family failed!\n", __LINE__);

    initlist = initlist->fwd;
  }

  return NO_ERROR;
}


void print_parsed_stuff(){
  int i, j;
  unsigned long temp;
  initDataLink list = nip_first_initData;

  /* Traverse through the list of parsed potentials. */
  while(list != NULL){
    int *indices; 
    int *temp_array;
    Variable *variables;

    if((indices = (int *) calloc(list->data->num_of_vars,
				 sizeof(int))) == NULL){
      report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
      return;
    }

    if((temp_array = (int *) calloc(list->data->num_of_vars,
				    sizeof(int))) == NULL){
      report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
      free(indices);
      return;
    }

    if((variables = (Variable *) calloc(list->data->num_of_vars,
					sizeof(Variable))) == NULL){
      report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
      free(indices);
      free(temp_array);
      return;
    }
    
    variables[0] = list->child;
    for(i = 1; i < list->data->num_of_vars; i++)
      variables[i] = (list->parents)[i - 1];

    /* reorder[i] is the place of i:th variable (in the sense of this program) 
     * in the array variables[] */
    
    /* init (needed?) */
    for(i = 0; i < list->data->num_of_vars; i++)
      temp_array[i] = 0;
    
    /* Create the reordering table: O(num_of_vars^2) i.e. stupid but working.
     * Note the temporary use of indices array. */
    for(i = 0; i < list->data->num_of_vars; i++){
      temp = get_id(variables[i]);
      for(j = 0; j < list->data->num_of_vars; j++){
	if(get_id(variables[j]) > temp)
	  temp_array[j]++; /* counts how many greater variables there are */
      }
    }
    
    /* Go through every number in the potential array. */
    for(i = 0; i < list->data->size_of_data; i++){
      
      inverse_mapping(list->data, i, indices);

      printf("P( %s = %s", list->child->symbol, 
	     (list->child->statenames)[indices[temp_array[0]]]);

      if(list->data->num_of_vars > 1)
	printf(" |");

      for(j = 0; j < list->data->num_of_vars - 1; j++)
	printf(" %s = %s", ((list->parents)[j])->symbol,
	       (((list->parents)[j])->statenames)[indices[temp_array[j + 1]]]);
      
      printf(" ) = %.2f \n", (list->data->data)[i]);
    }
    list = list->fwd;
    
    free(indices);
    free(temp_array);
    free(variables);
  }
}


void set_nip_statenames(char **states){
  nip_statenames = states;
}


char** get_nip_statenames(){
  return nip_statenames;
}


void set_nip_label(char *label){
  nip_label = label;
}


char* get_nip_label(){
  return nip_label;
}


void set_nip_next(char *next){
  nip_next = next;
}


char* get_nip_next(){
  return nip_next;
}


int get_nip_symbols_parsed(){
  return nip_symbols_parsed;
}


int get_nip_strings_parsed(){
  return nip_strings_parsed;
}
