/* huginnet.y $Id: huginnet.y,v 1.7 2004-03-23 13:25:23 mvkorpel Exp $
 * Grammar file for a subset of the Hugin Net language
 */

%{
#include <stdio.h>
#include "Graph.h"
#include "Variable.h"
#include "parser.h"
#include "errorhandler.h"
%}

/* BISON Declarations */
/* These are the data types for semantic values. */
%union {
  double numval;
  char *name;
}

%token node "node"
%token potential "potential"
%token states "states"
%token label "label"
%token <name> NODEID
%token <name> STRING
%token <numval> NUMBER
%token UNKNOWN

/* Grammar follows */
/* NOT READY!!! Procedures and arrays in C do not mix well with the more 
 * or less functional paradigm of the parser. */
%%
input:         /* empty string */
             | input declaration
;

declaration:   nodedecl {/* put the node somewhere */}
             | potdecl {/* put the potential somewhere */}
;

nodedecl:      node NODEID '{' parameters  '}' {/* new_variable() */}
;

/* probably not the easiest way... think again: LALR(1) */
parameters:    /* end of definitions */
             | labeldecl parameters
             | statesdecl parameters
	     | unknown_decl parameters
;

labeldecl:     label '=' STRING ';'
;

/* JJT: I think this is not the best way either... cardinality??? */
statesdecl:    states '=' '(' STRING strings ')' ';'
;

strings:       /* end of list */
             | STRING strings
;

unknown_decl:  UNKNOWN '=' value ';' /* ignore */
;

value:         STRING /* Should there be $$ = $1 or something*/
             | list
             | NUMBER
;

list:          '(' strlistitems ')'
             | '(' numlistitems ')' 
;

strlistitems:  /* empty */
             | STRING strlistitems
;

numlistitems:  /* sumthin */
/*
potdecl:       potential '{' '}' /* arbitrary array? */
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
    /* STRING (enclosed in double quotes) */
    if(token[0] == '"' &&
       token[tokenlength - 1] == '"'){
      char *stringval = (char *) calloc(tokenlength - 1, sizeof(char));
      if(!stringval){
	report_error(ERROR_OUTOFMEMORY, 0);
	free(token);
	return 0; /* In the case of an (unlikely) error, stop the parser */
      }
      /* For the semantic value of the string, strip off double quotes
       * and insert terminating null character. */
      strncpy(stringval, &(token[1]), tokenlength - 2);
      stringval[tokenlength - 2] = '\0';
      yylval.name = stringval;

      free(token);
      return STRING;
    }

    /* NUMBER */    

    /* NODEID */

    /* UNKNOWN */
    else
      return UNKNOWN;

    /* End of regular tokens */
  }

}

void
yyerror (const char *s)  /* Called by yyparse on error */
{
  printf ("%s\n", s);
}
