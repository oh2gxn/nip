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

/* Test program for the Hugin Net parser
 * Author: Janne Toivola
 * Version: $Id: bisontest.c,v 1.53 2010-12-07 17:23:19 jatoivol Exp $
 */

#include <stdlib.h>
#include "niplists.h"
#include "niperrorhandler.h"
#include "nipvariable.h"
#include "nippotential.h"
#include "nipjointree.h"
#include "nipparsers.h"
#include "huginnet.tab.h"

/*
#define DEBUG_BISONTEST
*/

FILE *open_net_file(const char *filename);
void close_net_file();
int yyparse();
nip_variable_list get_parsed_variables();
int get_cliques(nip_clique** clique_array_pointer);
/* Reminder: extern keyword is for declaring external _variables_ */

/*
 * Calculate the probability distribution of variable "var".
 * The family of var must be among the cliques in cliques[]
 * (the size of which is num_of_cliques).
 * This function allocates memory for the array "result". The size of
 * the array is returned in "size_of_result".
 */
static void test_probability(double** result, int* size_of_result,
			     nip_variable var, nip_clique cliques[],
			     int num_of_cliques){

  /* Find the clique that contains the family of the interesting variable. */
  nip_clique clique_of_interest = nip_find_family(cliques, num_of_cliques, var);
  if(!clique_of_interest){
    fprintf(stderr, "In bisontest.c : No clique found! Sorry.\n");
    *size_of_result = 0;
    return;
  }
  *size_of_result = NIP_CARDINALITY(var);

  /* Allocate memory for the result */
  *result = (double *) calloc(*size_of_result, sizeof(double));
  if(!result){
    fprintf(stderr, "In bisontest.c : Calloc failed.\n");
    *size_of_result = 0;
    return;
  }

  /* marginalisation */
  nip_marginalise_clique(clique_of_interest, var, *result);

  /* normalisation */
  nip_normalise_array(*result, *size_of_result);
}

int main(int argc, char *argv[]){

  int i, j, retval;
  int size_of_result;

  char* tailptr = NULL;
  double d;
  double* input;
  double* result;

  nip_clique* cliques;
  int num_of_cliques;

  nip_variable interesting;
  nip_variable* vars;
  int nvars;
  nip_variable_list var_list;
  nip_variable_iterator it;

#ifdef DEBUG_BISONTEST
  int temp;
#endif

  /* -- Start parsing the network definition file */
  if(argc < 5){
    fprintf(stderr, "Specify following parameters, please:\n");
    fprintf(stderr, "- Hugin model file name,\n");
    fprintf(stderr, "- symbol of the variable of interest,\n");
    fprintf(stderr, "- state of the variable, and\n");
    fprintf(stderr, "- probability of the state of the variable.\n");
    return 0;
  }
  else if(open_net_file(argv[1]) == NULL)
    return -1;

  retval = yyparse();

  close_net_file();

  if(retval != 0)
    return retval;
  /* The input file has been parsed. -- */

  // get the results of parsing
  num_of_cliques = get_cliques(&cliques);
  var_list = get_parsed_variables();
  it = NIP_LIST_ITERATOR(var_list);
  nvars = NIP_LIST_LENGTH(var_list);
  vars = (nip_variable*) calloc(nvars, sizeof(nip_variable));
  for(i = 0; i < nvars; i++)
    vars[i] = nip_next_variable(&it);

  // determine variable of interest
  interesting = nip_search_variable_array(vars, nvars, argv[2]);
  if(!interesting){
    fprintf(stderr, "In bisontest.c : variable %s not found.\n", argv[2]);
    return 1;
  }

  fprintf(stdout, "Before evidence:\n");
  for(i = 0; i < num_of_cliques; i++){
    fprintf(stdout, "Potential of ");
    nip_fprintf_clique(stdout, cliques[i]);
    nip_fprintf_potential(stdout, cliques[i]->p);
  }

  // generate evidence about the variable
  i = nip_variable_state_index(interesting, argv[3]);
  if (i < 0){
    fprintf(stderr, "In bisontest.c : variable %s state %s not found.\n", argv[2], argv[3]);
    return 1;
  }
  d = strtod(argv[4], &tailptr);
  if(tailptr == argv[4] || d < 0.0 || d > 1.0){
    fprintf(stderr, "In bisontest.c : invalid probability %s.\n", argv[4]);
    return 1;
  }
  input = (double *) calloc(NIP_CARDINALITY(interesting), sizeof(double));
  for(j = 0; j < NIP_CARDINALITY(interesting); j++){
    if(i==j)
      input[j] = d;
    else
      input[j] = (1.0 - d)/(NIP_CARDINALITY(interesting) - 1);
  }
  nip_enter_evidence(vars, nvars, cliques, num_of_cliques, interesting, input);

  /* a propagation */
  for(i = 0; i < num_of_cliques; i++)
    nip_unmark_clique(cliques[i]);
  nip_collect_evidence(NULL, NULL, cliques[0]);

  for(i = 0; i < num_of_cliques; i++)
    nip_unmark_clique(cliques[i]);
  nip_distribute_evidence(cliques[0]);

  fprintf(stdout, "After evidence:\n");
  for(i = 0; i < num_of_cliques; i++){
    fprintf(stdout, "Potential of ");
    nip_fprintf_clique(stdout, cliques[i]);
    nip_fprintf_potential(stdout, cliques[i]->p);
  }

  printf("Normalised probability of %s:\n", nip_variable_symbol(interesting));
  test_probability(&result, &size_of_result, interesting, cliques, num_of_cliques);
  for(i = 0; i < size_of_result; i++)
    printf("P(%s=%d) = %f\n", nip_variable_symbol(interesting), i, result[i]);

  for(i = 0; i < nvars; i++)
    nip_free_variable(vars[i]);
  free(vars);
  free(input);
  free(result);
  return 0;
}
