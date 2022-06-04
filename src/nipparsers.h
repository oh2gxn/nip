/**
 * @file
 * @brief Basic tokeniser functions for the Hugin Net file parser.
 * Contains also other parser functions and structs for data files.
 *
 * JJ Comment: Currently the parser is ugly as hell...
 *             Get rid of global variables and ad-hoc data structures!
 *             (add_X(), get_X(), and set_X() are probably the worst)
 *
 * @author Janne Toivola
 * @copyright &copy; 2007,2012 Janne Toivola <br>
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version. <br>
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details. <br>
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __NIPPARSERS_H__
#define __NIPPARSERS_H__

/**
 * Hard length limit (in bytes) when reading lines from files
 */
#define MAX_LINELENGTH 10000

/**
 * Comment character in input files. The rest of the line is ignored.
 */
#define NIP_COMMENT_CHAR '%'

#include <stdio.h> // FILE
#include <stdlib.h>
#include <string.h>
#include "niplists.h"
#include "nipstring.h"
#include "niperrorhandler.h"

/**
 * Structure for bookkeeping while reading data from a file, 
 * possibly before nip_variables have been created. */
typedef struct {
  char* name;     ///< file name
  char separator; ///< ASCII separator between records on the same line
  FILE* file;     ///< file handle
  int is_open;    ///< flag if the file is open
  int first_line_labels; ///< flag if the first line contains node symbols
  int current_line; /**< current position in the file 
		       (values are in [1...N] after nextline_tokens) */
  int label_line;   ///< line number of node labels, or -1 if headerless

  int ndatarows; ///< Number of time series (separated by empty lines)
  int* datarows; /**< Number of rows in the file for each time series, 
		    excluding the line containing node symbols */

  int num_of_nodes;    ///< Number of variables present (columns)
  char** node_symbols; ///< Names of the variables (aka attributes or nodes)

  int* num_of_states;  ///< Number of states for each variable
  char*** node_states; /**< String array array: Names of the possible values 
			  of the variables, \p node_states[node][state] */
} nip_data_file_struct;

typedef nip_data_file_struct* nip_data_file; ///< reference to a data file

/**
 * Opens a file and creates a struct, which can be used 
 * for reading / parsing or writing the file after opening.
 * The function sets the file position indicator to the beginning of the file.
 * @param filename Name of the file to be opened (null terminated string)
 * @param separator The separator character between fields (ASCII)
 * @param write 0 if the file is opened for reading only, 
 * or non-zero if the file is opened for writing.
 * @param nodenames Non-zero if the first row contains names of 
 * nodes / variables / columns (a header), else 0
 * @return reference to a new data file struct, free after use
 * @see nip_close_data_file() */
nip_data_file nip_open_data_file(char* filename, char separator,
				 int write, int nodenames);

/**
 * Closes a file described by the data file struct.
 * Also frees the memory allocated for the data file struct.
 * @param file Reference to the struct to be freed */
void nip_close_data_file(nip_data_file file);


/**
 * Gets the tokens (separated by separator) on the next line of
 * the given file. Allocates memory for an array of strings and its
 * contents. Skips the first line of the file, if it contains node labels 
 * (header row).
 * @param f Reference to an opened data file
 * @param separator The ASCII character to be considered field separator
 * @param tokens Pointer where an array of strings will be written
 * @return The number of tokens found, size of \p *tokens array, or
 * negative number in case of error or end of file. */
int nip_next_line_tokens(nip_data_file f, char separator, char ***tokens);


/**
 * Gets the next token from an opened hugin .net file.
 * If token_length == 0, there are no more tokens.
 * @param f File descriptor to a .net file
 * @param token_length Pointer where the length of a found token is written, 
 * or 0 if no more tokens to read.
 * NOTE: length does not include the null character
 * @return a null terminated string, free it after use */
char* nip_next_hugin_token(FILE* f, int* token_length);


#endif /* __PARSER_H__ */
