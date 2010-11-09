/* Definitions for the bison parser and for other parsers.
 *
 * JJ Comment: Currently the parser is ugly as hell...
 *             Get rid of global variables and ad-hoc data structures!
 *             (add_X(), get_X(), and set_X() are probably the worst)
 *
 * $Id: parser.h,v 1.55 2010-11-09 19:06:08 jatoivol Exp $
 */

#ifndef __PARSER_H__
#define __PARSER_H__

#define MAX_LINELENGTH 10000

/* Comment character in input files.
 * After this, the rest of the line is ignored.
 */
#define COMMENT_CHAR '%'

/* there are some reasons to include stuff here (lack of huginnet.h...) */
#include "nipvariable.h"
#include "lists.h"
#include <stdio.h>


typedef struct {
  char *name;
  char separator;
  FILE *file;
  int is_open;
  int firstline_labels; /* Does the first line contain node labels? */
  int line_now; /* Current position in file 
		 * ([1...N] after nextline_tokens) */
  int label_line;

  /* Number of continuous time series (no empty lines) */
  int ndatarows;
  /* Number of rows in the file for each time series, 
   * excluding the line containing node labels */
  int *datarows;

  char **node_symbols;
  int num_of_nodes;

  char ***node_states; /* 1st index = node, 2nd index = state */
  int *num_of_states;
} datafile;


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
char *next_token(int *token_length, FILE *f);


#endif /* __PARSER_H__ */
