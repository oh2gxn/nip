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

  int token_length;
  int ok = 1;
  char *token;
  char **tokens = NULL;
  FILE *nf = NULL;  

  if (argc < 2) {
    fprintf(stderr, "Filename must be given\n");
    return -1;
  }
  else if ((nf = open_net_file(argv[1])) == NULL) {
    fprintf(stderr, "Failed to open net file %s", argv[1]);
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

  return 0;
}
