/* huginnet.y $Id: huginnet.y,v 1.9 2004-05-24 13:58:34 jatoivol Exp $
 * Grammar file for a subset of the Hugin Net language
 */

%{
#include <stdio.h>
#include <stdlib.h>
#include "Graph.h"
#include "Variable.h"
#include "parser.h"
#include "errorhandler.h"
%}

/* BISON Declarations */
/* These are the data types for semantic values. */
%union {
  int intval;
  double numval;
  double *doublearray;
  char *name;
  char **stringarray;
}

%token node "node"
%token potential "potential"
%token states "states"
%token label "label"
%token <name> QUOTED_STRING
%token <name> UNQUOTED_STRING
%token <numval> NUMBER
%type <stringarray> strings
%type <doublearray> numbers
%type <name> labelDeclaration
%type <intval> statesDeclaration

/* Grammar follows */
/* NOT READY!!! Procedures and arrays in C do not mix well with the more 
 * or less functional paradigm of the parser. */
%%
input:         /* empty string */
             | declaration input
;

declaration:   nodeDeclaration {/* put the node somewhere */}
             | potentialDeclaration {/* put the potential somewhere */}
;

nodeDeclaration:    node UNQUOTED_STRING '{' labelDeclaration 
                                             statesDeclaration 
                                             parameters '}' {
  /* new_variable() ??? */
  add_pvar(new_variable($2, $4, $5)); 
  reset_strings();}
;

/* probably not the easiest way... think again: LALR(1) */
parameters:    /* end of definitions */
             | unknownDeclaration parameters
;

labelDeclaration:     label '=' QUOTED_STRING ';' { $$ = $3 }
;

/* JJT: cardinality??? */
statesDeclaration:    states '=' '(' strings ')' ';' { $$ = strings_parsed; }
;

unknownDeclaration:  UNQUOTED_STRING '=' value ';' {/* ignore */}
;

strings:       /* end of list */ { $$ = make_string_array(); }
             | QUOTED_STRING strings { add_string($1); }
;

numbers:       /* end of list */ { $$ = make_double_array(); }
             | NUMBER numbers { add_number($1); }
;

value:         QUOTED_STRING {/* ignore */}
             | numbers { reset_doubles(); }
             | strings { reset_strings(); }
             | NUMBER {/* ignore */}
;

potentialDeclaration: potential '{' '}' { /* <Some AI to make decisions> */ }
;

%%
/* Lexical analyzer */
/* Change this, this is copy-paste from a calculator example */
/* JJT: I did some reading. Might get nasty, if there has to be a token 
 *      for a potential array and yylval becomes a double array... Only 
 *      other option: leave the creation of an array to the parser. But 
 *      how can you add elements dynamically? Cardinality & stuff?
 *      List -> Array and Array -> Potential at the end of each potdecl? 
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
  double numval;

  /* EOF or error */
  if(tokenlength <= 0)
    return 0;

  /* Single character */
  else if(tokenlength == 1){
    int retval = *token;
    free(token);
    return retval;
  }

  /* Multicharacter tokens */
  else{
    /* Literal string tokens */ 
    /* label */
    if(tokenlength == 5 &&
       strncmp("label", token, 5) == 0){
      free(token);
      return label;
    }
    /* node */
    if(tokenlength == 4 &&
       strncmp("node", token, 4) == 0){
      free(token);
      return node;
    }
    /* potential */
    if(tokenlength == 9 &&
       strncmp("potential", token, 9) == 0){
      free(token);
      return potential;
    }
    /* states */
    if(tokenlength == 6 &&
       strncmp("states", token, 6) == 0){
      free(token);
      return states;
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
    errno = 0;
    numval = strtod(nullterminated, 0);
    /* No error, so the token is a valid double. */
    if(errno == 0){
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
