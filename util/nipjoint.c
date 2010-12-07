/* nipjoint.c 
 *
 * Computes joint probabilities...
 *
 * SYNOPSIS:
 * NIPJOINT <MODEL.NET> <DATA.TXT> <VAR1 VAR2...>
 *
 * - The model will be read from file <MODEL.NET>, 
 * - one time step worth of data will be read from <DATA.TXT>,
 * - optional parameters <VAR1...> are the symbols of the variables 
 *   whose probability distribution (given the data) will be computed
 *
 * EXAMPLE: ./nipjoint model.net data.txt height weight
 *
 * Author: Janne Toivola
 * Version: $Id: nipjoint.c,v 1.2 2010-12-07 17:23:19 jatoivol Exp $
 */

#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include "nipparsers.h"
#include "nipjointree.h"
#include "nipvariable.h"
#include "nippotential.h"
#include "niperrorhandler.h"
#include "nip.h"


int main(int argc, char *argv[]){

  int i, n;
  int num_of_vars = 0;
  nip_potential result = NULL;

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
  use_priors(model, !NIP_HAD_A_PREVIOUS_TIMESLICE);

  /* read the data */
  n = read_timeseries(model, argv[2], &ts_set);
  if(n == 0)
    return -1;
  ts = ts_set[0];

  make_consistent(model); /* no side effects ? */

  /* compute probability mass before entering evidence */
  m1 = nip_probability_mass(model->cliques, model->num_of_cliques);

  /* enter the evidence... */
  i = insert_ts_step(ts, 0, model, NIP_MARK_BOTH);
  /* ...and distribute it */
  make_consistent(model);

  /* compute probability mass after making the model consistent */
  m2 = nip_probability_mass(model->cliques, model->num_of_cliques);
  
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
    printf("%s, ", nip_variable_symbol(vars[i]));
  }
  printf("%s) equals: \n", nip_variable_symbol(vars[num_of_vars-1]));
  nip_fprintf_potential(stdout, result);

  printf("Marginal probability before evidence: m1 = %g\n", m1);
  printf("Marginal probability after evidence : m2 = %g\n", m2);
  printf("Log. likelihood: ln(m2/m1) = %g\n", log(m2/m1));
  
  free(vars);
  nip_free_potential(result);
  for(i = 0; i < n; i++)
    free_timeseries(ts_set[i]);
  free(ts_set);
  free_model(model);

  return 0;
}
