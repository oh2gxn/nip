/* huginnet.y $Id: huginnet.y,v 1.31 2004-06-07 11:38:01 mvkorpel Exp $
 * Grammar file for a subset of the Hugin Net language
 */

%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Graph.h"
#include "Variable.h"
#include "parser.h"
#include "errorhandler.h"
#include "huginnet.h"
#include "potential.h" /* for DEBUG purposes */

#define YYERROR_VERBOSE
/* #define DEBUG_BISON */

%}

/* BISON Declarations */
/* These are the data types for semantic values. */
%union {
  double numval;
  double *doublearray;
  char *name;
  char **stringarray;
  Variable variable;
}

/******************************************************************/
/* NOTE ABOUT STRINGS! ********************************************/
/* The memory for the strings is allocated in yylex() and should  */
/* be freed if the string is copied or not used!                  */
/* (The "ownership" of strings changes.)                          */
/******************************************************************/

%token token_node "node"
%token token_potential "potential"
%token token_states "states"
%token token_label "label"
%token token_position "position"
%token token_data "data"
%token <name> QUOTED_STRING
%token <name> UNQUOTED_STRING
%token <numval> NUMBER
%type <stringarray> statesDeclaration
%type <doublearray> dataList
%type <variable> nodeDeclaration child
%type <name> labelDeclaration

/* Grammar follows */
/* NOT READY!!! 
 * TODO:
 * - create cliques correctly (with Graph.h)
 * - create sepsets somehow   (with Graph.h)
 * - figure out a way to initialise the resulting jointree
 * - find out what to do with the parsed stuff! */

/* A PROBLEM: The graph is not entirely known until the end of the 
 * file has been reached, but the cliques are needed immediately!
 * Solutions:
 * - defer the creation of jointree and the initialisation of cliques 
 *   until the graph is ready 
 * - X ? */
%%
input:  nodes potentials {
  /* <final stuff here> */

  /*
   * Create the graph between parsing nodes and potentials.
   * Graph structure and clique initialisation data 
   * will be in initData after parsing potentials!
   */

#ifdef DEBUG_BISON
  int j, k;
  int temp_index;
  unsigned long biggest_found, biggest_ready;
  initDataLink list = nip_first_initData;
#endif

  int i;
  varlink list_of_vars = nip_first_var;
  initDataLink initlist = nip_first_initData;

  /* Add parsed variables to the graph. */
  while(list_of_vars != NULL){

    add_variable(nip_graph, list_of_vars->data);
    list_of_vars = list_of_vars->fwd;
  }

  /* Add child - parent relations to the graph. */
  while(initlist != NULL){

    for(i = 0; i < initlist->data->num_of_vars - 1; i++)
      add_child(nip_graph, initlist->parents[i], initlist->child);

    initlist = initlist->fwd;
  }

  /* Construct the join tree. */
  nip_num_of_cliques = find_cliques(nip_graph, &nip_cliques);

  printf("In huginnet.y: %d cliques found.\n", nip_num_of_cliques);

#ifdef DEBUG_BISON
  /*    while(list != NULL){
    printf("P(%s |", list->child->symbol);
    for(i = 0; i < list->data->num_of_vars - 1; i++)
      printf(" %s", (list->parents)[i]->symbol);
    printf(") = {");
    for(i = 0; i < list->data->size_of_data; i++)
      printf(" %.2f", (list->data->data)[i]);
    printf(" }\n\n");
    list = list->fwd;
    }
  */

  /* Traverse through the list of parsed potentials. */
  while(list != NULL){
    int *indices; 
    unsigned long *reorder;

    if((indices = (int *) calloc(list->data->num_of_vars,
				 sizeof(int))) == NULL)
      fprintf(stderr, "In huginnet.y: Calloc failed => crash.");

    if((reorder = (unsigned long *) calloc(list->data->num_of_vars,
					   sizeof(unsigned long))) == NULL)
      fprintf(stderr, "In huginnet.y: Calloc failed => crash.");

    /* Go through every number in the potential array. */
    for(i = 0; i < list->data->size_of_data; i++){
      inverse_mapping(list->data, i, indices);

      reorder[0] = get_id(list->child);
      for(j = 1; j < list->data->num_of_vars; j++)
	reorder[j] = get_id((list->parents)[j - 1]);
      
      /* Make a reorder array.
       * Smallest = 0, ..., Biggest = num_of_vars - 1
       */
      biggest_ready = 0; /* Initialisation doesn't matter. */
      temp_index = 0; /* Initialisation doesn't matter. */
      for(j = 0; j < list->data->num_of_vars; j++){
	biggest_found = VAR_MIN_ID - 1;
	for(k = 0; k < list->data->num_of_vars; k++){
	  if(j == 0){
	    if(reorder[k] > biggest_found){
	      temp_index = k;
	      biggest_found = reorder[k];
	    }
	  }
	  else if(reorder[k] > biggest_found && reorder[k] < biggest_ready){
	    temp_index = k;
	    biggest_found = reorder[k];
	  }
	}
	reorder[temp_index] = list->data->num_of_vars -1 - j;
	biggest_ready = biggest_found;
      }

      printf("P( %s = %s |", list->child->symbol, 
	     (list->child->statenames)[indices[reorder[0]]]);

      for(j = 0; j < list->data->num_of_vars - 1; j++)
	printf(" %s = %s", (list->parents)[j]->symbol,
	       ((list->parents[j])->statenames)[indices[reorder[j + 1]]]);
      
      printf(" ) = %.2f \n", (list->data->data)[i]);
    }
    list = list->fwd;
    
    free(indices);
    free(reorder);
  }


#endif

  reset_initData()}
;


nodes:             nodeDeclaration nodes { /* add_pvar($1) */ }
|         /* empty */ { nip_graph = new_graph(nip_vars_parsed)}

;


potentials:    /* empty */ {/* initialisation data ready at first_initData */}
             | potentialDeclaration potentials {/* potential somewhere? */}
;


nodeDeclaration:    token_node UNQUOTED_STRING '{' statesDeclaration 
                                             labelDeclaration
                                             positionDeclaration
                                             parameters '}' {
  /* new_variable() */
  Variable v = new_variable($2, $5, $4, nip_strings_parsed); 

  reset_strings();
  add_pvar(v);
  $$ = v}
;


parameters:    /* end of definitions */
             | unknownDeclaration parameters
;


labelDeclaration:     token_label '=' QUOTED_STRING ';' { $$ = $3 }
;


/* JJT: cardinality == strings_parsed ? */
statesDeclaration:    token_states '=' '(' strings ')' ';' { 
  /* makes an array of strings out of the parsed list of strings */
  $$ = make_string_array() }
;


positionDeclaration:  token_position '=' '(' NUMBER NUMBER ')' ';' {/* ignore */}
;


unknownDeclaration:  UNQUOTED_STRING '=' value ';' {/* ignore */}
;


potentialDeclaration: token_potential '(' child '|' symbols ')' '{' dataList '}' { 
  int i;
  Variable *vars = (Variable*) calloc(nip_symbols_parsed + 1,
				      sizeof(Variable));
  Variable *parents = make_variable_array();
  double *doubles = $8;

  if(!vars || !parents)
    YYABORT;

#ifdef DEBUG_BISON
  printf("nip_symbols_parsed = %d\n", nip_symbols_parsed);
#endif

  vars[0] = $3;
  for(i = 0; i < nip_symbols_parsed; i++)
    vars[i + 1] = parents[i];

  /* Yritelmiä metsästää bugia */
#ifdef DEBUG_BISON
  fprintf(stdout, "Datatesti tiedostossa huginnet.y:\n");
  for(i = 0; i < nip_doubles_parsed; i++)
    printf("data[%d] = %f\n", i, doubles[i]);
#endif

  add_initData(create_Potential(vars, nip_symbols_parsed + 1, doubles),
	       vars[0], parents); 
  free(doubles); // the data was copied at create_Potential
  reset_doubles();
  reset_symbols();
  free(vars);
}
;

child:        UNQUOTED_STRING { $$ = get_variable($1) }
;


symbols:       /* end of list */
             | symbol symbols { /* add_symbol($1) */ }
;


symbol:       UNQUOTED_STRING { add_symbol(get_variable($1)) }
;


strings:       /* end of list */
             | string strings { /* add_string($1) */ }
;


string:        QUOTED_STRING { add_string($1) }
;


/* This should ignore all brackets between numbers! */
numbers:     /* end of list */
           | num numbers { /* add_double($1) */ }
           | '(' numbers ')' numbers {/* ignore brackets */}
; 


num:       NUMBER { add_double($1) }
;


value:         QUOTED_STRING { free($1) } /* ignore */
;


dataList: token_data '=' '(' numbers ')' ';' { $$ = make_double_array() }
;

%%
/* Lexical analyzer */
/* JJT: I did some reading. Might get nasty, if there has to be a token 
 *      for a potential array and yylval becomes a double array... Only 
 *      other option: leave the creation of an array to the parser. But 
 *      how can you add elements dynamically? Cardinality & stuff?
 *      List -> Array and Array -> Potential at the end of each 
 *      potentialDeclaration? 
 *
 *      yylex will be able to parse strings i.e. strings are terminals: 
 *      "this is a string": char* -> [t, h, i, s, ... , g, \0] as lvalue 
 *
 *      BTW: How does the graph framework fit into this? All the structure 
 *      is more or less implicitly defined :-( */

#include <ctype.h>

int
yylex (void)
{
  int tokenlength;
  char *token = next_token(&tokenlength);
  char *nullterminated;
  char *endptr;
  double numval;

  /* EOF or error */
  if(tokenlength <= 0)
    return 0;

  /* Single character */
  else if(tokenlength == 1){
    int retval = *token;

    nullterminated = (char *) calloc(2, sizeof(char));
    if(!nullterminated){
      report_error(ERROR_OUTOFMEMORY, 0);
      free(token);
      return 0; /* In the case of an (unlikely) error, stop the parser */
    }
    nullterminated[0] = *token;
    nullterminated[1] = '\0';

    /* Single letter ('A' - 'Z' or 'a' - 'z') is UNQUOTED_STRING. */
    if(isalpha((int)*token)){
      yylval.name = nullterminated;
      free(token);

      return UNQUOTED_STRING;
    }

    /* Single digit ('0' - '9') is NUMBER. */
    else if(isdigit((int)*token)){
      yylval.numval = strtod(nullterminated, 0);
      free(token);
      free(nullterminated);
      return NUMBER;
    }

    /* Other chars (';' '(', ')', etc. ) */
    else {
      free(token);
      free(nullterminated);
      return retval;
    }
  }

  /* Multicharacter tokens */
  else{
    /* Literal string tokens */ 
    /* label */
    if(tokenlength == 5 &&
       strncmp("label", token, 5) == 0){
      free(token);

      return token_label;
    }
    /* node */
    if(tokenlength == 4){
      if(strncmp("node", token, 4) == 0){
	free(token);

	return token_node;
      }else if(strncmp("data", token, 4) == 0){
	free(token);

	return token_data;
      }
    }
    /* potential */
    if(tokenlength == 9 &&
       strncmp("potential", token, 9) == 0){
      free(token);

      return token_potential;
    }
    /* states */
    if(tokenlength == 6 &&
       strncmp("states", token, 6) == 0){
      free(token);

      return token_states;
    }
    /* position */
    if(tokenlength == 8 &&
       strncmp("position", token, 8) == 0){
      free(token);

      return token_position;
    }
    /* End of literal string tokens */

    /* Regular tokens (not literal string) */
    /* QUOTED_STRING (enclosed in double quotes) */
    if(token[0] == '"' &&
       token[tokenlength - 1] == '"'){
      nullterminated = (char *) calloc(tokenlength - 1, sizeof(char));
      if(!nullterminated){
	report_error(ERROR_OUTOFMEMORY, 0);
	free(token);
	return 0; /* In the case of an (unlikely) error, stop the parser */
      }
      /* For the semantic value of the string, strip off double quotes
       * and insert terminating null character. */
      strncpy(nullterminated, &(token[1]), tokenlength - 2);
      nullterminated[tokenlength - 2] = '\0';
      yylval.name = nullterminated;

      free(token);

      return QUOTED_STRING;
    }

    nullterminated = (char *) calloc(tokenlength + 1, sizeof(char));
    if(!nullterminated){
      report_error(ERROR_OUTOFMEMORY, 0);
      free(token);
      return 0; /* In the case of an (unlikely) error, stop the parser */
    }
    strncpy(nullterminated, token, tokenlength);
    nullterminated[tokenlength] = '\0';

    /* NUMBER ? */
    numval = strtod(nullterminated, &endptr);
    /* No error, so the token is a valid double. */
    if(!(nullterminated == endptr && numval == 0)){
      yylval.numval = numval;
      free(token);
      free(nullterminated);
      return NUMBER;
    }

    /* Everything else is UNQUOTED_STRING */
    yylval.name = nullterminated;

    free(token);

    return UNQUOTED_STRING;

  }

}

void
yyerror (const char *s)  /* Called by yyparse on error */
{
  printf ("%s\n", s);
}
