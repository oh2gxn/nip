/* Definitions for the bison parser. $Id: parser.h,v 1.5 2004-03-23 12:08:55 jatoivol Exp $
 */

#ifndef __PARSER_H__
#define __PARSER_H__

#define MAX_LINELENGTH 1000

#include "Variable.h"
#include "potential.h"
#include <stdio.h>

struct varlist {
  Variable data;
  struct varlist *fwd;
  struct varlist *bwd;
};

typedef struct varlist varelement;
typedef varelement *varlink;

static varlink first_var = 0; /* global stuff, sad but true */
static varlink last_var = 0;
static int vars_parsed = 0;


struct doublelist {
  double data;
  struct doublelist *fwd;
  struct doublelist *bwd;
};

typedef struct doublelist doubleelement;
typedef doubleelement *doublelink;

static varlink first_double = 0;
static varlink last_double = 0;
static int doubles_parsed = 0;


struct stringlist {
  char* data;
  struct stringlist *fwd;
  struct stringlist *bwd;
};

typedef struct stringlist stringelement;
typedef stringelement *stringlink;

static stringlink first_string = 0;
static varlink last_string = 0;
static int strings_parsed = 0;

/* The current input file */
static FILE *parser_infile = NULL;

/* Is there a file open? 0 if no, 1 if yes. */
static int file_open = 0;

/* Opens an input file. Returns 0 if file was opened or if some file was
 * already open. Returns 1 if an error occurred opening the file.
 */
int open_infile(const char *file);

/* Closes the current input file (if there is one).
 */
void close_infile();

/* Gets the next token from the input file.
 * Returns the token. token_length is the length of the token.
 * After the token has been used, PLEASE free the memory allocated for it.
 * If token_length == 0, there are no more tokens.
 */
char *next_token(int *token_length);

/* Adds a variable into the list of variables. */
int add_pvar(Variable var);

/* Adds a number into the list of parsed numbers. */
int add_double(double d);

/* Adds a string into the list of parsed strings. */
int add_string(char* string);

#endif /* __PARSER_H__ */
