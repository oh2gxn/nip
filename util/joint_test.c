/*
 * joint_test.c $Id: joint_test.c,v 1.2 2004-08-31 16:12:57 mvkorpel Exp $
 * Testing the calculation of joint probabilities.
 * Command line parameters: 1) a .net file, 2) clique number (0 ... N - 1)
 * where N is the number of cliques.
 */

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
  int selected_Clique = -1;
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
  else{

    /* It's possible to select a Clique (0 -- num_of_cliques - 1) as
     * the second command line parameter.
     */
    if(argc >= 3)
      selected_Clique = atoi(argv[2]);
    model = parse_model(argv[1]);
  }

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
  if(selected_Clique >= 0 && model->num_of_cliques > selected_Clique){
    clique_of_interest = model->cliques[selected_Clique];
    num_of_vars = clique_of_interest->p->num_of_vars;

    vars = (Variable *) calloc(num_of_vars, sizeof(Variable));
    if(!vars){
      report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
      free_model(model);
      return 1;
    }

    /*
    for(i = 0; i < num_of_vars; i++)
      vars[i] = clique_of_interest->variables[num_of_vars - 1 - i];
    */

    /*
     * FIXME: Muuttujien järjestys vaikuttaa hommaan.
     * Ylempi (pois kommentoitu) toimii, tämä ei.
     * Koodasin kyllä siinä uskossa, että mikä tahansa muuttujien järjestys
     * tulisi toimimaan...
     */
    for(i = 0; i < num_of_vars; i++)
      vars[i] = clique_of_interest->variables[i];

    /* Let get_joint_probability print the result */
    result = get_joint_probability(model, vars, num_of_vars, 1);

    free(vars);
    free(result);
  }

  free_model(model);

  return 0;
}
