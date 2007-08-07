/* loo_prediction_test.c
 * 
 * Executes leave-one-out testing on the prediction accuracy. 
 * For each time series, a model is estimated from the rest 
 * of the data sequences and predictive inference is done for 
 * a given variable... The EM learning algorithm requires the 
 * stopping threshold and minimum likelihood parameters. 
 * Inference results are written to a file.
 *
 * SYNOPSIS: 
 * LOO_PREDICTION_TEST <MODEL.NET> <INPUT_DATA.TXT> \ 
 *                     <THRESHOLD> <MINL> <VAR> <OUTPUT_DATA.TXT>
 *
 * EXAMPLE: 
 * ./loo_prediction_test model.net data.txt 0.00001 -1.2 A inferred_data.txt
 */

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "parser.h"
#include "clique.h"
#include "variable.h"
#include "potential.h"
#include "errorhandler.h"
#include "nip.h"

/* a lot of similarities with the inference tool (inftest)... */

int main(int argc, char *argv[]){

  int e, i, j, n_max;

  long seed;

  double probe, loglikelihood;
  double threshold, min_log_likelihood, last;

  char* tailptr = NULL;

  nip model = NULL;
  variable v = NULL;

  time_series ts = NULL;
  time_series *ts_set = NULL;
  time_series *loo_set = NULL; /* just a bunch of temporary pointers */
  uncertain_series *ucs_set = NULL;

  doublelist learning_curve = NULL;
  doublelink link = NULL;

  /*****************************************/
  /* Parse the model from a Hugin NET file */
  /*****************************************/
  if(argc < 6){
    printf("Specify the names of the net file, input data file, ");
    printf("EM stopping threshold, minimum average likelihood, ");
    printf("variable of interest, and output data file.\n");
    return 0;
  }

  model = parse_model(argv[1]);

  if(model == NULL)
    return -1;

  use_priors(model, 0); /* Only to be sure... */

  /*****************************/
  /* read the data from a file */
  /*****************************/
  n_max = read_timeseries(model, argv[2], &ts_set);

  if(n_max < 1){
    report_error(__FILE__, __LINE__, ERROR_INVALID_ARGUMENT, 1);
    fprintf(stderr, "%s\n", argv[2]);
    free_model(model);
    return -1;
  }

  ucs_set = (uncertain_series*) calloc(n_max, sizeof(uncertain_series));
  loo_set = (time_series*) calloc(n_max-1, sizeof(time_series));
  if(!ucs_set || !loo_set){
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    for(i = 0; i < n_max; i++)
      free_timeseries(ts_set[i]);
    free(ts_set);
    free_model(model);
    return -1;
  }

  /* read the threshold parameter */
  threshold = strtod(argv[3], &tailptr);
  if(threshold <= 0.0 || threshold > 1  || tailptr == argv[3]){
    fprintf(stderr, "Specify a valid threshold value: %s?\n", argv[3]);
    for(i = 0; i < n_max; i++)
      free_timeseries(ts_set[i]);
    free(ts_set);
    free(ucs_set);
    free(loo_set);
    free_model(model);
    return -1;
  }

  /* read the minimum average likelihood parameter */
  tailptr = NULL;
  min_log_likelihood = strtod(argv[4], &tailptr);
  if(min_log_likelihood >= 0.0 || tailptr == argv[4]){
    fprintf(stderr, "Specify a valid value for minimum log. likelihood");
    fprintf(stderr, " / time step: %s?\n", argv[4]);
    for(i = 0; i < n_max; i++)
      free_timeseries(ts_set[i]);
    free(ts_set);
    free(ucs_set);
    free(loo_set);
    free_model(model);
    return -1;
  }

  /****************/
  /* the variable */
  /****************/
  v = model_variable(model, argv[5]);
  if(!v){
    report_error(__FILE__, __LINE__, ERROR_INVALID_ARGUMENT, 1);
    fprintf(stderr, "No such variable in the model.\n");
    for(i = 0; i < n_max; i++)
      free_timeseries(ts_set[i]);
    free(ts_set);
    free(ucs_set);
    free(loo_set);
    free_model(model);
    return -1;
  }
  
  /*****************/
  /* The inference */
  /*****************/
  printf("## Computing ##\n");  
  loglikelihood = 0; /* init */
  seed = random_seed(NULL);
  printf("  Random seed = %ld\n", seed);
  learning_curve = make_doublelist();

  for(i = 0; i < n_max; i++){

    /* leave-one-out operation */
    for(j = 0; j < n_max; j++){
      if(j < i)
	loo_set[j] = ts_set[j];
      if(j > i)
	loo_set[j-1] = ts_set[j];
    }

    /* Use all the data (mark all variables) in EM */
    for(j = 0; j < model->num_of_vars; j++)
      mark_variable(model->variables[j]);

    /* repeat EM until good enough */
    do{
      last = 0; /* init */

      /* free the list if necessary */
      if(LIST_LENGTH(learning_curve) > 0)
	empty_doublelist(learning_curve);
      
      /* the EM algorithm */
      e = em_learn(loo_set, n_max-1, threshold, learning_curve);
      if(!(e == NO_ERROR || e == ERROR_BAD_LUCK)){
	fprintf(stderr, "There were errors during learning:\n");
	report_error(__FILE__, __LINE__, e, 1);
	for(i = 0; i < n_max; i++)
	  free_timeseries(ts_set[i]);
	free(ts_set);
	free(ucs_set);
	free(loo_set);
	free_model(model);
	empty_doublelist(learning_curve);
	free(learning_curve);
	return -1;
      }
      
      /* find out the last value in learning curve */
      if(LIST_LENGTH(learning_curve) > 0){

	/* Hack hack. This breaks the list abstraction... */
	link = learning_curve->first;
	while(link->fwd != NULL)
	  link = link->fwd;
	last = link->data;
      }

    } while(e == ERROR_BAD_LUCK || last < min_log_likelihood);

    /* the computation of posterior probabilities */
    ts = ts_set[i];
    
    /* ignore possible evidence about the variable to be predicted */
    unmark_variable(v);

    /* Run the inference procedure */
    ucs_set[i] = forward_backward_inference(ts, &v, 1, &probe);

    /* Compute average log likelihood */
    loglikelihood += probe / TIME_SERIES_LENGTH(ts);

    /* forget old evidence */
    reset_model(model);
    use_priors(model, 0);
  }
  loglikelihood /= n_max;

  /* write the output */
  write_uncertainseries(ucs_set, n_max, v, argv[6]);

  printf("  Average log. likelihood = %g\n", loglikelihood);
  printf("done.\n"); /* new line for the prompt */

  /* free some memory */
  if(LIST_LENGTH(learning_curve) > 0)
    empty_doublelist(learning_curve);
  free(learning_curve);
  for(i = 0; i < n_max; i++){
    free_timeseries(ts_set[i]);
    free_uncertainseries(ucs_set[i]);
  }
  free(ts_set);
  free(ucs_set);
  free(loo_set);
  free_model(model);
  
  return 0;
}
