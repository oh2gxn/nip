/* huginnet.y $Id: huginnet.y,v 1.35 2004-06-09 08:00:34 mvkorpel Exp $
 * Grammar file for a subset of the Hugin Net language
 */

%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Clique.h"
#include "Variable.h"
#include "parser.h"
#include "errorhandler.h"
#include "huginnet.h"

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

%token token_class "class"
%token token_data "data"
%token token_label "label"
%token token_node "node"
%token token_position "position"
%token token_potential "potential"
%token token_states "states"
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
 * - make compatible with the actual Hugin files
 * - find out what to do with the parsed stuff! */

/* A PROBLEM: The graph is not entirely known until the end of the 
 * file has been reached, but the cliques are needed immediately!
 * Solutions:
 * - defer the creation of jointree and the initialisation of cliques 
 *   until the graph is ready 
 * - X ? */
%%
input:  nodes potentials {

  parsedVars2Graph();

  Graph2JTree();

  parsedPots2JTree();

#ifdef DEBUG_BISON
  print_parsed_stuff();
#endif

  reset_initData()}

| token_class UNQUOTED_STRING '{' parameters nodes potentials '}' {

  parsedVars2Graph();

  Graph2JTree();

  parsedPots2JTree();

#ifdef DEBUG_BISON
  print_parsed_stuff();
#endif

  reset_initData()}
;


nodes:    /* empty */ { init_new_Graph() }         
|         nodeDeclaration nodes {/* a variable added */}
;


potentials:    /* empty */ {/* list of initialisation data ready */}
             | potentialDeclaration potentials {/* potential added */}
;


nodeDeclaration:    token_node UNQUOTED_STRING '{' node_params '}' {
  /* new_variable() */
  Variable v = new_variable($2, get_nip_label(),
			    get_nip_statenames(), nip_strings_parsed);

  reset_strings();
  add_pvar(v);
  $$ = v}
;

node_params: /* end of definitions */
|            unknownDeclaration node_params
|            statesDeclaration node_params { set_nip_statenames($1) }
|            labelDeclaration node_params { set_nip_label($1) }
|            positionDeclaration node_params
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

  add_initData(create_Potential(vars, nip_symbols_parsed + 1, doubles),
	       vars[0], parents); 
  free(doubles); /* the data was copied at create_Potential */
  reset_doubles();
  reset_symbols();
  free(vars)
}
|                     token_potential '(' child ')' '{' dataList '}' {
  Variable vars[1];
  double *doubles = $6;
  vars[0] = $3;
  add_initData(create_Potential(vars, 1, doubles), vars[0], NULL); 
  free(doubles); /* the data was copied at create_Potential */
  reset_doubles();
  reset_symbols()
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


/* This should ignore all brackets between numbers! */
ignored_numbers:     /* end of list */
           | NUMBER ignored_numbers { /* ignore the NUMBER */ }
           | '(' ignored_numbers ')' ignored_numbers {/* ignore brackets */}
;


num:       NUMBER { add_double($1) }
;


value:         QUOTED_STRING { free($1) } /* ignore */
|              ignored_numbers { /* ignore */ }
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
    /* label or class */
    if(tokenlength == 5){
      if(strncmp("label", token, 5) == 0){
	free(token);

	return token_label;
      }else if(strncmp("class", token, 5) == 0){
	free(token);

	return token_class;
      }
    }
    /* node or data */
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
