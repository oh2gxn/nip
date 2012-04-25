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

/*
#define PRINT_ALL
*/


int main(int argc, char *argv[]){

  int i, j;
  nip_data_file file;
#ifdef PRINT_ALL
  int num_of_tokens;
  char **tokens;
#endif

  if(argc < 2){
    printf("Filename must be given!\n");
    return -1;
  }
  else if((file = nip_open_data_file(argv[1], ',', 0, 1)) == NULL){
    printf("Problems opening file %s\n", argv[1]);
    return -1;
  }

  printf("Information about the data file:\n");
  printf("\tName: %s\n", file->name);
  printf("\tSeparator: %c\n", file->separator);
  printf("\tis_open: %d\n", file->is_open);
  printf("\tlabel_line: %d\n", file->label_line);
  printf("\tNumber of time series: %d\n", file->ndatarows);
  printf("\tNumber of data rows for each time series: \n");
  for(i = 0; i < file->ndatarows; i++)
    printf("\t  %d,\n", file->datarows[i]);
  printf("\tNumber of nodes: %d\n", file->num_of_nodes);
  printf("Nodes:\n");
  for(i = 0; i < file->num_of_nodes; i++){
    printf("\tNode <%s>, states:\n\t", file->node_symbols[i]);
    for(j = 0; j < file->num_of_states[i]; j++)
      printf("<%s>, ", file->node_states[i][j]);
    printf("\n");
  }

#ifdef PRINT_ALL
  printf("\nReading tokens one line at a time.\n");

  while((num_of_tokens = nip_next_line_tokens(file, ',', &tokens)) >= 0){

    for(i = 0; i < num_of_tokens; i++){
      printf("%s ", tokens[i]);
      free(tokens[i]);
    }
    printf("\n");
    free(tokens);
  }
#endif

  nip_close_data_file(file);

  return 0;
}
