/*
 * joint_test.c $Id: joint_test.c,v 1.16 2010-11-09 19:06:09 jatoivol Exp $
 * Testing the calculation of joint probabilities.
 * Command line parameters: 
 * 1) a .net file, 
 * 2) data file with one step,
 * 3) names of wanted variables
 */

#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include "parser.h"
#include "clique.h"
#include "nipvariable.h"
#include "potential.h"
#include "niperrorhandler.h"
#include "nip.h"


int main(int argc, char *argv[]){

  int i, n;
  int num_of_vars = 0;
  potential result = NULL;

  nip model = NULL;
  time_series ts = NULL;
  time_series *ts_set = NULL;
  nip_variable v = NULL;
  nip_variable *vars = NULL;

  double m1, m2;

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
  use_priors(model, !HAD_A_PREVIOUS_TIMESLICE);

  /* read the data */
  n = read_timeseries(model, argv[2], &ts_set);
  if(n == 0)
    return -1;
  ts = ts_set[0];

  make_consistent(model); /* no side effects ? */

  /* compute probability mass before entering evidence */
  m1 = probability_mass(model->cliques, model->num_of_cliques);

  /* enter the evidence... */
  i = insert_ts_step(ts, 0, model, NIP_MARK_BOTH);
  /* ...and distribute it */
  make_consistent(model);

  /* compute probability mass after making the model consistent */
  m2 = probability_mass(model->cliques, model->num_of_cliques);
  
  /* It's possible to select the variables as
   * the third...N'th command line parameters.
   */
  if(argc > 3){
    num_of_vars = argc - 3;
    vars = (nip_variable *) calloc(num_of_vars, sizeof(nip_variable));
    for(i = 0; i < num_of_vars; i++){
      v = model_variable(model, argv[3 + i]);
      if(!v){
	nip_report_error(__FILE__, __LINE__, NIP_ERROR_INVALID_ARGUMENT, 1);
	fprintf(stderr, "  Variable %s unknown\n", argv[3 + i]);
	return -1;
      }
      vars[i] = v;
    }
  }
  else{ /* argc == 3 */
    if(ts->num_of_hidden > 0){
      /* else we just take the hidden variables */
      num_of_vars = ts->num_of_hidden;
      vars = ts->hidden;
    }
    else{
      /* or only the first observed variable */
      num_of_vars = 1;
      vars = ts->observed;
    }
  }
  /* The inputs have been parsed. -- */
  
  /**************************************************/
  /* The joint probability computation to be tested */
  /**************************************************/
  result = get_joint_probability(model, vars, num_of_vars);

  /* Print stuff */
  printf("P(");
  for(i = 0; i < num_of_vars-1; i++){
    printf("%s, ", nip_get_symbol(vars[i]));
  }
  printf("%s) equals: \n", nip_get_symbol(vars[num_of_vars-1]));
  print_potential(result);

  printf("Mass before evidence: m1 = %g\n", m1);
  printf("Mass after evidence : m2 = %g\n", m2);
  printf("Log. likelihood: ln(m2/m1) = %g\n", log(m2/m1));
  
  free(vars);
  free_potential(result);
  for(i = 0; i < n; i++)
    free_timeseries(ts_set[i]);
  free(ts_set);
  free_model(model);

  return 0;
}
