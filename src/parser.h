/* Definitions for the bison parser. $Id: parser.h,v 1.7 2004-04-22 14:23:03 mvkorpel Exp $
 */

#ifndef __PARSER_H__
#define __PARSER_H__

#define MAX_LINELENGTH 1000

/* Comment character in input files.
 * After this, the rest of the line is ignored.
 */
#define COMMENT_CHAR '#'

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
static stringlink last_string = 0;
static int strings_parsed = 0;

/* The current input file */
static FILE *parser_infile = NULL;

/* Is there a file open? 0 if no, 1 if yes. */
static int file_open = 0;

/* Opens an input file. Returns 0 if file was opened or if some file was
 * already open. Returns ERROR_GENERAL if an error occurred
 * opening the file.
 */
int open_infile(const char *file);

/* Closes the current input file (if there is one).
 */
void close_infile();

/* Gets the next token from the input file.
 * Returns the token. token_length is the length of the token.
 * The token is a null terminated string.
 * *token_length doesn't include the null character.
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

/* Creates an array from the double values in the list. 
 * The size will be doubles_parsed. */
double* make_double_array();

/* Creates an array from the strings in the list. 
 * The size will be strings_parsed. */
char** make_string_array();

/* Removes everything from the list of doubles. This is likely to be used 
 * after the parser has parsed doubles to the list, created an array out 
 * of it and wants to reset the list for future use. */
int reset_doubles();

/* Removes everything from the list of strings. */
int reset_strings();

#endif /* __PARSER_H__ */
