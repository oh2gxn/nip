#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "parser.h"
#include "clique.h"
#include "variable.h"
#include "potential.h"
#include "errorhandler.h"
#include "nip.h"

/***********************************************************
 * The timeslice concept features some major difficulties 
 * because the actual calculations are done in the join tree
 * instead of the graph. The program should be able to 
 * figure out how the join tree repeats itself and store 
 * some kind of sepsets between the timeslices... Note that 
 * there can be only one sepset between two adjacent 
 * timeslices, because the join tree can't have loops. This 
 * implies that the variables, which have links to the 
 * variables in the next timeslice, can be found in the 
 * same clique.
 *
 * (JJT: Currently, the problems seem to be solved.)
 */

/*
#define TEST
#define TEST2
*/

int main(int argc, char *argv[]){

  int i, j, n, t = 0;

  nip model = NULL;
  variable temp = NULL;

  time_series ts = NULL;
  time_series *ts_set = NULL;
  uncertain_series ucs = NULL;

  /*************************************/
  /* Some experimental timeslice stuff */
  /*************************************/
  
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

  /*****************/
  /* Forward phase */
  /*****************/

  /* Try this kind of iteration:
   * + put data in
   * + pass the message from previous sepset to the next one
   * + print the result
   * + reset model
   */

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
    free_timeseries(ts_set[0]);
  free(ts_set);
  free_uncertainseries(ucs);
  free_model(model);
  
  return 0;
}
