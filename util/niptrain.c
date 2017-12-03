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
 * uses EM-algorithm for learning the parameters from the 
 * given data file and writes the estimated model to the 
 * specified output file.
 *
 * SYNOPSIS: 
 * NIPTRAIN <ORIGINAL.NET> <DATA.TXT> <THRESHOLD> <MINL> <RESULT.NET>
 *
 * - Structure of the model will be read from the file <ORIGINAL.NET>
 * - data for learning will be read from <DATA.TXT>
 * - <THRESHOLD> will provide the minimum change in average log. likelihood
 * - <MINL> sets the minimum average log. likelihood 
 *   (be careful not to demand too much)
 * - resulting model will be written to the file <RESULT.NET>
 *
 * EXAMPLE: ./niptrain model1.net data.txt 0.00001 -1.2 model2.net
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

  printf("niptrain:\n");

  if(argc < 6){
    printf("You must specify: \n"); 
    printf(" - the original NET file, \n");
    printf(" - data file, \n"); 
    printf(" - threshold value (0...1), \n");
    printf(" - minimum required log. likelihood/time step (<<0), and \n");
    printf(" - file name for the resulting model, please!\n");
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


#ifndef PRETTY_PRINT_CONVERTER_SKIPS_EM


  /* print a summary about the variables */
  ts = ts_set[0];
  printf("  Hidden variables are:\n");
  for(i = 0; i < ts->num_of_hidden; i++)
    printf("  %s", nip_variable_symbol(ts->hidden[i]));
  printf("\n  Observed variables are:\n");
  for(i = 0; i < model->num_of_vars - ts->num_of_hidden; i++)
    printf("  %s", nip_variable_symbol(ts->observed[i]));
  printf("\n");

  /* read the threshold value */
  threshold = strtod(argv[3], &tailptr);
  if(threshold <= 0.0 || threshold > 1  || tailptr == argv[3]){
    fprintf(stderr, "Specify a valid threshold value: %s?\n", argv[3]);
    for(i = 0; i < n; i++)
      free_timeseries(ts_set[i]);
    free(ts_set);
    free_model(model);
    return -1;
  }

  /* read the min log likelihood value */
  tailptr = NULL;
  min_log_likelihood = strtod(argv[4], &tailptr);
  if(min_log_likelihood >= 0.0 || tailptr == argv[4]){
    fprintf(stderr, "Specify a valid value for minimum log. likelihood");
    fprintf(stderr, " / time step: %s?\n", argv[4]);
    for(i = 0; i < n; i++)
      free_timeseries(ts_set[i]);
    free(ts_set);
    free_model(model);
    return -1;
  }

  /* THE algorithm (may take a while) */
  printf("  Computing... \n");

  seed = random_seed(NULL);
  printf("  Random seed = %ld\n", seed);

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
    if(NIP_LIST_LENGTH(learning_curve) == 0){
      printf("  Run %d failed 0.0  with 0 iterations, delta = 0.0 \n", t);
    }
    else{
      i = 1;

      /* Hack hack. This breaks the list abstraction... */
      link = learning_curve->first;
      while(link->fwd != NULL){
	link = link->fwd;
	i++;
      }
      last = link->data;
      
      if(link->bwd)
	printf("  Run %d reached %g  with %d iterations, delta = %g \n", 
	       t, last, i, last - link->bwd->data);
      else
	printf("  Run %d reached %g  with %d iterations, delta = 0.0 \n", 
	       t, last, i);
    }

    /* Try again, if not satisfied with the result */
  } while(e == NIP_ERROR_BAD_LUCK || 
	  last < min_log_likelihood);

  printf("  ...done.\n");

  /* Print the learning curve */
  link = learning_curve->first; t = 0;
  while(link != NULL){
    /* Reminder: rint() is NOT ANSI C. */
    printf("  Iteration %d: \t average loglikelihood = %g\n", t++, 
	   rint(link->data / threshold) * threshold);
    link = link->fwd;
  }



#endif /* PRETTY_PRINT_CONVERTER_SKIPS_EM */

  

  /* Write the results to a NET file */
  i =  write_model(model, argv[5]);
  if(i != NIP_NO_ERROR){
    fprintf(stderr, "Failed to write the model into %s\n", argv[5]);
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
