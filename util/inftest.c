/* inftest.c
 * 
 * Executes inference procedure with given model and time series. 
 * Inferred probabilities for the selected variable are written to 
 * the specified data file. NOTE: only the first time series from the 
 * input data is used.
 *
 * SYNOPSIS: INFTEST <MODEL.NET> <INPUT_DATA.TXT> <VARIABLE> <OUTPUT_DATA.TXT>
 *
 * EXAMPLE: ./inftest filter.net data.txt A inferred_data.txt
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

/*
#define PRINT_CLIQUE_TREE
*/


int main(int argc, char *argv[]){

  int i, n_max;

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

#ifdef PRINT_CLIQUE_TREE
  print_cliques(model);
#endif

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
  
  /*******************************************/
  /* The inference for the first time series */
  /*******************************************/
  printf("## Computing ##\n");  

  for(i = 0; i < n_max; i++){
    /* the computation of posterior probabilities */
    ts = ts_set[i];
    ucs_set[i] = forward_backward_inference(ts, &v, 1);

    /* forget old evidence */
    reset_model(model);
    use_priors(model, 0);
  }

  /* write the output */
  write_uncertainseries(ucs_set, n_max, v, argv[4]);

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
