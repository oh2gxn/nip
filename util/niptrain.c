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

/* niptrain.c
 *
 * Reads the structure for the model from a Hugin NET file,
 * uses EM-algorithm for learning the parameters
 * from the given data file and writes the estimated model
 * to the specified output file.
 *
 * SYNOPSIS:
 * NIPTRAIN <ORIGINAL.NET> <DATA.TXT> <SEED> <THRESHOLD> <MINL> <RESULT.NET>
 *
 * - Structure of the model will be read from the file <ORIGINAL.NET>
 * - data for learning will be read from <DATA.TXT>
 * - <SEED> will be used for initializing pseudo RNG
 * - <THRESHOLD> will provide the minimum change in average log. likelihood
 * - <MINL> sets the minimum average log. likelihood
 *   (be careful not to demand too much)
 * - resulting model will be written to the file <RESULT.NET>
 *
 * EXAMPLE: ./niptrain model1.net data.txt 73 0.00001 -1.2 model2.net
 *
 * Author: Janne Toivola
 * Version: $Id: niptrain.c,v 1.1 2010-12-03 17:21:29 jatoivol Exp $
 */

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <float.h>
#include <math.h>
#include "nip.h"
#include "niplists.h"
#include "nipvariable.h"

int main(int argc, char *argv[]) {

  int i, n, t, e;
  nip_model model = NULL;
  time_series *ts_set = NULL;
  time_series ts;
  double threshold = 0;
  double min_log_likelihood = 0;
  double last = 0;
  nip_double_list learning_curve = NULL;
  nip_double_link link = NULL;
  char* tailptr = NULL;
  long seed;

  fprintf(stderr, "niptrain:\n");

  if(argc < 7){
    fprintf(stderr, "You must specify: \n");
    fprintf(stderr, " - the original NET file, \n");
    fprintf(stderr, " - data file, \n");
    fprintf(stderr, " - random seed (integer), \n");
    fprintf(stderr, " - threshold value (0...1), \n");
    fprintf(stderr, " - minimum required log. likelihood/time step (<<0), and \n");
    fprintf(stderr, " - file name for the resulting model, please!\n");
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

  /* print a summary about the variables */
  ts = ts_set[0];
  fprintf(stderr, "  Hidden variables are:\n");
  for(i = 0; i < ts->num_of_hidden; i++)
    fprintf(stderr, "  %s", nip_variable_symbol(ts->hidden[i]));
  fprintf(stderr, "\n  Observed variables are:\n");
  for(i = 0; i < model->num_of_vars - ts->num_of_hidden; i++)
    fprintf(stderr, "  %s", nip_variable_symbol(ts->observed[i]));
  fprintf(stderr, "\n");
  if(model->num_of_vars == ts->num_of_hidden){
    fprintf(stderr, "No relevant data columns: check the header row.\n");
    for(i = 0; i < n; i++)
      free_timeseries(ts_set[i]);
    free(ts_set);
    free_model(model);
    return -1;
  }

  /* read random seed value */
  seed = strtol(argv[3], &tailptr, 10);
  if(tailptr == argv[3]){
    fprintf(stderr, "Specify a valid random seed: %s?\n", argv[3]);
    for(i = 0; i < n; i++)
      free_timeseries(ts_set[i]);
    free(ts_set);
    free_model(model);
    return -1;
  }
  seed = random_seed(&seed);
  fprintf(stderr, "  Random seed = %ld\n", seed);

  /* read the threshold value */
  threshold = strtod(argv[4], &tailptr);
  if(threshold <= 0.0 || threshold > 1  || tailptr == argv[4]){
    fprintf(stderr, "Specify a valid threshold value: %s?\n", argv[4]);
    for(i = 0; i < n; i++)
      free_timeseries(ts_set[i]);
    free(ts_set);
    free_model(model);
    return -1;
  }

  /* read the min log likelihood value */
  tailptr = NULL;
  min_log_likelihood = strtod(argv[5], &tailptr);
  if(min_log_likelihood >= 0.0 || tailptr == argv[5]){
    fprintf(stderr, "Specify a valid value for minimum log. likelihood");
    fprintf(stderr, " / time step: %s?\n", argv[5]);
    for(i = 0; i < n; i++)
      free_timeseries(ts_set[i]);
    free(ts_set);
    free_model(model);
    return -1;
  }

  /* THE algorithm (may take a while) */
  fprintf(stderr, "  Computing... \n");
  for(i = 0; i < model->num_of_vars; i++)
    nip_mark_variable(model->variables[i]); /* Make sure all the data is used */
  learning_curve = nip_new_double_list();
  t = 0;
  do{
    t++;
    last = 0; /* init */

    /* free the list if necessary */
    if(NIP_LIST_LENGTH(learning_curve) > 0){
      nip_empty_double_list(learning_curve);
    }

    /* EM algorithm */
    e = em_learn(ts_set, n, threshold, learning_curve);
    if(!(e == NIP_NO_ERROR || e == NIP_ERROR_BAD_LUCK)){
      fprintf(stderr, "There were errors during learning:\n");
      nip_report_error(__FILE__, __LINE__, e, 1);
      for(i = 0; i < n; i++)
        free_timeseries(ts_set[i]);
      free(ts_set);
      free_model(model);
      nip_empty_double_list(learning_curve);
      free(learning_curve);
      return -1;
    }

    /* find out the last value in learning curve */
    i = NIP_LIST_LENGTH(learning_curve);
    if(i == 0){
      fprintf(stderr, "  Run %d failed 0.0  with %d iterations, delta = 0.0 \n", t, i);
    }
    else{
      last = learning_curve->last->data;
      if(i > 1)
        fprintf(stderr, "  Run %d reached %g  with %d iterations, delta = %g \n",
                t, last, i, last - learning_curve->last->bwd->data);
      else
        fprintf(stderr,"  Run %d reached %g  with %d iterations, delta = 0.0 \n",
                t, last, i);
    }

    /* Try again, if not satisfied with the result */
  } while(e == NIP_ERROR_BAD_LUCK ||
          last < min_log_likelihood);

  fprintf(stderr, "  ...done.\n");

  /* Print the learning curve: iteration number, average log. likelihood */
  link = learning_curve->first; t = 1;
  while(link != NULL){
    /* Reminder: rint() is NOT ANSI C. */
    printf("%d,%g\n", t++, rint(link->data / threshold) * threshold);
    link = link->fwd;
  }

  /* Write the results to a NET file */
  i =  write_model(model, argv[6]);
  if(i != NIP_NO_ERROR){
    fprintf(stderr, "Failed to write the model into %s\n", argv[6]);
    nip_report_error(__FILE__, __LINE__, i, 1);
    nip_empty_double_list(learning_curve);
    free(learning_curve);
    for(i = 0; i < n; i++)
      free_timeseries(ts_set[i]);
    free(ts_set);
    free_model(model);
    return -1;
  }

  nip_empty_double_list(learning_curve);
  free(learning_curve);
  for(i = 0; i < n; i++)
    free_timeseries(ts_set[i]);
  free(ts_set);
  free_model(model);

  return 0;
}
