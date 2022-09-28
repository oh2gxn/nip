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

/* nipnext.c
 *
 * SYNOPSIS:
 * NIPNEXT <MODEL.NET> <INPUT_DATA.TXT> <VARIABLE> > <OUTPUT_DATA.TXT>
 *
 * Executes inference procedure with given model and time series.
 * Inferred log. probabilities for each state of the selected variable
 * at the final time step are written to stdout.
 * Tip: input data might contain earlier observations of the variable,
 * but the last time step should have null value
 * (or else this is just a cumbersome one-hot encoder).
 *
 * EXAMPLE: ./nipnext filter.net data.txt A > inferred_data.txt
 *
 * Author: Janne Toivola
 */

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "nip.h"

// Callback for witnessing I/O
static int ts_progress(int sequence, int length);
static int ts_progress(int sequence, int length){
  fprintf(stderr, "  %d: %d\r", sequence, length);
  return 0;
}

int main(int argc, char *argv[]){

  int i, j, t, n_max;

  double probe, loglikelihood;
  double *posterior = NULL;

  nip_model model = NULL;
  nip_variable v = NULL;

  time_series ts = NULL;
  time_series *ts_set = NULL;
  uncertain_series ucs = NULL;

  fprintf(stderr, "nipnext:\n");

  /*****************************************/
  /* Parse the model from a Hugin NET file */
  /*****************************************/
  if(argc < 4){
    fprintf(stderr, "Specify the names of the net file, input data file, ");
    fprintf(stderr, "and variable of interest.\n");
    return 0;
  }
  model = parse_model(argv[1]);
  if(model == NULL)
    return -1;

  /*****************************/
  /* read the data from a file */
  /*****************************/
  fprintf(stderr, "  Reading input data from %s... \n", argv[2]);
  n_max = read_timeseries(model, argv[2], &ts_set, &ts_progress);
  if(n_max < 1){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_INVALID_ARGUMENT, 1);
    fprintf(stderr, "%s\n", argv[2]);
    free_model(model);
    return -1;
  }
  fprintf(stderr, "  ...%d sequences found.\n", n_max);

  /****************/
  /* the variable */
  /****************/
  v = model_variable(model, argv[3]);
  if(!v){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_INVALID_ARGUMENT, 1);
    fprintf(stderr, "No such variable (%s) in the model.\n", argv[3]);
    for(i = 0; i < n_max; i++)
      free_timeseries(ts_set[i]);
    free(ts_set);
    free_model(model);
    return -1;
  }

  /* Print header row */
  for(j=0; j < NIP_CARDINALITY(v); j++){
    printf("%s", nip_variable_state_name(v, j));
    if(j+1 < NIP_CARDINALITY(v))
      printf(",");
    else
      printf("\n");
  }

  /* Use all the data (mark all variables) */
  for(i = 0; i < model->num_of_vars; i++)
    nip_mark_variable(model->variables[i]);

  /*****************/
  /* The inference */
  /*****************/
  fprintf(stderr, "  Computing...\n");
  loglikelihood = 0; /* init */

  for(i = 0; i < n_max; i++){
    /* the computation of posterior probabilities */
    ts = ts_set[i];
    ucs = forward_inference(ts, &v, 1, &probe);
    t = uncertainseries_length(ucs) - 1;
    posterior = get_posterior(ucs, v, t);

    // TODO: compute step likelihoods too?
    if(posterior)
      for(j=0; j < NIP_CARDINALITY(v); j++){
        printf("%g", log(posterior[j]));
        if(j+1 < NIP_CARDINALITY(v))
          printf(",");
        else
          printf("\n");
      }
    else
      fprintf(stderr, "Posterior of %s not found for series %d at %d\n",
              nip_variable_symbol(v), i, t);

    /* Compute average log likelihood */
    loglikelihood += probe / TIME_SERIES_LENGTH(ts);
    ts_progress(i, ts->length);

    /* Clean up */
    free_timeseries(ts_set[i]);
    free_uncertainseries(ucs);
    ucs = NULL;
    /* printf("\n"); // time series separator */
  }
  loglikelihood /= n_max;

  fprintf(stderr, "  Average log. likelihood = %g\n", loglikelihood);
  fprintf(stderr, "  ...computing done.\n"); /* new line for the prompt */

  /* free some memory */
  free(ts_set);
  free_model(model);

  return 0;
}
