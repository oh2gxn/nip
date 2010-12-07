/* 
 * Basic tokeniser function for the Hugin Net file parser.
 * Contains also another parser functions for handling data files.
 *
 * JJ Comment: Currently the parser is ugly as hell...
 *             Get rid of global variables and ad-hoc data structures!
 *             (add_X(), get_X(), and set_X() are probably the worst)
 *
 * $Id: nipparsers.h,v 1.1 2010-12-07 17:23:18 jatoivol Exp $
 */

#ifndef __NIPPARSERS_H__
#define __NIPPARSERS_H__

#define MAX_LINELENGTH 10000

/* Comment character in input files.
 * After this, the rest of the line is ignored.
 */
#define NIP_COMMENT_CHAR '%'

/* there are some reasons to include stuff here (lack of huginnet.h...) */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "niplists.h"
#include "nipstring.h"
#include "nipvariable.h"
#include "niperrorhandler.h"

typedef struct {
  char* name;     /* file name */
  char separator; /* ASCII separator between records on the same line */
  FILE* file;     /* file handle */
  int is_open;    /* flag if the file is open */
  int first_line_labels; /* flag if the first line contains node symbols */
  int current_line; /* current position in the file 
		     * (values are in [1...N] after nextline_tokens) */
  int label_line;   /* line number of node labels, or -1 */

  /* Number of time series (continuous data rows, without empty lines) */
  int ndatarows;
  /* Number of rows in the file for each time series, 
   * excluding the line containing node symbols */
  int* datarows;

  /* String array: Names of the variables (aka attributes or nodes) */
  char** node_symbols;
  int num_of_nodes; /* number of variables present (columns) */

  /* String array array: Names of the possible values of the variables */
  char*** node_states; /* 1st index = node, 2nd index = state */
  int* num_of_states; /* number of states for each variable */
} nip_data_file_struct;
typedef nip_data_file_struct* nip_data_file;

/*
 * Opens a file by creating a nip_data_file, which can be used 
 * for reading or writing the file after opening.
 * The function sets the file position indicator to the beginning of the file.
 * Parameters:
 * - filename : the name of the file to be opened
 * - separator : the separator character between fields (ASCII)
 * - write : 0 if the file is opened for reading only, 
 *           e.g. 1 if the file is opened for writing only.
 * - nodenames : 1 if the first row contains names of nodes, else 0
 */
nip_data_file nip_open_data_file(char* filename, char separator,
				 int write, int nodenames);

/*
 * Closes a file described by the data file struct.
 * Also frees the memory allocated for the data file struct.
 */
void nip_close_data_file(nip_data_file file);


/*
 * Gets the tokens (separated by separator) on the next line of
 * the given file. Allocates memory for an array of strings and its
 * contents. Skips the first line of the file, if it contains node labels.
 * Returns the number of tokens found.
 * Negative number indicates error or end of file.
 */
int nip_next_line_tokens(nip_data_file f, char separator, char ***tokens);


/* Gets the next token from the hugin .net file.
 * Returns the token. token_length is the length of the token.
 * The token is a NUL terminated string.
 * *token_length doesn't include the NULL character.
 * After the token has been used, PLEASE free the memory allocated for it.
 * If token_length == 0, there are no more tokens.
 */
char* nip_next_hugin_token(int* token_length, FILE* f);


#endif /* __PARSER_H__ */
