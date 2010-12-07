/* Test program for the Hugin Net parser
 * Author: Janne Toivola
 * Version: $Id: bisontest.c,v 1.53 2010-12-07 17:23:19 jatoivol Exp $ 
 */

#include <stdlib.h>
#include "niplists.h"
#include "niperrorhandler.h"
#include "nipvariable.h"
#include "nippotential.h"
#include "nipjointree.h"
#include "nipparsers.h"
#include "huginnet.tab.h"

/*
#define PRINT_POTENTIALS
*/

/*
#define DEBUG_BISONTEST
*/

/*
#define EVIDENCE
*/

/*
#define EVIDENCE1
*/

/*
#define EVIDENCE2
*/

#define EVIDENCE_SOTKU

#define TEST_RETRACTION

/*
#define PRINT_JOINTREE
*/

FILE *open_net_file(const char *filename);
void close_net_file();
int yyparse();
nip_variable_list get_parsed_variables();
int get_cliques(nip_clique** clique_array_pointer);
/* Reminder: extern keyword is for declaring external _variables_ */

/*
 * Calculate the probability distribution of variable "var".
 * The family of var must be among the cliques in cliques[]
 * (the size of which is num_of_cliques).
 * This function allocates memory for the array "result". The size of
 * the array is returned in "size_of_result".
 */
static void test_probability(double** result, int* size_of_result,
			     nip_variable var, nip_clique cliques[],
			     int num_of_cliques){

  /* Find the clique that contains the family of the interesting variable. */
  nip_clique clique_of_interest = 
    nip_find_family(cliques, num_of_cliques, var);
  if(!clique_of_interest){
    printf("In bisontest.c : No clique found! Sorry.\n");
    *size_of_result = 0;
    return;
  }  

  *size_of_result = NIP_CARDINALITY(var);

  /* Allocate memory for the result */
  *result = (double *) calloc(*size_of_result, sizeof(double));
  if(!result){
    printf("In bisontest.c : Calloc failed.\n");
    *size_of_result = 0;
    return;
  }

  /* marginalisation */
  nip_marginalise_clique(clique_of_interest, var, *result);

  /* normalisation */
  nip_normalise_array(*result, *size_of_result);

}

#ifdef EVIDENCE
/*
 * Enter some evidence of variable "observed".
 */
static void test_evidence(nip_variable* vars, int nvars, nip_clique* cliques, 
			  int num_of_cliques, variable observed, 
			  double data[]){

#ifdef DEBUG_BISONTEST
  int evidence_retval;
#endif

#ifndef DEBUG_BISONTEST
  /* FIXME (copy some code from parser.c) */
  nip_enter_evidence(vars, nvars, cliques, num_of_cliques, 
		     observed, data);
#endif

#ifdef DEBUG_BISONTEST
  /* FIXME (copy some code from parser.c) */
  evidence_retval = nip_enter_evidence(vars, nvars, cliques, num_of_cliques, 
				       observed, data);
  printf("\n\nEntered evidence into ");
  print_clique(clique_of_interest);
  printf("enter_evidence returned %d.\n", evidence_retval);
#endif /* DEBUG_BISONTEST */

}
#endif /* EVIDENCE */

int main(int argc, char *argv[]){

  char *var_name;
  int i, retval;
  int size_of_result;
  
  double* result;

  nip_clique* cliques;
  int num_of_cliques;

  nip_variable interesting;
  nip_variable* vars;
  int nvars;
  nip_variable_list var_list;
  nip_variable_iterator it;

#ifdef PRINT_POTENTIALS
  int j;
#endif

#ifdef DEBUG_BISONTEST
  int temp;
#endif

#ifdef EVIDENCE

#ifdef EVIDENCE1
  double probB[] = {0.25, 0.25, 0.40, 0.10};
  double probD[] = {0.2, 0.3, 0.5};
#endif /* EVIDENCE1 */

#ifdef EVIDENCE2
  double probB[] = {0.75, 0.21, 0.03, 0.01};
  double probD[] = {0.6, 0.1, 0.3};
#endif /* EVIDENCE2 */

#ifdef EVIDENCE_SOTKU
  double probC1[] = { 0.395, 0.605 };
  double probC4[] = { 0.018, 0.982 };
  double probC19[] = { 0.492, 0.508 };

  double probC1_h[] = { 0, 1 };
  double probC4_h[] = { 1, 0 };
  double probC19_h[] = { 0.2, 0.8 };
#endif /* EVIDENCE_SOTKU */

  nip_variable observed[3];
  double *probs[3];
#endif /* EVIDENCE */

#ifdef EVIDENCE

#ifdef TEST_RETRACTION
  probs[0] = probC1_h;
  probs[1] = probC4_h;
  probs[2] = probC19_h;
#endif

#ifndef EVIDENCE_SOTKU
  probs[0] = probB;
  probs[1] = probD;
#endif

#endif /* EVIDENCE */

  /* -- Start parsing the network definition file */
  if(argc < 2){
    printf("Give a file name please!\n");
    return 0;
  }
  else if(open_net_file(argv[1]) == NULL)
    return -1;

  retval = yyparse();

  close_net_file();

  if(retval != 0)
    return retval;
  /* The input file has been parsed. -- */

  num_of_cliques = get_cliques(&cliques);
  var_list = get_parsed_variables();
  it = NIP_LIST_ITERATOR(var_list);
  nvars = NIP_LIST_LENGTH(var_list);
  vars = (nip_variable*) calloc(nvars, sizeof(nip_variable));
  
  for(i = 0; i < nvars; i++)
    vars[i] = nip_next_variable(&it);

#ifdef PRINT_JOINTREE
  nip_join_tree_dfs(cliques[0], nip_print_clique, nip_print_sepset);
#endif


  /* *********************************************************** */
#ifdef PRINT_POTENTIALS
  for(i = 0; i < num_of_cliques; i++){
    printf("In bisontest.c : clique of ");
    for(j = 0; j < nip_clique_size(cliques[i]) - 1; j++)
      printf("%s ", nip_variable_symbol(
                      nip_clique_variable(cliques[i], j)));
    printf("and %s.\n", nip_variable_symbol(
			  nip_clique_variable(cliques[i], j)));
    
    nip_fprintf_potential(stdout, cliques[i]->p);
  }
#endif /* PRINT_POTENTIALS */
  /* *********************************************************** */




  /* *********************************************************** */
#ifdef EVIDENCE
  /* add some evidence */
#ifndef EVIDENCE_SOTKU
  observed[0] = nip_search_variable_array(vars, nvars, "B");
  observed[1] = nip_search_variable_array(vars, nvars, "D");
#endif

#ifdef EVIDENCE_SOTKU
  observed[0] = nip_search_variable_array(vars, nvars, "C1");
  observed[1] = nip_search_variable_array(vars, nvars, "C4");
  observed[2] = nip_search_variable_array(vars, nvars, "C19");
#endif



  test_evidence(vars, nvars, cliques, num_of_cliques, 
		observed[0], probs[0]);

  test_evidence(vars, nvars, cliques, num_of_cliques, 
		observed[1], probs[1]);
  
#ifdef EVIDENCE_SOTKU

#ifdef TEST_RETRACTION
  /* some VERY FINE evidence */
  test_evidence(vars, nvars, cliques, num_of_cliques, 
		observed[2], probs[2]);

  /* a propagation */
  for(i = 0; i < num_of_cliques; i++)
    nip_unmark_clique(cliques[i]);
  nip_collect_evidence(NULL, NULL, cliques[0]);

  for(i = 0; i < num_of_cliques; i++)
    nip_unmark_clique(cliques[i]);
  nip_distribute_evidence(cliques[0]);



  /* marginalisation */
  if(argc > 2)
    var_name = argv[2];
  else
    var_name = "B";

  interesting = nip_search_variable_array(vars, nvars, var_name);

  if(!interesting){
    printf("In bisontest.c : variable %s not found.\n", var_name);
    return 1;
  }

  test_probability(&result, &size_of_result, interesting, cliques,
		   num_of_cliques);

  printf("Normalised probability of %s:\n", nip_variable_symbol(interesting));
  for(i = 0; i < size_of_result; i++)
    printf("P(%s=%d) = %f\n", nip_variable_symbol(interesting), i, result[i]);

  printf("\n\n");

#endif


  /* new proper evidence */
  probs[0] = probC1;
  probs[1] = probC4;
  probs[2] = probC19;

  test_evidence(vars, nvars, cliques, num_of_cliques, 
		observed[0], probs[0]);

  test_evidence(vars, nvars, cliques, num_of_cliques, 
		observed[1], probs[1]);

  test_evidence(vars, nvars, cliques, num_of_cliques, 
		observed[2], probs[2]);

#endif /* EVIDENCE_SOTKU */

#endif /* EVIDENCE */
  /* *********************************************************** */



  /* a propagation */
  for(i = 0; i < num_of_cliques; i++)
    nip_unmark_clique(cliques[i]);
  nip_collect_evidence(NULL, NULL, cliques[0]);

  for(i = 0; i < num_of_cliques; i++)
    nip_unmark_clique(cliques[i]);
  nip_distribute_evidence(cliques[0]);

  if(argc > 2)
    var_name = argv[2];
  else
    var_name = "B";
  
  interesting = nip_search_variable_array(vars, nvars, var_name);
    
  if(!interesting){
    printf("In bisontest.c : variable %s not found.\n", var_name);
    return 1;
  }

  test_probability(&result, &size_of_result, interesting, 
		   cliques, num_of_cliques);

  printf("Normalised probability of %s:\n", nip_variable_symbol(interesting));
  for(i = 0; i < size_of_result; i++)
    printf("P(%s=%d) = %f\n", nip_variable_symbol(interesting), i, result[i]);

  free(result);
  return 0;
}
