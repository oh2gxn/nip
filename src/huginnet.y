/* huginnet.y $Id: huginnet.y,v 1.27 2004-06-04 06:29:23 mvkorpel Exp $
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

#define YYERROR_VERBOSE
  //#define DEBUG_BISON
%}

/* BISON Declarations */
/* These are the data types for semantic values. */
%union {
  double numval;
  double *doublearray;
  char *name;
  char **stringarray;
  Variable variable;
  Variable *variablearray;
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
%type <stringarray> strings statesDeclaration
%type <doublearray> numbers dataList
%type <variable> nodeDeclaration symbol
/* %type <variablearray> symbols */
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
  $$ = $4 }
;


positionDeclaration:  token_position '=' '(' NUMBER NUMBER ')' ';' {/* ignore */}
;


unknownDeclaration:  UNQUOTED_STRING '=' value ';' {/* ignore */}
;


potentialDeclaration: token_potential '(' symbol '|' symbols ')' '{' dataList '}' { 
  int i;
  Variable *vars = (Variable*) calloc(nip_symbols_parsed + 1,
				      sizeof(Variable));
  Variable *parents = make_variable_array();
  potential p;

  if(!vars || !parents)
    YYABORT;
  printf("nip_symbols_parsed = %d\n", nip_symbols_parsed); /* DEBUG */
  vars[0] = $3;
  for(i = 0; i < nip_symbols_parsed; i++)
    vars[i + 1] = parents[i];

  p = create_Potential(vars, nip_symbols_parsed + 1, $8);

  printf("WTF happens here !?!?\n");

  add_initData(p, $3, parents); 
  free($8); // the data was copied at create_Potential
  reset_doubles();
  reset_symbols();
  free(vars);
}
;

symbol:        UNQUOTED_STRING { $$ = get_variable($1) }

;


symbols:       /* end of list */ { /* $$ = make_variable_array() */ }
             | symbol2 symbols { /* add_symbol($1) */ }
;


symbol2:       UNQUOTED_STRING { add_symbol(get_variable($1)) }

;


strings:       /* end of list */ { $$ = make_string_array() }
             | string strings { /* add_string($1) */ }
;


string:        QUOTED_STRING { add_string($1) }
;


/* This should ignore all brackets between numbers! */
numbers:       /* end of list */ { $$ = make_double_array() }
             | num numbers { /* add_double($1) */ }
             | '(' numbers ')' {/* ignore */} // nested lists?
             | '(' numbers ')' '(' numbers ')' {/* ignore */} // nested lists?
;


num:           NUMBER { add_double($1) }
;


value:         QUOTED_STRING { free($1) /* ignore */}
             | '(' numbers ')' { reset_doubles() /* ignore */}
             | NUMBER {/* ignore */}
;


dataList: token_data '=' '(' numbers ')' ';' { $$ = $4 }
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
#ifdef DEBUG_BISON
      printf("yylex returns UNQUOTED_STRING.\n");
#endif
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
#ifdef DEBUG_BISON
      printf("yylex returns token_label.\n");
#endif
      return token_label;
    }
    /* node */
    if(tokenlength == 4){
      if(strncmp("node", token, 4) == 0){
	free(token);
#ifdef DEBUG_BISON
	printf("yylex returns token_node.\n");
#endif
	return token_node;
      }else if(strncmp("data", token, 4) == 0){
	free(token);
#ifdef DEBUG_BISON
	printf("yylex returns token_data.\n");
#endif
	return token_data;
      }
    }
    /* potential */
    if(tokenlength == 9 &&
       strncmp("potential", token, 9) == 0){
      free(token);
#ifdef DEBUG_BISON
      printf("yylex returns token_potential.\n");
#endif
      return token_potential;
    }
    /* states */
    if(tokenlength == 6 &&
       strncmp("states", token, 6) == 0){
      free(token);
#ifdef DEBUG_BISON
      printf("yylex returns token_states.\n");
#endif
      return token_states;
    }
    /* position */
    if(tokenlength == 8 &&
       strncmp("position", token, 8) == 0){
      free(token);
#ifdef DEBUG_BISON
      printf("yylex returns token_position.\n");
#endif
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
#ifdef DEBUG_BISON
      printf("yylex returns QUOTED_STRING.\n");
#endif
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
#ifdef DEBUG_BISON
      printf("yylex returns NUMBER = %f.\n", numval);
#endif
      free(nullterminated);
      return NUMBER;
    }

#ifdef DEBUG_BISON
    printf("Not a number: %s\n", nullterminated);
#endif
      
    /* Everything else is UNQUOTED_STRING */
    yylval.name = nullterminated;

    free(token);

#ifdef DEBUG_BISON
    printf("yylex returns UNQUOTED_STRING.\n");
#endif

    return UNQUOTED_STRING;

  }

}

void
yyerror (const char *s)  /* Called by yyparse on error */
{
  printf ("%s\n", s);
}
