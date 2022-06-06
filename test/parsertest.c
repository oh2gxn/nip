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

/* Program for testing parser utility functions
 * Author: Janne Toivola
 * Version: $Id: parsertest.c,v 1.10 2010-12-07 17:23:19 jatoivol Exp $
 */

#include <stdio.h>
#include "nipparsers.h"

/* see nip.c */
FILE *open_net_file(const char *filename);
void close_net_file();

/* Tries out the Huginnet parser stuff and low level data parsing. */
int main(int argc, char *argv[]){

  int i, j, t, token_length, n_tokens;
  int ok = 1;
  char *token;
  char **tokens = NULL;
  FILE *nf = NULL;
  nip_data_file df = NULL;
  char SEP = ',';

  if (argc < 3) {
    fprintf(stderr, "Two filenames must be given: net and data file\n");
    return -1;
  }
  else if ((nf = open_net_file(argv[1])) == NULL) {
    fprintf(stderr, "Failed to open net file %s", argv[1]);
    return -1;
  }
  else if ((df = nip_open_data_file(argv[2], SEP, 0, 1)) == NULL) {
    fprintf(stderr, "Failed to open data file %s", argv[2]);
    return -1;
  }

  // Try reading NET file tokens
  printf("%s:\n", argv[1]);
  while(ok){
    token = nip_next_hugin_token(nf, &token_length);

    if(token_length == 0)
      ok = 0; /* no more tokens */

    /* Print each "token" on a new line */
    if(ok)
      printf("%s\n", token);
    
  }
  close_net_file();

  // Print stats found by open_data_file
  printf("%s:\n", df->name);
  printf("sequences:%d\n", df->ndatarows);
  printf("sequence lengths:");
  for(i = 0; i < df->ndatarows - 1; i++)
    printf("%d%c", df->datarows[i], df->separator);
  printf("%d\n", df->datarows[df->ndatarows - 1]);
  printf("columns:%d\n", df->num_of_nodes);
  printf("column names:");
  for(i = 0; i < df->num_of_nodes - 1; i++)
    printf("%s%c", df->node_symbols[i], df->separator);
  printf("%s\n", df->node_symbols[df->num_of_nodes - 1]);

  for(i = 0; i < df->num_of_nodes; i++){
    printf("column %s values:", df->node_symbols[i]);
    for(j = 0; j < df->num_of_states[i] - 1; j++)
      printf("%s%c", df->node_states[i][j], df->separator);
    printf("%s\n", df->node_states[i][df->num_of_states[i] - 1]);
  }

  // Print data accessed by next_line_tokens
  i=0; j=0;
  while (0 < (n_tokens = nip_next_line_tokens(df, SEP, &tokens))) {
    printf("series %d, row %d :", i, j);
    for (t=0; t < n_tokens-1; t++)
      printf("%s%c", tokens[t], df->separator);
    printf("%s\n", tokens[n_tokens-1]);
    j++;
    if (i >= df->ndatarows)
      break;
    if (j >= df->datarows[i]){
      i++;
      j=0;
    }
  }

  return 0;
}
