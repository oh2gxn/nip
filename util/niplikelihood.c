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

/* niplikelihood.c
 *
 * Reads the structure for the model from a Hugin NET file, 
 * reads the given data file and prints the conditional likelihood
 * of one set of variables {ABC} given the rest of the observed 
 * variables {DEF}.
 * For example, you can compute p( a b c | d e f ) for each record of 
 * {abcdef} data in a file by specifying {ABC} as the variables of interest.
 *
 * SYNOPSIS: 
 * NIPLIKELIHOOD <MODEL.NET> <DATA.TXT> <A B C...>
 *
 * - Structure of the model will be read from <MODEL.NET>
 * - data will be read from <DATA.TXT>
 * - <A B C> are the variables of interest (space delimited labels) 
 * - all the other observed variables will be the reference
 * - resulting likelihood values will be written to stdout
 *
 * EXAMPLE: ./niplikelihood model.net data.txt A B C
 * If data.txt contained data about ABCDEF, then the result will be
 * p(a b c | d e f) for each record {abcdef} in the file.
 *
 * Author: Janne Toivola
 * Version: $Id: niplikelihood.c,v 1.2 2010-12-07 17:23:19 jatoivol Exp $
 */

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <float.h>
#include <math.h>
#include "nip.h"
#include "nipvariable.h"

int main(int argc, char *argv[]) {

  int i, n, t;
  nip_model model = NULL;
  time_series *ts_set = NULL;
  time_series ts = NULL;
  double m1, m2;
  double log_likelihood = 0;
  nip_variable v = NULL;

  printf("niplikelihood:\n");

  if(argc < 4){
    printf("You must specify: \n"); 
    printf(" - the NET file, e.g. model.net, \n");
    printf(" - the data file, e.g. data.txt, and \n"); 
    printf(" - variable(s) of interest, e.g. A B C \n");
    return 0;
  }
  
  /* read the model */
  model = parse_model(argv[1]);
  if(!model){
    fprintf(stderr, "Unable to parse the NET file: %s?\n", argv[1]);
    return -1;
  }

  /* read the data */
  n = read_timeseries(model, argv[2], &ts_set);
  if(n == 0){
    fprintf(stderr, "Unable to parse the data file: %s?\n", argv[2]);
    free_model(model);
    return -1;
  }

  /* Read the variable labels */
  for(i = 0; i < model->num_of_vars; i++)
    nip_unmark_variable(model->variables[i]); /* Unmark all to be sure */

  for(i = 3; i < argc; i++){
    v = model_variable(model, argv[i]);
    if(v == NULL){
      ;
      /*fprintf(stderr, "Unrecognized variable: %s?\n", argv[i]);
      
	for(i = 0; i < n; i++)
	  free_timeseries(ts_set[i]);
	free(ts_set);
	free_model(model);
	return -1;
      */
    }
    else{
      nip_mark_variable(v); /* Mark the variables of interest */
    }
  }

  /* THE work */
  for(i = 0; i < n; i++){ /* For each time series */
    ts = ts_set[i];
    reset_model(model); /* Reset the clique tree */
    use_priors(model, !NIP_HAD_A_PREVIOUS_TIMESLICE);
    
    for(t = 0; t < TIME_SERIES_LENGTH(ts); t++){ /* For each time step */      

      insert_ts_step(ts, t, model, NIP_MARK_OFF); /* Only unmarked variables */
      make_consistent(model);      
      m1 = model_prob_mass(model); /* the reference mass */

      insert_ts_step(ts, t, model, NIP_MARK_ON); /* Only marked variables */
      make_consistent(model);
      m2 = model_prob_mass(model); /* the final mass */

      /* log_likelihood == ln p( marked | unmarked ) */
      log_likelihood = log(m2) - log(m1);

      printf("%g %g %g\n", m1, m2, log_likelihood); /* One of the results */

      reset_model(model); /* Reset the clique tree */
      use_priors(model, NIP_HAD_A_PREVIOUS_TIMESLICE);
    }
    printf("\n"); /* time series separator */
  }

  /* Free stuff */
  for(i = 0; i < n; i++)
    free_timeseries(ts_set[i]);
  free(ts_set);
  free_model(model);

  return 0;
}
