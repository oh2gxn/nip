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
 * NIPTRAIN <ORIGINAL.NET> <DATA.TXT> <SEED> <THRESHOLD> <MINL> <MAXI> <RESULT.NET>
 *
 * - Structure of the model will be read from the file <ORIGINAL.NET>
 * - data for learning will be read from <DATA.TXT>
 * - <SEED> will be used for initializing pseudo RNG, non-number for skipping init
 * - <THRESHOLD> will provide the minimum change in average log. likelihood
 * - <MINL> sets the minimum average log. likelihood
 *   (be careful not to demand too much)
 * - <MAXI> sets the maximum number of iterations, non-number for unlimited
 * - resulting model will be written to the file <RESULT.NET>
 *
 * EXAMPLE: ./niptrain model1.net data.txt 73 0.00001 -1.2 128 model2.net
 *
 * Author: Janne Toivola
 * Version: $Id: niptrain.c,v 1.1 2010-12-03 17:21:29 jatoivol Exp $
 */

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <float.h>
#include <math.h>
#include "nip.h"
#include "niplists.h"
#include "nipvariable.h"

/* Number of EM iterations before saving an intermediate model into file
 * avoiding loss of data during long runs */
#define BATCH_ITERATIONS 32L

// Callback for witnessing I/O
static int ts_progress(int sequence, int length);
static int ts_progress(int sequence, int length){
  fprintf(stderr, "  series %8d: %8d\r", sequence, length);
  return 0;
}

// Callback for witnessing EM progress
static int em_progress(nip_double_list learning_curve, double mean_log_likelihood);
static int em_progress(nip_double_list learning_curve, double mean_log_likelihood){
  fprintf(stderr, "                            iteration %4d: %16g\r",
          learning_curve->length+1, mean_log_likelihood);
  return nip_append_double(learning_curve, mean_log_likelihood);
}

int main(int argc, char *argv[]) {

  int i, n, k, e;
  nip_model model = NULL;
  time_series *ts_set = NULL;
  int n_ts;
  time_series ts;
  double threshold = 0;
  double min_log_likelihood = 0;
  double last = 0;
  nip_double_list learning_curve = NULL;
  nip_double_link link = NULL;
  nip_convergence stopping_criterion = LIKELIHOOD;
  char* tailptr = NULL;
  long seed;
  long max_iterations, current_iterations, left_iterations;
  int have_random_init;

  // TODO: version numbering scheme for checking compatibility
  fprintf(stderr, "niptrain:\n");

  // TODO: utilize getopt for proper optional command line arguments
  if(argc < 8){
    fprintf(stderr, "You must specify: \n");
    fprintf(stderr, " - the original NET file, \n");
    fprintf(stderr, " - data file, \n");
    fprintf(stderr, " - random seed (integer if random init), \n");
    fprintf(stderr, " - threshold value (0.0 ... 1.0), \n");
    fprintf(stderr, " - minimum required log. likelihood/time step (<<0.0), \n");
    fprintf(stderr, " - maximum number of iterations (int>3 if limited), and \n");
    fprintf(stderr, " - file name for the resulting model, please!\n");
    return 0;
  }

  /* read the model */
  fprintf(stderr, "  Reading model from %s... \n", argv[1]);
  model = parse_model(argv[1]);
  if(!model){
    fprintf(stderr, "Unable to parse the NET file: %s?\n", argv[1]);
    return -1;
  }
  fprintf(stderr, "  ...model found.\n");

  /* read the data */
  fprintf(stderr, "  Reading input data from %s... \n", argv[2]);
  n_ts = read_timeseries(model, argv[2], &ts_set, &ts_progress);
  if(n_ts == 0){
    fprintf(stderr, "Unable to parse the data file: %s?\n", argv[2]);
    free_model(model);
    return -1;
  }
  fprintf(stderr, "  ...%8d sequences found.\n", n_ts);

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
    for(i = 0; i < n_ts; i++)
      free_timeseries(ts_set[i]);
    free(ts_set);
    free_model(model);
    return -1;
  }

  /* read random seed value */
  seed = strtol(argv[3], &tailptr, 10);
  if(tailptr == argv[3]){
    seed = random_seed(NULL);
    have_random_init = 0;
    fprintf(stderr, "  Using model without randomization. Not seeding with %s\n", argv[3]);
  }
  else{
    seed = random_seed(&seed);
    have_random_init = 1;
  }
  fprintf(stderr, "  Random seed = %ld\n", seed);

  /* read the threshold value */
  threshold = strtod(argv[4], &tailptr);
  if(threshold <= 0.0 || threshold > 1  || tailptr == argv[4]){
    fprintf(stderr, "Specify a valid threshold value: %s?\n", argv[4]);
    for(i = 0; i < n_ts; i++)
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
    for(i = 0; i < n_ts; i++)
      free_timeseries(ts_set[i]);
    free(ts_set);
    free_model(model);
    return -1;
  }

  /* read max number of iterations value */
  max_iterations = strtol(argv[6], &tailptr, 10);
  if(tailptr == argv[6]){
    max_iterations = LONG_MAX;
    fprintf(stderr, "  Not limiting iterations. Not using %s\n", argv[6]);
  }
  fprintf(stderr, "  Max. number of iterations = %ld\n", max_iterations);

  /* THE algorithm (may take a while) */
  fprintf(stderr, "  Computing... \n");
  for(i = 0; i < model->num_of_vars; i++)
    nip_mark_variable(model->variables[i]); /* Make sure all the data is used */
  learning_curve = nip_new_double_list();
  n = 0;
  do{
    n++;
    last = 0; /* init */

    /* free the list if necessary */
    if(NIP_LIST_LENGTH(learning_curve) > 0){
      nip_empty_double_list(learning_curve);
    }

    /* EM algorithm, with intermediate save after each batch */
    k = 0;
    left_iterations = max_iterations;
    do{
      k++;
      current_iterations = (left_iterations > BATCH_ITERATIONS) ? BATCH_ITERATIONS : left_iterations;

      e = em_learn(model, ts_set, n_ts, have_random_init, current_iterations,
                   threshold, learning_curve, &stopping_criterion,
                   &em_progress, &ts_progress);
      if(!(e == NIP_NO_ERROR || e == NIP_ERROR_BAD_LUCK)){
        fprintf(stderr, "There were errors during learning:\n");
        nip_report_error(__FILE__, __LINE__, e, 1);
        for(i = 0; i < n_ts; i++)
          free_timeseries(ts_set[i]);
        free(ts_set);
        free_model(model);
        nip_empty_double_list(learning_curve);
        free(learning_curve);
        return -1;
      }
      left_iterations -= current_iterations; // maintain max cumulative count

      /* Write the results to a NET file */
      i =  write_model(model, argv[7]);
      if(i == NIP_NO_ERROR){
        fprintf(stderr, "\n  Wrote intermediate model into %s\n", argv[7]);
      }

      /* See if em_learn quit early due to threshold */
      switch (stopping_criterion) {
      case ITERATIONS : // max iteration limit reached
        have_random_init = 0; break; // learn more with the same model
        // TODO: minor drop in learning curve, but recovered during the 3 minimum iterations ?
      case DELTA : // true convergence
        left_iterations = 0; break; // drop remaining iterations
      // default : continue with the next batch
      }
    } while (left_iterations > 0);

    /* find out the last value in learning curve */
    // i = NIP_LIST_LENGTH(learning_curve);
    if(i == 0){
      fprintf(stderr, "  Run %4d failed 0.0 with %4d iterations, delta = 0.0 \n", n, i);
    }
    else{
      last = learning_curve->last->data;
      if(i > 1)
        fprintf(stderr, "  Run %4d reached %16g with %4d iterations, delta = %16g \n",
                n, last, i, last - learning_curve->last->bwd->data);
      else
        fprintf(stderr,"  Run %4d reached %16g with %4d iterations, delta = 0.0 \n",
                n, last, i);
    }

    /* Try again, if not satisfied with the result */
    have_random_init = 1; // cannot continue from the same model
  } while(e == NIP_ERROR_BAD_LUCK ||
          last < min_log_likelihood); // stopping_criterion == LIKELIHOOD

  fprintf(stderr, "  ...computing done.\n");
  for(i = 0; i < n_ts; i++)
    free_timeseries(ts_set[i]);
  free(ts_set);

  /* Print the learning curve: iteration number, average log. likelihood */
  link = learning_curve->first; n = 1;
  while(link != NULL){
    /* Reminder: rint() is NOT ANSI C. */
    printf("%d,%g\n", n++, rint(link->data / threshold) * threshold);
    link = link->fwd;
  }
  nip_empty_double_list(learning_curve);
  free(learning_curve);

  /* Write the results to a NET file */
  i =  write_model(model, argv[7]);
  if(i == NIP_NO_ERROR){
    fprintf(stderr, "  Wrote the final model into %s\n", argv[7]);
  }
  else {
    fprintf(stderr, "  Failed to write the model into %s\n", argv[7]);
    nip_report_error(__FILE__, __LINE__, i, 1);
    free_model(model);
    return -1;
  }
  free_model(model);

  return 0;
}
