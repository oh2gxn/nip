/*  NIP - Dynamic Bayesian Network library
    Copyright (C) 2012  Janne Toivola

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, see <http://www.gnu.org/licenses/>.
*/

/* Test program for seeing if the data file abstraction works...
 * Author: Janne Toivola
 * Version: $Id: datafiletest.c,v 1.10 2010-12-07 17:23:19 jatoivol Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include "nipparsers.h"

int main(int argc, char *argv[]){

  int i, j;
  nip_data_file file = NULL;
  int num_of_tokens;
  char **tokens;
  char SEP = ',';

  if(argc < 2){
    fprintf(stderr, "Filename must be given\n");
    return -1;
  }
  else if((file = nip_open_data_file(argv[1], SEP, 0, 1)) == NULL){
    fprintf(stderr, "Problems opening file %s\n", argv[1]);
    return -1;
  }
  else if (nip_analyse_data_file(file) < 1) {
    fprintf(stderr, "Failed to read data file %s", argv[1]);
    return -1;
  }

  printf("Information about the data file:\n");
  printf("Name: %s\n", file->name);
  printf("Separator: %c\n", file->separator);
  printf("is_open: %d\n", file->is_open);
  printf("label_line: %d\n", file->label_line);
  printf("Number of sequences: %d\n", file->ndatarows);
  printf("Sequence lengths: ");
  for(i = 0; i < file->ndatarows - 1; i++)
    printf("%d%c", file->datarows[i], file->separator);
  printf("%d\n", file->datarows[file->ndatarows-1]);
  printf("Number of columns: %d\n", file->num_of_nodes);
  printf("Columns:\n");
  for(i = 0; i < file->num_of_nodes; i++){
    printf("- Node %s, states: ", file->node_symbols[i]);
    for(j = 0; j < file->num_of_states[i] - 1; j++)
      printf("%s%c", file->node_states[i][j], file->separator);
    printf("%s\n", file->node_states[i][file->num_of_states[i]-1]);
  }

  printf("\nReading tokens one line at a time:\n");

  while((num_of_tokens = nip_next_line_tokens(file, SEP, &tokens)) > 0){
    for(i = 0; i < num_of_tokens-1; i++){
      printf("%s%c", tokens[i], file->separator);
    }
    printf("%s\n", tokens[num_of_tokens-1]);
    for(i = 0; i < num_of_tokens; i++)
      free(tokens[i]);
    free(tokens);
  }

  nip_close_data_file(file);

  return 0;
}
