/* huginnet.y $Id: huginnet.y,v 1.1 2004-03-05 13:38:56 mvkorpel Exp $
 * Grammar file for a subset of the Hugin Net language
 */

%{
#include <stdio.h>
%}

/* BISON Declarations */
%union {
  double numval;
  char *name;
}

%token node "node"
%token potential "potential"
%token states "states"
%token label "label"
%token <name> NODEID

/* Grammar follows */
/* NOT READY!!! */
%%
input:         /* empty string */
             | input declaration
;

declaration:   nodedecl
             | potdecl
;

nodedecl:      node NODEID '{' '}'
;

potdecl:       potential '{' '}'

%%
/* Lexical analyzer */
/* Change this, this is copy-paste from a calculator example */

#include <ctype.h>

int
yylex (void)
{
  int c;

  /* skip white space  */
  while ((c = getchar ()) == ' ' || c == '\t')
    ;
  /* process numbers   */
  if (c == '.' || isdigit (c))
    {
      ungetc (c, stdin);
      scanf ("%lf", &yylval);
      return NUM;
    }
  /* return end-of-file  */
  if (c == EOF)
    return 0;
  /* return single chars */
  return c;
}

void
yyerror (const char *s)  /* Called by yyparse on error */
{
  printf ("%s\n", s);
}
