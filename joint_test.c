/*
 * joint_test.c $Id: joint_test.c,v 1.10 2005-06-23 11:07:34 jatoivol Exp $
 * Testing the calculation of joint probabilities.
 * Command line parameters: 1) a .net file, 2) data file with one step,
 * 3) names of wanted variables
 */

#include <stdlib.h>
#include <assert.h>
#include "parser.h"
#include "clique.h"
#include "variable.h"
#include "potential.h"
#include "errorhandler.h"
#include "nip.h"


int main(int argc, char *argv[]){

  int i, n;
  int num_of_vars = 0;
  potential result = NULL;

  nip model = NULL;
  time_series ts = NULL;
  time_series *ts_set = NULL;
  variable v = NULL;
  variable *vars = NULL;

  /*****************************************/
  /* Parse the model from a Hugin NET file */
  /*****************************************/

  /* -- Start parsing the network definition file */
  if(argc < 3){
    printf("Give the names of the net-file and data file please!\n");
    return 0;
  }
  
  /* read the model */
  model = parse_model(argv[1]);
  if(!model)
    return -1;
  use_priors(model, 1);

  /* read the data */
  n = read_timeseries(model, argv[2], &ts_set);
  if(n == 0)
    return -1;
  ts = ts_set[0];

  /**********************/
  /* Compute likelihood */
  /**********************/
  if(timeseries_length(ts) > 0)
    printf("Log. likelihood: %f \n", 
	   momentary_loglikelihood(model, ts->observed, ts->data[0], 
				   model->num_of_vars - ts->num_of_hidden));
  
  /* enter the evidence and distribute it */
  i = insert_ts_step(ts, 0, model);
  make_consistent(model);
  
  /* It's possible to select the variables as
   * the third...N'th command line parameters.
   */
  if(argc > 3){
    num_of_vars = argc - 3;
    vars = (variable *) calloc(num_of_vars, sizeof(variable));
    for(i = 0; i < num_of_vars; i++){
      v = model_variable(model, argv[3 + i]);
      if(!v){
	report_error(__FILE__, __LINE__, ERROR_INVALID_ARGUMENT, 1);
	fprintf(stderr, "  Variable %s unknown\n", argv[3 + i]);
	return -1;
      }
      vars[i] = v;
    }
  }
  else{ /* argc == 3 */
    /* else we just take the hidden variables */
    num_of_vars = ts->num_of_hidden;
    vars = ts->hidden;
  }
  /* The inputs have been parsed. -- */
  
  /**************************************************/
  /* The joint probability computation to be tested */
  /**************************************************/
  result = get_joint_probability(model, vars, num_of_vars);

  /* Print stuff */
  printf("P(");
  for(i = 0; i < num_of_vars-1; i++){
    printf("%s, ", get_symbol(vars[i]));
  }
  printf("%s) equals: \n", get_symbol(vars[num_of_vars-1]));
  print_potential(result);
  
  free(vars);
  free_potential(result);
  for(i = 0; i < n; i++)
    free_timeseries(ts_set[i]);
  free(ts_set);
  free_model(model);

  return 0;
}
