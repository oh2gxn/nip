#include <stdlib.h>
#include "parser.h"
#include "Clique.h"
#include "Variable.h"
#include "potential.h"
#include "errorhandler.h"

#define TIMESLICE

extern int yyparse();

/*
 * Calculate the probability distribution of Variable "var".
 * The family of var must be among the Cliques in cliques[]
 * (the size of which is num_of_cliques).
 * This function allocates memory for the array "result". The size of
 * the array is returned in "size_of_result".
 */
static void test_probability(double **result, 
			     Variable var, Clique cliques[],
			     int num_of_cliques){

  /* Find the Clique that contains the family of the interesting Variable. */
  Clique clique_of_interest = find_family(cliques, num_of_cliques, 
					  &var, 1);
  if(!clique_of_interest){
    printf("In hmmtest.c : No clique found! Sorry.\n");
    return;
  }  

  /* Allocate memory for the result */
  *result = (double *) calloc(number_of_values(var), sizeof(double));
  if(!result){
    printf("In hmmtest.c : Calloc failed.\n");
    return;
  }

  /* marginalisation */
  marginalise(clique_of_interest, var, *result);

  /* normalisation */
  normalise(*result, number_of_values(var));

}


int main(int argc, char *argv[]){

  int i, retval;
  int nip_num_of_cliques;

  double* result;

  Clique *nip_cliques;

  Variable interesting;

  datafile* timeseries;


  /* -- Start parsing the network definition file */
  if(argc < 4){
    printf("Give the names of the net-file, ");
    printf("data file and variable, please!\n");
    return 0;
  }
  else if(open_yyparse_infile(argv[1]) != NO_ERROR)
    return -1;

  retval = yyparse();

  close_yyparse_infile();

  if(retval != 0)
    return retval;
  /* The input file has been parsed. -- */

  nip_cliques = *get_cliques_pointer();
  nip_num_of_cliques = get_num_of_cliques();


  /* read the data from file */
  timeseries = open_datafile(argv[2], ',', 0, 1);
  if(timeseries == NULL){
    report_error(__FILE__, __LINE__, ERROR_FILENOTFOUND, 1);
    fprintf(stderr, "%s\n", argv[2]);
    return -1;
  }


  /*****************************************/
  /* FIXME: insert the reading stuff here. */
  /*****************************************/


  /* propagation */
  for(i = 0; i < nip_num_of_cliques; i++)
    unmark_Clique(nip_cliques[i]);
  collect_evidence(NULL, NULL, nip_cliques[0]);

  for(i = 0; i < nip_num_of_cliques; i++)
    unmark_Clique(nip_cliques[i]);
  distribute_evidence(nip_cliques[0]);

  interesting = get_variable(argv[3]); /* THE variable */

  if(!interesting){
    printf("In hmmtest.c : Variable of interest not found.\n");
    return 1;
  }

  test_probability(&result, interesting, nip_cliques, nip_num_of_cliques);

  printf("Normalised probability of %s:\n", get_symbol(interesting));
  for(i = 0; i < number_of_values(interesting); i++)
    printf("P(%s=%d) = %f\n", get_symbol(interesting), i, result[i]);

  printf("\n");

  free(result);
  return 0;
}
