/* loo_prediction_test.c
 * 
 * Executes leave-one-out testing on the prediction accuracy. 
 * For each time series, a model is estimated from the rest 
 * of the data sequences and predictive inference is done for 
 * a given variable... Inference results are written to a file.
 *

 * TODO: more parameters, like the threshold for EM...

 * SYNOPSIS: 
 * LOO_PREDICTION_TEST <MODEL.NET> <INPUT_DATA.TXT> <VAR> <OUTPUT_DATA.TXT>
 *
 * EXAMPLE: ./loo_prediction_test model.net data.txt A inferred_data.txt
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

  int i, n_max;

  double probe, loglikelihood;

  nip model = NULL;
  variable v = NULL;

  time_series ts = NULL;
  time_series *ts_set = NULL;
  uncertain_series *ucs_set = NULL;

  /*****************************************/
  /* Parse the model from a Hugin NET file */
  /*****************************************/
  if(argc < 4){
    printf("Specify the names of the net file, input data file, ");
    printf("variable, and output data file.\n");
    return 0;
  }
  else{
    model = parse_model(argv[1]);
  }

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
  if(!ucs_set){
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
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
    report_error(__FILE__, __LINE__, ERROR_INVALID_ARGUMENT, 1);
    fprintf(stderr, "No such variable in the model.\n");
    for(i = 0; i < n_max; i++)
      free_timeseries(ts_set[i]);
    free(ts_set);
    free(ucs_set);
    free_model(model);
    return -1;
  }
  
  /*****************/
  /* The inference */
  /*****************/
  printf("## Computing ##\n");  
  loglikelihood = 0; /* init */

  for(i = 0; i < n_max; i++){



    /* TODO: leaving one time series out and running EM on the rest... */
    printf("Error: Not yet implemented...\n");
    return 1;
    


    /* the computation of posterior probabilities */
    ts = ts_set[i];


    /* FIXME: what about the variable..? it is probably observed in ts! 
     * Should f_b_inference ignore the data about the variable..?       */
    ucs_set[i] = forward_backward_inference(ts, &v, 1, &probe);


    /* Compute average log likelihood */
    loglikelihood += probe / TIME_SERIES_LENGTH(ts);

    /* forget old evidence */
    reset_model(model);
    use_priors(model, 0);
  }
  loglikelihood /= n_max;

  /* write the output */
  write_uncertainseries(ucs_set, n_max, v, argv[4]);

  printf("  Average log. likelihood = %g\n", loglikelihood);
  printf("done.\n"); /* new line for the prompt */

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
