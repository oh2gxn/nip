#include <stdlib.h>
#include <assert.h>
#include "parser.h"
#include "Clique.h"
#include "Variable.h"
#include "potential.h"
#include "errorhandler.h"
#include "nip.h"


int main(int argc, char *argv[]){

  int i;
  int num_of_vars;
  double *result;

  Nip model = NULL;
  Clique clique_of_interest = NULL;
  Variable *vars = NULL;

  /*****************************************/
  /* Parse the model from a Hugin NET file */
  /*****************************************/

  /* -- Start parsing the network definition file */
  if(argc < 2){
    printf("Give the name of the net-file, please!\n");
    return 0;
  }
  else
    model = parse_model(argv[1]);

  if(model == NULL)
    return -1;
  /* The input file has been parsed. -- */

  print_Cliques(model);

  /********************/
  /* Do the inference */
  /********************/
    
  make_consistent(model);
    
  /*********************************/
  /* Check the result of inference */
  /*********************************/
    
  /* Let's take the third Clique and check something */
  if(model->num_of_cliques >= 5){
    clique_of_interest = model->cliques[4];
    num_of_vars = clique_of_interest->p->num_of_vars;

    vars = (Variable *) calloc(num_of_vars, sizeof(Variable));
    if(!vars){
      report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
      free_model(model);
      return 1;
    }

    for(i = 0; i < num_of_vars; i++)
      vars[i] = clique_of_interest->variables[num_of_vars - 1 - i];

    result = get_joint_probability(model, vars, num_of_vars, 1);
    

    free(vars);
  }
    
  free_model(model);

  return 0;
}
