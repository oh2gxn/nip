/* huginnet.y $Id: huginnet.y,v 1.12 2004-05-26 14:40:49 jatoivol Exp $
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
  double numval;
  double *doublearray;
  char *name;
  char **stringarray;
  Variable *variablearray;
}

/******************************************************************/
/* NOTE ABOUT STRINGS! ********************************************/
/* The memory for the strings is allocated in yylex() and should  */
/* be freed if the string is copied or not used!                  */
/* (The "ownership" of strings changes.)                          */
/******************************************************************/

%token node "node"
%token potential "potential"
%token states "states"
%token label "label"
%token position "position"
%token data "data"
%token <name> QUOTED_STRING
%token <name> UNQUOTED_STRING
%token <numval> NUMBER
%type <stringarray> strings statesDeclaration
%type <doublearray> numbers dataList
%type <variablearray> symbols
%type <name> labelDeclaration

/* Grammar follows */
/* NOT READY!!! 
 * TODO:
 * - create sepsets somehow
 * - make it parse nested lists as data
 * - find out what to do with the parsed stuff! */
%%
input:         /* empty string */
             | declaration input
;

declaration:   nodeDeclaration {/* put the node somewhere */}
             | potentialDeclaration {/* put the clique somewhere */}
;

nodeDeclaration:    node UNQUOTED_STRING '{' labelDeclaration 
                                             statesDeclaration
                                             positionDeclaration
                                             parameters '}' {
  /* new_variable() ??? */
  add_pvar(new_variable($2, $4, $5, strings_parsed)); 
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

positionDeclaration:  position '=' '(' NUMBER NUMBER ')' ';' {/* ignore */}
;

unknownDeclaration:  UNQUOTED_STRING '=' value ';' {/* ignore */}
;

potentialDeclaration: potential '(' symbols ')' '{' dataList '}' { 
  /* <Some AI to make decisions> */ 

  /* FIXME: This is totally wrong. This part should use the graph 
   *        implementation to create cliques and sepsets according to 
   *        the variables denoted by symbols. */

  Clique c = make_Clique($3, symbols_parsed);
  potential p = create_Potential($3, symbols_parsed, $6); 
  add_clique(c);
  // This assumes that the first symbol is the one and only child variable!
  initialise(c, $3, p);

  reset_symbols();}
;

symbols:       /* end of list */ { $$ = make_variable_array(); }
             | QUOTED_STRING symbols { add_symbol($1); }
             | QUOTED_STRING '|' symbols { add_symbol($1); }
;

strings:       /* end of list */ { $$ = make_string_array(); }
             | QUOTED_STRING strings { add_string($1); }
;

numbers:       /* end of list */ { $$ = make_double_array(); }
             | NUMBER numbers { add_number($1); }
;

value:         QUOTED_STRING { free($1); /* ignore */}
             | '(' numbers ')' { reset_doubles(); }
             | NUMBER {/* ignore */}
;

dataList: data '=' '(' numbers ')' ';' { $$ = $4; }
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
