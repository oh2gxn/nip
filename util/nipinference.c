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

/* nipinference.c
 *
 * SYNOPSIS:
 * NIPINFERENCE <MODEL.NET> <INPUT_DATA.TXT> <VARIABLE> <OUTPUT_DATA.TXT>
 *
 * Executes inference procedure with given model and time series.
 * Inferred probabilities for the selected variable are written to
 * the specified data file.
 *
 * EXAMPLE: ./nipinference filter.net data.txt A inferred_data.txt
 *
 * Author: Janne Toivola
 * Version: $Id: nipinference.c,v 1.2 2010-12-07 17:23:19 jatoivol Exp $
 */

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "nip.h"

/*
#define PRINT_CLIQUE_TREE
*/

// Callback for witnessing I/O
static int ts_progress(int sequence, int length);
static int ts_progress(int sequence, int length){
  fprintf(stderr, "  %d: %d\r", sequence, length);
  return 0;
}

int main(int argc, char *argv[]){

  int i, n_max;

  double probe, loglikelihood;

  nip_model model = NULL;
  nip_variable v = NULL;

  time_series ts = NULL;
  time_series *ts_set = NULL;
  uncertain_series *ucs_set = NULL;

  fprintf(stderr, "nipinference:\n");

  /*****************************************/
  /* Parse the model from a Hugin NET file */
  /*****************************************/
  if(argc < 4){
    fprintf(stderr, "Specify the names of the net file, input data file, ");
    fprintf(stderr, "variable, and output data file.\n");
    return 0;
  }
  else{
    model = parse_model(argv[1]);
  }

  if(model == NULL)
    return -1;

#ifdef PRINT_CLIQUE_TREE
  print_cliques(model);
#endif

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

  ucs_set = (uncertain_series*) calloc(n_max, sizeof(uncertain_series));
  if(!ucs_set){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_OUTOFMEMORY, 1);
    for(i = 0; i < n_max; i++)
      free_timeseries(ts_set[i]);
    free(ts_set);
    free_model(model);
    return -1;
  }

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
    free(ucs_set);
    free_model(model);
    return -1;
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
    ucs_set[i] = forward_backward_inference(ts, &v, 1, &probe);

    /* Compute average log likelihood */
    loglikelihood += probe / TIME_SERIES_LENGTH(ts);
    ts_progress(i, ts->length);
  }
  loglikelihood /= n_max;

  /* write the output */
  write_uncertainseries(ucs_set, n_max, v, argv[4]);

  fprintf(stderr, "  Average log. likelihood = %g\n", loglikelihood);
  fprintf(stderr, "  ...computing done.\n"); /* new line for the prompt */

  /* free some memory */
  for(i = 0; i < n_max; i++){
    free_timeseries(ts_set[i]);
    free_uncertainseries(ucs_set[i]);
  }
  free(ts_set);
  free(ucs_set);
  free_model(model);

  return 0;
}
