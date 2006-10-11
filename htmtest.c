/* htmtest.c
 *
 * Experimental code for handling the inference with time slices. 
 * Prints a MAP estimate for the hidden variables during the first 
 * time series in the given data file.
 *
 * SYNOPSIS: HTMTEST <MODEL.NET> <DATA.TXT>
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

int main(int argc, char *argv[]){

  int i, j, n, t = 0;

  nip model = NULL;
  variable temp = NULL;

  time_series ts = NULL;
  time_series *ts_set = NULL;
  uncertain_series ucs = NULL;

  /*****************************************/
  /* Parse the model from a Hugin NET file */
  /*****************************************/
  /* -- Start parsing the network definition file */
  if(argc < 3){
    printf("Give the names of the net-file and data file, please!\n");
    return 0;
  }
  else{
    model = parse_model(argv[1]);
  }

  if(model == NULL)
    return -1;
  /* The input file has been parsed. -- */

  use_priors(model, 0); /* Only to be sure... */

  /*****************************/
  /* read the data from a file */
  /*****************************/
  n = read_timeseries(model, argv[2], &ts_set);

  if(n < 1){
    report_error(__FILE__, __LINE__, ERROR_INVALID_ARGUMENT, 1);
    fprintf(stderr, "%s\n", argv[2]);
    free_model(model);
    return -1;
  }

  ts = ts_set[0];

  /** DEBUG **/
  printf("Observed variables:\n  ");
  for(i = 0; i < model->num_of_vars - ts->num_of_hidden; i++)
    printf("%s ", ts->observed[i]->symbol);
  printf("\nHidden variables:\n  ");
  for(i = 0; i < ts->num_of_hidden; i++)
    printf("%s ", ts->hidden[i]->symbol);
  printf("\n");
  /*print_cliques(model);*/
  printf("\n");

  /*****************/
  /* Forward phase */
  /*****************/

  printf("## Forward phase ##\n");  

  ucs = forward_inference(ts, ts->hidden, ts->num_of_hidden);

  for(t = 0; t < ucs->length; t++){ /* FOR EVERY TIMESLICE */
    
    printf("-- t = %d --\n", t+1);

    /* Print some intermediate results */
    for(i = 0; i < ucs->num_of_vars; i++){
      temp = ucs->variables[i];            
      /* Print a value */
      for(j = 0; j < number_of_values(temp); j++)
	printf("P(%s=%s) = %f\n", get_symbol(temp),
	       (temp->statenames)[j], ucs->data[t][i][j]);
      printf("\n"); 
    }
  }  

  
  /******************/
  /* Backward phase */
  /******************/

  printf("## Backward phase ##\n");  

  free_uncertainseries(ucs); /* REMEMBER THIS */

  ucs = forward_backward_inference(ts, ts->hidden, ts->num_of_hidden);

  /* forget old evidence */
  reset_model(model);
  use_priors(model, 0);
 
  for(t = 0; t < ucs->length; t++){ /* FOR EVERY TIMESLICE */
    
    printf("-- t = %d --\n", t+1);
    
    /* Print the final results */
    for(i = 0; i < ucs->num_of_vars; i++){
      temp = ucs->variables[i];
      /* Print a value */
      for(j = 0; j < number_of_values(temp); j++)
	printf("P(%s=%s) = %f\n", get_symbol(temp),
	       (temp->statenames)[j], ucs->data[t][i][j]);
      printf("\n"); 
    }
  }

  for(i = 0; i < n; i++)
    free_timeseries(ts_set[i]);
  free(ts_set);
  free_uncertainseries(ucs);
  free_model(model);
  
  return 0;
}
