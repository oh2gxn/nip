/*
 * Definitions for the bison parser and for other parsers.
 * $Id: parser.h,v 1.32 2004-06-30 08:31:42 mvkorpel Exp $
 */

#ifndef __PARSER_H__
#define __PARSER_H__

#define MAX_LINELENGTH 10000

/* Comment character in input files.
 * After this, the rest of the line is ignored.
 */
#define COMMENT_CHAR '%'

#include "Clique.h"
#include "Variable.h"
#include "potential.h"
#include "Graph.h"
#include <stdio.h>

struct doublelist {
  double data;
  struct doublelist *fwd;
  struct doublelist *bwd;
};

typedef struct doublelist doubleelement;
typedef doubleelement *doublelink;

struct stringlist {
  char* data;
  struct stringlist *fwd;
  struct stringlist *bwd;
};

typedef struct stringlist stringelement;
typedef stringelement *stringlink;

struct initDataList {
  potential data;
  Variable child;
  Variable* parents;
  struct initDataList *fwd;
  struct initDataList *bwd;
};

typedef struct initDataList initDataElement;
typedef initDataElement *initDataLink;

typedef struct {
  char *name;
  char separator;
  FILE *file;
  int is_open;
  int firstline_labels; /* Does the first line contain node labels?*/
  int line_now; /* Current position in file */

  char **node_symbols;
  int num_of_nodes;

  char ***node_states; /* 1st index = node, 2nd index = state */
  int *num_of_states;

} datafile;


/* Opens an input file. Returns 0 if file was opened or if some file was
 * already open. Returns ERROR_GENERAL if an error occurred
 * opening the file.
 */
int open_yyparse_infile(const char *filename);


/* Closes the current input file (if there is one).
 */
void close_yyparse_infile();


/*
 * Opens a file by creating a datafile struct. The struct can be used 
 * for reading or writing after opening.
 * The function sets the file-position indicator to the beginning of the file.
 * Parameters:
 * - filename : the name of the file to be opened
 * - separator : the separator character between fields
 * - write : 0 if the file is opened for reading only, 
 *           e.g. 1 if the file is opened for writing only.
 * - nodenames : 1 if the first row contains names of nodes
 */
datafile *open_datafile(char *filename, char separator,
			int write, int nodenames);


/*
 * Closes a file described by the datafile struct.
 * Also releases the memory of the datafile struct.
 */
void close_datafile(datafile *file);


/*
 * Gets the tokens (separated by separator) on the next line of
 * the given file. Allocates memory for an array of strings and its
 * contents. Skips the first line of the file, if it contains node labels.
 * Returns the number of tokens found.
 * Negative number indicates error or end of file.
 */
int nextline_tokens(datafile *f, char separator, char ***tokens);


/* Gets the next token from the hugin .net file.
 * Returns the token. token_length is the length of the token.
 * The token is a NUL terminated string.
 * *token_length doesn't include the NULL character.
 * After the token has been used, PLEASE free the memory allocated for it.
 * If token_length == 0, there are no more tokens.
 */
char *next_token(int *token_length);


/* Adds a variable into a list for creating an array. The variable is 
 * chosen from THE list of variables according to the given symbol. */
int add_symbol(Variable v);


/* Gets the parsed variable according to the symbol. */
Variable get_variable(char *symbol);


/* Adds a potential and the correspondent variable references into a list.
 * The "ownership" of the vars[] array changes! */
int add_initData(potential p, Variable child, Variable* parents);


/* Adds a number into the list of parsed numbers. */
int add_double(double d);


/* Adds a string into the list of parsed strings. */
int add_string(char* string);


/* Creates an array from the variable references in the temp list. 
 * The size will be symbols_parsed. */
Variable* make_variable_array();


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


/* Removes everything from the list of strings and resets the counter. */
int reset_strings();


/* Removes everything from the temporary list of variables. */
int reset_symbols();


/* Frees some memory after parsing. */
int reset_initData();


/* Initialises a new graph. */
void init_new_Graph();


/* Inserts the parsed variables and their relations into the graph.
 * Returns an error code. (0 is O.K.) */
int parsedVars2Graph();


/* Constructs the join tree from the graph (which is hopefully ready)
 * Returns an error code. (0 is O.K.) */
int Graph2JTree();


/* Initialises the join tree (Clique array) with parsed potentials. 
 * Returns an error code. (0 is O.K.) */
int parsedPots2JTree();


/* Some debug printing about what was parsed. */
void print_parsed_stuff();


void set_nip_statenames(char **states);
char** get_nip_statenames();


void set_nip_label(char *label);
char* get_nip_label();


int get_nip_symbols_parsed();


int get_nip_strings_parsed();

#endif /* __PARSER_H__ */
